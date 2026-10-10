#pragma once
#include <cstdint>
#include <fstream>
#include <span>
#include <stdexcept>

class WavCapture {
    std::ofstream stream_;
    uint32_t bytes_ = 0;
    void u16(uint16_t value) { stream_.put(value & 255); stream_.put(value >> 8); }
    void u32(uint32_t value) { u16(value & 65535); u16(value >> 16); }
public:
    WavCapture(const std::string& path, uint32_t hz) : stream_(path, std::ios::binary) {
        if (!stream_) throw std::runtime_error("Cannot open audio capture");
        stream_.write("RIFF", 4); u32(36); stream_.write("WAVEfmt ", 8);
        u32(16); u16(1); u16(2); u32(hz); u32(hz * 4); u16(4); u16(16);
        stream_.write("data", 4); u32(0);
    }
    void append(std::span<const int16_t> samples) {
        if (samples.size() * 2 > UINT32_MAX - 36ull - bytes_)
            throw std::runtime_error("WAV capture exceeds RIFF size limit");
        for (auto sample : samples) u16(static_cast<uint16_t>(sample));
        bytes_ += static_cast<uint32_t>(samples.size() * 2);
        if (!stream_) throw std::runtime_error("Cannot write audio capture");
        finish(); // Keep the capture readable if another prototype subsystem aborts.
    }
    void finish() {
        stream_.seekp(4); u32(bytes_ + 36); stream_.seekp(40); u32(bytes_);
        stream_.seekp(0, std::ios::end);
        stream_.flush();
        if (!stream_) throw std::runtime_error("Cannot finalize audio capture");
    }
};
