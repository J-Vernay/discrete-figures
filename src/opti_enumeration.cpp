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

#include "../MartinAlgoOpti.hpp"
#include <vector>
#include <cstdint>
#include <chrono>
#include <numeric>

int main() {
    MartinAlgoOpti<N_, B_, W_, (W_ == 0 ? GridBehaviour::Minimal : GridBehaviour::Accurate)> martin;

    std::vector<uintmax_t> expected;
    if constexpr (B_ == 4) 
        expected = { 1, 1, 2, 6, 19, 63, 216, 760, 2725, 9910, 36446, 135268, 505861, 1903890,
            7204874, 27394666, 104592937, 400795844, 1540820542, 5940738676, 22964779660 };
    else
        expected = { 1, 1, 4, 20, 110, 638, 3832, 23592, 147941, 940982, 6053180, 39299408,
            257105146, 1692931066, 11208974860, 74570549714, 498174818986, 3340366308393 };

    std::array<uintmax_t, martin.N + 1> result;
    result.fill(0);

    auto t_begin = std::chrono::high_resolution_clock::now();
    martin.Init();
    do {
        ++result[martin.level + 1];
        martin.NextStep();
    } while (martin.level > 0);
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
