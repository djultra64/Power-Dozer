#include "audio_pcm.h"
#include <array>
#include <cstdlib>
#include <cstdio>
static void check(bool value) { if (!value) std::abort(); }
int main() {
    // N64 big-endian [L,R] becomes [R,L] in word-swapped host RDRAM.
    const std::array<int16_t, 6> rdram{0, 32767, -32768, 0, -123, 456};
    const auto pcm = n64_stereo_pcm(rdram);
    check(pcm == std::vector<int16_t>({32767, 0, 0, -32768, 456, -123}));
    check(n64_stereo_pcm({}).empty());
    bool rejected = false;
    try { n64_stereo_pcm(std::span(rdram).first(1)); }
    catch (const std::invalid_argument&) { rejected = true; }
    check(rejected);
    auto output=pcm;apply_audio_output(output,100,false);check(output==pcm);
    apply_audio_output(output,50,false);check(output==std::vector<int16_t>({16383,0,0,-16384,228,-61}));
    apply_audio_output(output,100,true);check(output==std::vector<int16_t>(6,0));
    output=pcm;apply_audio_output(output,0,false);check(output==std::vector<int16_t>(6,0));
    std::puts("N64 stereo channel order: passed");
}
