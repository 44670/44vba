// Using SDL2

#include <stdio.h>
#include <stdlib.h>

#include "SDL.h"
#include "gba.h"
#include "globals.h"
#include "memory.h"
#include "sound.h"
#include "ui.h"

int frameDrawn = 0;
int audioSent = 0;
uint32_t frameCount = 0;
int emuFPS = 0;
char savFilePath[512];

SDL_Texture *texture;
SDL_Renderer *renderer;
SDL_AudioDeviceID audioDevice;
uint32_t lastTime = SDL_GetTicks();
uint8_t rom[32 * 1024 * 1024];

#define AUDIO_FIFO_CAP (8192)
volatile int16_t audioFifo[AUDIO_FIFO_CAP];
volatile int audioFifoHead = 0;
volatile int audioFifoLen = 0;
volatile int turboMode = 0;
// "a", "b", "select", "start", "right", "left", "up", "down", "r", "l"
volatile int emuKeyState[10][2];
int emuKeyboardMap[10] = {SDLK_x,     SDLK_z,    SDLK_SPACE, SDLK_RETURN,
                          SDLK_RIGHT, SDLK_LEFT, SDLK_UP,    SDLK_DOWN,
                          SDLK_s,     SDLK_a};
int emuJoystickMap[10] = {0, 1, 11, 10, 14, 12, 13, 15, 7, 6};
int emuJoystickDeadzone = 1000;

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
    emuFPS = 120 * 1000 / delta;
    printf("FPS: %d\n", emuFPS);
    lastTime = currentTime;
  }
  if (turboMode) {
    if (frameCount % 10 != 0) {
      return;
    }
  }
  uint16_t *rgb565Buf = pix;
  uiDrawBoxDim(0, 0, 240, 10);
  char buf[32];
  snprintf(buf, sizeof(buf), "FPS: %d", emuFPS);
  uiDrawText(0, 0, buf, COLOR_WHITE);
  SDL_UpdateTexture(texture, NULL, rgb565Buf, 256 * 2);
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, NULL, NULL);
  SDL_RenderPresent(renderer);
}

void systemOnWriteDataToSoundBuffer(int16_t *finalWave, int length) {
  audioSent = 1;
  if (turboMode) {
    // Do not send audio in turbo mode
    return;
  }
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

void audioCallback(void *userdata, Uint8 *stream, int len) {
  memset(stream, 0, len);
  if (turboMode) {
    return;
  }
  uint16_t *wptr = (uint16_t *)stream;
  int samples = len / 2;
  if (audioFifoLen < samples) {
    printf("audio underrun: %d < %d\n", audioFifoLen, samples);
    return;
  }
  for (int i = 0; i < samples; i++) {
    int16_t sample = audioFifo[audioFifoHead];
    audioFifoHead = (audioFifoHead + 1) % AUDIO_FIFO_CAP;
    audioFifoLen--;
    *wptr = sample;
    wptr++;
  }
}

int emuLoadROM(const char *path) {
  memset(rom, 0, sizeof(rom));
  memset(libretro_save_buf, 0, LIBRETRO_SAVE_BUF_LEN);
  FILE *f = fopen(path, "rb");
  if (!f) {
    printf("Failed to open ROM: %s\n", path);
    return -1;
  }
  fseek(f, 0, SEEK_END);
  int size = ftell(f);
  fseek(f, 0, SEEK_SET);
  int bytesRead = fread(rom, 1, size, f);
  fclose(f);
  printf("Loaded %d bytes\n", bytesRead);

  cpuSaveType = 0;
  flashSize = 0x10000;
  enableRtc = false;
  mirroringEnable = false;
  CPUSetupBuffers();
  CPUInit(NULL, false);
  // gba_init();
  void load_image_preferences(void);
  load_image_preferences();
  CPUReset();
  soundSetSampleRate(48000);
  soundReset();
  rtcEnable(true);
  // Load Save File
  snprintf(savFilePath, sizeof(savFilePath), "%s.4gs", path);
  FILE *savFile = fopen(savFilePath, "rb");
  if (savFile) {
    printf("Loading save file: %s\n", savFilePath);
    fread(libretro_save_buf, 1, LIBRETRO_SAVE_BUF_LEN, savFile);
    fclose(savFile);
  }
  return 0;
}

int emuUpdateSaveFile() {
  if (strlen(savFilePath) == 0) {
    return 0;
  }
  FILE *savFile = fopen(savFilePath, "wb");
  if (!savFile) {
    printf("Failed to open save file: %s\n", savFilePath);
    return -1;
  }
  fwrite(libretro_save_buf, 1, LIBRETRO_SAVE_BUF_LEN, savFile);
  fclose(savFile);
  printf("Saved save file: %s\n", savFilePath);
  return 0;
}

void emuHandleKey(int key, int down) {
  for (int i = 0; i < 10; i++) {
    if (emuKeyboardMap[i] == key) {
      emuKeyState[i][0] = down;
      return;
    }
  }
}

int main(int argc, char *argv[]) {
  if (emuLoadROM("rom.gba") != 0) {
    return 2;
  }
#ifdef _WIN32
  printf("We are on windows! Using opengl...\n");
  SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
#endif

  // Init SDL2
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK);
  // Init video to RGB565
  SDL_Window *window =
      SDL_CreateWindow("GBA", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                       240, 160, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565,
                              SDL_TEXTUREACCESS_STREAMING, 240, 160);

  // Init audio
  SDL_AudioSpec desiredSpec = {
      .freq = 48000,
      .format = AUDIO_S16SYS,
      .channels = 2,
      .samples = 1024,
      .callback = audioCallback,
  };
  SDL_AudioSpec obtainedSpec;
  SDL_Joystick *joystick = SDL_JoystickOpen(0);
  if (!joystick) {
    printf("Failed to open joystick\n");
  }
  SDL_JoystickEventState(SDL_DISABLE);
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
    frameDrawn = 0;
    if (turboMode) {
      emuRunFrame();
    } else {
      SDL_LockAudioDevice(audioDevice);
      while (audioFifoLen < 4800) {
        emuRunAudio();
      }
      SDL_UnlockAudioDevice(audioDevice);
    }
    // Handle event when frame is drawn
    if (frameDrawn) {
      while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
          goto bed;
        }
        if (event.type == SDL_KEYDOWN) {
          int keyCode = event.key.keysym.sym;
          emuHandleKey(keyCode, 1);
        }
        if (event.type == SDL_KEYUP) {
          int keyCode = event.key.keysym.sym;
          emuHandleKey(keyCode, 0);
        }
      }
      // Poll joysticks
      if (joystick) {
        SDL_JoystickUpdate();
        for (int i = 0; i < 10; i++) {
          if (emuJoystickMap[i] == -1) {
            continue;
          }
          int value = SDL_JoystickGetButton(joystick, emuJoystickMap[i]);
          emuKeyState[i][1] = value;
        }
        int xaxis = SDL_JoystickGetAxis(joystick, 0);
        int yaxis = SDL_JoystickGetAxis(joystick, 1);
        emuKeyState[4][1] |= xaxis > emuJoystickDeadzone;
        emuKeyState[5][1] |= xaxis < -emuJoystickDeadzone;
        emuKeyState[6][1] |= yaxis > emuJoystickDeadzone;
        emuKeyState[7][1] |= yaxis < -emuJoystickDeadzone;
      }

      joy = 0;
      for (int i = 0; i < 10; i++) {
        if (emuKeyState[i][0] || emuKeyState[i][1]) {
          joy |= (1 << i);
        }
      }
      UpdateJoypad();
    }
  }
bed:
  emuUpdateSaveFile();
  return 0;
}
