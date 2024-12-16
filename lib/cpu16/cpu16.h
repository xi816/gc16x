// CPU identificator: GC16X
#include <cpu16/proc/std.h>
#include <cpu16/proc/interrupts.h>
#define MEMSIZE 65536 // Maximum for a 16-bit cpu

struct gcregs {
  gcword A;   // Accumulator
  gcword B;   // Base
  gcword C;   // Counter
  gcword D;   // Data
  gcword S;   // Segment (address)
  gcword G;   // Segment #2 (address)
  gcbyte H;   // High 8 bits
  gcbyte L;   // Low 8 bits
  gcbyte STI; // Interrupt flag
  gcbyte SEI; // Equal flag
  gcbyte SCI; // Carry flag
  gcword SP;  // Stack pointer
  gcword BP;  // Base pointer
  gcword PC;  // Program counter
};
struct gcrc {
  gcbyte x;
  gcbyte y;
};
typedef struct gcregs gcregs_t;
typedef struct gcrc gcrc_t;

struct GC16X {
  gcregs_t r;
  gcbyte mem[MEMSIZE];
};
typedef struct GC16X GC;

gcbyte ReadByte(GC gc, U16 addr) {
  return gc.mem[addr];
}

gcword ReadWord(GC gc, U16 addr) {
  return (gc.mem[addr]) + (gc.mem[addr+1] << 8);
}

gcword* ReadReg(GC* gc, U8 regid) {
  // PC register cannot be changed from
  // {REG} addressing instruction. It
  // can only be changed using JMP, CALL,
  // and other control flow instructions.
  U16* regids[10] = {
    &(gc->r.A), &(gc->r.B), &(gc->r.C), &(gc->r.D),
    &(gc->r.S), &(gc->r.G), (U16*)&(gc->r.H),
    (U16*)&(gc->r.L), &(gc->r.SP), &(gc->r.BP)
  };
  return regids[regid];
}

gcbyte StackPush(GC* gc, U16 val) {
  gc->mem[gc->r.SP--] = (val >> 8);
  gc->mem[gc->r.SP--] = (val % 256);
  return 0;
}

gcword StackPop(GC* gc) {
  gc->r.SP += 2;
  return ReadWord(*gc, gc->r.SP-1);
}

// Register clusters can only be used for reading data,
// writing is not allowed.
gcrc_t ReadRegClust(U8 clust) { // Read a register cluster
  gcrc_t rc = {clust/10, clust%10};
  return rc;
}

U8 UNK(GC* gc) {    // Unknown instruction
  fprintf(stderr, "Unknown instruction %02X\nAt position %04X\n", gc->mem[gc->r.PC], gc->r.PC);
  old_st_legacy;
  return 1;
}

U8 JME0(GC* gc) {   // 0F 29
  if (gc->r.SEI) {
    gc->r.PC = ReadWord(*gc, gc->r.PC+1);
    gc->r.SEI = 0x00;
  }
  else {
    gc->r.PC += 3;
  }
  return 0;
}

U8 JMNE0(GC* gc) {   // 0F 2A
  if (!gc->r.SEI) {
    gc->r.PC = ReadWord(*gc, gc->r.PC+1);
  }
  else {
    gc->r.PC += 3;
  }
  return 0;
}

U8 JMP0(GC* gc) {   // 0F 30
  gc->r.PC = ReadWord(*gc, gc->r.PC+1);
  return 0;
}

U8 JMP1(GC* gc) {   // 0F 31
  gc->r.PC = *ReadReg(gc, gc->mem[gc->r.PC+1]);
  return 0;
}

U8 JMP2(GC* gc) {   // 0F 32
  gc->r.PC = gc->mem[ReadWord(*gc, gc->r.PC+1)];
  return 0;
}

U8 POP1(GC* gc) {   // 0F 80
  *ReadReg(gc, gc->mem[gc->r.PC+1]) = StackPop(gc);
  gc->r.PC += 2;
  return 0;
}

U8 PUSH0(GC* gc) {  // 0F 84
  StackPush(gc, ReadWord(*gc, gc->r.PC+1));
  gc->r.PC += 3;
  return 0;
}

U8 PUSH1(GC* gc) {  // 0F 90
  StackPush(gc, *ReadReg(gc, gc->mem[gc->r.PC+1]));
  gc->r.PC += 2;
  return 0;
}

