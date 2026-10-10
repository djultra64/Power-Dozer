#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>
#include <filesystem>
#include "ultramodern/renderer_context.hpp"
extern std::vector<uint8_t> bc_rom;
extern std::atomic<uint64_t> bc_vis, bc_gfx, bc_audio, bc_polls;
extern std::atomic_bool exited;
extern double bc_scale;
extern bool bc_wide;
extern int bc_refresh;
extern std::filesystem::path bc_data_directory;
std::unique_ptr<ultramodern::renderer::RendererContext> bc_create_context(uint8_t*, ultramodern::renderer::WindowHandle, bool);
[[noreturn]] void bc_fatal(const char*,uint32_t=0);

bool bc_audio_room();

extern std::atomic_bool bc_dp_sync;

bool bc_take_task_swap(uint32_t);
