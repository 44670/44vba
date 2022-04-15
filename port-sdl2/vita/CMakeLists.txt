cmake_minimum_required(VERSION 3.5)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

# Define project
project(44gba)
include("${VITASDK}/share/vita.cmake" REQUIRED)
set(VITA_APP_NAME "44GBA")
set(VITA_TITLEID  "VSDK04400")
set(VITA_VERSION  "01.00")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL2 REQUIRED)
message(STATUS "Found SDL2 at path: ${SDL2_INCLUDE_DIRS}")



add_definitions(-D__VITA__ -DUSE_FRAME_SKIP -DFRONTEND_SUPPORTS_RGB565 -DHAVE_STDINT_H -DINLINE=inline -DLSB_FIRST -DUSE_STATIC_WRAM)

add_executable(${PROJECT_NAME}
"../main.cpp" 
"../../src/gba.cpp"
"../../src/memory.cpp"
"../../src/sound.cpp"
"../../src/system.cpp"
"../../src/ui.cpp"
"../../libretro/libretro.cpp"

)

target_compile_options(${PROJECT_NAME} PRIVATE  -O3)

target_include_directories(${PROJECT_NAME} PRIVATE
    "../../src" "../../libretro" "../../libretro-common/include"
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
SDL2
SceDisplay_stub
SceCtrl_stub
SceAudio_stub
SceAudioIn_stub
SceSysmodule_stub
SceGxm_stub
SceCommonDialog_stub
SceAppMgr_stub
SceTouch_stub
SceHid_stub
SceMotion_stub
m
)

vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME})
vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE sce_sys/icon0.png sce_sys/icon0.png
  FILE sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
  FILE sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
  FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
)
