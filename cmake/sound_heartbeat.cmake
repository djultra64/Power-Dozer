# Keep generated originals private and route every post through the same
# scoped hook on Linux and Windows (including same-translation-unit calls).
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
 "${CMAKE_SOURCE_DIR}/tools/prepare_sound_heartbeat.py"
 "${CMAKE_SOURCE_DIR}/.local/generated/functions/funcs_1.c"
 "${CMAKE_SOURCE_DIR}/.local/generated/functions/funcs_23.c")
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/prepare_sound_heartbeat.py"
 "${CMAKE_SOURCE_DIR}/.local/generated/functions" "${CMAKE_BINARY_DIR}"
 RESULT_VARIABLE BC_SOUND_HEARTBEAT_RESULT)
if(NOT BC_SOUND_HEARTBEAT_RESULT EQUAL 0)
 message(FATAL_ERROR "Cannot prepare fingerprinted original sound heartbeat functions")
endif()
get_target_property(BC_HEARTBEAT_SOURCES blastcorps-prototype SOURCES)
list(REMOVE_ITEM BC_HEARTBEAT_SOURCES "${CMAKE_SOURCE_DIR}/.local/generated/functions/funcs_23.c")
set_property(TARGET blastcorps-prototype PROPERTY SOURCES "${BC_HEARTBEAT_SOURCES}")
set(BC_HEARTBEAT_GUEST "${CMAKE_BINARY_DIR}/blastcorps-sound-heartbeat-guest.cpp")
if(MSVC)
 set_source_files_properties("${BC_HEARTBEAT_GUEST}" PROPERTIES COMPILE_OPTIONS "/w;/O1")
else()
 set_source_files_properties("${BC_HEARTBEAT_GUEST}" PROPERTIES COMPILE_OPTIONS "-w;-O1")
endif()
target_sources(blastcorps-prototype PRIVATE src/sound_heartbeat.cpp "${BC_HEARTBEAT_GUEST}")

foreach(BC_HEARTBEAT_MODE protected baseline)
 add_executable(sound_heartbeat_${BC_HEARTBEAT_MODE}_test tests/sound_heartbeat.cpp)
 target_include_directories(sound_heartbeat_${BC_HEARTBEAT_MODE}_test PRIVATE
  include "${CMAKE_BINARY_DIR}" "${RUNTIME}/N64Recomp/include")
 if(MSVC)
  # The bounded guest-spin fixture throws through generated C-linkage calls.
  target_compile_options(sound_heartbeat_${BC_HEARTBEAT_MODE}_test PRIVATE /EHs /EHc-)
 endif()
 if(BC_HEARTBEAT_MODE STREQUAL "protected")
  target_sources(sound_heartbeat_protected_test PRIVATE src/sound_heartbeat.cpp)
 else()
  target_compile_definitions(sound_heartbeat_baseline_test PRIVATE BC_SOUND_HEARTBEAT_BASELINE)
 endif()
 foreach(BC_HEARTBEAT_CASE saturation cancellation)
  set(BC_HEARTBEAT_TEST original_sound_heartbeat_${BC_HEARTBEAT_MODE}_${BC_HEARTBEAT_CASE})
  add_test(NAME ${BC_HEARTBEAT_TEST} COMMAND sound_heartbeat_${BC_HEARTBEAT_MODE}_test ${BC_HEARTBEAT_CASE})
  set_tests_properties(${BC_HEARTBEAT_TEST} PROPERTIES TIMEOUT 5)
  if(BC_HEARTBEAT_MODE STREQUAL "baseline")
   set_tests_properties(${BC_HEARTBEAT_TEST} PROPERTIES WILL_FAIL TRUE)
  endif()
 endforeach()
endforeach()
add_test(NAME original_sound_heartbeat_scope_and_payload COMMAND sound_heartbeat_protected_test scope)
set_tests_properties(original_sound_heartbeat_scope_and_payload PROPERTIES TIMEOUT 5)
