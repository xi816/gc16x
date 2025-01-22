// CPU identificator: GC16X
#include <cpu16/proc/std.h>
#include <cpu16/proc/interrupts.h>
#include <cpu16/gpu.h>
#define ROMSIZE 65536 // Maximum for a 16-bit cpu
#define MEMSIZE 65536 // Maximum for a 16-bit cpu

union gcreg {
  gcword word;
  gcbyte hl[2];
};
typedef union gcreg gcreg;

// Register cluster
struct gcrc {
  gcbyte x;
  gcbyte y;
};
typedef struct gcregs gcregs_t;
typedef struct gcrc gcrc_t;

struct GC16X {
  // Registers
  gcreg AX;  // Accumulator              $00
  gcreg BX;  // Base                     $01
  gcreg CX;  // Counter                  $02
  gcreg DX;  // Data                     $03
  gcreg SI;  // Segment (address)        $04
  gcreg GI;  // Segment #2 (address)     $05
  gcreg SP;  // Stack pointer            $06
  gcreg BP;  // Base pointer             $07

  gcreg EX;  // Extra accumulator        $08
  gcreg FX;  // Extra accumulator        $09
  gcreg HX;  // High byte                $0A
  gcreg LX;  // Low byte                 $0B
  gcreg X;   // X character              $0C
  gcreg Y;   // Y character              $0D
  gcreg IX;  // X index                  $0E
  gcreg IY;  // Y index                  $0F

  gcbyte PS; // -I---ZNC                 Unaddressable
  gcreg PC;  // Program counter          Unaddressable

  // Memory and ROM
  gcbyte mem[MEMSIZE];
  gcbyte rom[ROMSIZE];
  gcbyte pin;

  // Prefix flags
  // disp_prefix - 0x90 | si -> bit 3 (least significant)
  gcbyte disp_prefix; // Displacement prefix: define registers added to the address used in the instruction.
                      // Can be: $91,$92,$93,$94,$95,$96,$97,$98,$99,$9A,$9B,$9C,$9D,$9E,$9F

  // GPU
  gc_gg16 gg;
  SDL_Renderer* renderer;
};
typedef struct GC16X GC;

U8 errno;

U0 PageDump(GC gc, U8 page);
U0 StackDump(GC gc, U16 c);
U0 RegDump(GC gc);

gcbyte ReadByte(GC gc, U16 addr) {
  return gc.mem[addr];
}

gcword ReadWord(GC gc, U16 addr) {
  return (gc.mem[addr]) + (gc.mem[addr+1] << 8);
}

gcword ReadWordRev(GC gc, U16 addr) {
  return (gc.mem[addr] << 8) + (gc.mem[addr+1]);
}

U0 WriteWord(GC* gc, U16 addr, U16 val) {
  // Least significant byte goes first
  // 1448 -> $48,$14
  gc->mem[addr] = (val % 256);
  gc->mem[addr+1] = (val >> 8);
}

gcword* ReadReg(GC* gc, U8 regid) {
  // PC register cannot be changed from
  // {REG} addressing instruction. It
  // can only be changed using JMP, CALL,
  // and other control flow instructions.
  U16* regids[16] = {
    &(gc->AX.word), &(gc->BX.word), &(gc->CX.word), &(gc->DX.word), // 00-03
    &(gc->SI.word), &(gc->GI.word), &(gc->SP.word), &(gc->BP.word), // 04-07
    &(gc->EX.word), &(gc->FX.word), &(gc->HX.word), &(gc->LX.word), // 08-0B
    &(gc->X.word),  &(gc->Y.word),  &(gc->IX.word), &(gc->IY.word), // 0C-0F
  };
  return regids[regid];
}

gcbyte StackPush(GC* gc, U16 val) {
  gc->mem[gc->SP.word--] = (val >> 8);
  gc->mem[gc->SP.word--] = (val % 256);
  return 0;
}

gcword StackPop(GC* gc) {
  gc->SP.word += 2;
  return ReadWord(*gc, gc->SP.word-1);
}

gcrc_t ReadRegClust(U8 clust) { // Read a register cluster
  // The register address is 4 bytes
  gcrc_t rc = {((clust&0b11110000)>>4), (clust&0b00001111)};
  return rc;
}

U8 UNK(GC* gc) {    // Unknown instruction
  if (gc->mem[gc->PC.word-1] == 0x0F) {
    fprintf(stderr, "From page $0F\n");
  }
  else if (gc->mem[gc->PC.word-1] == 0x10) {
    fprintf(stderr, "From page $10\n");
  }
  else if (gc->mem[gc->PC.word-1] == 0x66) {
    fprintf(stderr, "From the shitty page\n");
  }
  fprintf(stderr, "\033[31mIllegal\033[0m instruction \033[33m%02X\033[0m\nAt position %04X\n", gc->mem[gc->PC.word], gc->PC.word);
  old_st_legacy;
  errno = 1;
  return 1;
}

// 0F 29 -- Jump if zero flag set to imm16 address
U8 JME0(GC* gc) {
  if (gc->PS & 0b00000100) { gc->PC.word = ReadWord(*gc, gc->PC.word+1); gc->PS &= 0b11111011; }
  else { gc->PC.word += 3; }
  return 0;
}

// 0F 2A -- Jump set to imm16 address if zero flag not
U8 JMNE0(GC* gc) {
  if (!((gc->PS & 0b00000100) >> 2)) { gc->PC.word = ReadWord(*gc, gc->PC.word+1); gc->PS &= 0b11111011; }
  else { gc->PC.word += 3; }
  return 0;
}

// 0F BB -- Jump to imm16 address if negative flag set
U8 JL0(GC* gc) {
  if (!(gc->PS & 0b00000010)) { gc->PC.word = ReadWord(*gc, gc->PC.word+1); gc->PS &= 0b11111101; }
  else { gc->PC.word += 3; }
  return 0;
}

