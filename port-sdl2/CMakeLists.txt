cmake_minimum_required(VERSION 3.5)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
find_package(SDL2 REQUIRED)
message(STATUS "Found SDL2 at path: ${SDL2_INCLUDE_DIRS}")

# Define project
project(gba)

add_definitions(-DFRONTEND_SUPPORTS_RGB565 -DHAVE_STDINT_H -DINLINE=inline -DLSB_FIRST)

add_executable(gba
"main.cpp" 
"../src/gba.cpp"
"../src/memory.cpp"
"../src/sound.cpp"
"../src/system.cpp"
"../src/thread.c"
"../libretro-common/memalign.c")

target_compile_options(gba PRIVATE  -O3)

target_include_directories(gba PRIVATE
    "../src" "../libretro" "../libretro-common/include"
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(gba PRIVATE
    ${SDL2_LIBRARIES}
)