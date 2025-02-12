// GPU identificator: GovnGraphics 16
#include <cpu16/gpuh.h>

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
  SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
  SDL_RenderClear(r);
}

U0 GGtype(gc_gg16* gg, SDL_Renderer* r, U16 pos, U8 byte) {
  gg->vga[pos] = (byte << 4) + (byte);
}

U0 GGpage(gc_gg16* gg, SDL_Renderer* r) {
  U8 byte;
  for (U16 i = 0; i < VGASIZE; i++) {
    byte = gg->vga[i];
    SDL_SetRenderDrawColor(r, rgbv[byte/16].r, rgbv[byte/16].g, rgbv[byte/16].b, 0xFF);
    SDL_RenderDrawPoint(r, i%WINW, i/WINW);
    SDL_RenderDrawPoint(r, (i+1)%WINW, (i+1)/WINW);
  }
  SDL_RenderPresent(r);
}