// 0F BB -- Jump to imm16 address unconditionally
U8 JMP0(GC* gc) {
  gc->PC.word = ReadWord(*gc, gc->PC.word+1);
  return 0;
}

// 0F 31 -- Jump to *reg16 address unconditionally
U8 JMP1(GC* gc) {
  gc->PC.word = *ReadReg(gc, gc->mem[gc->PC.word+1]);
  return 0;
}

// 0F 32 -- Jump to m16 address (*m16) unconditionally
U8 JMP2(GC* gc) {
  gc->PC.word = gc->mem[ReadWord(*gc, gc->PC.word+1)];
  return 0;
}

// 0F 80 -- Pop value from stack into a register
U8 POP1(GC* gc) {   // 0F 80
  *ReadReg(gc, gc->mem[gc->PC.word+1]) = StackPop(gc);
  gc->PC.word += 2;
  return 0;
}

U8 PUSH0(GC* gc) {  // 0F 84
  StackPush(gc, ReadWord(*gc, gc->PC.word+1));
  gc->PC.word += 3;
  return 0;
}

U8 PUSH1(GC* gc) {  // 0F 90
  StackPush(gc, *ReadReg(gc, gc->mem[gc->PC.word+1]));
  gc->PC.word += 2;
  return 0;
}

U8 PUSHp(GC* gc) {  // 0F 82
  StackPush(gc, gc->mem[*ReadReg(gc, gc->mem[gc->PC.word+1])]);
  gc->PC.word += 2;
  return 0;
}

U8 INT(GC* gc, bool ri) {
  if (!((gc->PS & 0b01000000) >> 6)) {
    gc->PC.word += 2;
    return 0;
  }
  U8 val;
  if (ri) {
    val = *ReadReg(gc, ReadByte(*gc, gc->PC.word+1));
  }
  else {
    val = ReadByte(*gc, gc->PC.word+1);
  }
  switch (val) {
    case INT_EXIT: {
      old_st_legacy;
      errno = StackPop(gc);
      return 1;
    }
    case INT_READ: {
      StackPush(gc, getchar());
      break;
    }
    case INT_WRITE: {
      putchar(StackPop(gc));
      fflush(stdout);
      break;
    }
    case INT_VIDEO_WRITE: {
      GGtype(&(gc->gg), gc->renderer, gc->SI.word, (U8)gc->AX.word);
      break;
    }
    case INT_VIDEO_FLUSH: {
      GGpage(&(gc->gg), gc->renderer);
      break;
    }
    case INT_RAND: {
      gc->DX.word = rand() % 65536;
      break;
    }
    case INT_WAIT: {
      usleep((U32)(gc->DX.word)*1000); // the maximum is about 65.5 seconds
      break;
    }
    default:
      printf("Illegal interrupt %02X\n", val);
      return 1;
  }
  gc->PC.word += 2;
  return 0;
}

U8 INT0(GC* gc) { // 0F C2
  return INT(gc, false);
}

U8 INT1(GC* gc) { // 0F C3
  return INT(gc, true);
}

U8 TRAP(GC* gc) { // 0F 9D
  printf("\n\033[31mTrapped\033[0m at \033[33m%04X\033[0m\n", gc->PC.word);
  StackDump(*gc, 10);
  RegDump(*gc);
  puts("-- Press any key to continue --");
  getchar();
  gc->PC.word++;
  return 0;
}

// 0F E9 -- cpuid -- Get processor info
U8 CPUID(GC* gc) {
  switch (gc->DX.word) {
    case 0x0000: { // Get processor type
      gc->DX.word = PROC_TYPE_GC16X;
      break;
    }
    case 0x0001: { // Get connected drive
      gc->DX.word = ((gc->pin & 0b10000000) >> 7);
      break;
    }
    case 0x0002: { // Get memory size (0 for 65,536 bytes and then looping back)
      gc->DX.word = (U16)MEMSIZE;
      break;
    }
    default:
      fprintf(stderr, "Illegal CPUID value: %04X\n", gc->DX.word);
      return 1;
  }
  gc->PC.word++;
  return 0;
}

U8 ADD11(GC* gc) {  // 10 00
  gcrc_t rc = ReadRegClust(gc->mem[gc->PC.word+1]);
  *ReadReg(gc, rc.x) += *ReadReg(gc, rc.y);
  gc->PC.word += 2;
  return 0;
}

U8 SUB11(GC* gc) {  // 10 01
  gcrc_t rc = ReadRegClust(gc->mem[gc->PC.word+1]);
  *ReadReg(gc, rc.x) -= *ReadReg(gc, rc.y);
  gc->PC.word += 2;
  return 0;
}

U8 MUL11(GC* gc) {  // 10 02
  gcrc_t rc = ReadRegClust(gc->mem[gc->PC.word+1]);
  *ReadReg(gc, rc.x) *= *ReadReg(gc, rc.y);
  gc->PC.word += 2;
  return 0;
}

U8 DIV11(GC* gc) {  // 10 03
  gcrc_t rc = ReadRegClust(gc->mem[gc->PC.word+1]);
  *ReadReg(gc, rc.x) /= *ReadReg(gc, rc.y);
  gc->DX.word = (*ReadReg(gc, rc.x) % *ReadReg(gc, rc.y)); // The remainder is always stored into D
  gc->PC.word += 2;
  return 0;
}

U8 ADDA0(GC* gc) {  // 10 08
  gc->AX.word += ReadWord(*gc, gc->PC.word+1);
  gc->PC.word += 3;
  return 0;
}

U8 ADDB0(GC* gc) {  // 10 09
  gc->BX.word += ReadWord(*gc, gc->PC.word+1);
  gc->PC.word += 3;
  return 0;
}

U8 ADDC0(GC* gc) {  // 10 0A
  gc->CX.word += ReadWord(*gc, gc->PC.word+1);
  gc->PC.word += 3;
  return 0;
}

U8 ADDD0(GC* gc) {  // 10 0B
  gc->DX.word += ReadWord(*gc, gc->PC.word+1);
  gc->PC.word += 3;
  return 0;
}

