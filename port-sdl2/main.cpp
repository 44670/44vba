// Using SDL2

#include <stdio.h>
#include <stdlib.h>

#include "SDL.h"
#include "gba.h"
#include "globals.h"
#include "memory.h"
#include "sound.h"

int frameDrawn = 0;
int audioSent = 0;
uint32_t frameCount = 0;
uint8_t *libretro_save_buf;

SDL_Texture *texture;
SDL_Renderer *renderer;
SDL_AudioDeviceID audioDevice;
uint32_t lastTime = SDL_GetTicks();

void emuRunAudio() {
  audioSent = 0;
  while (!audioSent) {
    CPULoop();
  }
}

void systemMessage(const char *fmt, ...) {
  char buf[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  printf("GBA: %s\n", buf);
}

void systemDrawScreen(void) {
  frameDrawn = 1;
  frameCount++;
  if (frameCount % 120 == 0) {
    uint32_t currentTime = SDL_GetTicks();
    uint32_t delta = currentTime - lastTime;
    if (delta <= 0) {
      delta = 1;
    }
    printf("FPS: %d\n", 120 * 1000 / delta);
    lastTime = currentTime;
  }
  uint16_t *rgb565Buf = pix;
  SDL_UpdateTexture(texture, NULL, rgb565Buf, 256 * 2);
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, NULL, NULL);
  SDL_RenderPresent(renderer);
}

void systemOnWriteDataToSoundBuffer(int16_t *finalWave, int length) {
  audioSent = 1;
  SDL_QueueAudio(audioDevice, finalWave, length * 2);
}

void emuInit() {
  void load_image_preferences(void);
  cpuSaveType = 0;
  flashSize = 0x10000;
  enableRtc = false;
  mirroringEnable = false;
  CPUSetupBuffers();
  CPUInit(NULL, false);
  //gba_init();
  load_image_preferences();
  CPUReset();
  soundSetSampleRate(48000);
  soundReset();
  rtcEnable(true);
}

int main(int argc, char *argv[]) {
  // workRAM = (uint8_t *)malloc(0x40000);
  // bios = (uint8_t *)malloc(0x4000);
  // pix = (uint16_t *)malloc(4 * 256 * 160);
  // vram = (uint8_t *)malloc(0x20000);
  libretro_save_buf = (uint8_t *)malloc(0x20000 + 0x2000);
  rom = (uint8_t *)malloc(32 * 1024 * 1024);
  FILE *f = fopen("rom.gba", "rb");
  if (!f) {
    printf("Could not open rom.gba\n");
    return 1;
  }
  int bytesRead = fread(rom, 1, 32 * 1024 * 1024, f);
  printf("Read %d bytes\n", bytesRead);
  fclose(f);
  emuInit();
#ifdef _WIN32
  printf("We are on windows! Using opengl...\n");
  SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
#endif
  // Init SDL2
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
  // Init video to RGB565
  SDL_Window *window =
      SDL_CreateWindow("GBA", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                       240, 160, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  renderer = SDL_CreateRenderer(window, -1, 0);
  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565,
                              SDL_TEXTUREACCESS_STREAMING, 240, 160);
  // Init audio
  SDL_AudioSpec desiredSpec = {
      .freq = 48000,
      .format = AUDIO_S16SYS,
      .channels = 2,
      .samples = 512,
      .callback = NULL,
  };
  SDL_AudioSpec obtainedSpec;

  audioDevice = SDL_OpenAudioDevice(NULL, 0, &desiredSpec, &obtainedSpec, 0);
  if (audioDevice == 0) {
    printf("Could not open audio device\n");
    return 1;
  }
  printf("Audio device opened\n");
  // Play audio
  SDL_PauseAudioDevice(audioDevice, 0);

  SDL_Event event;

  while (1) {
    while (SDL_GetQueuedAudioSize(audioDevice) < 1600 * 4) {
      emuRunAudio();
    }

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        return 0;
      }
    }
  }

  return 0;
}
