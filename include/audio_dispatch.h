#pragma once
#include <cstdint>
namespace bc {
// Resume the original scheduler protocol when the audio producer is waiting for
// a reply and a ready audio task is stranded with no current RSP task.
// Returns true only when a normal nonblocking 0x29E request was enqueued.
bool request_pending_audio_after_vi(uint8_t* rdram,uint32_t queue,uint32_t message);
}