U8 ADDS0(GC* gc) {  // 10 0C
  gc->SI.word += ReadWord(*gc, gc->PC.word+1);
  gc->PC.word += 3;
  return 0;
}

U8 ADDG0(GC* gc) {  // 10 0D
  gc->GI.word += ReadWord(*gc, gc->PC.word+1);
  gc->PC.word += 3;
  return 0;
}

U8 SUBA0(GC* gc) {  // 10 18
  gc->AX.word -= ReadWord(*gc, gc->PC.word+1);
  gc->PC.word += 3;
  return 0;
}

U8 SUBB0(GC* gc) {  // 10 19
  gc->BX.word -= ReadWord(*gc, gc->PC.word+1);
  gc->PC.word += 3;
  return 0;
}

U8 SUBC0(GC* gc) {  // 10 1A
  gc->CX.word -= ReadWord(*gc, gc->PC.word+1);
  gc->PC.word += 3;
  return 0;
}

U8 SUBD0(GC* gc) {  // 10 1B
  gc->DX.word -= ReadWord(*gc, gc->PC.word+1);
  gc->PC.word += 3;
  return 0;
}

U8 SUBS0(GC* gc) {  // 10 1C
  gc->SI.word -= ReadWord(*gc, gc->PC.word+1);
  gc->PC.word += 3;
  return 0;
}

U8 SUBG0(GC* gc) {  // 10 1D
  gc->GI.word -= ReadWord(*gc, gc->PC.word+1);
  gc->PC.word += 3;
  return 0;
}

U8 MULA0(GC* gc) {  // 10 28
  gc->AX.word *= ReadWord(*gc, gc->PC.word+1);
  gc->PC.word += 3;
  return 0;
}

U8 MULB0(GC* gc) {  // 10 29
  gc->BX.word *= ReadWord(*gc, gc->PC.word+1);
  gc->PC.word += 3;
  return 0;
}

U8 MULC0(GC* gc) {  // 10 2A
  gc->CX.word *= ReadWord(*gc, gc->PC.word+1);
  gc->PC.word += 3;
  return 0;
}

U8 MULD0(GC* gc) {  // 10 2B
  gc->DX.word *= ReadWord(*gc, gc->PC.word+1);
  gc->PC.word += 3;
  return 0;
}

U8 MULS0(GC* gc) {  // 10 2C
  gc->SI.word *= ReadWord(*gc, gc->PC.word+1);
  gc->PC.word += 3;
  return 0;
}

U8 MULG0(GC* gc) {  // 10 2D
  gc->GI.word *= ReadWord(*gc, gc->PC.word+1);
  gc->PC.word += 3;
  return 0;
}

U8 DIVA0(GC* gc) {  // 10 38
  U16 val = ReadWord(*gc, gc->PC.word+1);
  gc->DX.word = gc->AX.word % val; // The remainder is always stored into D
  gc->AX.word /= val;
  gc->PC.word += 3;
  return 0;
}

U8 DIVB0(GC* gc) {  // 10 39
  U16 val = ReadWord(*gc, gc->PC.word+1);
  gc->DX.word = gc->BX.word % val; // The remainder is always stored into D
  gc->BX.word /= val;
  gc->PC.word += 3;
  return 0;
}

U8 DIVC0(GC* gc) {  // 10 3A
  U16 val = ReadWord(*gc, gc->PC.word+1);
  gc->DX.word = gc->CX.word % val; // The remainder is always stored into D
  gc->CX.word /= val;
  gc->PC.word += 3;
  return 0;
}

U8 DIVD0(GC* gc) {  // 10 3B
  gc->DX.word /= ReadWord(*gc, gc->PC.word+1);
  gc->PC.word += 3;
  return 0;
}

U8 DIVS0(GC* gc) {  // 10 3C
  U16 val = ReadWord(*gc, gc->PC.word+1);
  gc->DX.word = gc->SI.word % val; // The remainder is always stored into D
  gc->SI.word /= val;
  gc->PC.word += 3;
  return 0;
}

U8 DIVG0(GC* gc) {  // 10 3D
  U16 val = ReadWord(*gc, gc->PC.word+1);
  gc->DX.word = gc->GI.word % val; // The remainder is always stored into D
  gc->GI.word /= val;
  gc->PC.word += 3;
  return 0;
}

U8 STORB(GC* gc) {  // 10 80
  gc->mem[gc->SI.word] = *ReadReg(gc, gc->mem[gc->PC.word+1]);
  gc->PC.word += 2;
  return 0;
}

U8 STGRB(GC* gc) {  // 10 81
  gc->mem[gc->GI.word] = *ReadReg(gc, gc->mem[gc->PC.word+1]);
  gc->PC.word += 2;
  return 0;
}

U8 LODSB(GC* gc) {  // 10 87
  gc->SI.word = ReadByte(*gc, gc->SI.word);
  gc->PC.word++;
  return 0;
}

U8 LODGB(GC* gc) {  // 10 88
  gc->GI.word = ReadByte(*gc, gc->GI.word);
  gc->PC.word++;
  return 0;
}

U8 STOSB(GC* gc) {  // 10 89
  gc->mem[gc->SI.word] = ReadByte(*gc, gc->PC.word+1);
  gc->PC.word += 2;
  return 0;
}

U8 STOGB(GC* gc) {  // 10 8A
  gc->mem[gc->GI.word] = ReadByte(*gc, gc->PC.word+1);
  gc->PC.word += 2;
  return 0;
}

U8 LDDS(GC* gc) {  // 10 8B
  gc->AX.word = gc->rom[*gc, gc->SI.word];
  gc->PC.word++;
  return 0;
}

U8 LDDG(GC* gc) {  // 10 9B
  gc->GI.word = gc->rom[*gc, gc->GI.word];
  gc->PC.word++;
  return 0;
}

U8 STDS(GC* gc) {  // 10 AB
  gc->rom[gc->SI.word] = *ReadReg(gc, gc->PC.word+1);
  gc->PC.word += 2;
  return 0;
}

