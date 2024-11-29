// CPU identificator: GC16X
#include <cpu16/inst.h>
#include <cpu16/proc/std.h>
#define MEMSIZE 65536 // Maximum for a 16-bit cpu

struct Regs {
  U16 A;  // Accumulator
  U16 B;  // Base
  U16 C;  // Counter
  U16 D;  // Data
  U16 S;  // Segment (address)
  U16 G;  // Segment #2 (address)
  U8  H;  // High 8 bits
  U8  L;  // Low 8 bits
  U16 SP; // Stack pointer
  U16 BP; // Base pointer
  U16 PC; // Program counter
};
struct RegClust {
  U8 x;
  U8 y;
};
typedef struct Regs Regs;
typedef struct RegClust RegClust;

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

U16* ReadReg(GC gc, U8 regid) {
  // PC register cannot be changed from
  // {REG} addressing instruction. It
  // can only be changed using JMP, CALL,
  // and other control flow instructions.
  U16* regids[10] = {
    &(gc.r.A), &(gc.r.B), &(gc.r.C), &(gc.r.D),
    &(gc.r.S), &(gc.r.G), (U16*)&(gc.r.H),
    (U16*)&(gc.r.L), &(gc.r.SP), &(gc.r.BP)
  };
  return regids[regid];
}

U8 StackPush(GC* gc, U16 val) {
  gc->mem[gc->r.SP--] = (val << 8);
  gc->mem[gc->r.SP--] = (val % 256);
  return 0;
}

U16 StackPop(GC* gc) {
  gc->r.SP += 2;
  return ReadWord(*gc, gc->r.SP-1);
}

// Register clusters can only be used for reading data,
// writing is not allowed.
RegClust ReadRegClust(U8 clust) { // Read a register cluster
  RegClust rc = {clust/12, clust%12};
  return rc;
}

U8 UNK(GC* gc) {    // Unknown instruction
  fprintf(stderr, "Unknown instruction %02X\nAt position %04X\n", gc->mem[gc->r.PC], gc->r.PC);
  old_st_legacy;
  return 1;
}

U8 JMP0(GC* gc) {   // 0F 30
  gc->r.PC = ReadWord(*gc, gc->r.PC+1);
  return 0;
}

U8 JMP1(GC* gc) {   // 0F 31
  gc->r.PC = *ReadReg(*gc, gc->r.PC+1);
  return 0;
}

U8 JMP2(GC* gc) {   // 0F 32
  gc->r.PC = gc->mem[ReadWord(*gc, gc->r.PC+1)];
  return 0;
}

U8 PUSH0(GC* gc) {  // 0F 84
  StackPush(gc, ReadWord(*gc, gc->r.PC+1));
  gc->r.PC += 3;
  return 0;
}

U8 PUSH1(GC* gc) {  // 0F 90
  StackPush(gc, *ReadReg(*gc, gc->mem[gc->r.PC+1]));
  gc->r.PC += 2;
  return 0;
}

U8 CPUID(GC* gc) {  // 0F E9
  switch (gc->r.D) {
    case 0x00: {
      return PROC_TYPE_GC16X;
    }
    default:
      fputs("Illegal CPUID value", stderr);
      return 1;
  }
  gc->r.PC += 1;
  return 0;
}

U8 ADDA1(GC* gc) {  // 10 00
  gc->r.A += *ReadReg(*gc, gc->mem[gc->r.PC+1]);
  gc->r.PC += 2;
  return 0;
}

U8 ADDB1(GC* gc) {  // 10 01
  gc->r.B += *ReadReg(*gc, gc->mem[gc->r.PC+1]);
  gc->r.PC += 2;
  return 0;
}

U8 ADDC1(GC* gc) {  // 10 02
  gc->r.C += *ReadReg(*gc, gc->mem[gc->r.PC+1]);
  gc->r.PC += 2;
  return 0;
}

U8 ADDD1(GC* gc) {  // 10 03
  gc->r.D += *ReadReg(*gc, gc->mem[gc->r.PC+1]);
  gc->r.PC += 2;
  return 0;
}

U8 ADDS1(GC* gc) {  // 10 04
  gc->r.S += *ReadReg(*gc, gc->mem[gc->r.PC+1]);
  gc->r.PC += 2;
  return 0;
}

U8 ADDG1(GC* gc) {  // 10 05
  gc->r.G += *ReadReg(*gc, gc->mem[gc->r.PC+1]);
  gc->r.PC += 2;
  return 0;
}

