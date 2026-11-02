#include "prx/libkernel/KernelErrors.hpp"
#include "prx/libkernel/include/Api.hpp"
#include "SceTypes.hpp"
#include "prx/libkernel/Pthread/include/Pthread.hpp"
#include "prx/libkernel/Time/include/Time.hpp"
#include <atomic>
#include <bit>
#include <cstdint>
#include <cstdlib>

static constexpr int SCE_OK = 0;
static constexpr int LONE_YIELDS = 4096;
static constexpr int ROUNDS = 64;
static constexpr std::uint64_t YIELD_LIMIT = 100000ull;

static void Require(bool value) { if (!value) std::abort(); }

static std::atomic<bool> workerPinned{false};
static std::atomic<bool> workerReady{false};
static std::atomic<bool> start{false};
static std::atomic<int> turn{0};

static void WaitTurn(int value, std::uint64_t& yields) {
    while (turn.load() != value) {
        scePthreadYield();
        Require(++yields < YIELD_LIMIT);
    }
}

static void* APS5_VABI PingPong(void* arg) {
    const auto cpu = *static_cast<const KernelCpumask*>(arg);
    workerPinned.store(scePthreadSetaffinity(scePthreadSelf(), cpu) == SCE_OK);
    workerReady.store(true);
    workerReady.notify_one();
    start.wait(false);
    std::uint64_t yields = 0;
    for (int round = 0; round < ROUNDS; ++round) {
        WaitTurn(2 * round + 1, yields);
        turn.store(2 * round + 2);
    }
    return nullptr;
}

int main() {
    const KernelCpumask available = sceKernelGetAvailableCpumask();
    Require(available != 0);
    const KernelCpumask cpu = KernelCpumask{1} << std::countr_zero(available);
    Require(scePthreadSetaffinity(scePthreadSelf(), cpu) == SCE_OK);

    for (int call = 0; call < LONE_YIELDS; ++call) scePthreadYield();

    Pthread worker = nullptr;
    Require(scePthreadCreate(&worker, nullptr, PingPong, const_cast<KernelCpumask*>(&cpu), nullptr) == SCE_OK);
    Require(sceKernelUsleep_nid_postfix(200000) == SCE_OK);
    workerReady.wait(false);
    Require(workerPinned.load());

    std::uint64_t yields = 0;
    start.store(true);
    start.notify_one();
    for (int round = 0; round < ROUNDS; ++round) {
        Require(turn.load() == 2 * round);
        turn.store(2 * round + 1);
        WaitTurn(2 * round + 2, yields);
    }
    Require(scePthreadJoin(worker, nullptr) == SCE_OK);
    return 0;
}
