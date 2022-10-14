cmake_minimum_required(VERSION 3.5)
# Define project
project(gba)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_definitions(-DUSE_KEYBOARD -DFRONTEND_SUPPORTS_RGB565 -DHAVE_STDINT_H -DINLINE=inline -DLSB_FIRST -DUSE_STATIC_WRAM -DUSE_CHEATS=1)

add_executable(44gba
"main.cpp" 
"../src/gba.cpp"
"../src/memory.cpp"
"../src/sound.cpp"
"../src/system.cpp"
"../src/ui.cpp"
"../libretro/libretro.cpp"

)

target_compile_options(44gba PRIVATE -flto -O3)

target_include_directories(44gba PRIVATE
    "../src" "../libretro" "../libretro-common/include"
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(44gba PRIVATE
    ${SDL2_LIBRARIES}
)

target_link_options(44gba PRIVATE -flto -O3 --emit-symbol-map -sEXPORTED_FUNCTIONS=['_emuGetSymbol','_main','_emuRunFrame','_emuSetSampleRate','_emuLoadROM','_emuUpdateSavChangeFlag','_emuResetCpu','_emuAddCheat']  -sEXPORT_ALL=1 -sTOTAL_MEMORY=134217728 -sLLD_REPORT_UNDEFINED)
