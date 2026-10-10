cmake_minimum_required(VERSION 3.20)
project(blastcorps_feasibility LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(TETRISPHERE_ROOT "" CACHE PATH "Existing Tetrisphere checkout with verified toolchain and M1 libraries")
set(DEPS "${TETRISPHERE_ROOT}/.local/deps")
set(RUNTIME "${DEPS}/N64ModernRuntime")
set(RT64 "${DEPS}/rt64")
set(PREBUILT "${TETRISPHERE_ROOT}/.local/m1/build-linux")
if(NOT EXISTS "${PREBUILT}/rt64/rt64.a")
 message(FATAL_ERROR "Set TETRISPHERE_ROOT to the checkout used for the viability investigation")
endif()
add_subdirectory("${RUNTIME}/ultramodern" "${CMAKE_BINARY_DIR}/ultramodern")
# Blast Corps submits several lists per frame. Only gDPFullSync generates DP.
file(READ "${RUNTIME}/ultramodern/src/events.cpp" BC_EVENTS)
string(REPLACE "                dp_complete();" "                if (bc_should_signal_dp()) dp_complete();" BC_EVENTS "${BC_EVENTS}")
string(REPLACE "                sp_complete();" "                // Signal SP after HLE consumes Blast Corps' reused display lists." BC_EVENTS "${BC_EVENTS}")
string(REPLACE "                renderer_context->send_dl(&task_action->task);" "                auto gfx_address = bc_graphics_task_address(task_action->task.t.data_ptr);\n                renderer_context->send_dl(&task_action->task);\n                if (bc_complete_graphics(gfx_address)) sp_complete();" BC_EVENTS "${BC_EVENTS}")
file(WRITE "${CMAKE_BINARY_DIR}/blastcorps-events.cpp" "#include <cstdint>\nextern bool bc_should_signal_dp();\nextern uint32_t bc_graphics_task_address(uint32_t);\nextern bool bc_complete_graphics(uint32_t);\n${BC_EVENTS}")
get_target_property(BC_UM_SOURCES ultramodern SOURCES)
list(REMOVE_ITEM BC_UM_SOURCES "${RUNTIME}/ultramodern/src/events.cpp")
list(APPEND BC_UM_SOURCES "${CMAKE_BINARY_DIR}/blastcorps-events.cpp")
set_property(TARGET ultramodern PROPERTY SOURCES "${BC_UM_SOURCES}")
file(GLOB GUEST CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/.local/generated/functions/funcs_*.c")
set_source_files_properties(${GUEST} PROPERTIES LANGUAGE CXX COMPILE_OPTIONS "-w;-O1")
add_executable(blastcorps-prototype src/main.cpp src/audio_sdl.cpp src/runtime.cpp src/renderer.cpp src/input.cpp src/input_model.cpp src/input_capture.cpp src/controls_store.cpp src/atomic_file.cpp src/l3d.cpp src/save_store.cpp src/pak_store.cpp src/pak_runtime.cpp src/pak_session.cpp src/save_migration.cpp src/settings.cpp src/settings_menu.cpp src/pc_options.cpp src/controls_menu.cpp
 src/game_prompts.cpp src/game_prompt_runtime.cpp src/input_glyph.cpp
 "${CMAKE_SOURCE_DIR}/.local/generated/fallbacks.cpp"
 "${CMAKE_SOURCE_DIR}/.local/audio-rsp.cpp"
 "${RUNTIME}/librecomp/src/ultra_translation.cpp"
 "${RUNTIME}/librecomp/src/cont.cpp"
 "${RUNTIME}/librecomp/src/vi.cpp"
 "${RUNTIME}/librecomp/src/rsp.cpp"
 ${GUEST})
target_compile_options(blastcorps-prototype PRIVATE -mssse3 -msse4.1)
target_compile_definitions(blastcorps-prototype PRIVATE PLUME_SDL_VULKAN_ENABLED HLSL_CPU IMGUI_IMPL_VULKAN_NO_PROTOTYPES FFX_GCC BC_ROOT="${CMAKE_SOURCE_DIR}")
target_include_directories(blastcorps-prototype PRIVATE include .local/generated/functions
 "${RUNTIME}/N64Recomp/include" "${RUNTIME}/ultramodern/include"
 "${RUNTIME}/librecomp/include" "${RUNTIME}/librecomp/include/librecomp"
 "${RUNTIME}/thirdparty" "${RUNTIME}/thirdparty/concurrentqueue" "${RUNTIME}/thirdparty/json"
 "${RT64}/src/contrib/plume" "${RT64}/src/contrib/json" "${RT64}/src/contrib/volk" "${RT64}/src" "${RT64}/src/contrib" "${RT64}/src/contrib/hlslpp/include"
 "${RT64}/src/contrib/imgui" "${RT64}/src/contrib/mupen64plus-core/src/api"
 /home/linuxbrew/.linuxbrew/include/SDL2 /home/linuxbrew/.linuxbrew/include /home/linuxbrew/.linuxbrew/opt/xorgproto/include)
target_link_libraries(blastcorps-prototype PRIVATE
 ultramodern
 "${PREBUILT}/rt64/rt64.a"
 "${PREBUILT}/rt64/src/contrib/re-spirv/libre-spirv.a"
 "${PREBUILT}/rt64/src/contrib/nativefiledialog-extended/src/libnfd.a"
 "${PREBUILT}/rt64/src/contrib/zstd/build/cmake/lib/libzstd.a"
 "${PREBUILT}/rt64/src/contrib/plume/libplume.a"
 /home/linuxbrew/.linuxbrew/lib/libSDL2.so
 /home/linuxbrew/.linuxbrew/lib/libdbus-1.so
 pthread dl)
set_target_properties(blastcorps-prototype PROPERTIES BUILD_RPATH "/home/linuxbrew/.linuxbrew/lib;${RT64}/src/contrib/dxc/lib/x64")

target_link_options(blastcorps-prototype PRIVATE -Wl,--wrap=osCreateThread_recomp -Wl,--wrap=osStartThread_recomp -Wl,--wrap=osSetThreadPri_recomp -Wl,--wrap=osRecvMesg_recomp)

enable_testing()
add_executable(audio_fifo_test tests/audio_fifo.cpp)
target_include_directories(audio_fifo_test PRIVATE include)
add_test(NAME audio_fifo COMMAND audio_fifo_test)
add_executable(rsp_scheduler_test tests/rsp_scheduler.cpp)
target_include_directories(rsp_scheduler_test PRIVATE include)
add_test(NAME rsp_audio_preemption COMMAND rsp_scheduler_test)
add_executable(audio_pcm_test tests/audio_pcm.cpp)
target_include_directories(audio_pcm_test PRIVATE include)
add_test(NAME audio_stereo_channels COMMAND audio_pcm_test)
add_executable(line_quad_test tests/line_quad.cpp)
target_include_directories(line_quad_test PRIVATE include)
add_test(NAME l3d_line_geometry COMMAND line_quad_test)
add_executable(save_store_test tests/save_store.cpp src/save_store.cpp src/atomic_file.cpp)
target_include_directories(save_store_test PRIVATE include)
add_test(NAME save_recovery_and_isolation COMMAND save_store_test)
add_executable(translucent_mode_test tests/translucent_mode.cpp)
target_include_directories(translucent_mode_test PRIVATE include)
add_test(NAME original_and_pass_translucent_modes COMMAND translucent_mode_test)
add_executable(settings_test tests/settings.cpp src/settings.cpp src/atomic_file.cpp)
target_include_directories(settings_test PRIVATE include "${RUNTIME}/thirdparty/json")
add_test(NAME durable_validated_settings COMMAND settings_test)
add_executable(settings_menu_test tests/settings_menu.cpp src/settings_menu.cpp src/settings.cpp src/atomic_file.cpp)
target_include_directories(settings_menu_test PRIVATE include "${RUNTIME}/thirdparty/json")
add_test(NAME original_style_settings_navigation COMMAND settings_menu_test)
add_executable(pc_options_test tests/pc_options.cpp src/pc_options.cpp src/settings_menu.cpp src/settings.cpp src/atomic_file.cpp)
target_include_directories(pc_options_test PRIVATE include .local/generated/functions
 "${RUNTIME}/N64Recomp/include" "${RUNTIME}/ultramodern/include"
 "${RUNTIME}/librecomp/include" "${RUNTIME}/librecomp/include/librecomp" "${RUNTIME}/thirdparty" "${RUNTIME}/thirdparty/json")
add_test(NAME pc_options_guest_bridge COMMAND pc_options_test)
target_sources(pc_options_test PRIVATE src/controls_menu.cpp src/input_model.cpp src/input_glyph.cpp)
target_include_directories(pc_options_test PRIVATE /home/linuxbrew/.linuxbrew/include/SDL2)
target_link_libraries(pc_options_test PRIVATE /home/linuxbrew/.linuxbrew/lib/libSDL2.so)
add_executable(game_prompts_test tests/game_prompts.cpp src/game_prompts.cpp src/game_prompt_runtime.cpp src/input_model.cpp)
target_include_directories(game_prompts_test PRIVATE include .local/generated/functions
 "${RUNTIME}/N64Recomp/include" "${RUNTIME}/ultramodern/include"
 "${RUNTIME}/librecomp/include" "${RUNTIME}/librecomp/include/librecomp"
 "${RUNTIME}/thirdparty/json" /home/linuxbrew/.linuxbrew/include/SDL2)
target_link_libraries(game_prompts_test PRIVATE /home/linuxbrew/.linuxbrew/lib/libSDL2.so)
target_sources(game_prompts_test PRIVATE src/input_glyph.cpp)
add_test(NAME original_game_prompts_follow_actions COMMAND game_prompts_test)
add_executable(input_glyph_test tests/input_glyph.cpp src/input_glyph.cpp src/input_model.cpp)
target_include_directories(input_glyph_test PRIVATE include /home/linuxbrew/.linuxbrew/include/SDL2)
target_link_libraries(input_glyph_test PRIVATE /home/linuxbrew/.linuxbrew/lib/libSDL2.so)
add_test(NAME device_glyphs_and_graphics_state_restoration COMMAND input_glyph_test)
add_executable(pak_store_test tests/pak_store.cpp src/pak_store.cpp src/atomic_file.cpp)
target_include_directories(pak_store_test PRIVATE include)
add_test(NAME controller_pak_files_and_recovery COMMAND pak_store_test)
add_executable(pak_runtime_test tests/pak_runtime.cpp src/pak_runtime.cpp src/pak_session.cpp src/pak_store.cpp src/save_migration.cpp src/save_store.cpp src/atomic_file.cpp)
target_include_directories(pak_runtime_test PRIVATE include .local/generated/functions
 "${RUNTIME}/N64Recomp/include" "${RUNTIME}/ultramodern/include"
 "${RUNTIME}/librecomp/include" "${RUNTIME}/librecomp/include/librecomp" "${RUNTIME}/thirdparty")
add_test(NAME controller_pak_guest_abi COMMAND pak_runtime_test)
add_executable(save_migration_test tests/save_migration.cpp src/save_migration.cpp src/pak_store.cpp src/atomic_file.cpp)
target_include_directories(save_migration_test PRIVATE include)
add_test(NAME preserve_legacy_player_on_pak_upgrade COMMAND save_migration_test)
add_executable(pak_session_test tests/pak_session.cpp src/pak_session.cpp src/pak_store.cpp src/atomic_file.cpp)
target_include_directories(pak_session_test PRIVATE include)
add_test(NAME complete_pak_transactions_survive_interruptions COMMAND pak_session_test)
add_executable(input_model_test tests/input_model.cpp src/input_model.cpp)
target_include_directories(input_model_test PRIVATE include /home/linuxbrew/.linuxbrew/include/SDL2)
target_link_libraries(input_model_test PRIVATE /home/linuxbrew/.linuxbrew/lib/libSDL2.so)
add_test(NAME input_actions_families_and_focus COMMAND input_model_test)
add_executable(input_sdl_test tests/input_sdl.cpp src/input.cpp src/input_model.cpp src/input_capture.cpp src/controls_store.cpp src/atomic_file.cpp)
target_compile_definitions(input_sdl_test PRIVATE BC_ROOT="${CMAKE_SOURCE_DIR}")
target_include_directories(input_sdl_test PRIVATE include /home/linuxbrew/.linuxbrew/include/SDL2 "${RUNTIME}/ultramodern/include" "${RUNTIME}/thirdparty" "${RUNTIME}/thirdparty/json")
target_link_libraries(input_sdl_test PRIVATE /home/linuxbrew/.linuxbrew/lib/libSDL2.so)
add_test(NAME virtual_sdl_actions_families_and_reconnect COMMAND input_sdl_test)
set_tests_properties(virtual_sdl_actions_families_and_reconnect PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy")
add_executable(controls_store_test tests/controls_store.cpp src/controls_store.cpp src/input_model.cpp src/atomic_file.cpp)
target_include_directories(controls_store_test PRIVATE include /home/linuxbrew/.linuxbrew/include/SDL2 "${RUNTIME}/thirdparty/json")
target_link_libraries(controls_store_test PRIVATE /home/linuxbrew/.linuxbrew/lib/libSDL2.so)
add_test(NAME durable_validated_action_bindings COMMAND controls_store_test)
add_executable(input_capture_test tests/input_capture.cpp src/input_capture.cpp)
target_include_directories(input_capture_test PRIVATE include /home/linuxbrew/.linuxbrew/include/SDL2)
target_link_libraries(input_capture_test PRIVATE /home/linuxbrew/.linuxbrew/lib/libSDL2.so)
target_sources(input_capture_test PRIVATE src/input_model.cpp)
add_test(NAME remap_capture_release_cancel_and_devices COMMAND input_capture_test)
add_executable(controls_menu_test tests/controls_menu.cpp src/controls_menu.cpp src/input_model.cpp)
target_include_directories(controls_menu_test PRIVATE include /home/linuxbrew/.linuxbrew/include/SDL2)
target_link_libraries(controls_menu_test PRIVATE /home/linuxbrew/.linuxbrew/lib/libSDL2.so)
add_test(NAME original_style_controls_help_and_editor COMMAND controls_menu_test)

add_executable(audio_output_test tests/audio_output.cpp)
target_include_directories(audio_output_test PRIVATE include)
add_test(NAME audio_output_loss_timing_and_recovery COMMAND audio_output_test)
add_executable(audio_sdl_test tests/audio_sdl.cpp src/audio_sdl.cpp)
target_include_directories(audio_sdl_test PRIVATE include /home/linuxbrew/.linuxbrew/include/SDL2)
target_link_libraries(audio_sdl_test PRIVATE /home/linuxbrew/.linuxbrew/lib/libSDL2.so)
add_test(NAME real_sdl_audio_output_loss_and_recovery COMMAND audio_sdl_test)
set_tests_properties(real_sdl_audio_output_loss_and_recovery PROPERTIES ENVIRONMENT "SDL_AUDIODRIVER=dummy")

add_executable(game_prompt_glyphs_test tests/game_prompt_glyphs.cpp src/game_prompts.cpp src/game_prompt_runtime.cpp src/input_model.cpp src/input_glyph.cpp)
target_include_directories(game_prompt_glyphs_test PRIVATE include .local/generated/functions
 "${RUNTIME}/N64Recomp/include" "${RUNTIME}/ultramodern/include" "${RUNTIME}/librecomp/include" "${RUNTIME}/librecomp/include/librecomp" "${RUNTIME}/thirdparty/json" /home/linuxbrew/.linuxbrew/include/SDL2)
target_link_libraries(game_prompt_glyphs_test PRIVATE /home/linuxbrew/.linuxbrew/lib/libSDL2.so)
add_test(NAME original_instruction_glyph_queue_and_bounds COMMAND game_prompt_glyphs_test)
