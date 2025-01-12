// GPU identificator: GovnGraphics 16
#include <SDL2/SDL.h>
#define VGASIZE 64600 // The size of the VGA for the 340x190 screen
#define WINW 340
#define WINH 190

#define gravno_start \
  SDL_Init(SDL_INIT_EVERYTHING); \
  SDL_Window* WIN = SDL_CreateWindow( \
      "Gravno Display", 100, 100, WINW, WINH, SDL_WINDOW_SHOWN); \
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

ggrgb rgbv[] = {
  (ggrgb){.r = 0x00, .g = 0x00, .b = 0x00},
  (ggrgb){.r = 0xAA, .g = 0x00, .b = 0x00},
  (ggrgb){.r = 0x00, .g = 0xAA, .b = 0x00},
  (ggrgb){.r = 0xAA, .g = 0x55, .b = 0x00},
  (ggrgb){.r = 0x00, .g = 0x00, .b = 0xAA},
  (ggrgb){.r = 0xAA, .g = 0x00, .b = 0xAA},
  (ggrgb){.r = 0x00, .g = 0xAA, .b = 0xAA},
  (ggrgb){.r = 0xAA, .g = 0xAA, .b = 0xAA},

  (ggrgb){.r = 0x55, .g = 0x55, .b = 0x55},
  (ggrgb){.r = 0xFF, .g = 0x55, .b = 0x55},
  (ggrgb){.r = 0x55, .g = 0xFF, .b = 0x55},
  (ggrgb){.r = 0xFF, .g = 0xFF, .b = 0x55},
  (ggrgb){.r = 0x55, .g = 0x55, .b = 0xFF},
  (ggrgb){.r = 0xFF, .g = 0x55, .b = 0xFF},
  (ggrgb){.r = 0x55, .g = 0xFF, .b = 0xFF},
  (ggrgb){.r = 0xFF, .g = 0xFF, .b = 0xFF},
};

enum ggcolors {
  BLACK    = 0, // Standard 8 colors
  RED      = 1,
  GREEN    = 2,
  YELLOW   = 3,
  BLUE     = 4,
  MAGENTA  = 5,
  CYAN     = 6,
  WHITE    = 7,

  EBLACK   = 8, // Bright 8 colors
  ERED     = 9,
  EGREEN   = 10,
  EYELLOW  = 11,
  EBLUE    = 12,
  EMAGENTA = 13,
  ECYAN    = 14,
  EWHITE   = 15,
};

U0 GGinit(gc_gg16* gg, SDL_Renderer* r) {
  gg->status = 0b00000000;
  SDL_SetRenderDrawColor(r, 0, 0, 0, 0);
  SDL_RenderClear(r);
}

U0 GGtype(gc_gg16* gg, SDL_Renderer* r, U16 pos, U8 byte) {
  if (!(pos % 2)) {
    gg->vga[pos] &= 0b00001111;
    gg->vga[pos] |= (byte % 16) << 4;
    gg->vga[pos+1] &= 0b00001111;
    gg->vga[pos+1] |= (byte % 16) << 4;
  }
  else {
    gg->vga[pos] &= 0b11110000;
    gg->vga[pos] |= byte % 16;
    gg->vga[pos+1] &= 0b11110000;
    gg->vga[pos+1] |= byte % 16;
  }
}

U0 GGpage(gc_gg16* gg, SDL_Renderer* r) {
  U8 byte;
  for (U16 i = 0; i < VGASIZE; i++) {
    byte = gg->vga[i];
    SDL_SetRenderDrawColor(r, rgbv[byte/16].r, rgbv[byte/16].g, rgbv[byte/16].b, 0xFF);
    SDL_RenderDrawPoint(r, i%WINW, i/WINW);
    SDL_SetRenderDrawColor(r, rgbv[byte%16].r, rgbv[byte%16].g, rgbv[byte%16].b, 0xFF);
    SDL_RenderDrawPoint(r, (i+1)%WINW, (i+1)/WINW);
  }
  SDL_RenderPresent(r);
}

