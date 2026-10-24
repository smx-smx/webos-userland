# NetCast 4 test programs, built against the stub libraries of the nc4 preset:
#
#   cmake --preset nc4 -DCMAKE_TOOLCHAIN_FILE=$SDK/share/buildroot/toolchainfile.cmake \
#         -DCMAKE_INSTALL_PREFIX=$PWD/build/nc4-install
#   cmake --build build/nc4 && cmake --install build/nc4
#   cmake -S examples/nc4 -B build/nc4-examples -DCMAKE_TOOLCHAIN_FILE=... \
#         -DUSERLAND_PREFIX=$PWD/build/nc4-install
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

install(TARGETS lgnc-info lgnc-gles lgnc-tone DESTINATION .)
install(PROGRAMS run.sh DESTINATION .)
