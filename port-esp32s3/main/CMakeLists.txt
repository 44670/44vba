idf_component_register(SRCS "main.cpp" 
"../../src/gba.cpp"
"../../src/memory.cpp"
"../../src/sound.cpp"
"../../src/system.cpp"
"../../src/thread.c"
"../../libretro-common/memalign.c"
INCLUDE_DIRS "." "../../src" "../../libretro" "../../libretro-common/include")

idf_build_set_property(COMPILE_OPTIONS "-DFRONTEND_SUPPORTS_RGB565" APPEND) 
idf_build_set_property(COMPILE_OPTIONS "-DHAVE_STDINT_H" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-DINLINE=inline" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-DLSB_FIRST" APPEND)