idf_component_register(SRCS "main.cpp" 
"../../src/gba.cpp"
"../../src/memory.cpp"
"../../src/sound.cpp"
"../../src/system.cpp"
"../../src/thread.c"
"../../libretro-common/memalign.c"
INCLUDE_DIRS "." "../../src" "../../libretro" "../../libretro-common/include")
target_compile_options(${COMPONENT_LIB} PRIVATE -DFRONTEND_SUPPORTS_RGB565 -DHAVE_STDINT_H -DINLINE=inline -DLSB_FIRST -DUSE_FRAME_SKIP -O3)