U8 STDG(GC* gc) {  // 10 BB
  gc->rom[gc->GI.word] = *ReadReg(gc, gc->PC.word+1);
  gc->PC.word += 2;
  return 0;
}

U8 INXA(GC* gc) {   // 10 C0
  gc->AX.word++;
  gc->PC.word++;
  return 0;
}

U8 INXB(GC* gc) {   // 10 C1
  gc->BX.word++;
  gc->PC.word++;
  return 0;
}

U8 INXC(GC* gc) {   // 10 C2
  gc->CX.word++;
  gc->PC.word++;
  return 0;
}

U8 INXD(GC* gc) {   // 10 C3
  gc->DX.word++;
  gc->PC.word++;
  return 0;
}

U8 INXS(GC* gc) {   // 10 C4
  gc->SI.word++;
  gc->PC.word++;
  return 0;
}

U8 INXG(GC* gc) {   // 10 C5
  gc->GI.word++;
  gc->PC.word++;
  return 0;
}

U8 DEXA(GC* gc) {   // 10 D0
  gc->AX.word--;
  gc->PC.word++;
  return 0;
}

U8 DEXB(GC* gc) {   // 10 D1
  gc->BX.word--;
  gc->PC.word++;
  return 0;
}

U8 DEXC(GC* gc) {   // 10 D2
  gc->CX.word--;
  gc->PC.word++;
  return 0;
}

U8 DEXD(GC* gc) {   // 10 D3
  gc->DX.word--;
  gc->PC.word++;
  return 0;
}

U8 DEXS(GC* gc) {   // 10 D4
  gc->SI.word--;
  gc->PC.word++;
  return 0;
}

U8 DEXG(GC* gc) {   // 10 D5
  gc->GI.word--;
  gc->PC.word++;
  return 0;
}

U8 AND11(GC* gc) {  // 10 D8
  gcrc_t rc = ReadRegClust(gc->mem[gc->PC.word+1]);
  *ReadReg(gc, rc.x) &= *ReadReg(gc, rc.y);
  gc->PC.word += 2;
  return 0;
}

U8 OR11(GC* gc) {  // 10 D9
  gcrc_t rc = ReadRegClust(gc->mem[gc->PC.word+1]);
  *ReadReg(gc, rc.x) |= *ReadReg(gc, rc.y);
  gc->PC.word += 2;
  return 0;
}

U8 CMP11(GC* gc) {  // 10 F6
  gcrc_t rc = ReadRegClust(gc->mem[gc->PC.word+1]);
  if (*ReadReg(gc, rc.x) == *ReadReg(gc, rc.y)) gc->PS |= 0b00000100;
  else gc->PS &= 0b11111011;
  if ((I16)(*ReadReg(gc, rc.x) - *ReadReg(gc, rc.y)) < 0) gc->PS |= 0b00000010;
  else gc->PS &= 0b11111101;
  gc->PC.word += 2; // Set equal flag if two register values
  return 0;      // are equal
}

U8 CMP10(GC* gc) {  // 10 EE
  if ((*ReadReg(gc, gc->mem[gc->PC.word+1]) == ReadWord(*gc, gc->PC.word+2))) gc->PS |= 0b00000100;
  else gc->PS &= 0b11111011;
  if (((I16)(*ReadReg(gc, gc->mem[gc->PC.word+1]) - ReadWord(*gc, gc->PC.word+2)) < 0)) gc->PS |= 0b00000010;
  else gc->PS &= 0b11111101;
  gc->PC.word += 4; // Set equal flag if a register and
  return 0;      // immediate are equal
}

U8 LDRp(GC* gc) {   // 11-20 -- Load register to *reg16
  *ReadReg(gc, gc->mem[gc->PC.word]-0x11) = gc->mem[gc->PC.word+1];
  gc->PC.word += 2;
  return 0;
}

U8 RC(GC* gc) {   // 23 - Return if carry set
  if (gc->PS & 0b00000001) gc->PC.word = StackPop(gc);
  else gc->PC.word++;
  return 0;
}

U8 RE(GC* gc) {   // 2B - Return if equal
  if (gc->PS & 0b00000100) gc->PC.word = StackPop(gc);
  else gc->PC.word++;
  return 0;
}

U8 RET(GC* gc) {   // 33
  gc->PC.word = StackPop(gc);
  gc->PS &= 0b11111011;
  return 0;
}

U8 RNE(GC* gc) {   // 2B - Return if not equal
  if (!((gc->PS & 0b00000100) >> 2)) gc->PC.word = StackPop(gc);
  else gc->PC.word++;
  return 0;
}

U8 STI(GC* gc) {   // 34
  gc->PS |= 0b01000000;
  gc->PC.word++;
  return 0;
}

U8 CLC(GC* gc) {   // 36
  gc->PS &= 0b11111110;
  gc->PC.word++;
  return 0;
}

U8 HLT(GC* gc) {   // 51
  while(1) {}
  gc->PC.word++;
  return 0;
}

U8 CLI(GC* gc) {   // 52
  gc->PS &= 0b10111111;
  gc->PC.word++;
  return 0;
}

U8 LDA0(GC* gc) {   // 40 -- LDA imm16
  gc->AX.word = ReadWord(*gc, gc->PC.word+1);
  gc->PC.word += 3;
  return 0;
}

U8 LDB0(GC* gc) {   // 41 -- LDB imm16
  gc->BX.word = ReadWord(*gc, gc->PC.word+1);
  gc->PC.word += 3;
  return 0;
}

U8 LDC0(GC* gc) {   // 42 -- LDC imm16
  gc->CX.word = ReadWord(*gc, gc->PC.word+1);
  gc->PC.word += 3;
  return 0;
}

U8 LDD0(GC* gc) {   // 43 -- LDD imm16
  gc->DX.word = ReadWord(*gc, gc->PC.word+1);
  gc->PC.word += 3;
  return 0;
}