U8 INT(GC* gc, bool ri) {
  if (gc->r.STI) return 0;
  U8 val;
  if (ri) {
    val = *ReadReg(gc, ReadByte(*gc, gc->r.PC+1));
  }
  else {
    val = ReadByte(*gc, gc->r.PC+1);
  }
  switch (val) {
    case INT_EXIT: {
      old_st_legacy;
      exit(StackPop(gc));
    }
    case INT_READ: {
      StackPush(gc, getchar());
      break;
    }
    case INT_WRITE: {
      putchar(StackPop(gc));
      break;
    }
    default:
      printf("Illegal interrupt %02X\n", val);
      return 1;
  }
  gc->r.PC += 2;
  return 0;
}

U8 INT0(GC* gc) { // 0F C2
  return INT(gc, false);
}

U8 INT1(GC* gc) { // 0F C3
  return INT(gc, true);
}

U8 CPUID(GC* gc) {  // 0F E9
  switch (gc->r.D) {
    case 0x0000: {
      gc->r.D = PROC_TYPE_GC16X;
      break;
    }
    default:
      fprintf(stderr, "Illegal CPUID value: %04X\n", gc->r.D);
      return 1;
  }
  gc->r.PC += 1;
  return 0;
}

U8 ADD11(GC* gc) {  // 10 00
  gcrc_t rc = ReadRegClust(gc->mem[gc->r.PC+1]);
  *ReadReg(gc, rc.x) += *ReadReg(gc, rc.y);
  gc->r.PC += 2;
  return 0;
}

U8 SUB11(GC* gc) {  // 10 01
  gcrc_t rc = ReadRegClust(gc->mem[gc->r.PC+1]);
  *ReadReg(gc, rc.x) -= *ReadReg(gc, rc.y);
  gc->r.PC += 2;
  return 0;
}

U8 MUL11(GC* gc) {  // 10 02
  gcrc_t rc = ReadRegClust(gc->mem[gc->r.PC+1]);
  *ReadReg(gc, rc.x) *= *ReadReg(gc, rc.y);
  gc->r.PC += 2;
  return 0;
}

