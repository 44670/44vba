idf_component_register(SRCS 
"../../../src/gba.cpp"
"../../../src/memory.cpp"
"../../../src/sound.cpp"
"../../../src/system.cpp"
INCLUDE_DIRS "." "../../../src" "../../../libretro" "../../../libretro-common/include")
target_compile_options(${COMPONENT_LIB} PRIVATE  -O2 -DFRONTEND_SUPPORTS_RGB565 -DHAVE_STDINT_H -DINLINE=inline -DLSB_FIRST -DUSE_FRAME_SKIP )

