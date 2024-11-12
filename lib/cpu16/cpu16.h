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

U16 ReadReg(GC gc, U8 regid) {
  // PC register cannot be changed from
  // {REG} addressing instruction. It
  // can only be changed using JMP, CALL,
  // and other control flow instructions.
  U16* regids[9] = {
    &(gc.r.A), &(gc.r.B), &(gc.r.C), &(gc.r.D),
    &(gc.r.S), (U16*)&(gc.r.H), (U16*)&(gc.r.L), &(gc.r.SP),
    &(gc.r.BP)
  };
  return *regids[regid];
}

U8 StackPush(GC* gc, U16 val) {
  gc->mem[gc->r.SP--] = (val << 8);
  gc->mem[gc->r.SP--] = (val % 256);
  printf("NEW SP: %04X\n", gc->r.SP);
  return 0;
}

U16 StackPop(GC* gc) {
  gc->r.SP += 2;
  return ReadWord(*gc, gc->r.SP-1);
}

U8 UNK(GC* gc) {    // Unknown instruction
  fprintf(stderr, "Unknown instruction %02X\nAt position %04X\n", gc->mem[gc->r.PC], gc->r.PC);
  old_st_legacy;
  return 1;
}

U8 PUSH0(GC* gc) {   // 19H
  for (U16 i = 0x0000; i < 0x0010; i++) {
    printf("%02X ", gc->mem[i]);
  }
  putchar(10);
  StackPush(gc, ReadWord(*gc, gc->r.PC+1));
  gc->r.PC += 3;
  return 0;
}

U8 JMP0(GC* gc) {   // 30H
  gc->r.PC = ReadWord(*gc, gc->r.PC+1);
  return 0;
}

U8 JMP1(GC* gc) {   // 31H
  gc->r.PC = ReadReg(*gc, gc->r.PC+1);
  return 0;
}

U8 JMP2(GC* gc) {   // 32H
  gc->r.PC = gc->mem[ReadWord(*gc, gc->r.PC+1)];
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

U8 LDH0(GC* gc) {   // 5AH
  gc->r.H = ReadByte(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 LDL0(GC* gc) {   // 5AH
  gc->r.L = ReadByte(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 LDA2(GC* gc) {   // 65H
  gc->r.A = gc->mem[ReadWord(*gc, gc->r.PC+1)];
  gc->r.PC += 3;
  return 0;
}

U8 LDB2(GC* gc) {   // 66H
  gc->r.B = gc->mem[ReadWord(*gc, gc->r.PC+1)];
  gc->r.PC += 3;
  return 0;
}

U8 LDC2(GC* gc) {   // 67H
  gc->r.C = gc->mem[ReadWord(*gc, gc->r.PC+1)];
  gc->r.PC += 3;
  return 0;
}

U8 LDD2(GC* gc) {   // 68H
  gc->r.D = gc->mem[ReadWord(*gc, gc->r.PC+1)];
  gc->r.PC += 3;
  return 0;
}

U8 LDS2(GC* gc) {   // 69H
  gc->r.S = gc->mem[ReadWord(*gc, gc->r.PC+1)];
  gc->r.PC += 3;
  return 0;
}

U8 LDH2(GC* gc) {   // 6AH
  gc->r.H = gc->mem[ReadByte(*gc, gc->r.PC+1)];
  gc->r.PC += 2;
  return 0;
}

U8 LDL2(GC* gc) {   // 6BH
  gc->r.L = gc->mem[ReadByte(*gc, gc->r.PC+1)];
  gc->r.PC += 2;
  return 0;
}

U8 PUSH1(GC* gc) {  // 90H
  StackPush(gc, ReadReg(*gc, gc->mem[gc->r.PC+1]));
  gc->r.PC += 2;
  return 0;
}

U8 LDA1(GC* gc) {   // 97H
  gc->r.A = ReadReg(*gc, gc->mem[gc->r.PC+1]);
  gc->r.PC += 2;
  return 0;
}

U8 LDB1(GC* gc) {   // 98H
  gc->r.B = ReadReg(*gc, gc->mem[gc->r.PC+1]);
  gc->r.PC += 2;
  return 0;
}

U8 LDC1(GC* gc) {   // 99H
  gc->r.C = ReadReg(*gc, gc->mem[gc->r.PC+1]);
  gc->r.PC += 2;
  return 0;
}

U8 LDD1(GC* gc) {   // 9AH
  gc->r.D = ReadReg(*gc, gc->mem[gc->r.PC+1]);
  gc->r.PC += 2;
  return 0;
}

U8 LDS1(GC* gc) {   // 9BH
  gc->r.S = ReadReg(*gc, gc->mem[gc->r.PC+1]);
  gc->r.PC += 2;
  return 0;
}

U8 LDH1(GC* gc) {   // 9CH
  gc->r.H = ReadReg(*gc, gc->mem[gc->r.PC+1]) % 256;
  gc->r.PC += 2;
  return 0;
}

U8 LDL1(GC* gc) {   // 9DH
  gc->r.L = ReadReg(*gc, gc->mem[gc->r.PC+1]) % 256;
  gc->r.PC += 2;
  return 0;
}

U8 INT (GC* gc) {   // C2H
  switch (ReadByte(*gc, gc->r.PC+1)) {
    case 0x00: {
                 old_st_legacy;
                 exit(StackPop(gc));
               }
    case 0x02: {
                 putchar(StackPop(gc));
               }
  }
  gc->r.PC += 2;
  return 0;
}

U8 NOP(GC* gc) {    // EAH
  return 0;
}

// Page 00h instructions
U8 (*insts[256])() = {
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &PUSH0, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &JMP0 , &JMP1 , &JMP2 , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LDA0 , &LDB0 , &LDC0 , &LDD0 , &LDS0 , &LDH0 , &LDL0 , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LDA2 , &LDB2 , &LDC2 , &LDD2 , &LDS2 , &LDH2 , &LDL2 , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &PUSH1, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LDA1 , &LDB1 , &LDC1 , &LDD1 , &LDS1 , &LDH1 , &LDL1 , &UNK  , &UNK  ,
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
    StackDump(gc);
    RegDump(gc);
  }
  return exc;
}