U8 DIV11(GC* gc) {  // 10 03
  gcrc_t rc = ReadRegClust(gc->mem[gc->r.PC+1]);
  *ReadReg(gc, rc.x) /= *ReadReg(gc, rc.y);
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
  gc->r.H += ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 ADDL0(GC* gc) {  // 10 0F
  gc->r.L += ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 SUBA0(GC* gc) {  // 10 18
  gc->r.A -= ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 SUBB0(GC* gc) {  // 10 19
  gc->r.B -= ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 SUBC0(GC* gc) {  // 10 1A
  gc->r.C += ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 SUBD0(GC* gc) {  // 10 1B
  gc->r.D -= ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 SUBS0(GC* gc) {  // 10 1C
  gc->r.S -= ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 SUBG0(GC* gc) {  // 10 1D
  gc->r.G -= ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 SUBH0(GC* gc) {  // 10 1E
  gc->r.H -= ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 SUBL0(GC* gc) {  // 10 1F
  gc->r.L -= ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 MULA0(GC* gc) {  // 10 28
  gc->r.A *= ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 MULB0(GC* gc) {  // 10 29
  gc->r.B *= ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 MULC0(GC* gc) {  // 10 2A
  gc->r.C *= ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 MULD0(GC* gc) {  // 10 2B
  gc->r.D *= ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 MULS0(GC* gc) {  // 10 2C
  gc->r.S *= ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 MULG0(GC* gc) {  // 10 2D
  gc->r.G *= ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 MULH0(GC* gc) {  // 10 2E
  gc->r.H *= ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 MULL0(GC* gc) {  // 10 2F
  gc->r.L *= ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 DIVA0(GC* gc) {  // 10 38
  gc->r.A /= ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 DIVB0(GC* gc) {  // 10 39
  gc->r.B /= ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 DIVC0(GC* gc) {  // 10 3A
  gc->r.C /= ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 DIVD0(GC* gc) {  // 10 3B
  gc->r.D /= ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 DIVS0(GC* gc) {  // 10 3C
  gc->r.S /= ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 DIVG0(GC* gc) {  // 10 3D
  gc->r.G /= ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 DIVH0(GC* gc) {  // 10 3E
  gc->r.H /= ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 DIVL0(GC* gc) {  // 10 3F
  gc->r.L /= ReadWord(*gc, gc->r.PC+1);
  gc->r.PC += 3;
  return 0;
}

U8 STORB(GC* gc) {  // 10 80
  gc->mem[gc->r.S] = *ReadReg(gc, gc->mem[gc->r.PC+1]);
  gc->r.PC += 2;
  return 0;
}

U8 STGRB(GC* gc) {  // 10 81
  gc->mem[gc->r.G] = *ReadReg(gc, gc->mem[gc->r.PC+1]);
  gc->r.PC += 2;
  return 0;
}

U8 LODSB(GC* gc) {  // 10 87
  gc->r.S = ReadByte(*gc, gc->r.S);
  gc->r.PC++;
  return 0;
}

U8 LODGB(GC* gc) {  // 10 88
  gc->r.G = ReadByte(*gc, gc->r.G);
  gc->r.PC++;
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

U8 INXA(GC* gc) {   // 10 C0
  gc->r.A++;
  gc->r.PC++;
  return 0;
}

U8 INXB(GC* gc) {   // 10 C1
  gc->r.B++;
  gc->r.PC++;
  return 0;
}

U8 INXC(GC* gc) {   // 10 C2
  gc->r.C++;
  gc->r.PC++;
  return 0;
}

U8 INXD(GC* gc) {   // 10 C3
  gc->r.D++;
  gc->r.PC++;
  return 0;
}

U8 INXS(GC* gc) {   // 10 C4
  gc->r.S++;
  gc->r.PC++;
  return 0;
}

U8 INXG(GC* gc) {   // 10 C5
  gc->r.G++;
  gc->r.PC++;
  return 0;
}

U8 INXH(GC* gc) {   // 10 C6
  gc->r.H++;
  gc->r.PC++;
  return 0;
}

U8 INXL(GC* gc) {   // 10 C7
  gc->r.L++;
  gc->r.PC++;
  return 0;
}

U8 DEXA(GC* gc) {   // 10 D0
  gc->r.A--;
  gc->r.PC++;
  return 0;
}

U8 DEXB(GC* gc) {   // 10 D1
  gc->r.B--;
  gc->r.PC++;
  return 0;
}

U8 DEXC(GC* gc) {   // 10 D2
  gc->r.C--;
  gc->r.PC++;
  return 0;
}

U8 DEXD(GC* gc) {   // 10 D3
  gc->r.D--;
  gc->r.PC++;
  return 0;
}

U8 DEXS(GC* gc) {   // 10 D4
  gc->r.S--;
  gc->r.PC++;
  return 0;
}

U8 DEXG(GC* gc) {   // 10 D5
  gc->r.G--;
  gc->r.PC++;
  return 0;
}

U8 DEXH(GC* gc) {   // 10 D6
  gc->r.H--;
  gc->r.PC++;
  return 0;
}

U8 DEXL(GC* gc) {   // 10 D7
  gc->r.L--;
  gc->r.PC++;
  return 0;
}

U8 AND11(GC* gc) {  // 10 D8
  gcrc_t rc = ReadRegClust(gc->mem[gc->r.PC+1]);
  *ReadReg(gc, rc.x) &= *ReadReg(gc, rc.y);
  gc->r.PC += 2;
  return 0;
}

U8 OR11(GC* gc) {  // 10 D9
  gcrc_t rc = ReadRegClust(gc->mem[gc->r.PC+1]);
  *ReadReg(gc, rc.x) |= *ReadReg(gc, rc.y);
  gc->r.PC += 2;
  return 0;
}

U8 CMP11(GC* gc) {  // 10 F6
  gcrc_t rc = ReadRegClust(gc->mem[gc->r.PC+1]);
  gc->r.SEI = (*ReadReg(gc, rc.x) == *ReadReg(gc, rc.y));
  gc->r.PC += 2; // Set equal flag if two register values
  return 0;      // are equal
}

U8 STI(GC* gc) {   // 34
  gc->r.STI = 0x01;
  gc->r.PC++;
  return 0;
}

U8 CLC(GC* gc) {   // 36
  gc->r.SCI = 0x00;
  gc->r.PC++;
  return 0;
}

U8 HLT(GC* gc) {   // 51
  while(1) {}
  gc->r.PC++;
  return 0;
}

U8 CLI(GC* gc) {   // 52
  gc->r.STI = 0x00;
  gc->r.PC++;
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
  gc->r.A = *ReadReg(gc, gc->mem[gc->r.PC+1]);
  gc->r.PC += 2;
  return 0;
}

U8 LDB1(GC* gc) {   // 66 42
  gc->r.B = *ReadReg(gc, gc->mem[gc->r.PC+1]);
  gc->r.PC += 2;
  return 0;
}

U8 LDC1(GC* gc) {   // 66 43
  gc->r.C = *ReadReg(gc, gc->mem[gc->r.PC+1]);
  gc->r.PC += 2;
  return 0;
}

U8 LDD1(GC* gc) {   // 66 44
  gc->r.D = *ReadReg(gc, gc->mem[gc->r.PC+1]);
  gc->r.PC += 2;
  return 0;
}

U8 LDS1(GC* gc) {   // 66 45
  gc->r.S = *ReadReg(gc, gc->mem[gc->r.PC+1]);
  gc->r.PC += 2;
  return 0;
}

U8 LDG1(GC* gc) {   // 66 46
  gc->r.G = *ReadReg(gc, gc->mem[gc->r.PC+1]);
  gc->r.PC += 2;
  return 0;
}

U8 LDH1(GC* gc) {   // 66 47
  gc->r.H = *ReadReg(gc, gc->mem[gc->r.PC+1]) % 256;
  gc->r.PC += 2;
  return 0;
}

U8 LDL1(GC* gc) {   // 66 48
  gc->r.L = *ReadReg(gc, gc->mem[gc->r.PC+1]) % 256;
  gc->r.PC += 2;
  return 0;
}

U8 LOOP(GC* gc) {   // B8
  if (gc->r.C != gc->r.D) {
    gc->r.C--;
    gc->r.PC = ReadWord(*gc, gc->r.PC+1);
  }
  else {
    gc->r.PC += 3;
  }
  return 0;
}

U8 NOP(GC* gc) { // EA
  return 0;
}

U8 PG0F(GC*); // Page 0F - Stack operations
U8 PG10(GC*); // Page 10 - Register operations
U8 PG66(GC*); // Page 66 - Load/Store operations

// Zero page instructions
U8 (*INSTS[256])() = {
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &PG0F ,
  &PG10 , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &STI  , &UNK  , &CLC  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &HLT  , &CLI  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &PG66 , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LOOP , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &NOP  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK
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
  &POP1 , &UNK  , &UNK  , &UNK  , &PUSH0, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &PUSH1, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &INT0 , &INT1 , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &CPUID, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK
};

U8 (*INSTS_PG10[256])() = {
  &ADD11, &SUB11, &MUL11, &DIV11, &UNK  , &UNK  , &UNK  , &UNK  , &ADDA0, &ADDB0, &ADDC0, &ADDD0, &ADDS0, &ADDG0, &ADDH0, &ADDL0,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &SUBA0, &SUBB0, &SUBC0, &SUBD0, &SUBS0, &SUBG0, &SUBH0, &SUBL0,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &MULA0, &MULB0, &MULC0, &MULD0, &MULS0, &MULG0, &MULH0, &MULL0,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &DIVA0, &DIVB0, &DIVC0, &DIVD0, &DIVS0, &DIVG0, &DIVH0, &DIVL0,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &STORB, &STGRB, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LODSB, &STOSB, &LODGB, &STOGB, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &INXA , &INXB , &INXC , &INXD , &INXS , &INXG , &INXH , &INXL , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &DEXA , &DEXB , &DEXC , &DEXD , &DEXS , &DEXG , &DEXH , &DEXL , &AND11, &OR11 , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &CMP11, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK
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
  
  gc->r.STI = 0x00;
  gc->r.SCI = 0x00;
}

U0 StackDump(GC gc, U16 c) {
  printf("SP: %04X\n", gc.r.SP);
  for (U16 i = 0x1000; i > 0x1000-c; i--) {
    printf("%04X: %02X\n", i, gc.mem[i]);
  }
}

U0 RegDump(GC gc) {
  printf("\033[44mA  %04X\033[0m\n", gc.r.A);
  printf("\033[44mB  %04X\033[0m\n", gc.r.B);
  printf("\033[44mC  %04X\033[0m\n", gc.r.C);
  printf("\033[44mD  %04X\033[0m\n", gc.r.D);
  printf("\033[44mS  %04X\033[0m\n", gc.r.S);
  printf("\033[44mG  %04X\033[0m\n", gc.r.G);
  printf("\033[44mH  %02X\033[0m\n", gc.r.H);
  printf("\033[44mL  %02X\033[0m\n", gc.r.L);
  printf("\033[44mSP %04X\033[0m\n", gc.r.SP);
  printf("\033[44mBP %04X\033[0m\n", gc.r.BP);
  printf("\033[44mPC %04X\033[0m\n", gc.r.PC);
}

U8 Exec(GC gc, const U32 memsize) {
  U8 exc = 0;
  while (!exc) {
    exc = (INSTS[gc.mem[gc.r.PC]])(&gc);
  }
  return exc;
}

