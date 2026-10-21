cmake_minimum_required(VERSION 3.16)
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "Vita toolchain")
  else()
    message(FATAL_ERROR "Set VITASDK or CMAKE_TOOLCHAIN_FILE before building")
  endif()
endif()
project(VitaMotionLabAlpha C)
include("${VITASDK}/share/vita.cmake" REQUIRED)
set(SOURCES
  src/main.c src/music.c src/decoder.c
  src/audio_analysis.c src/scene_renderer.c
)
add_executable(VitaMotionLabAlpha ${SOURCES})
target_include_directories(VitaMotionLabAlpha PRIVATE src)
target_compile_options(VitaMotionLabAlpha PRIVATE -Os -Wall -Wextra -ffunction-sections -fdata-sections)
target_link_options(VitaMotionLabAlpha PRIVATE "-Wl,--gc-sections")
target_compile_definitions(VitaMotionLabAlpha PRIVATE MA_NO_DEVICE_IO MA_NO_RUNTIME_LINKING)
target_link_libraries(VitaMotionLabAlpha
    vita2d
    SceCommonDialog_stub SceGxm_stub SceDisplay_stub SceCtrl_stub
    SceAudioIn_stub SceAudio_stub SceSysmodule_stub
    SceTouch_stub SceAppMgr_stub ScePgf_stub SceIofilemgr_stub
    SceKernelThreadMgr_stub SceProcessmgr_stub
    png jpeg z m
)
vita_create_self(eboot.bin VitaMotionLabAlpha)
file(GLOB_RECURSE VIS_ASSETS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
  "assets/scenes/*/sheet_*.png"
  "assets/scenes/*/*/sheet_*.png"
  "assets/scenes/*/cabinet.png"
  "assets/scenes/*/background.png")
list(SORT VIS_ASSETS)
set(VPK_FILE_ARGS)
foreach(ASSET IN LISTS VIS_ASSETS)
  list(APPEND VPK_FILE_ARGS FILE "${ASSET}" "${ASSET}")
endforeach()
vita_create_vpk(VitaMotionLabAlpha.vpk VMLA10001 eboot.bin VERSION 01.00 NAME "Vita Motion Lab Alpha" ${VPK_FILE_ARGS})
