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
add_executable(blastcorps-prototype src/main.cpp src/runtime.cpp src/renderer.cpp src/input.cpp src/l3d.cpp src/save_store.cpp
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
add_executable(save_store_test tests/save_store.cpp src/save_store.cpp)
target_include_directories(save_store_test PRIVATE include)
add_test(NAME save_recovery_and_isolation COMMAND save_store_test)
add_executable(translucent_mode_test tests/translucent_mode.cpp)
target_include_directories(translucent_mode_test PRIVATE include)
add_test(NAME original_and_pass_translucent_modes COMMAND translucent_mode_test)
