#include "prx/libSceAgcDriver/Execution/include/Driver.hpp"
#include "prx/libSceAgcDriver/Execution/include/GuestMemory.hpp"
#include "prx/libSceAgcDriver/Graphics/include/Draw.hpp"
#include "prx/libSceAgcDriver/Submit/include/Dcb.hpp"
#include "prx/libc/include/GuestAllocations.hpp"
#include "prx/libc/include/GuestArena.hpp"
#include "prx/libc/include/GuestWriteWatch.hpp"
#include "VulkanTestDevice.hpp"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <sys/mman.h>
#endif
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <iostream>
#include <string>
#include <vector>

namespace {

using AgcDriver::Graphics::Require;

constexpr std::array<std::uint32_t, 12> Code{0x7e000204u, 0x7e020205u, 0x7e040206u, 0x7e060207u, 0x7e080208u, 0x7e0a0280u, 0xe0781000u, 0x80000005u, 0xe0701010u, 0x80000405u, 0xbf8c3f70u, 0xbf810000u};
constexpr std::size_t CommandsOffset = 4096;
constexpr std::size_t CommandWords = 64;
constexpr std::size_t MarkerOffset = 8192;
constexpr std::size_t BlockBytes = 65536;
constexpr std::uint32_t Filler = 0xffff1000u;

class GuestBlock {
public:
    GuestBlock() {
#ifdef _WIN32
        block = static_cast<std::uint8_t*>(GuestArena::GuestArenaAllocate_nid_postfix(BlockBytes, 65536));
        if (block != nullptr) GuestArena::GuestArenaCommit_nid_postfix(block, BlockBytes, PAGE_READWRITE, BlockBytes);
#else
        void* mapped = mmap(nullptr, BlockBytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (mapped != MAP_FAILED) {
            block = static_cast<std::uint8_t*>(mapped);
            GuestWriteWatch::GuestWriteWatchRegister_nid_postfix(block, BlockBytes);
        }
#endif
        Require(block != nullptr, "GPU-written indirect buffer: cannot allocate the guest block");
        GuestAllocations::Mutation().Add(block, BlockBytes, true, true, true);
    }

    GuestBlock(const GuestBlock&) = delete;
    GuestBlock& operator=(const GuestBlock&) = delete;

    std::uint64_t At(std::size_t offset) const { return static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(block + offset)); }
    std::uint8_t* Bytes(std::size_t offset) const { return block + offset; }

private:
    std::uint8_t* block = nullptr;
};

std::vector<std::uint32_t> Pm4(std::uint32_t opcode, std::initializer_list<std::uint32_t> body) {
    std::vector<std::uint32_t> words{0xc0000000u | (static_cast<std::uint32_t>(body.size() - 1) << 16u) | (opcode << 8u)};
    words.insert(words.end(), body);
    return words;
}

std::uint32_t Low(std::uint64_t address) { return static_cast<std::uint32_t>(address); }
std::uint32_t High(std::uint64_t address) { return static_cast<std::uint32_t>(address >> 32u); }

void Run(const GuestBlock& block, std::uint32_t marker, bool conditional) {
    std::array<std::uint32_t, CommandWords> fillers{};
    fillers.fill(Filler);
    std::memcpy(block.Bytes(CommandsOffset), fillers.data(), sizeof(fillers));
    std::memset(block.Bytes(MarkerOffset), 0, sizeof(std::uint32_t));
    const auto kernel = block.At(0);
    const auto commands = block.At(CommandsOffset);
    const auto target = block.At(MarkerOffset);
    const auto writeData = Pm4(0x37, {0x500u, Low(target), High(target), marker});
    const std::vector<std::vector<std::uint32_t>> packets{
        Pm4(0x76, {0x20c, static_cast<std::uint32_t>(kernel >> 8u)}),
        Pm4(0x76, {0x20d, static_cast<std::uint32_t>(kernel >> 40u) & 0xffu}),
        Pm4(0x76, {0x207, 1}), Pm4(0x76, {0x208, 1}), Pm4(0x76, {0x209, 1}),
        Pm4(0x76, {0x213, 9u << 1u}),
        Pm4(0x76, {0x240, Low(commands), High(commands) & 0xffffu, CommandWords * 4u, 0x31016facu, writeData[0], writeData[1], writeData[2], writeData[3], writeData[4]}),
        Pm4(0x15, {1, 1, 1, 0x41}),
        Pm4(0x46, {0x407}),
        conditional ? Pm4(0x3f, {1, 0, 0, 0, 0, 0, 0, Low(commands), High(commands) & 0xffffu, CommandWords, 0, 0, 0}) : Pm4(0x3f, {Low(commands), High(commands) & 0xffffu, CommandWords}),
    };
    std::vector<std::uint32_t> words;
    for (const auto& packet : packets) words.insert(words.end(), packet.begin(), packet.end());
    Packet dcb{words.data(), static_cast<std::uint32_t>(words.size()), 0, {}};
    Require(sceAgcDriverSubmitDcb(&dcb) == 0, "GPU-written indirect buffer: the submission failed");
    AgcDriverWaitIdle_nid_postfix();
    AgcDriver::GuestMemory::FlushGpuWrites(commands, CommandWords * sizeof(std::uint32_t));
    AgcDriver::GuestMemory::FlushGpuWrites(target, sizeof(std::uint32_t));
    std::array<std::uint32_t, 5> written{};
    std::memcpy(written.data(), block.Bytes(CommandsOffset), sizeof(written));
    Require(std::equal(written.begin(), written.end(), writeData.begin()), "GPU-written indirect buffer: the dispatch did not write its WRITE_DATA packet into the command buffer");
    std::uint32_t seen = 0;
    std::memcpy(&seen, block.Bytes(MarkerOffset), sizeof(seen));
    const std::string kind = conditional ? "a COND_INDIRECT_BUFFER" : "an INDIRECT_BUFFER";
    Require(seen == marker, kind + " ran the command buffer as it was at submission, not as the dispatch before it wrote it: the marker is " + std::to_string(seen) + ", the WRITE_DATA the dispatch wrote stores " + std::to_string(marker));
}

}

int main() {
    try {
        {
            const auto device = OpenVulkanTestDevice();
            if (!device) return VulkanTestSkipped;
        }
        GuestBlock block;
        std::memcpy(block.Bytes(0), Code.data(), sizeof(Code));
        Run(block, 0x5eed0001u, true);
        Run(block, 0x5eed0002u, false);
        AgcDriverShutdown_nid_postfix();
        std::puts("GPU-written indirect buffer: indirect buffers run what the dispatch before them wrote");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        try { AgcDriverShutdown_nid_postfix(); } catch (...) {}
        return 1;
    }
}
