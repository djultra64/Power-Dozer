#pragma once
#include <cstdint>
#include <deque>
#include <stdexcept>

// AI reports the currently playing buffer, rather than the whole SDL queue.
class AudioFifo {
    uint64_t written_ = 0;
    std::deque<uint64_t> ends_;
    uint64_t reap(uint64_t queued) {
        if (queued > written_) throw std::logic_error("Audio queue accounting mismatch");
        const auto consumed = written_ - queued;
        while (!ends_.empty() && ends_.front() <= consumed) ends_.pop_front();
        return consumed;
    }
public:
    void reset() { written_ = 0; ends_.clear(); }
    void push(uint32_t bytes) { written_ += bytes; ends_.push_back(written_); }
    bool room(uint64_t queued) { reap(queued); return ends_.size() < 2; }
    uint64_t head_bytes(uint64_t queued) {
        const auto consumed = reap(queued);
        return ends_.empty() ? 0 : ends_.front() - consumed;
    }
};
