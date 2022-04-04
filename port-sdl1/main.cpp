
#ifdef __3DS__
#include <3ds.h>
#endif

// Using SDL1
#include <stdio.h>
#include <stdlib.h>

#include "SDL.h"
#include "gba.h"
#include "globals.h"
#include "memory.h"
#include "sound.h"

int audioOpened = 0;
int frameDrawn = 0;
int audioSent = 0;
uint32_t frameCount = 0;
SDL_Surface *screen;

#define AUDIO_FIFO_CAP (8192)
int16_t audioFifo[AUDIO_FIFO_CAP];
int audioFifoHead = 0;
int audioFifoLen = 0;

uint32_t lastTime = SDL_GetTicks();

uint8_t rom[32 * 1024 * 1024];

void emuRunAudio() {
  audioSent = 0;
  while (!audioSent) {
    CPULoop();
  }
}

void emuRunFrame() {
  frameDrawn = 0;
  while (!frameDrawn) {
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
  memcpy(screen->pixels, rgb565Buf, 256 * 160 * 2);
  SDL_UpdateRect(screen, 0, 0, 0, 0);
}

void systemOnWriteDataToSoundBuffer(int16_t *finalWave, int length) {
  audioSent = 1;
  int wpos = (audioFifoHead + audioFifoLen) % AUDIO_FIFO_CAP;
  length = (length / 2) * 2;
  for (int i = 0; i < length; i++) {
    if (audioFifoLen >= AUDIO_FIFO_CAP) {
      break;
    }
    audioFifo[wpos] = finalWave[i];
    wpos = (wpos + 1) % AUDIO_FIFO_CAP;
    audioFifoLen++;
  }
}

void emuInit() {
  void load_image_preferences(void);
  cpuSaveType = 0;
  flashSize = 0x10000;
  enableRtc = false;
  mirroringEnable = false;
  CPUSetupBuffers();
  CPUInit(NULL, false);
  // gba_init();
  load_image_preferences();
  CPUReset();
  soundSetSampleRate(48000);
  soundReset();
  rtcEnable(true);
}

void audioCallback(void *userdata, Uint8 *stream, int len) {
  uint16_t *wptr = (uint16_t *)stream;
  int samples = len / 2;
  while (audioFifoLen < samples) {
    emuRunAudio();
  }
  for (int i = 0; i < samples; i++) {
    int16_t sample = audioFifo[audioFifoHead];
    audioFifoHead = (audioFifoHead + 1) % AUDIO_FIFO_CAP;
    audioFifoLen--;
    *wptr = sample;
    wptr++;
  }
}

#undef main

int main(int argc, char *argv[]) {
  
#ifdef __3DS__
  osSetSpeedupEnable(true);
#endif

  FILE *f = fopen("rom.gba", "rb");
  if (!f) {
    printf("Could not open rom.gba\n");
    return 1;
  }
  fseek(f, 0, SEEK_END);
  int romLen = ftell(f);
  fseek(f, 0, SEEK_SET);
  int bytesRead = fread(rom, romLen, 1, f);
  fclose(f);
  printf("Read %d bytes\n", bytesRead);
  emuInit();

  // Init SDL 1
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
    printf("Unable to init SDL: %s\n", SDL_GetError());
  }
  // Set Video Mode
  screen = SDL_SetVideoMode(256, 160, 16, SDL_SWSURFACE);
  if (screen == NULL) {
    printf("Unable to set video mode: %s\n", SDL_GetError());
    return 1;
  }
  // Disable mouse cursor
  SDL_ShowCursor(SDL_DISABLE);
  
#ifdef __3DS__
  consoleInit(GFX_BOTTOM, NULL);
#endif

  SDL_AudioSpec spec = {
      .freq = 48000,
      .format = AUDIO_S16,
      .channels = 2,
      .samples = 512,
      .callback = audioCallback,
      .userdata = NULL,
  };
  if (SDL_OpenAudio(&spec, NULL) < 0) {
    printf("Unable to open audio: %s\n", SDL_GetError());

  } else {
    audioOpened = 1;
    SDL_PauseAudio(0);
  }

  // Set caption
  SDL_WM_SetCaption("GBA", NULL);
  while (1) {
    if (!audioOpened) {
      emuRunFrame();
    }
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:
          break;
      }
    }
  }
  return 0;
}