U8 LDS0(GC* gc) {   // 44 -- LDS imm16
  gc->SI.word = ReadWord(*gc, gc->PC.word+1);
  gc->PC.word += 3;
  return 0;
}

U8 LDG0(GC* gc) {   // 45 -- LDG imm16
  gc->GI.word = ReadWord(*gc, gc->PC.word+1);
  gc->PC.word += 3;
  return 0;
}

U8 LDAZ(GC* gc) {   // -- LDA mz8
  gc->AX.word = gc->mem[gc->mem[gc->PC.word+1]];
  gc->PC.word += 2;
  return 0;
}

U8 LDBZ(GC* gc) {   // 66 16 -- LDB Zero Page
  gc->BX.word = gc->mem[gc->mem[gc->PC.word+1]];
  gc->PC.word += 2;
  return 0;
}

U8 LDCZ(GC* gc) {   // 66 17 -- LDC Zero Page
  gc->CX.word = gc->mem[gc->mem[gc->PC.word+1]];
  gc->PC.word += 2;
  return 0;
}

U8 LDDZ(GC* gc) {   // 66 18 -- LDD Zero Page
  gc->DX.word = gc->mem[gc->mem[gc->PC.word+1]];
  gc->PC.word += 2;
  return 0;
}

U8 LDSZ(GC* gc) {   // 66 19 -- LDS Zero Page
  gc->SI.word = gc->mem[gc->mem[gc->PC.word+1]];
  gc->PC.word += 2;
  return 0;
}

U8 LDGZ(GC* gc) {   // 66 1A -- LDG Zero Page
  gc->GI.word = gc->mem[gc->mem[gc->PC.word+1]];
  gc->PC.word += 2;
  return 0;
}

U8 LDAZS(GC* gc) {   // 66 25 -- LDG Zero Page,S
  gc->AX.word = gc->mem[gc->mem[gc->PC.word+1]+gc->SI.word];
  gc->PC.word += 2;
  return 0;
}

U8 LDBZS(GC* gc) {   // 66 26 -- LDG Zero Page,S
  gc->BX.word = gc->mem[gc->mem[gc->PC.word+1]+gc->SI.word];
  gc->PC.word += 2;
  return 0;
}

U8 LDCZS(GC* gc) {   // 66 27 -- LDG Zero Page,S
  gc->CX.word = gc->mem[gc->mem[gc->PC.word+1]+gc->SI.word];
  gc->PC.word += 2;
  return 0;
}

U8 LDDZS(GC* gc) {   // 66 28 -- LDG Zero Page,S
  gc->DX.word = gc->mem[gc->mem[gc->PC.word+1]+gc->SI.word];
  gc->PC.word += 2;
  return 0;
}

U8 LDSZS(GC* gc) {   // 66 29 -- LDG Zero Page,S
  gc->SI.word = gc->mem[gc->mem[gc->PC.word+1]+gc->SI.word];
  gc->PC.word += 2;
  return 0;
}

U8 LDGZS(GC* gc) {   // 66 2A -- LDG Zero Page,S
  gc->GI.word = gc->mem[gc->mem[gc->PC.word+1]+gc->SI.word];
  gc->PC.word += 2;
  return 0;
}

U8 LDAZG(GC* gc) {   // 66 35 -- LDG Zero Page,G
  gc->AX.word = gc->mem[gc->mem[gc->PC.word+1]+gc->GI.word];
  gc->PC.word += 2;
  return 0;
}

U8 LDBZG(GC* gc) {   // 66 36 -- LDG Zero Page,G
  gc->BX.word = gc->mem[gc->mem[gc->PC.word+1]+gc->GI.word];
  gc->PC.word += 2;
  return 0;
}

U8 LDCZG(GC* gc) {   // 66 37 -- LDG Zero Page,G
  gc->CX.word = gc->mem[gc->mem[gc->PC.word+1]+gc->GI.word];
  gc->PC.word += 2;
  return 0;
}

U8 LDDZG(GC* gc) {   // 66 38 -- LDG Zero Page,G
  gc->DX.word = gc->mem[gc->mem[gc->PC.word+1]+gc->GI.word];
  gc->PC.word += 2;
  return 0;
}

U8 LDSZG(GC* gc) {   // 66 39 -- LDG Zero Page,G
  gc->SI.word = gc->mem[gc->mem[gc->PC.word+1]+gc->GI.word];
  gc->PC.word += 2;
  return 0;
}

U8 LDGZG(GC* gc) {   // 66 3A -- LDG Zero Page,G
  gc->GI.word = gc->mem[gc->mem[gc->PC.word+1]+gc->GI.word];
  gc->PC.word += 2;
  return 0;
}

// 66 55 -- Load ax with m16
U8 LDAA(GC* gc) {
  gc->AX.word = gc->mem[ReadWord(*gc, gc->PC.word+1)] + gc->disp_prefix;
  gc->disp_prefix = 0xFF;
  gc->PC.word += 3;
  return 0;
}

U8 LDBA(GC* gc) {    // 66 56 -- LDB Absolute
  gc->BX.word = gc->mem[ReadWord(*gc, gc->PC.word+1)];
  gc->PC.word += 3;
  return 0;
}

U8 LDCA(GC* gc) {    // 66 57 -- LDC Absolute
  gc->CX.word = gc->mem[ReadWord(*gc, gc->PC.word+1)];
  gc->PC.word += 3;
  return 0;
}

U8 LDDA(GC* gc) {    // 66 58 -- LDD Absolute
  gc->DX.word = gc->mem[ReadWord(*gc, gc->PC.word+1)];
  gc->PC.word += 3;
  return 0;
}

U8 LDSA(GC* gc) {    // 66 59 -- LDS Absolute
  gc->SI.word = gc->mem[ReadWord(*gc, gc->PC.word+1)];
  gc->PC.word += 3;
  return 0;
}

