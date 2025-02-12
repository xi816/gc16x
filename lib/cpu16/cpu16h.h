// Header include file for lib/cpu/cpu16.h
#ifndef CPU16H_H
#define CPU16H_H 1

#define ROMSIZE 65536 // Maximum for a 16-bit cpu
#define MEMSIZE 65536 // Maximum for a 16-bit cpu

union gcreg {
  uint32_t dword;
  uint16_t word;
  uint8_t hl;
};
typedef union gcreg gcreg;

// Register cluster
struct gcrc {
  gcbyte x;
  gcbyte y;
};
typedef struct gcrc gcrc_t;

// SIB byte
struct gcsib {
  gcbyte scale;
  gcbyte index;
  gcbyte base;
};
typedef struct gcrc SIBs;

struct GC16X {
  gcreg reg[16];

  gcbyte PS;   // -I---ZNC                 Unaddressable
  uint32_t PC; // Program counter          Unaddressable

  // Memory and ROM
  gcbyte mem[MEMSIZE];
  gcbyte rom[ROMSIZE];
  gcbyte pin;

  // Prefix flags
  // disp_prefix - 0x90 | si -> bit 3 (least significant)
  gcbyte disp_prefix; // Displacement prefix: define registers added to the address used in the instruction.
                      // Can be: $91,$92,$93,$94,$95,$96,$97,$98,$99,$9A,$9B,$9C,$9D,$9E,$9F
  gcbyte sib_prefix;  // Scale-Index-Base prefix: define SIB added to the immediate used in the instruction.
                      // Opcode: $24

  // GPU
  gc_gg16 gg;
  SDL_Renderer* renderer;
};
typedef struct GC16X GC;

#endif
