#ifndef GPU16H_H
#define GPU16H_H 1

#include <SDL2/SDL.h>

#define VGASIZE 64600 // The size of the VGA for the 340x190 screen
#define WINW 340
#define WINH 190

#define gravno_start \
  SDL_Init(SDL_INIT_EVERYTHING); \
  SDL_Window* WIN = SDL_CreateWindow( \
      "Gravno Display", 500, 100, WINW, WINH, SDL_WINDOW_SHOWN); \
  SDL_Renderer* renderer = SDL_CreateRenderer( \
      WIN, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

#define gravno_end \
  SDL_DestroyRenderer(renderer); \
  SDL_DestroyWindow(WIN); \
  SDL_Quit();

struct gc_gg16 {
  U8 vga[65536];
  U8 status;
};
typedef struct gc_gg16 gc_gg16;

struct ggrgb {
  U8 r;
  U8 g;
  U8 b;
};
typedef struct ggrgb ggrgb;

#endif