U8 LDGA(GC* gc) {    // 66 5A -- LDG Absolute
  gc->GI.word = gc->mem[ReadWord(*gc, gc->PC.word+1)];
  gc->PC.word += 3;
  return 0;
}

U8 LDAAS(GC* gc) {   // 66 65 -- LDA Absolute,S
  gc->AX.word = gc->mem[ReadWord(*gc, gc->PC.word+1)+gc->SI.word];
  gc->PC.word += 3;
  return 0;
}

U8 LDBAS(GC* gc) {   // 66 66 -- LDB Absolute,S
  gc->BX.word = gc->mem[ReadWord(*gc, gc->PC.word+1)+gc->SI.word];
  gc->PC.word += 3;
  return 0;
}

U8 LDCAS(GC* gc) {   // 66 67 -- LDC Absolute,S
  gc->CX.word = gc->mem[ReadWord(*gc, gc->PC.word+1)+gc->SI.word];
  gc->PC.word += 3;
  return 0;
}

U8 LDDAS(GC* gc) {   // 66 68 -- LDD Absolute,S
  gc->DX.word = gc->mem[ReadWord(*gc, gc->PC.word+1)+gc->SI.word];
  gc->PC.word += 3;
  return 0;
}

U8 LDSAS(GC* gc) {   // 66 69 -- LDS Absolute,S
  gc->SI.word = gc->mem[ReadWord(*gc, gc->PC.word+1)+gc->SI.word];
  gc->PC.word += 3;
  return 0;
}

U8 LDGAS(GC* gc) {   // 66 6A -- LDG Absolute,S
  gc->GI.word = gc->mem[ReadWord(*gc, gc->PC.word+1)+gc->SI.word];
  gc->PC.word += 3;
  return 0;
}

U8 LDAAG(GC* gc) {   // 66 75 -- LDA Absolute,G
  gc->AX.word = gc->mem[ReadWord(*gc, gc->PC.word+1)+gc->SI.word];
  gc->PC.word += 3;
  return 0;
}

U8 LDBAG(GC* gc) {   // 66 76 -- LDB Absolute,G
  gc->BX.word = gc->mem[ReadWord(*gc, gc->PC.word+1)+gc->SI.word];
  gc->PC.word += 3;
  return 0;
}

U8 LDCAG(GC* gc) {   // 66 77 -- LDC Absolute,G
  gc->CX.word = gc->mem[ReadWord(*gc, gc->PC.word+1)+gc->SI.word];
  gc->PC.word += 3;
  return 0;
}

U8 LDDAG(GC* gc) {   // 66 78 -- LDD Absolute,G
  gc->DX.word = gc->mem[ReadWord(*gc, gc->PC.word+1)+gc->SI.word];
  gc->PC.word += 3;
  return 0;
}

U8 LDSAG(GC* gc) {   // 66 79 -- LDS Absolute,G
  gc->SI.word = gc->mem[ReadWord(*gc, gc->PC.word+1)+gc->SI.word];
  gc->PC.word += 3;
  return 0;
}

U8 LDGAG(GC* gc) {   // 66 7A -- LDG Absolute,G
  gc->GI.word = gc->mem[ReadWord(*gc, gc->PC.word+1)+gc->SI.word];
  gc->PC.word += 3;
  return 0;
}

U8 LDA0S(GC* gc) {   // 66 85 -- LDA Immediate,S
  gc->AX.word = ReadWord(*gc, gc->PC.word+1)+gc->SI.word;
  gc->PC.word += 3;
  return 0;
}

U8 LDB0S(GC* gc) {   // 66 86 -- LDB Immediate,S
  gc->BX.word = ReadWord(*gc, gc->PC.word+1)+gc->SI.word;
  gc->PC.word += 3;
  return 0;
}

U8 LDC0S(GC* gc) {   // 66 87 -- LDC Immediate,S
  gc->CX.word = ReadWord(*gc, gc->PC.word+1)+gc->SI.word;
  gc->PC.word += 3;
  return 0;
}

U8 LDD0S(GC* gc) {   // 66 88 -- LDD Immediate,S
  gc->DX.word = ReadWord(*gc, gc->PC.word+1)+gc->SI.word;
  gc->PC.word += 3;
  return 0;
}

U8 LDS0S(GC* gc) {   // 66 89 -- LDS Immediate,S
  gc->SI.word = ReadWord(*gc, gc->PC.word+1)+gc->SI.word;
  gc->PC.word += 3;
  return 0;
}

U8 LDG0S(GC* gc) {   // 66 8A -- LDG Immediate,S
  gc->GI.word = ReadWord(*gc, gc->PC.word+1)+gc->SI.word;
  gc->PC.word += 3;
  return 0;
}

U8 LDA0G(GC* gc) {   // 66 95 -- LDA Immediate,G
  gc->AX.word = ReadWord(*gc, gc->PC.word+1)+gc->GI.word;
  gc->PC.word += 3;
  return 0;
}

U8 LDB0G(GC* gc) {   // 66 96 -- LDB Immediate,G
  gc->BX.word = ReadWord(*gc, gc->PC.word+1)+gc->GI.word;
  gc->PC.word += 3;
  return 0;
}

U8 LDC0G(GC* gc) {   // 66 97 -- LDC Immediate,G
  gc->CX.word = ReadWord(*gc, gc->PC.word+1)+gc->GI.word;
  gc->PC.word += 3;
  return 0;
}

U8 LDD0G(GC* gc) {   // 66 98 -- LDD Immediate,G
  gc->DX.word = ReadWord(*gc, gc->PC.word+1)+gc->GI.word;
  gc->PC.word += 3;
  return 0;
}

U8 LDS0G(GC* gc) {   // 66 99 -- LDS Immediate,G
  gc->SI.word = ReadWord(*gc, gc->PC.word+1)+gc->GI.word;
  gc->PC.word += 3;
  return 0;
}

U8 LDG0G(GC* gc) {   // 66 9A -- LDG Immediate,G
  gc->GI.word = ReadWord(*gc, gc->PC.word+1)+gc->GI.word;
  gc->PC.word += 3;
  return 0;
}

