#pragma once
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

inline std::vector<int16_t> n64_stereo_pcm(std::span<const int16_t> rdram) {
    if (rdram.size() % 2) throw std::invalid_argument("Incomplete stereo frame");
    std::vector<int16_t> pcm(rdram.size());
    for (size_t i = 0; i < rdram.size(); i += 2) {
        pcm[i] = rdram[i + 1];
        pcm[i + 1] = rdram[i];
    }
    return pcm;
}
