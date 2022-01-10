#include "../MartinAlgo.hpp"

#include <cstdio>
#include <unordered_map>

// Objective : measuring repartition of hashes with the coordinate hasher
// to ensure it has relatively good behaviour.

int main() {
    Coordinate c;
    std::hash<Coordinate> hasher;
    std::unordered_map<std::uint32_t, int> hash_distrib;

    // In hashmaps, hash values are modulo, so we need to ensure
    // correct properties with modulo.
    int hash_modulo = (1 << 16);
    int nb_coordinates = 0;

    for (c.x = -1000; c.x <= 1000; ++c.x) {
        for (c.y = 0; c.y <= 1000; ++c.y) {
            std::uint32_t hash = hasher(c) % hash_modulo;
            auto [it, inserted] = hash_distrib.insert({hash, 1});
            if (!inserted) ++it->second;
            ++nb_coordinates;
        }
    }

    std::unordered_map<int, int> hashfreq_distrib;
    for (auto [hash, freq] : hash_distrib) {
        auto [it, inserted] = hashfreq_distrib.insert({freq, 1});
        if (!inserted) ++it->second;
    }

    for (auto [hashfreq, freq] : hashfreq_distrib) {
        std::printf("%d hashes have occured %d times.\n", freq, hashfreq);
    }

    std::printf("Expected if uniform: all hashes having about %.3f occurences.\n", (float)nb_coordinates / hash_modulo);
    
    return 0;
}