U8 LDA1(GC* gc) {   // 66 A5
  gc->AX.word = *ReadReg(gc, gc->mem[gc->PC.word+1]);
  gc->PC.word += 2;
  return 0;
}

U8 LDB1(GC* gc) {   // 66 A6
  gc->BX.word = *ReadReg(gc, gc->mem[gc->PC.word+1]);
  gc->PC.word += 2;
  return 0;
}

U8 LDC1(GC* gc) {   // 66 A7
  gc->CX.word = *ReadReg(gc, gc->mem[gc->PC.word+1]);
  gc->PC.word += 2;
  return 0;
}

U8 LDD1(GC* gc) {   // 66 A8
  gc->DX.word = *ReadReg(gc, gc->mem[gc->PC.word+1]);
  gc->PC.word += 2;
  return 0;
}

U8 LDS1(GC* gc) {   // 66 A9
  gc->SI.word = *ReadReg(gc, gc->mem[gc->PC.word+1]);
  gc->PC.word += 2;
  return 0;
}

U8 LDG1(GC* gc) {   // 66 AA
  gc->GI.word = *ReadReg(gc, gc->mem[gc->PC.word+1]);
  gc->PC.word += 2;
  return 0;
}

U8 LDSP1(GC* gc) {   // 66 AB
  gc->SP.word = *ReadReg(gc, gc->mem[gc->PC.word+1]);
  gc->PC.word += 2;
  return 0;
}

U8 LDBP1(GC* gc) {   // 66 AC
  gc->BP.word = *ReadReg(gc, gc->mem[gc->PC.word+1]);
  gc->PC.word += 2;
  return 0;
}

U8 CMPpi(GC* gc) {   // 69 -- Compare *reg16 and imm16
  // 69 [00] [40 00]
  if (gc->mem[*ReadReg(gc, gc->mem[gc->PC.word+1])] == ReadWord(*gc, gc->PC.word+2)) gc->PS |= 0b00000100;
  else gc->PS &= 0b11111011;
  if (((I16)(gc->mem[*ReadReg(gc, gc->mem[gc->PC.word+1])] - ReadWord(*gc, gc->PC.word+2)) < 0)) gc->PS |= 0b00000010;
  else gc->PS &= 0b11111101;
  gc->PC.word += 4;
  return 0;
}

U8 XCHG4(GC* gc) {  // 88
  gcrc_t rc = ReadRegClust(gc->mem[gc->PC.word+1]);
  U16 temp = rc.x;
  *ReadReg(gc, rc.x) = rc.y;
  *ReadReg(gc, rc.y) = temp;
  gc->PC.word += 2;
  return 0;
}

// 8A - Load a word (16 bits) from [si] into ax
U8 LODSW(GC* gc) {
  gc->AX.word = ReadWord(*gc, gc->SI.word);
  gc->PC.word++;
  return 0;
}

// 8B - Store a word (16 bits) into [si] from ax
U8 STOSW(GC* gc) {
  WriteWord(gc, gc->SI.word, gc->AX.word);
  gc->PC.word++;
  return 0;
}

U8 DEXM(GC* gc) {   // 90
  gc->mem[ReadWord(*gc, gc->PC.word+1)]--;
  gc->PC.word += 3;
  return 0;
}

U8 _PREF(GC* gc) {
  gc->disp_prefix = gc->mem[gc->PC.word++] - 0x90;
  return 0;
}

U8 ASL(GC* gc) {    // A0-A7
  U16* rgptr = ReadReg(gc, gc->mem[gc->PC.word]-0xA0);
  *rgptr = *rgptr << gc->mem[gc->PC.word+1];
  gc->PC.word += 2;
  return 0;
}

U8 ASR(GC* gc) {    // E0-E8
  *ReadReg(gc, gc->mem[gc->PC.word]-0xE0) >> gc->mem[gc->PC.word+1];
  gc->PC.word += 2;
  return 0;
}

U8 INXM(GC* gc) {   // B0
  gc->mem[ReadWord(*gc, gc->PC.word+1)]++;
  gc->PC.word += 3;
  return 0;
}

U8 STRb(GC* gc) {   // B1 -- Store byte into [%s] and increment %s
  gc->mem[gc->SI.word++] = gc->mem[gc->PC.word+1];
  gc->PC.word += 2;
  return 0;
}

U8 LOOP(GC* gc) {   // B8
  if (gc->CX.word) {
    gc->CX.word--;
    gc->PC.word = ReadWord(*gc, gc->PC.word+1);
  }
  else {
    gc->PC.word += 3;
  }
  return 0;
}

U8 CALL(GC* gc) {   // C7
  StackPush(gc, gc->PC.word+3);
  gc->PC.word = ReadWord(*gc, gc->PC.word+1);
  return 0;
}

U8 COP1(GC* gc) {   // D7
  *ReadReg(gc, gc->mem[gc->PC.word+1]) = StackPop(gc);
  gc->SP.word -= 2;
  gc->PC.word += 2;
  return 0;
}

U8 NOP(GC* gc) { // EA
  return 0;
}

U8 LFA(GC* gc) { // F9
  gc->AX.word &= 0b1111111100000000;
  gc->AX.word |= gc->PS;
  return 0;
}

U8 LAF(GC* gc) { // FA
  gc->PS = (U8)gc->AX.word;
  return 0;
}

U8 PG0F(GC*); // Page 0F - Stack operations
U8 PG10(GC*); // Page 10 - Register operations
U8 PG66(GC*); // Page 66 - Load/Store operations

