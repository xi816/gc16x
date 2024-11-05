// CPU identificator: GC16X
#include <cpu16/inst.h>
#define MEMSIZE 65536 // Maximum for a 16-bit cpu

struct Regs {
  U16 A;  // Accumulator
  U16 B;  // Base
  U16 C;  // Counter
  U16 D;  // Data
  U16 S;  // Segment (address)
  U8  H;  // High 8 bits
  U8  L;  // Low 8 bits
  U16 SP; // Stack pointer
  U16 BP; // Base pointer
  U16 PC; // Program counter
};
typedef struct Regs Regs;

struct GC16X {
  Regs r;
  U8 mem[MEMSIZE];
};
typedef struct GC16X GC;

U8 ReadByte(GC gc, U16 addr) {
  return gc.mem[addr];
}

U16 ReadWord(GC gc, U16 addr) {
  return (gc.mem[addr]) + (gc.mem[addr+1] << 8);
}

U8 ReadReg(GC gc, U8 regid) {
  switch (regid) { // PC register cannot be changed from
                   // {REG} addressing instruction. It
                   // can only be changed using JMP, CALL,
                   // and other control flow instructions.
    case 0x00: return gc.r.A;  break;
    case 0x01: return gc.r.B;  break;
    case 0x02: return gc.r.C;  break;
    case 0x03: return gc.r.D;  break;
    case 0x04: return gc.r.S;  break;
    case 0x05: return gc.r.H;  break;
    case 0x06: return gc.r.L;  break;
    case 0x07: return gc.r.SP; break;
    case 0x08: return gc.r.BP; break;
  }
}

U8 StackPush(GC* gc, U16 val) {
  gc->mem[gc->r.SP--] = (val << 8);
  gc->mem[gc->r.SP--] = (val % 256);
  printf("NEW SP: %04X\n", gc->r.SP);
  return 0;
}

U16 StackPop(GC* gc) {
  return ReadWord(*gc, gc->r.SP+1);
}

U8 UNK(GC* gc) {    // Unknown instruction
  fprintf(stderr, "Unknown instruction %02X\nAt position %04X\n", gc->mem[gc->r.PC], gc->r.PC);
  old_st_legacy;
  return 1;
}

U8 PUSH(GC* gc) {   // 19H
  U16 val = ReadWord(*gc, gc->r.PC+1);
  for (U16 i = 0x0000; i < 0x0010; i++) {
    printf("%02X ", gc->mem[i]);
  }
  putchar(10);
  StackPush(gc, val);
  gc->r.PC += 3;
  return 0;
}

U8 LDA0(GC* gc) {   // 55H
  gc->r.A = ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 LDB0(GC* gc) {   // 56H
  gc->r.B = ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 LDC0(GC* gc) {   // 57H
  gc->r.C = ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 LDD0(GC* gc) {   // 58H
  gc->r.D = ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 LDS0(GC* gc) {   // 59H
  gc->r.S = ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 LDA1(GC* gc) {   // 97H
  gc->r.A = ReadReg(*gc, gc->mem[gc->r.PC+1]);
  gc->r.PC += 2;
  return 0;
}

U8 INT (GC* gc) {   // C2H
  switch (ReadByte(*gc, gc->r.PC+1)) {
    case 0x00: {
      old_st_legacy;
      exit(StackPop(gc));
    }
  }
  return 0;
}

U8 NOP(GC* gc) {    // EAH
  return 0;
}

U8 (*insts[256])() = {
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &PUSH , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LDA0 , &LDB0 , &LDC0 , &LDD0 , &LDS0 , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LDA1 , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &INT  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &NOP  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK
};

U0 Reset(GC* gc) {
  gc->r.SP = 0x1000;
  gc->r.BP = 0x1000;
  gc->r.PC = 0x0000;
}

U0 StackDump(GC gc) {
  printf("SP: %04X\n", gc.r.SP);
  for (U16 i = 0x1000; i > 0x0FF0; i--) {
    printf("%04X: %02X\n", i, gc.mem[i]);
  }
}

U0 RegDump(GC gc) {
  printf("A:  %04X\n", gc.r.A);
  printf("B:  %04X\n", gc.r.B);
  printf("C:  %04X\n", gc.r.C);
  printf("D:  %04X\n", gc.r.D);
  printf("S:  %04X\n", gc.r.S);
  printf("H:  %02X\n", gc.r.H);
  printf("L:  %02X\n", gc.r.L);
  printf("SP: %04X\n", gc.r.SP);
  printf("BP: %04X\n", gc.r.BP);
  printf("PC: %04X\n", gc.r.PC);
}

U8 Exec(GC gc, const U32 memsize) {
  U8 exc = 0;
  while (!exc) {
    // printf("\033[32m%04X\033[0m\n", gc.r.PC);
    getchar();
    // printf("\033[32mExecuting\033[0m\n", gc.r.PC);
    exc = (insts[gc.mem[gc.r.PC]])(&gc);
    // StackDump(gc);
    RegDump(gc);
  }
  return exc;
}