U8 ADDH1(GC* gc) {  // 10 06
  gc->r.H += *ReadReg(*gc, gc->mem[gc->r.PC+1]);
  gc->r.PC += 2;
  return 0;
}

U8 ADDL1(GC* gc) {  // 10 07
  gc->r.L += *ReadReg(*gc, gc->mem[gc->r.PC+1]);
  gc->r.PC += 2;
  return 0;
}

U8 ADDA0(GC* gc) {  // 10 08
  gc->r.A += ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 ADDB0(GC* gc) {  // 10 09
  gc->r.B += ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 ADDC0(GC* gc) {  // 10 0A
  gc->r.C += ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 ADDD0(GC* gc) {  // 10 0B
  gc->r.D += ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 ADDS0(GC* gc) {  // 10 0C
  gc->r.S += ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 ADDG0(GC* gc) {  // 10 0D
  gc->r.G += ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 ADDH0(GC* gc) {  // 10 0E
  gc->r.A += ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 ADDL0(GC* gc) {  // 10 0F
  gc->r.A += ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 LODSB(GC* gc) {  // 10 87
  gc->r.S = ReadByte(*gc, gc->r.S);
  gc->r.PC += 1;
  return 0;
}

U8 LODGB(GC* gc) {  // 10 88
  gc->r.G = ReadByte(*gc, gc->r.G);
  gc->r.PC += 1;
  return 0;
}

U8 STOSB(GC* gc) {  // 10 89
  gc->mem[gc->r.S] = ReadByte(*gc, gc->r.PC+1);
  gc->r.PC += 2;
  return 0;
}

U8 STOGB(GC* gc) {  // 10 8A
  gc->mem[gc->r.G] = ReadByte(*gc, gc->r.PC+1);
  gc->r.PC += 2;
  return 0;
}

U8 LDA0(GC* gc) {   // 66 05
  gc->r.A = ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 LDB0(GC* gc) {   // 66 06
  gc->r.B = ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 LDC0(GC* gc) {   // 66 07
  gc->r.C = ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 LDD0(GC* gc) {   // 66 08
  gc->r.D = ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 LDS0(GC* gc) {   // 66 09
  gc->r.S = ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 LDG0(GC* gc) {   // 66 0A
  gc->r.G = ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 LDH0(GC* gc) {   // 66 0B
  gc->r.H = ReadByte(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 LDL0(GC* gc) {   // 66 0C
  gc->r.L = ReadByte(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 LDA2(GC* gc) {   // 66 92
  gc->r.A = gc->mem[ReadWord(*gc, gc->r.PC+1)];
  gc->r.PC += 3;
  return 0;
}

U8 LDB2(GC* gc) {   // 66 93
  gc->r.B = gc->mem[ReadWord(*gc, gc->r.PC+1)];
  gc->r.PC += 3;
  return 0;
}

U8 LDC2(GC* gc) {   // 66 94
  gc->r.C = gc->mem[ReadWord(*gc, gc->r.PC+1)];
  gc->r.PC += 3;
  return 0;
}

U8 LDD2(GC* gc) {   // 66 95
  gc->r.D = gc->mem[ReadWord(*gc, gc->r.PC+1)];
  gc->r.PC += 3;
  return 0;
}

U8 LDS2(GC* gc) {   // 66 96
  gc->r.S = gc->mem[ReadWord(*gc, gc->r.PC+1)];
  gc->r.PC += 3;
  return 0;
}

U8 LDG2(GC* gc) {   // 66 97
  gc->r.G = gc->mem[ReadWord(*gc, gc->r.PC+1)];
  gc->r.PC += 3;
  return 0;
}

U8 LDH2(GC* gc) {   // 66 98
  gc->r.H = gc->mem[ReadByte(*gc, gc->r.PC+1)];
  gc->r.PC += 2;
  return 0;
}

U8 LDL2(GC* gc) {   // 66 99
  gc->r.L = gc->mem[ReadByte(*gc, gc->r.PC+1)];
  gc->r.PC += 2;
  return 0;
}

U8 LDA1(GC* gc) {   // 66 41
  gc->r.A = *ReadReg(*gc, gc->mem[gc->r.PC+1]);
  gc->r.PC += 2;
  return 0;
}

U8 LDB1(GC* gc) {   // 66 42
  gc->r.B = *ReadReg(*gc, gc->mem[gc->r.PC+1]);
  gc->r.PC += 2;
  return 0;
}

U8 LDC1(GC* gc) {   // 66 43
  gc->r.C = *ReadReg(*gc, gc->mem[gc->r.PC+1]);
  gc->r.PC += 2;
  return 0;
}

U8 LDD1(GC* gc) {   // 66 44
  gc->r.D = *ReadReg(*gc, gc->mem[gc->r.PC+1]);
  gc->r.PC += 2;
  return 0;
}

U8 LDS1(GC* gc) {   // 66 45
  gc->r.S = *ReadReg(*gc, gc->mem[gc->r.PC+1]);
  gc->r.PC += 2;
  return 0;
}

U8 LDG1(GC* gc) {   // 66 46
  gc->r.G = *ReadReg(*gc, gc->mem[gc->r.PC+1]);
  gc->r.PC += 2;
  return 0;
}

U8 LDH1(GC* gc) {   // 66 47
  gc->r.H = *ReadReg(*gc, gc->mem[gc->r.PC+1]) % 256;
  gc->r.PC += 2;
  return 0;
}

U8 LDL1(GC* gc) {   // 66 48
  gc->r.L = *ReadReg(*gc, gc->mem[gc->r.PC+1]) % 256;
  gc->r.PC += 2;
  return 0;
}

U8 INT(GC* gc, bool ri) {   // C2
  char val;
  if (ri) {
    val = *ReadReg(*gc, ReadByte(*gc, gc->r.PC+1));
    printf("Register interrupt: %02X\n", val);
  }
  else {
    val = ReadByte(*gc, gc->r.PC+1);
  }
  switch (val) {
    case 0x00: {
      old_st_legacy;
      exit(StackPop(gc));
    }
    case 0x02: {
      putchar(StackPop(gc));
      break;
    }
    default:
      printf("Unknown interrupt %02X\n", val);
      return 1;
  }
  gc->r.PC += 2;
  return 0;
}

U8 INT0(GC* gc) {
  return INT(gc, false);
}

U8 INT1(GC* gc) {
  return INT(gc, true);
}

U8 NOP(GC* gc) {    // EA
  return 0;
}

U8 PG0F(GC*); // Page 0F - Stack operations
U8 PG10(GC*); // Page 10 - Register operations
U8 PG66(GC*); // Page 66 - Load/Store operations

// Page 00h instructions
U8 (*INSTS[256])() = {
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &PG0F ,
  &PG10 , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &PUSH0, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &PG66 , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &NOP  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK
};

U8 (*INSTS_PG0F[256])() = {
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &JMP0 , &JMP1 , &JMP2 , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &PUSH0, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &PUSH1, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &INT0 , &INT1 , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &CPUID, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK
};

U8 (*INSTS_PG10[256])() = {
  &ADDA1, &ADDB1, &ADDC1, &ADDD1, &ADDS1, &ADDG1, &ADDH1, &ADDL1, &ADDA0, &ADDB0, &ADDC0, &ADDD0, &ADDS0, &ADDG0, &ADDH0, &ADDL0,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LODSB, &STOSB, &LODGB, &STOGB, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK
};

U8 (*INSTS_PG66[256])() = {
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LDA0 , &LDB0 , &LDC0 , &LDD0 , &LDS0 , &LDG0 , &LDH0 , &LDL0 , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &LDA1 , &LDB1 , &LDC1 , &LDD1 , &LDS1 , &LDG1 , &LDH1 , &LDL1 , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &LDA2 , &LDB2 , &LDC2 , &LDD2 , &LDS2 , &LDG2 , &LDH2 , &LDL2 , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK
};

U8 PG0F(GC* gc) {   // 0FH
  gc->r.PC++;
  return (INSTS_PG0F[gc->mem[gc->r.PC]])(gc);
}

U8 PG10(GC* gc) {   // 10H
  gc->r.PC++;
  return (INSTS_PG10[gc->mem[gc->r.PC]])(gc);
}

U8 PG66(GC* gc) {   // 66H
  gc->r.PC++;
  return (INSTS_PG66[gc->mem[gc->r.PC]])(gc);
}

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
  printf("G:  %04X\n", gc.r.G);
  printf("H:  %02X\n", gc.r.H);
  printf("L:  %02X\n", gc.r.L);
  printf("SP: %04X\n", gc.r.SP);
  printf("BP: %04X\n", gc.r.BP);
  printf("PC: %04X\n", gc.r.PC);
}

U8 Exec(GC gc, const U32 memsize) {
  U8 exc = 0;
  while (!exc) {
    printf("\033[32m%04X\033[0m\n", gc.r.PC);
    getchar();
    printf("\033[32mExecuting\033[0m\n", gc.r.PC);
    exc = (INSTS[gc.mem[gc.r.PC]])(&gc);
    StackDump(gc);
    RegDump(gc);
  }
  return exc;
}

