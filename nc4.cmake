# NetCast 4.x preset: stub libraries mirror the exports of the real libraries found in
# /mnt/lg/res/lglib on NC4 firmware, so programs can't link against webOS-only APIs.

set(NC4_LDS_DIR ${CMAKE_SOURCE_DIR}/src/nc4)

add_library(EGL SHARED src/dummy.c)
set_target_properties(EGL PROPERTIES VERSION 1.4 SOVERSION 1)
target_link_options(EGL PRIVATE ${NC4_LDS_DIR}/EGL.lds)

add_library(GLESv1_CM SHARED src/dummy.c)
set_target_properties(GLESv1_CM PROPERTIES VERSION 1.1 SOVERSION 1)
target_link_options(GLESv1_CM PRIVATE ${NC4_LDS_DIR}/GLESv1_CM.lds)

add_library(GLESv2 SHARED src/dummy.c)
set_target_properties(GLESv2 PROPERTIES VERSION 2.0 SOVERSION 2)
target_link_options(GLESv2 PRIVATE ${NC4_LDS_DIR}/GLESv2.lds)

add_library(lgncopenapi SHARED src/dummy.c)
set_target_properties(lgncopenapi PROPERTIES VERSION 2.1.2 SOVERSION 2)
target_link_options(lgncopenapi PRIVATE ${NC4_LDS_DIR}/lgncopenapi.lds)

# Installed headers default to the NC4 API surface without needing -DLGNC_PLATFORM_NC4=1
file(WRITE ${CMAKE_BINARY_DIR}/include/lgncopenapi/lgnc_config.h
     "#pragma once\n\n#ifndef LGNC_PLATFORM_NC4\n#define LGNC_PLATFORM_NC4 1\n#endif\n")

install(TARGETS EGL GLESv1_CM GLESv2 lgncopenapi
        DESTINATION lib)

# NC4 ships PowerVR SGX GLES 1.1/2.0 only, no Wayland and no GLES3
foreach(dir EGL GLES GLES2 KHR lgncopenapi)
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/${dir}
          DESTINATION include
          FILES_MATCHING PATTERN "*.h")
endforeach()
install(FILES ${CMAKE_BINARY_DIR}/include/lgncopenapi/lgnc_config.h DESTINATION include/lgncopenapi)

install(FILES share/pkgconfig/egl.pc share/pkgconfig/glesv1_cm.pc share/pkgconfig/glesv2.pc
              share/pkgconfig/nc4/lgncopenapi.pc
        DESTINATION share/pkgconfig)
