// Simple iteration of all figures.

// Using preprocessor to configure N, B and W.
#ifdef N
constexpr int N_ = N;
#undef N
#else
constexpr int N_ = 10;
#endif

#ifdef B
constexpr int B_ = B;
#undef B
#else
constexpr int B_ = 4;
#endif


#ifdef W
constexpr int W_ = W;
#undef W
#else
constexpr int W_ = 4;
#endif

#ifdef T
constexpr int T_ = T;
#undef T
#else
constexpr int T_ = 8;
#endif

#include "../MartinAlgoOpti.hpp"
#include <vector>
#include <cstdint>
#include <chrono>
#include <numeric>
#include "thread_pool.hpp"

int main() {
    MartinAlgoOpti<N_, B_, W_, (W_ == 0 ? GridBehaviour::Minimal : GridBehaviour::Accurate)> martin;

    std::vector<uintmax_t> expected;
    if constexpr (B_ == 4) 
        expected = { 1, 1, 2, 6, 19, 63, 216, 760, 2725, 9910, 36446, 135268, 505861, 1903890,
            7204874, 27394666, 104592937, 400795844, 1540820542, 5940738676, 22964779660 };
    else
        expected = { 1, 1, 4, 20, 110, 638, 3832, 23592, 147941, 940982, 6053180, 39299408,
            257105146, 1692931066, 11208974860, 74570549714, 498174818986, 3340366308393 };

    std::mutex mtx_result;
    std::array<uintmax_t, martin.N + 1> result;
    result.fill(0);

    // A segment is an independent part of the enumeration which can be done in parallel.
    std::vector<decltype(martin)> segments;

    auto t_begin = std::chrono::high_resolution_clock::now();
    martin.Init();
    do {
        ++result[martin.level + 1];
        martin.NextStep<T_>();
        // Exploring deeper levels will be done in a thread pool.
        if (martin.level == T_ - 1)
            segments.push_back(martin); 
    } while (martin.level > 0);

    int nb_tasks = segments.size();
    std::atomic<int> nb_done;
    synced_stream sync_output;
    
    thread_pool{}.parallelize_loop(0, nb_tasks, [&](int imin, int imax) {
        // We are given a block of indices to process for this thread.
        std::array<uintmax_t, martin.N + 1> my_result;
        my_result.fill(0);
        for (int i = imin; i < imax; ++i) {
            auto& martin = segments[i];
            do {
                ++my_result[martin.level+1];
                martin.NextStep();
            } while (martin.level >= T_);
#ifdef SHOW_PROGRESS
            int j = ++nb_done;
            if (j % 256 == 0)
                sync_output.println((j * 100.f)/nb_tasks, " % (", j, " / ", nb_tasks, ")");
#endif
        }
        // Merge "my_result" into "result"
        mtx_result.lock();
        for (int lvl = T_ + 1; lvl <= N_; ++lvl)
            result[lvl] += my_result[lvl];
        mtx_result.unlock();
    });

    auto t_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> time = t_end - t_begin;

    std::uintmax_t total = std::accumulate(result.begin(), result.end(), std::uintmax_t{0});

    std::printf("Generation of (%d,%d)-connected figures (%ju in total) in %f s\n\t(avg: %.2f * 10^6 figures/s).\n",
                martin.B, martin.W, total, time.count(), total / time.count() / 1000000);
    std::printf("NOTE: \"expected\" is the OEIS data, lacking white-connexity. \"0\" is used when unknown.\n");

    std::printf("%15s, %15s, %15s\n", "n", "result", "expected");
    for (int i = 1; i <= martin.N; ++i) {
        std::uintmax_t e = (i < expected.size() ? expected[i] : 0);
        std::printf("%15d, %15ju, %15ju\n", i, result[i], expected[i]);
    }
    return 0;
}
