# NetCast 4 test programs, built against the stub libraries of the nc4 preset:
#
#   cmake --preset nc4 -DCMAKE_TOOLCHAIN_FILE=$SDK/share/buildroot/toolchainfile.cmake \
#         -DCMAKE_INSTALL_PREFIX=$PWD/build/nc4-install
#   cmake --build build/nc4 && cmake --install build/nc4
#   cmake -S examples/nc4 -B build/nc4-examples -DCMAKE_TOOLCHAIN_FILE=... \
#         -DUSERLAND_PREFIX=$PWD/build/nc4-install -DCMAKE_INSTALL_PREFIX=<USB stick root>
#   cmake --build build/nc4-examples && cmake --install build/nc4-examples
cmake_minimum_required(VERSION 3.12)
project(nc4_examples LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(USERLAND_PREFIX "" CACHE PATH "Install prefix of webos-userland built with the nc4 preset")
if (NOT USERLAND_PREFIX)
  message(FATAL_ERROR "Set USERLAND_PREFIX")
endif()

include_directories(${USERLAND_PREFIX}/include ${USERLAND_PREFIX}/include/lgncopenapi)
link_directories(${USERLAND_PREFIX}/lib)
add_compile_options(-Wall -Wextra)

add_executable(lgnc-info lgnc-info.c)
target_link_libraries(lgnc-info lgncopenapi dl pthread)

add_executable(lgnc-gles lgnc-gles.c)
target_link_libraries(lgnc-gles lgncopenapi EGL GLESv2 dl pthread)

add_executable(lgnc-tone lgnc-tone.c)
target_link_libraries(lgnc-tone lgncopenapi m dl pthread)

# key code -> name table for lgnc-input, from the target's kernel headers
find_file(LINUX_INPUT_H linux/input.h)
if (NOT LINUX_INPUT_H)
  message(FATAL_ERROR "linux/input.h not found")
endif()
find_file(LINUX_INPUT_EVENT_CODES_H linux/input-event-codes.h)
set(INPUT_HEADERS ${LINUX_INPUT_H})
if (LINUX_INPUT_EVENT_CODES_H)
  list(APPEND INPUT_HEADERS ${LINUX_INPUT_EVENT_CODES_H})
endif()
add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/keynames.h
  COMMAND ${CMAKE_COMMAND} "-DINPUT_HEADERS=${INPUT_HEADERS}" -DOUTPUT=${CMAKE_BINARY_DIR}/keynames.h
          -P ${CMAKE_SOURCE_DIR}/gen-keynames.cmake
  DEPENDS ${CMAKE_SOURCE_DIR}/gen-keynames.cmake ${INPUT_HEADERS})

add_executable(lgnc-input lgnc-input.c ${CMAKE_BINARY_DIR}/keynames.h)
target_include_directories(lgnc-input PRIVATE ${CMAKE_BINARY_DIR})
target_link_libraries(lgnc-input lgncopenapi EGL GLESv2 dl pthread)

# Everything goes to nc4-homebrew/ at the root of a USB stick. On the TV,
# "nc4-homebrew/nc4app setup" registers it as the "NC4 Homebrew" system app
# (nc4-launcher, which starts the program in autostart) and
# "nc4app run <program> [args]" starts programs through the app manager.
# run.sh starts them from a shell instead, without app manager integration.
set(TOOLS_DIR ${CMAKE_SOURCE_DIR}/../../tools/nc4)
install(TARGETS lgnc-info lgnc-gles lgnc-tone lgnc-input DESTINATION nc4-homebrew)
install(PROGRAMS run.sh ${TOOLS_DIR}/nc4app ${TOOLS_DIR}/nc4-launcher DESTINATION nc4-homebrew)
install(FILES icon.png autostart DESTINATION nc4-homebrew)
