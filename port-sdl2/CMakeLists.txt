cmake_minimum_required(VERSION 3.5)
# Define project
project(gba)


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL2 REQUIRED)


add_definitions(-DUSE_KEYBOARD -DFRONTEND_SUPPORTS_RGB565 -DHAVE_STDINT_H -DINLINE=inline -DLSB_FIRST -DUSE_STATIC_WRAM)

add_executable(44gba
"main.cpp" 
"../src/gba.cpp"
"../src/memory.cpp"
"../src/sound.cpp"
"../src/system.cpp"
"../src/ui.cpp"
"../libretro/libretro.cpp"

)

target_compile_options(44gba PRIVATE  -O3)

target_include_directories(44gba PRIVATE
    "../src" "../libretro" "../libretro-common/include"
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(44gba PRIVATE
    ${SDL2_LIBRARIES}
)