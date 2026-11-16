#include "prx/libc/include/General.hpp"
#include "prx/libc/include/GuestArena.hpp"
#include <chrono>
#include <cstdio>
#include <vector>
#include <windows.h>
template <class F> double Best(F&& f, int iterations) {
    double best = 1e30;
    for (int round = 0; round < 7; ++round) {
        const auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < iterations; ++i) f();
        const double ns = std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - start).count() / iterations;
        if (ns < best) best = ns;
    }
    return best;
}
int main() {
    constexpr std::size_t bytes = 1 << 20;
    auto* arena = static_cast<unsigned char*>(GuestArena::GuestArenaAllocate_nid_postfix(bytes, 65536));
    GuestArena::GuestArenaCommit_nid_postfix(arena, bytes, PAGE_READWRITE, bytes);
    for (std::size_t i = 0; i < bytes; ++i) arena[i] = static_cast<unsigned char>(i);
    std::vector<unsigned char> host(bytes, 1), out(bytes);
    volatile bool sink = false;
    for (std::size_t size : {std::size_t{8}, std::size_t{64}, std::size_t{4096}, std::size_t{65536}}) {
        const int n = size >= 65536 ? 20000 : 500000;
        const double a = Best([&] { sink = GuestCopyIn_nid_no_patch(out.data(), arena, size); }, n);
        const double h = Best([&] { sink = GuestCopyIn_nid_no_patch(out.data(), host.data(), size); }, n);
        std::printf("%6zu B: arena %8.1f ns  host %8.1f ns\n", size, a, h);
    }
    return sink ? 0 : 0;
}
