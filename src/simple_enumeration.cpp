// Enumeration of all figures using the simple implementation.

#include "../MartinAlgoSimple.hpp"
#include <vector>
#include <cstdint>
#include <chrono>
#include <numeric>

// N=18 | 1.0384s | 2006.35 Mf/s | 2083404030
// N=18 | 3.0083s | 575.07 Mf/s  | 1729972168


int main(int argc, char** argv) {
    if (argc >= 2 && std::strcmp(argv[1], "help") == 0) {
        printf("Usage: %s <N> <B> <W>\n", argv[0]);
        return 0;
    }

    int N = (argc >= 2 ? atoi(argv[1]) : 10);
    int B = (argc >= 3 ? atoi(argv[2]) : 4);
    int W = (argc >= 4 ? atoi(argv[3]) : 0);

    if (not (B == 4 || B == 8)) {
        printf("You passed B=%d, but black-connexity must be 4 or 8.\n", B);
        return 1;
    }
    if (not (W == 0 || W == 4 || W == 8)) {
        printf("You passed W=%d, but white-connexity must be 0, 4 or 8.\n", W);
        return 1;
    }
    if (B == 8 && W == 8) {
        printf("Not implemented yet.\n");
        return 1;
    }

    MartinAlgoSimple martin;
    std::vector<uintmax_t> expected;
    if (B == 4) 
        expected = { 1, 1, 2, 6, 19, 63, 216, 760, 2725, 9910, 36446, 135268, 505861, 1903890,
            7204874, 27394666, 104592937, 400795844, 1540820542, 5940738676, 22964779660 };
    else
        expected = { 1, 1, 4, 20, 110, 638, 3832, 23592, 147941, 940982, 6053180, 39299408,
            257105146, 1692931066, 11208974860, 74570549714, 498174818986, 3340366308393 };

    std::vector<uintmax_t> result(N+1, 0);

    auto t_begin = std::chrono::high_resolution_clock::now();
    martin.Init();
    do {
        if (martin.level >= N || martin.next_free == martin.candidates.size()) {
            if (!martin.Pop()) // Cannot pop, out of figures
                break;
        } else {
            Coordinate coord = martin.Push(martin.next_free);
            if (W == 4 && martin.WouldBreakWhiteLocal4(coord)
                    || W == 8 && martin.WouldBreakWhiteLocal8(coord)) {
                martin.Pop(); // Skip this figure.
            }
            else {
                ++result[martin.level];
                if (B == 4)
                    martin.AddCandidates4(coord);
                else
                    martin.AddCandidates8(coord);
            }
        }
    } while (martin.level > 0);
    auto t_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> time = t_end - t_begin;

    std::uintmax_t total = std::accumulate(result.begin(), result.end(), std::uintmax_t{0});

    std::printf("Generation of (%d,%d)-connected figures (%ju in total) in %f s\n\t(avg: %.2f * 10^6 figures/s).\n",
                B, W, total, time.count(), total / time.count() / 1000000);
    std::printf("NOTE: \"expected\" is the OEIS data, lacking white-connexity. \"0\" is used when unknown.\n");

    std::printf("%15s, %15s, %15s\n", "n", "result", "expected");
    for (int i = 1; i <= N; ++i) {
        std::uintmax_t e = (i < expected.size() ? expected[i] : 0);
        std::printf("%15d, %15ju, %15ju\n", i, result[i], expected[i]);
    }
    return 0;
}
