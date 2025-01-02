// CPU identificator: GC16X
#include <cpu16/proc/std.h>
#include <cpu16/proc/interrupts.h>
#define ROMSIZE 65536 // Maximum for a 16-bit cpu
#define MEMSIZE 65536 // Maximum for a 16-bit cpu

struct gcregs {
  gcword A;   // Accumulator
  gcword B;   // Base
  gcword C;   // Counter
  gcword D;   // Data
  gcword S;   // Segment (address)
  gcword G;   // Segment #2 (address)
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
  gcbyte rom[ROMSIZE];
  gcbyte pin;
};
typedef struct GC16X GC;

gcbyte ReadByte(GC gc, U16 addr) {
  return gc.mem[addr];
}

gcword ReadWord(GC gc, U16 addr) {
  return (gc.mem[addr]) + (gc.mem[addr+1] << 8);
}

gcword ReadWordRev(GC gc, U16 addr) {
  return (gc.mem[addr] << 8) + (gc.mem[addr+1]);
}

gcword* ReadReg(GC* gc, U8 regid) {
  // PC register cannot be changed from
  // {REG} addressing instruction. It
  // can only be changed using JMP, CALL,
  // and other control flow instructions.
  U16* regids[10] = {
    &(gc->r.A), &(gc->r.B), &(gc->r.C), &(gc->r.D),
    &(gc->r.S), &(gc->r.G), &(gc->r.SP), &(gc->r.BP)
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
  gcrc_t rc = {clust/8, clust%8};
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
      fflush(stdout);
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
    case 0x0000: { // Get processor type
      gc->r.D = PROC_TYPE_GC16X;
      break;
    }
    case 0x0001: { // Get connected drive
      gc->r.D = ((gc->pin & 0b10000000) >> 7);
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
  gc->r.D = (*ReadReg(gc, rc.x) % *ReadReg(gc, rc.y)); // The remainder is always stored into D
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

U8 LDDS(GC* gc) {  // 10 8B
  gc->r.S = gc->rom[*gc, gc->r.S];
  gc->r.PC += 2;
  return 0;
}

U8 LDDG(GC* gc) {  // 10 9B
  gc->r.G = gc->rom[*gc, gc->r.G];
  gc->r.PC += 2;
  return 0;
}

U8 STDS(GC* gc) {  // 10 AB
  gc->rom[gc->r.S] = *ReadReg(gc, gc->r.PC+1);
  gc->r.PC += 2;
  return 0;
}

U8 STDG(GC* gc) {  // 10 BB
  gc->rom[gc->r.G] = *ReadReg(gc, gc->r.PC+1);
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

U8 CMP10(GC* gc) {  // 10 EE
  gc->r.SEI = (*ReadReg(gc, gc->mem[gc->r.PC+1]) == ReadWord(*gc, gc->r.PC+2));
  gc->r.PC += 4; // Set equal flag if a register and
  return 0;      // immediate are equal
}

U8 RC(GC* gc) {   // 23 - Return if carry set
  if (gc->r.SCI) gc->r.PC = StackPop(gc);
  else gc->r.PC++;
  return 0;
}

U8 RE(GC* gc) {   // 2B - Return if equal
  if (gc->r.SEI) gc->r.PC = StackPop(gc);
  else gc->r.PC++;
  return 0;
}

U8 RET(GC* gc) {   // 33
  gc->r.PC = StackPop(gc);
  return 0;
}

U8 RNE(GC* gc) {   // 2B - Return if not equal
  if (!gc->r.SEI) gc->r.PC = StackPop(gc);
  else gc->r.PC++;
  return 0;
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

U8 LDAZ(GC* gc) {   // 66 15 -- LDA Zero Page
  gc->r.A = gc->mem[gc->mem[gc->r.PC+1]];
  gc->r.PC += 2;
  return 0;
}

U8 LDBZ(GC* gc) {   // 66 16 -- LDB Zero Page
  gc->r.B = gc->mem[gc->mem[gc->r.PC+1]];
  gc->r.PC += 2;
  return 0;
}

U8 LDCZ(GC* gc) {   // 66 17 -- LDC Zero Page
  gc->r.C = gc->mem[gc->mem[gc->r.PC+1]];
  gc->r.PC += 2;
  return 0;
}

U8 LDDZ(GC* gc) {   // 66 18 -- LDD Zero Page
  gc->r.D = gc->mem[gc->mem[gc->r.PC+1]];
  gc->r.PC += 2;
  return 0;
}

U8 LDSZ(GC* gc) {   // 66 19 -- LDS Zero Page
  gc->r.S = gc->mem[gc->mem[gc->r.PC+1]];
  gc->r.PC += 2;
  return 0;
}

U8 LDGZ(GC* gc) {   // 66 1A -- LDG Zero Page
  gc->r.G = gc->mem[gc->mem[gc->r.PC+1]];
  gc->r.PC += 2;
  return 0;
}

U8 LDAZS(GC* gc) {   // 66 25 -- LDG Zero Page,S
  gc->r.A = gc->mem[gc->mem[gc->r.PC+1]+gc->r.S];
  gc->r.PC += 2;
  return 0;
}

U8 LDBZS(GC* gc) {   // 66 26 -- LDG Zero Page,S
  gc->r.B = gc->mem[gc->mem[gc->r.PC+1]+gc->r.S];
  gc->r.PC += 2;
  return 0;
}

U8 LDCZS(GC* gc) {   // 66 27 -- LDG Zero Page,S
  gc->r.C = gc->mem[gc->mem[gc->r.PC+1]+gc->r.S];
  gc->r.PC += 2;
  return 0;
}

U8 LDDZS(GC* gc) {   // 66 28 -- LDG Zero Page,S
  gc->r.D = gc->mem[gc->mem[gc->r.PC+1]+gc->r.S];
  gc->r.PC += 2;
  return 0;
}

U8 LDSZS(GC* gc) {   // 66 29 -- LDG Zero Page,S
  gc->r.S = gc->mem[gc->mem[gc->r.PC+1]+gc->r.S];
  gc->r.PC += 2;
  return 0;
}

U8 LDGZS(GC* gc) {   // 66 2A -- LDG Zero Page,S
  gc->r.G = gc->mem[gc->mem[gc->r.PC+1]+gc->r.S];
  gc->r.PC += 2;
  return 0;
}

U8 LDAZG(GC* gc) {   // 66 35 -- LDG Zero Page,G
  gc->r.A = gc->mem[gc->mem[gc->r.PC+1]+gc->r.G];
  gc->r.PC += 2;
  return 0;
}

U8 LDBZG(GC* gc) {   // 66 36 -- LDG Zero Page,G
  gc->r.B = gc->mem[gc->mem[gc->r.PC+1]+gc->r.G];
  gc->r.PC += 2;
  return 0;
}

U8 LDCZG(GC* gc) {   // 66 37 -- LDG Zero Page,G
  gc->r.C = gc->mem[gc->mem[gc->r.PC+1]+gc->r.G];
  gc->r.PC += 2;
  return 0;
}

U8 LDDZG(GC* gc) {   // 66 38 -- LDG Zero Page,G
  gc->r.D = gc->mem[gc->mem[gc->r.PC+1]+gc->r.G];
  gc->r.PC += 2;
  return 0;
}

U8 LDSZG(GC* gc) {   // 66 39 -- LDG Zero Page,G
  gc->r.S = gc->mem[gc->mem[gc->r.PC+1]+gc->r.G];
  gc->r.PC += 2;
  return 0;
}

U8 LDGZG(GC* gc) {   // 66 3A -- LDG Zero Page,G
  gc->r.G = gc->mem[gc->mem[gc->r.PC+1]+gc->r.G];
  gc->r.PC += 2;
  return 0;
}

U8 LDAA(GC* gc) {    // 66 55 -- LDA Absolute
  gc->r.A = gc->mem[ReadWord(*gc, gc->r.PC+1)];
  gc->r.PC += 3;
  return 0;
}

U8 LDBA(GC* gc) {    // 66 56 -- LDB Absolute
  gc->r.B = gc->mem[ReadWord(*gc, gc->r.PC+1)];
  gc->r.PC += 3;
  return 0;
}

U8 LDCA(GC* gc) {    // 66 57 -- LDC Absolute
  gc->r.C = gc->mem[ReadWord(*gc, gc->r.PC+1)];
  gc->r.PC += 3;
  return 0;
}

U8 LDDA(GC* gc) {    // 66 58 -- LDD Absolute
  gc->r.D = gc->mem[ReadWord(*gc, gc->r.PC+1)];
  gc->r.PC += 3;
  return 0;
}

U8 LDSA(GC* gc) {    // 66 59 -- LDS Absolute
  gc->r.S = gc->mem[ReadWord(*gc, gc->r.PC+1)];
  gc->r.PC += 3;
  return 0;
}

U8 LDGA(GC* gc) {    // 66 5A -- LDG Absolute
  gc->r.G = gc->mem[ReadWord(*gc, gc->r.PC+1)];
  gc->r.PC += 3;
  return 0;
}

U8 LDAAS(GC* gc) {   // 66 65 -- LDA Absolute,S
  gc->r.A = gc->mem[ReadWord(*gc, gc->r.PC+1)+gc->r.S];
  gc->r.PC += 3;
  return 0;
}

U8 LDBAS(GC* gc) {   // 66 66 -- LDB Absolute,S
  gc->r.B = gc->mem[ReadWord(*gc, gc->r.PC+1)+gc->r.S];
  gc->r.PC += 3;
  return 0;
}

U8 LDCAS(GC* gc) {   // 66 67 -- LDC Absolute,S
  gc->r.C = gc->mem[ReadWord(*gc, gc->r.PC+1)+gc->r.S];
  gc->r.PC += 3;
  return 0;
}

U8 LDDAS(GC* gc) {   // 66 68 -- LDD Absolute,S
  gc->r.D = gc->mem[ReadWord(*gc, gc->r.PC+1)+gc->r.S];
  gc->r.PC += 3;
  return 0;
}

U8 LDSAS(GC* gc) {   // 66 69 -- LDS Absolute,S
  gc->r.S = gc->mem[ReadWord(*gc, gc->r.PC+1)+gc->r.S];
  gc->r.PC += 3;
  return 0;
}

U8 LDGAS(GC* gc) {   // 66 6A -- LDG Absolute,S
  gc->r.G = gc->mem[ReadWord(*gc, gc->r.PC+1)+gc->r.S];
  gc->r.PC += 3;
  return 0;
}

U8 LDAAG(GC* gc) {   // 66 75 -- LDA Absolute,G
  gc->r.A = gc->mem[ReadWord(*gc, gc->r.PC+1)+gc->r.S];
  gc->r.PC += 3;
  return 0;
}

U8 LDBAG(GC* gc) {   // 66 76 -- LDB Absolute,G
  gc->r.B = gc->mem[ReadWord(*gc, gc->r.PC+1)+gc->r.S];
  gc->r.PC += 3;
  return 0;
}

U8 LDCAG(GC* gc) {   // 66 77 -- LDC Absolute,G
  gc->r.C = gc->mem[ReadWord(*gc, gc->r.PC+1)+gc->r.S];
  gc->r.PC += 3;
  return 0;
}

U8 LDDAG(GC* gc) {   // 66 78 -- LDD Absolute,G
  gc->r.D = gc->mem[ReadWord(*gc, gc->r.PC+1)+gc->r.S];
  gc->r.PC += 3;
  return 0;
}

U8 LDSAG(GC* gc) {   // 66 79 -- LDS Absolute,G
  gc->r.S = gc->mem[ReadWord(*gc, gc->r.PC+1)+gc->r.S];
  gc->r.PC += 3;
  return 0;
}

U8 LDGAG(GC* gc) {   // 66 7A -- LDG Absolute,G
  gc->r.G = gc->mem[ReadWord(*gc, gc->r.PC+1)+gc->r.S];
  gc->r.PC += 3;
  return 0;
}

U8 LDA0S(GC* gc) {   // 66 85 -- LDA Immediate,S
  gc->r.A = ReadWord(*gc, gc->r.PC+1)+gc->r.S;
  gc->r.PC += 3;
  return 0;
}

U8 LDB0S(GC* gc) {   // 66 86 -- LDB Immediate,S
  gc->r.B = ReadWord(*gc, gc->r.PC+1)+gc->r.S;
  gc->r.PC += 3;
  return 0;
}

U8 LDC0S(GC* gc) {   // 66 87 -- LDC Immediate,S
  gc->r.C = ReadWord(*gc, gc->r.PC+1)+gc->r.S;
  gc->r.PC += 3;
  return 0;
}

U8 LDD0S(GC* gc) {   // 66 88 -- LDD Immediate,S
  gc->r.D = ReadWord(*gc, gc->r.PC+1)+gc->r.S;
  gc->r.PC += 3;
  return 0;
}

U8 LDS0S(GC* gc) {   // 66 89 -- LDS Immediate,S
  gc->r.S = ReadWord(*gc, gc->r.PC+1)+gc->r.S;
  gc->r.PC += 3;
  return 0;
}

U8 LDG0S(GC* gc) {   // 66 8A -- LDG Immediate,S
  gc->r.G = ReadWord(*gc, gc->r.PC+1)+gc->r.S;
  gc->r.PC += 3;
  return 0;
}

U8 LDA0G(GC* gc) {   // 66 95 -- LDA Immediate,G
  gc->r.A = ReadWord(*gc, gc->r.PC+1)+gc->r.G;
  gc->r.PC += 3;
  return 0;
}

U8 LDB0G(GC* gc) {   // 66 96 -- LDB Immediate,G
  gc->r.B = ReadWord(*gc, gc->r.PC+1)+gc->r.G;
  gc->r.PC += 3;
  return 0;
}

U8 LDC0G(GC* gc) {   // 66 97 -- LDC Immediate,G
  gc->r.C = ReadWord(*gc, gc->r.PC+1)+gc->r.G;
  gc->r.PC += 3;
  return 0;
}

U8 LDD0G(GC* gc) {   // 66 98 -- LDD Immediate,G
  gc->r.D = ReadWord(*gc, gc->r.PC+1)+gc->r.G;
  gc->r.PC += 3;
  return 0;
}

U8 LDS0G(GC* gc) {   // 66 99 -- LDS Immediate,G
  gc->r.S = ReadWord(*gc, gc->r.PC+1)+gc->r.G;
  gc->r.PC += 3;
  return 0;
}

U8 LDG0G(GC* gc) {   // 66 9A -- LDG Immediate,G
  gc->r.G = ReadWord(*gc, gc->r.PC+1)+gc->r.G;
  gc->r.PC += 3;
  return 0;
}

U8 LDA1(GC* gc) {   // 66 A5
  gc->r.A = *ReadReg(gc, gc->mem[gc->r.PC+1]);
  gc->r.PC += 2;
  return 0;
}

U8 LDB1(GC* gc) {   // 66 A6
  gc->r.B = *ReadReg(gc, gc->mem[gc->r.PC+1]);
  gc->r.PC += 2;
  return 0;
}

U8 LDC1(GC* gc) {   // 66 A7
  gc->r.C = *ReadReg(gc, gc->mem[gc->r.PC+1]);
  gc->r.PC += 2;
  return 0;
}

U8 LDD1(GC* gc) {   // 66 A8
  gc->r.D = *ReadReg(gc, gc->mem[gc->r.PC+1]);
  gc->r.PC += 2;
  return 0;
}

U8 LDS1(GC* gc) {   // 66 A9
  gc->r.S = *ReadReg(gc, gc->mem[gc->r.PC+1]);
  gc->r.PC += 2;
  return 0;
}

U8 LDG1(GC* gc) {   // 66 AA
  gc->r.G = *ReadReg(gc, gc->mem[gc->r.PC+1]);
  gc->r.PC += 2;
  return 0;
}

U8 BHCl(GC* gc) {   // 83 00 - 83 07
  *ReadReg(gc, gc->mem[gc->r.PC]) &= 0b0000000011111111;
  gc->r.PC++;
  return 0;
}

U8 BLCl(GC* gc) {   // 83 08 - 83 0F
  *ReadReg(gc, gc->mem[gc->r.PC]-0x08) &= 0b1111111100000000;
  gc->r.PC++;
  return 0;
}

U8 DEXM(GC* gc) {   // 90
  gc->mem[ReadWord(*gc, gc->r.PC+1)]--;
  gc->r.PC += 3;
  return 0;
}

U8 ASL(GC* gc) {    // A0-A7
  puts("asl %a");
  U16* rgptr = ReadReg(gc, gc->mem[gc->r.PC]-0xA0) 
  *rgptr = *rgptr << gc->mem[gc->r.PC+1];
  gc->r.PC += 2;
  return 0;
}

U8 ASR(GC* gc) {    // E0-E8
  *ReadReg(gc, gc->mem[gc->r.PC]-0xE0) >> gc->mem[gc->r.PC+1];
  gc->r.PC += 2;
  return 0;
}

U8 INXM(GC* gc) {   // B0
  gc->mem[ReadWord(*gc, gc->r.PC+1)]++;
  gc->r.PC += 3;
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

U8 CALL(GC* gc) {   // C7
  StackPush(gc, gc->r.PC+3);
  gc->r.PC = ReadWord(*gc, gc->r.PC+1);
  return 0;
}

U8 COP1(GC* gc) {   // D7
  *ReadReg(gc, gc->mem[gc->r.PC+1]) = StackPop(gc);
  gc->r.SP -= 2;
  gc->r.PC += 2;
  return 0;
}

U8 NOP(GC* gc) { // EA
  return 0;
}

U8 PG0F(GC*); // Page 0F - Stack operations
U8 PG10(GC*); // Page 10 - Register operations
U8 PG66(GC*); // Page 66 - Load/Store operations
U8 PG83(GC*); // Page 83 - Load/Store byte operations

// Zero page instructions
U8 (*INSTS[256])() = {
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &PG0F ,
  &PG10 , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &RC   , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &RE   , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &RET  , &STI  , &UNK  , &CLC  , &UNK  , &UNK  , &UNK  , &UNK  , &RNE  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &HLT  , &CLI  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &PG66 , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &PG83 , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &DEXM , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &ASL  , &ASL  , &ASL  , &ASL  , &ASL  , &ASL  , &ASL  , &ASL  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &INXM , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LOOP , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &CALL , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &ASL  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &COP1 , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &ASR  , &ASR  , &ASR  , &ASR  , &ASR  , &ASR  , &ASR  , &ASR  , &UNK  , &UNK  , &NOP  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
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
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LDA0 , &LDB0 , &LDC0 , &LDD0 , &LDS0 , &LDG0 , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LDAZ , &LDBZ , &LDCZ , &LDDZ , &LDSZ , &LDGZ , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LDAZS, &LDBZS, &LDCZS, &LDDZS, &LDSZS, &LDGZS, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LDAZG, &LDBZG, &LDCZG, &LDDZG, &LDSZG, &LDGZG, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LDAA , &LDBA , &LDCA , &LDDA , &LDSA , &LDGA , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LDAAS, &LDBAS, &LDCAS, &LDDAS, &LDSAS, &LDGAS, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LDAAG, &LDBAG, &LDCAG, &LDDAG, &LDSAG, &LDGAG, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LDA0S, &LDB0S, &LDC0S, &LDD0S, &LDS0S, &LDG0S, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LDA0G, &LDB0G, &LDC0G, &LDD0G, &LDS0G, &LDG0G, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LDA1 , &LDB1 , &LDC1 , &LDD1 , &LDS1 , &LDG1 , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK
};

U8 (*INSTS_PG83[256])() = {
  &BHCl , &BHCl , &BHCl , &BHCl , &BHCl , &BHCl , &BHCl , &BHCl , &BLCl , &BLCl , &BLCl , &BLCl , &BLCl , &BLCl , &BLCl , &BLCl ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
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

U8 PG83(GC* gc) {   // 83H
  gc->r.PC++;
  return (INSTS_PG83[gc->mem[gc->r.PC]])(gc);
}

U0 Reset(GC* gc, U16 driveboot) {
  gc->r.SP = 0x1000;
  gc->r.BP = 0x1000;
  gc->r.PC = 0x0000;
  
  gc->r.STI = 0x00;
  gc->r.SCI = 0x00;
}

U0 PageDump(GC gc, U8 page) {
  for (U16 i = (page*256); i < (page*256)+256; i++) {
    if (!(i % 16)) putchar(10);
    printf("%02X ", gc.mem[i]);
  }
}

U0 StackDump(GC gc, U16 c) {
  printf("SP: %04X\n", gc.r.SP);
  for (U16 i = 0x1000; i > 0x1000-c; i--) {
    printf("%04X: %02X\n", i, gc.mem[i]);
  }
}

U0 RegDump(GC gc) {
  printf("\033[1;20H\033[44mA  %04X\033[0m\n", gc.r.A);
  printf("\033[2;20H\033[44mB  %04X\033[0m\n", gc.r.B);
  printf("\033[3;20H\033[44mC  %04X\033[0m\n", gc.r.C);
  printf("\033[4;20H\033[44mD  %04X\033[0m\n", gc.r.D);
  printf("\033[5;20H\033[44mS  %04X\033[0m ASCII: %c\n", gc.r.S, gc.r.S);
  printf("\033[6;20H\033[44mG  %04X\033[0m ASCII: %c\n", gc.r.G, gc.r.G);
  printf("\033[9;20H\033[44mSP %04X\033[0m\n", gc.r.SP);
  printf("\033[10;20H\033[44mBP %04X\033[0m\n", gc.r.BP);
  printf("\033[11;20H\033[44mPC %04X\033[0m\n", gc.r.PC);
}

U8 Exec(GC gc, const U32 memsize) {
  U8 exc = 0;
  while (!exc) {
    exc = (INSTS[gc.mem[gc.r.PC]])(&gc);
    getchar();
    fputs("\033[H\033[2J", stdout);
    StackDump(gc, 10);
    RegDump(gc);
    /*
    for (U32 i = 0; i < 0x20; i++) {
      printf("%02X ", gc.mem[0x619 + i]);
    }
    printf("  \033[32m%04X %04X\033[0m\n", gc.r.S, gc.r.G);
    */
    /*
    printf("PC: %04X\n", gc.r.PC);
    puts("\0");
    */
  }
  return exc;
}

