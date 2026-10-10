#pragma once
#include <cstdint>
#include <optional>
#include <stdexcept>

// Serialize access externally. A yielded HLE list remains owned by RT64 until
// parsing finishes; audio runs independently, with one SP event per task.
class RspScheduler {
    struct Graphics { uint32_t address; bool active = true; bool parsed = false; };
    std::optional<Graphics> graphics_;
    uint32_t status_ = 0x201;
public:
    enum class Start { Submit, Resume, Complete };
    Start start_graphics(uint32_t address) {
        status_ = 0;
        if (!graphics_) { graphics_ = Graphics{address}; return Start::Submit; }
        if (graphics_->address != address || graphics_->active)
            throw std::logic_error("RSP graphics task ownership mismatch");
        if (graphics_->parsed) { graphics_.reset(); status_ = 0x201; return Start::Complete; }
        graphics_->active = true;
        return Start::Resume;
    }
    bool yield_graphics() {
        if (!graphics_ || !graphics_->active) return false;
        graphics_->active = false;
        status_ = 0x181; // HALT, SIG0 (yield acknowledged), SIG1 (yielded).
        return true;
    }
    bool finish_graphics(uint32_t address) {
        if (!graphics_ || graphics_->address != address)
            throw std::logic_error("Unexpected RSP graphics completion");
        graphics_->parsed = true;
        if (!graphics_->active) return false;
        graphics_.reset(); status_ = 0x201;
        return true;
    }
    void finish_audio() { status_ = 0x201; }
    uint32_t status() const { return status_; }
};