// Zero page instructions
U8 (*INSTS[256])() = {
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &PG0F ,
  &PG10 , &LDRp , &LDRp , &LDRp , &LDRp , &LDRp , &LDRp , &LDRp , &LDRp , &LDRp , &LDRp , &LDRp , &LDRp , &LDRp , &LDRp , &LDRp ,
  &LDRp , &UNK  , &UNK  , &RC   , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &RE   , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &RET  , &STI  , &UNK  , &CLC  , &UNK  , &UNK  , &UNK  , &UNK  , &RNE  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &LDA0 , &LDB0 , &LDC0 , &LDD0 , &LDS0 , &LDG0 , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &HLT  , &CLI  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &PG66 , &UNK  , &UNK  , &CMPpi, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LDA1 , &LDB1 , &LDC1 , &LDD1 , &LDS1 , &LDG1 , &LDSP1, &LDBP1, &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &XCHG4, &UNK  , &LODSW, &STOSW, &UNK  , &UNK  , &UNK  , &UNK  ,
  &DEXM , &_PREF, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &INXM , &STRb , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LOOP , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &CALL , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &COP1 , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &NOP  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LFA  , &LAF  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK
};

U8 (*INSTS_PG0F[256])() = {
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &JME0 , &JMNE0, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &JMP0 , &JMP1 , &JMP2 , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &POP1 , &UNK  , &PUSHp, &UNK  , &PUSH0, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &PUSH1, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &TRAP , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &INT0 , &INT1 , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &CPUID, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK
};

U8 (*INSTS_PG10[256])() = {
  &ADD11, &SUB11, &MUL11, &DIV11, &UNK  , &UNK  , &UNK  , &UNK  , &ADDA0, &ADDB0, &ADDC0, &ADDD0, &ADDS0, &ADDG0, &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &SUBA0, &SUBB0, &SUBC0, &SUBD0, &SUBS0, &SUBG0, &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &MULA0, &MULB0, &MULC0, &MULD0, &MULS0, &MULG0, &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &DIVA0, &DIVB0, &DIVC0, &DIVD0, &DIVS0, &DIVG0, &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &STORB, &STGRB, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LODSB, &STOSB, &LODGB, &STOGB, &LDDS , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LDDG , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &STDS , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &STDG , &UNK  , &UNK  , &UNK  , &UNK  ,
  &INXA , &INXB , &INXC , &INXD , &INXS , &INXG , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &DEXA , &DEXB , &DEXC , &DEXD , &DEXS , &DEXG , &UNK  , &UNK  , &AND11, &OR11 , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &CMP10, &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &CMP11, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK
};

U8 (*INSTS_PG66[256])() = {
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LDAZ , &LDBZ , &LDCZ , &LDDZ , &LDSZ , &LDGZ , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LDAZS, &LDBZS, &LDCZS, &LDDZS, &LDSZS, &LDGZS, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LDAZG, &LDBZG, &LDCZG, &LDDZG, &LDSZG, &LDGZG, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LDAA , &LDBA , &LDCA , &LDDA , &LDSA , &LDGA , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LDAAS, &LDBAS, &LDCAS, &LDDAS, &LDSAS, &LDGAS, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LDAAG, &LDBAG, &LDCAG, &LDDAG, &LDSAG, &LDGAG, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LDA0S, &LDB0S, &LDC0S, &LDD0S, &LDS0S, &LDG0S, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LDA0G, &LDB0G, &LDC0G, &LDD0G, &LDS0G, &LDG0G, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK
};

U8 PG0F(GC* gc) {   // 0FH
  gc->PC.word++;
  return (INSTS_PG0F[gc->mem[gc->PC.word]])(gc);
}

U8 PG10(GC* gc) {   // 10H
  gc->PC.word++;
  return (INSTS_PG10[gc->mem[gc->PC.word]])(gc);
}

U8 PG66(GC* gc) {   // 66H
  gc->PC.word++;
  return (INSTS_PG66[gc->mem[gc->PC.word]])(gc);
}

U0 Reset(GC* gc, U16 driveboot) {
  gc->SP.word = 0x1000;
  gc->BP.word = 0x1000;
  gc->PC.word = 0x0000;
  
  gc->PS = 0b01000000;
}

U0 PageDump(GC gc, U8 page) {
  for (U16 i = (page*256); i < (page*256)+256; i++) {
    if (!(i % 16)) putchar(10);
    printf("%02X ", gc.mem[i]);
  }
}

U0 StackDump(GC gc, U16 c) {
  printf("SP: %04X\n", gc.SP);
  for (U16 i = 0x1000; i > 0x1000-c; i--) {
    printf("%04X: %02X\n", i, gc.mem[i]);
  }
}

U0 RegDump(GC gc) {
  printf("\033[11A\033[10CAX %04X\n",   gc.AX);
  printf("\033[10CBX %04X\n",           gc.BX);
  printf("\033[10CCX %04X\n",           gc.CX);
  printf("\033[10CDX %04X\n",           gc.DX);
  printf("\033[10CSI %04X ASCII: %c\n", gc.SI, gc.SI);
  printf("\033[10CGI %04X ASCII: %c\n", gc.GI, gc.GI);
  printf("\033[10CEX %04X\n",           gc.EX);
  printf("\033[10CFX %04X\n",           gc.FX);
  printf("\033[10CHX %04X\n",           gc.HX);
  printf("\033[10CLX %04X\n",           gc.LX);
  printf("\033[10CX  %04X\n",           gc.X);
  printf("\033[10CY  %04X\n",           gc.Y);
  printf("\033[10CIX %04X ASCII: %c\n", gc.IX, gc.IX);
  printf("\033[10CIY %04X ASCII: %c\n", gc.IY, gc.IY);
  printf("\033[10CSP %04X\n",           gc.SP);
  printf("\033[10CBP %04X\n",           gc.BP);
  printf("\033[10CPC %04X\n",           gc.PC);
  printf("\033[10CPS %08b\n",           gc.PS);
  printf("\033[10C   -I---Z-C\033[0m\n");
}

U8 Exec(GC* gc, const U32 memsize) {
  U8 exc = 0;
  execloop:
    exc = (INSTS[gc->mem[gc->PC.word]])(gc);
    if (exc != 0) exit(errno);
    goto execloop;
  return exc;
}

