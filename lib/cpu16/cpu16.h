// CPU identificator: GC16X
#include <cpu16/proc/std.h>
#include <cpu16/proc/interrupts.h>
#include <cpu16/gpu.h>

#ifdef TRAKTORIST_MM
  #undef MEMSIZE
  #define MEMSIZE 300000
#endif

// CPU info:
// Speed: 1THz
// State: Holy

#define AX 0x00
#define BX 0x01
#define CX 0x02
#define DX 0x03
#define SI 0x04
#define GI 0x05
#define SP 0x06
#define BP 0x07
#define EX 0x08
#define FX 0x09
#define HX 0x0A
#define LX 0x0B
#define X  0x0C
#define Y  0x0D
#define IX 0x0E
#define IY 0x0F

#define IF(ps) (ps & 0b01000000)
#define ZF(ps) (ps & 0b00000100)
#define NF(ps) (ps & 0b00000010)
#define CF(ps) (ps & 0b00000001)

#define SET_IF(ps) (ps |= 0b01000000)
#define SET_ZF(ps) (ps |= 0b00000100)
#define SET_NF(ps) (ps |= 0b00000010)
#define SET_CF(ps) (ps |= 0b00000001)

#define RESET_IF(ps) (ps &= 0b10111111)
#define RESET_ZF(ps) (ps &= 0b11111011)
#define RESET_NF(ps) (ps &= 0b11111101)
#define RESET_CF(ps) (ps &= 0b11111110)

U8 errno;

U0 Reset(GC* gc);
U0 PageDump(GC* gc, U8 page);
U0 StackDump(GC* gc, U16 c);
U0 RegDump(GC* gc);

gcbyte ReadByte(GC* gc, U16 addr) {
  return gc->mem[addr];
}

gcword ReadWord(GC* gc, U16 addr) {
  return (gc->mem[addr]) + (gc->mem[addr+1] << 8);
}

gcword ReadWordRev(GC* gc, U16 addr) {
  return (gc->mem[addr] << 8) + (gc->mem[addr+1]);
}

U0 WriteWord(GC* gc, U16 addr, U16 val) {
  // Least significant byte goes first
  // 1448 -> $48,$14
  gc->mem[addr] = (val % 256);
  gc->mem[addr+1] = (val >> 8);
}

gcword* ReadReg(GC* gc, U8 regid) {
  return &gc->reg[regid].word;
}

gcbyte StackPush(GC* gc, U16 val) {
  gc->mem[gc->reg[SP].word--] = (val >> 8);
  gc->mem[gc->reg[SP].word--] = (val % 256);
  return 0;
}

gcword StackPop(GC* gc) {
  gc->reg[SP].word += 2;
  return ReadWord(gc, gc->reg[SP].word-1);
}

gcrc_t ReadRegClust(U8 clust) { // Read a register cluster
  // The register address is 4 bytes
  gcrc_t rc = {((clust&0b11110000)>>4), (clust&0b00001111)};
  return rc;
}

U8 UNK(GC* gc) {    // Unknown instruction
  if (gc->mem[gc->PC-1] == 0x0F) {
    fprintf(stderr, "From page $0F\n");
  }
  else if (gc->mem[gc->PC-1] == 0x10) {
    fprintf(stderr, "From page $10\n");
  }
  else if (gc->mem[gc->PC-1] == 0x66) {
    fprintf(stderr, "This page shouln't exist\n");
    fprintf(stderr, "From the shitty page\n");
  }
  fprintf(stderr, "\033[31mIllegal\033[0m instruction \033[33m%02X\033[0m\nAt position %04X\n", gc->mem[gc->PC], gc->PC);
  old_st_legacy;
  errno = 1;
  return 1;
}

// 0F 29 -- Jump if zero flag set to imm16 address
U8 JME0(GC* gc) {
  if (ZF(gc->PS)) { gc->PC = ReadWord(gc, gc->PC+1); RESET_ZF(gc->PS); }
  else { gc->PC += 3; }
  return 0;
}

// 0F 2A -- Jump set to imm16 address if zero flag not
U8 JMNE0(GC* gc) {
  if (!ZF(gc->PS)) { gc->PC = ReadWord(gc, gc->PC+1); }
  else { gc->PC += 3; }
  return 0;
}

// 0F BB -- Jump to imm16 address if negative flag set
U8 JL0(GC* gc) {
  if (NF(gc->PS)) { gc->PC = ReadWord(gc, gc->PC+1); gc->PS &= 0b11111101; }
  else { gc->PC += 3; }
  return 0;
}

// 0F CB -- Jump to imm16 address if negative flag set
U8 JG0(GC* gc) {
  if (!NF(gc->PS)) { gc->PC = ReadWord(gc, gc->PC+1); RESET_NF(gc->PS); }
  else { gc->PC += 3; }
  return 0;
}

// 0F 30 -- Jump to imm16 address unconditionally
U8 JMP0(GC* gc) {
  gc->PC = ReadWord(gc, gc->PC+1);
  return 0;
}

// 0F 31 -- Jump to *reg16 address unconditionally
U8 JMP1(GC* gc) {
  gc->PC = gc->reg[gc->mem[gc->PC+1]].word;
  return 0;
}

// 0F 32 -- Jump to m16 address (*m16) unconditionally
U8 JMP2(GC* gc) {
  gc->PC = gc->mem[ReadWord(gc, gc->PC+1)];
  return 0;
}

// 0F 80 -- Pop value from stack into a register
U8 POP1(GC* gc) {   // 0F 80
  *ReadReg(gc, gc->mem[gc->PC+1]) = StackPop(gc);
  gc->PC += 2;
  return 0;
}

// 0F 84 - Push imm16
U8 PUSH0(GC* gc) {
  StackPush(gc, ReadWord(gc, gc->PC+1));
  gc->PC += 3;
  return 0;
}

// 0F 90 - push reg16 - Push a register
U8 PUSH1(GC* gc) {
  StackPush(gc, *ReadReg(gc, gc->mem[gc->PC+1]));
  gc->PC += 2;
  return 0;
}

// 0F 82 - push *reg16 - Push a value from memory from an address as a register
U8 PUSHp(GC* gc) {
  StackPush(gc, gc->mem[*ReadReg(gc, gc->mem[gc->PC+1])]);
  gc->PC += 2;
  return 0;
}

// Interrupt calls
U8 INT(GC* gc, bool ri) {
  if (!IF(gc->PS & 0b01000000)) {
    gc->PC += 2;
    return 0;
  }
  U8 val;
  if (ri) {
    val = *ReadReg(gc, ReadByte(gc, gc->PC+1));
  }
  else {
    val = ReadByte(gc, gc->PC+1);
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
    case INT_RESET: {
      Reset(gc);
      return 0;
      break;
    }
    case INT_VIDEO_WRITE: {
      GGtype(&(gc->gg), gc->renderer, gc->reg[SI].word, (U8)gc->reg[AX].word);
      break;
    }
    case INT_VIDEO_FLUSH: {
      GGpage(&(gc->gg), gc->renderer);
      break;
    }
    case INT_RAND: {
      gc->reg[DX].word = rand() % 65536;
      break;
    }
    case INT_DATE: {
      gc->reg[DX].word = GC_GOVNODATE();
      break;
    }
    case INT_WAIT: {
      usleep((U32)(gc->reg[DX].word)*1000); // the maximum is about 65.5 seconds
      break;
    }
    default:
      printf("Illegal interrupt %02X\n", val);
      return 1;
  }
  gc->PC += 2;
  return 0;
}

// 0F C3 - Call an interrupt from imm16
U8 INT0(GC* gc) { // 0F C2
  return INT(gc, false);
}

// 0F C3 - Call an interrupt from reg16
U8 INT1(GC* gc) {
  return INT(gc, true);
}

// 0F 9D - Trap and show the stack and registers (debug)
U8 TRAP(GC* gc) {
  // printf("\n\033[31mTrapped\033[0m at \033[33m%04X\033[0m\n", gc->PC);
  // StackDump(gc, 20);
  // RegDump(gc);
  // puts("-- Press any key to continue --");
  // getchar();
  old_st_legacy;
  ExecD(gc, 1);
  gc->PC++;
  return 0;
}

// 0F E9 - cpuid - Get processor info
U8 CPUID(GC* gc) {
  switch (gc->reg[DX].word) {
    case 0x0000: { // Get processor type
      gc->reg[DX].word = PROC_TYPE_GC16X;
      break;
    }
    case 0x0001: { // Get connected drive
      gc->reg[DX].word = ((gc->pin & 0b10000000) >> 7);
      break;
    }
    case 0x0002: { // Get memory size (0 for 65,536 bytes and then looping back)
      gc->reg[DX].word = (U16)MEMSIZE;
      break;
    }
    case 0x0003: { // Get disk size (0 for 65,536 bytes and then looping back)
      gc->reg[DX].word = (U16)ROMSIZE;
      break;
    }
    default:
      fprintf(stderr, "Illegal CPUID value: %04X\n", gc->reg[DX].word);
      return 1;
  }
  gc->PC++;
  return 0;
}

U8 ADD11(GC* gc) {  // 10 00
  gcrc_t rc = ReadRegClust(gc->mem[gc->PC+1]);
  gc->reg[rc.x].word += gc->reg[rc.y].word;
  gc->PC += 2;
  return 0;
}

U8 SUB11(GC* gc) {  // 10 01
  gcrc_t rc = ReadRegClust(gc->mem[gc->PC+1]);
  *ReadReg(gc, rc.x) -= *ReadReg(gc, rc.y);
  gc->PC += 2;
  return 0;
}

U8 MUL11(GC* gc) {  // 10 02
  gcrc_t rc = ReadRegClust(gc->mem[gc->PC+1]);
  *ReadReg(gc, rc.x) *= *ReadReg(gc, rc.y);
  gc->PC += 2;
  return 0;
}

U8 DIV11(GC* gc) {  // 10 03
  gcrc_t rc = ReadRegClust(gc->mem[gc->PC+1]);
  *ReadReg(gc, rc.x) /= *ReadReg(gc, rc.y);
  gc->reg[DX].word = (*ReadReg(gc, rc.x) % *ReadReg(gc, rc.y)); // The remainder is always stored into D
  gc->PC += 2;
  return 0;
}

U8 ADDA0(GC* gc) {  // 10 08
  gc->reg[AX].word += ReadWord(gc, gc->PC+1);
  gc->PC += 3;
  return 0;
}

U8 ADDB0(GC* gc) {  // 10 09
  gc->reg[BX].word += ReadWord(gc, gc->PC+1);
  gc->PC += 3;
  return 0;
}

U8 ADDC0(GC* gc) {  // 10 0A
  gc->reg[CX].word += ReadWord(gc, gc->PC+1);
  gc->PC += 3;
  return 0;
}

U8 ADDD0(GC* gc) {  // 10 0B
  gc->reg[DX].word += ReadWord(gc, gc->PC+1);
  gc->PC += 3;
  return 0;
}

U8 ADDS0(GC* gc) {  // 10 0C
  gc->reg[SI].word += ReadWord(gc, gc->PC+1);
  gc->PC += 3;
  return 0;
}

U8 ADDG0(GC* gc) {  // 10 0D
  gc->reg[GI].word += ReadWord(gc, gc->PC+1);
  gc->PC += 3;
  return 0;
}

U8 SUBA0(GC* gc) {  // 10 18
  gc->reg[AX].word -= ReadWord(gc, gc->PC+1);
  gc->PC += 3;
  return 0;
}

U8 SUBB0(GC* gc) {  // 10 19
  gc->reg[BX].word -= ReadWord(gc, gc->PC+1);
  gc->PC += 3;
  return 0;
}

U8 SUBC0(GC* gc) {  // 10 1A
  gc->reg[CX].word -= ReadWord(gc, gc->PC+1);
  gc->PC += 3;
  return 0;
}

U8 SUBD0(GC* gc) {  // 10 1B
  gc->reg[DX].word -= ReadWord(gc, gc->PC+1);
  gc->PC += 3;
  return 0;
}

U8 SUBS0(GC* gc) {  // 10 1C
  gc->reg[SI].word -= ReadWord(gc, gc->PC+1);
  gc->PC += 3;
  return 0;
}

U8 SUBG0(GC* gc) {  // 10 1D
  gc->reg[GI].word -= ReadWord(gc, gc->PC+1);
  gc->PC += 3;
  return 0;
}

U8 MULA0(GC* gc) {  // 10 28
  gc->reg[AX].word *= ReadWord(gc, gc->PC+1);
  gc->PC += 3;
  return 0;
}

U8 MULB0(GC* gc) {  // 10 29
  gc->reg[BX].word *= ReadWord(gc, gc->PC+1);
  gc->PC += 3;
  return 0;
}

U8 MULC0(GC* gc) {  // 10 2A
  gc->reg[CX].word *= ReadWord(gc, gc->PC+1);
  gc->PC += 3;
  return 0;
}

U8 MULD0(GC* gc) {  // 10 2B
  gc->reg[DX].word *= ReadWord(gc, gc->PC+1);
  gc->PC += 3;
  return 0;
}

U8 MULS0(GC* gc) {  // 10 2C
  gc->reg[SI].word *= ReadWord(gc, gc->PC+1);
  gc->PC += 3;
  return 0;
}

U8 MULG0(GC* gc) {  // 10 2D
  gc->reg[GI].word *= ReadWord(gc, gc->PC+1);
  gc->PC += 3;
  return 0;
}

U8 DIVA0(GC* gc) {  // 10 38
  U16 val = ReadWord(gc, gc->PC+1);
  gc->reg[DX].word = gc->reg[AX].word % val; // The remainder is always stored into D
  gc->reg[AX].word /= val;
  gc->PC += 3;
  return 0;
}

U8 DIVB0(GC* gc) {  // 10 39
  U16 val = ReadWord(gc, gc->PC+1);
  gc->reg[DX].word = gc->reg[BX].word % val; // The remainder is always stored into D
  gc->reg[BX].word /= val;
  gc->PC += 3;
  return 0;
}

U8 DIVC0(GC* gc) {  // 10 3A
  U16 val = ReadWord(gc, gc->PC+1);
  gc->reg[DX].word = gc->reg[CX].word % val; // The remainder is always stored into D
  gc->reg[CX].word /= val;
  gc->PC += 3;
  return 0;
}

U8 DIVD0(GC* gc) {  // 10 3B
  gc->reg[DX].word /= ReadWord(gc, gc->PC+1);
  gc->PC += 3;
  return 0;
}

U8 DIVS0(GC* gc) {  // 10 3C
  U16 val = ReadWord(gc, gc->PC+1);
  gc->reg[DX].word = gc->reg[SI].word % val; // The remainder is always stored into D
  gc->reg[SI].word /= val;
  gc->PC += 3;
  return 0;
}

U8 DIVG0(GC* gc) {  // 10 3D
  U16 val = ReadWord(gc, gc->PC+1);
  gc->reg[DX].word = gc->reg[GI].word % val; // The remainder is always stored into D
  gc->reg[GI].word /= val;
  gc->PC += 3;
  return 0;
}

U8 STORB(GC* gc) {  // 10 80
  gc->mem[gc->reg[SI].word] = *ReadReg(gc, gc->mem[gc->PC+1]);
  gc->PC += 2;
  return 0;
}

U8 STGRB(GC* gc) {  // 10 81
  gc->mem[gc->reg[GI].word] = *ReadReg(gc, gc->mem[gc->PC+1]);
  gc->PC += 2;
  return 0;
}

U8 LODSB(GC* gc) {  // 10 87
  gc->reg[SI].word = ReadByte(gc, gc->reg[SI].word);
  gc->PC++;
  return 0;
}

U8 LODGB(GC* gc) {  // 10 88
  gc->reg[GI].word = ReadByte(gc, gc->reg[GI].word);
  gc->PC++;
  return 0;
}

U8 STOSB(GC* gc) {  // 10 89
  gc->mem[gc->reg[SI].word] = ReadByte(gc, gc->PC+1);
  gc->PC += 2;
  return 0;
}

U8 STOGB(GC* gc) {  // 10 8A
  gc->mem[gc->reg[GI].word] = ReadByte(gc, gc->PC+1);
  gc->PC += 2;
  return 0;
}

U8 LDDS(GC* gc) {  // 10 8B
  gc->reg[AX].word = gc->rom[*gc, gc->reg[SI].word];
  gc->PC++;
  return 0;
}

U8 LDDG(GC* gc) {  // 10 9B
  gc->reg[GI].word = gc->rom[*gc, gc->reg[GI].word];
  gc->PC++;
  return 0;
}

U8 STDS(GC* gc) {  // 10 AB
  gc->rom[gc->reg[SI].word] = *ReadReg(gc, gc->mem[gc->PC+1]);
  gc->PC += 2;
  return 0;
}

U8 STDG(GC* gc) {  // 10 BB
  gc->rom[gc->reg[GI].word] = *ReadReg(gc, gc->PC+1);
  gc->PC += 2;
  return 0;
}

// 10C0-10CF -- Increment reg16
U8 INXr(GC* gc) {
  (*ReadReg(gc, gc->mem[gc->PC]-0xC0))++;
  gc->PC++;
  return 0;
}

// 10D0-10DF -- Decrement reg16
U8 DEXr(GC* gc) {
  (*ReadReg(gc, gc->mem[gc->PC]-0xD0))--;
  gc->PC++;
  return 0;
}

U8 AND11(GC* gc) {  // 10 D8
  gcrc_t rc = ReadRegClust(gc->mem[gc->PC+1]);
  *ReadReg(gc, rc.x) &= *ReadReg(gc, rc.y);
  gc->PC += 2;
  return 0;
}

U8 OR11(GC* gc) {  // 10 D9
  gcrc_t rc = ReadRegClust(gc->mem[gc->PC+1]);
  *ReadReg(gc, rc.x) |= *ReadReg(gc, rc.y);
  gc->PC += 2;
  return 0;
}

U8 CMP11(GC* gc) {  // 10 F6
  gcrc_t rc = ReadRegClust(gc->mem[gc->PC+1]);
  if (*ReadReg(gc, rc.x) == *ReadReg(gc, rc.y)) SET_ZF(gc->PS);
  else RESET_ZF(gc->PS);
  if (((I16)*ReadReg(gc, rc.x) - *ReadReg(gc, rc.y)) < 0) SET_NF(gc->PS);
  else RESET_NF(gc->PS);
  gc->PC += 2; // Set equal flag if two register values
  return 0;      // are equal
}

U8 CMP10(GC* gc) {  // 10 EE
  if ((*ReadReg(gc, gc->mem[gc->PC+1]) == ReadWord(gc, gc->PC+2))) SET_ZF(gc->PS);
  else RESET_ZF(gc->PS);
  if (((I16)(*ReadReg(gc, gc->mem[gc->PC+1]) - ReadWord(gc, gc->PC+2)) < 0)) SET_NF(gc->PS);
  else RESET_NF(gc->PS);
  gc->PC += 4; // Set equal flag if a register and
  return 0;      // immediate are equal
}

// 11-20 -- Load register to *reg16
U8 LDRp(GC* gc) {
  *ReadReg(gc, gc->mem[gc->PC]-0x11) = gc->mem[*ReadReg(gc, gc->mem[gc->PC+1])];
  gc->PC += 2;
  return 0;
}

U8 RC(GC* gc) {   // 23 - Return if carry set
  if (CF(gc->PS)) gc->PC = StackPop(gc);
  else gc->PC++;
  return 0;
}

U8 RE(GC* gc) {   // 2B - Return if equal
  if (ZF(gc->PS)) gc->PC = StackPop(gc);
  else gc->PC++;
  return 0;
}

U8 RET(GC* gc) {   // 33
  gc->PC = StackPop(gc);
  return 0;
}

U8 RNE(GC* gc) {   // 2B - Return if not equal
  if (!ZF(gc->PS & 0b00000100)) gc->PC = StackPop(gc);
  else gc->PC++;
  return 0;
}

U8 STI(GC* gc) {   // 34
  SET_IF(gc->PS);
  gc->PC++;
  return 0;
}

U8 CLC(GC* gc) {   // 36
  RESET_CF(gc->PS);
  gc->PC++;
  return 0;
}

U8 HLT(GC* gc) {   // 51
  while(1) {}
  gc->PC++;
  return 0;
}

U8 CLI(GC* gc) {   // 52
  RESET_IF(gc->PS);
  gc->PC++;
  return 0;
}

// 40-4F -- Load imm16 to reg16
U8 LDr0(GC* gc) {
  *ReadReg(gc, gc->mem[gc->PC]-0x40) = ReadWord(gc, gc->PC+1);
  gc->PC += 3;
  return 0;
}

U8 LDAZ(GC* gc) {   // -- LDA mz8
  gc->reg[AX].word = gc->mem[gc->mem[gc->PC+1]];
  gc->PC += 2;
  return 0;
}

U8 LDBZ(GC* gc) {   // 66 16 -- LDB Zero Page
  gc->reg[BX].word = gc->mem[gc->mem[gc->PC+1]];
  gc->PC += 2;
  return 0;
}

U8 LDCZ(GC* gc) {   // 66 17 -- LDC Zero Page
  gc->reg[CX].word = gc->mem[gc->mem[gc->PC+1]];
  gc->PC += 2;
  return 0;
}

U8 LDDZ(GC* gc) {   // 66 18 -- LDD Zero Page
  gc->reg[DX].word = gc->mem[gc->mem[gc->PC+1]];
  gc->PC += 2;
  return 0;
}

U8 LDSZ(GC* gc) {   // 66 19 -- LDS Zero Page
  gc->reg[SI].word = gc->mem[gc->mem[gc->PC+1]];
  gc->PC += 2;
  return 0;
}

U8 LDGZ(GC* gc) {   // 66 1A -- LDG Zero Page
  gc->reg[GI].word = gc->mem[gc->mem[gc->PC+1]];
  gc->PC += 2;
  return 0;
}

U8 LDAZS(GC* gc) {   // 66 25 -- LDG Zero Page,S
  gc->reg[AX].word = gc->mem[gc->mem[gc->PC+1]+gc->reg[SI].word];
  gc->PC += 2;
  return 0;
}

U8 LDBZS(GC* gc) {   // 66 26 -- LDG Zero Page,S
  gc->reg[BX].word = gc->mem[gc->mem[gc->PC+1]+gc->reg[SI].word];
  gc->PC += 2;
  return 0;
}

U8 LDCZS(GC* gc) {   // 66 27 -- LDG Zero Page,S
  gc->reg[CX].word = gc->mem[gc->mem[gc->PC+1]+gc->reg[SI].word];
  gc->PC += 2;
  return 0;
}

U8 LDDZS(GC* gc) {   // 66 28 -- LDG Zero Page,S
  gc->reg[DX].word = gc->mem[gc->mem[gc->PC+1]+gc->reg[SI].word];
  gc->PC += 2;
  return 0;
}

U8 LDSZS(GC* gc) {   // 66 29 -- LDG Zero Page,S
  gc->reg[SI].word = gc->mem[gc->mem[gc->PC+1]+gc->reg[SI].word];
  gc->PC += 2;
  return 0;
}

U8 LDGZS(GC* gc) {   // 66 2A -- LDG Zero Page,S
  gc->reg[GI].word = gc->mem[gc->mem[gc->PC+1]+gc->reg[SI].word];
  gc->PC += 2;
  return 0;
}

U8 LDAZG(GC* gc) {   // 66 35 -- LDG Zero Page,G
  gc->reg[AX].word = gc->mem[gc->mem[gc->PC+1]+gc->reg[GI].word];
  gc->PC += 2;
  return 0;
}

U8 LDBZG(GC* gc) {   // 66 36 -- LDG Zero Page,G
  gc->reg[BX].word = gc->mem[gc->mem[gc->PC+1]+gc->reg[GI].word];
  gc->PC += 2;
  return 0;
}

U8 LDCZG(GC* gc) {   // 66 37 -- LDG Zero Page,G
  gc->reg[CX].word = gc->mem[gc->mem[gc->PC+1]+gc->reg[GI].word];
  gc->PC += 2;
  return 0;
}

U8 LDDZG(GC* gc) {   // 66 38 -- LDG Zero Page,G
  gc->reg[DX].word = gc->mem[gc->mem[gc->PC+1]+gc->reg[GI].word];
  gc->PC += 2;
  return 0;
}

U8 LDSZG(GC* gc) {   // 66 39 -- LDG Zero Page,G
  gc->reg[SI].word = gc->mem[gc->mem[gc->PC+1]+gc->reg[GI].word];
  gc->PC += 2;
  return 0;
}

U8 LDGZG(GC* gc) {   // 66 3A -- LDG Zero Page,G
  gc->reg[GI].word = gc->mem[gc->mem[gc->PC+1]+gc->reg[GI].word];
  gc->PC += 2;
  return 0;
}

// 66 55 -- Load ax with m16
U8 LDAA(GC* gc) {
  gc->reg[AX].word = gc->mem[ReadWord(gc, gc->PC+1)]      + \
    (((gc->disp_prefix & 0b00000001))      * gc->reg[SI].word) + \
    (((gc->disp_prefix & 0b00000010) >> 1) * gc->reg[GI].word) + \
    (((gc->disp_prefix & 0b00000100) >> 2) * gc->reg[IX].word) + \
    (((gc->disp_prefix & 0b00001000) >> 3) * gc->reg[IY].word);
  gc->disp_prefix = 0x00;
  gc->PC += 3;
  return 0;
}

U8 LDBA(GC* gc) {    // 66 56 -- LDB Absolute
  gc->reg[BX].word = gc->mem[ReadWord(gc, gc->PC+1)];
  gc->PC += 3;
  return 0;
}

U8 LDCA(GC* gc) {    // 66 57 -- LDC Absolute
  gc->reg[CX].word = gc->mem[ReadWord(gc, gc->PC+1)];
  gc->PC += 3;
  return 0;
}

U8 LDDA(GC* gc) {    // 66 58 -- LDD Absolute
  gc->reg[DX].word = gc->mem[ReadWord(gc, gc->PC+1)];
  gc->PC += 3;
  return 0;
}

U8 LDSA(GC* gc) {    // 66 59 -- LDS Absolute
  gc->reg[SI].word = gc->mem[ReadWord(gc, gc->PC+1)];
  gc->PC += 3;
  return 0;
}

U8 LDGA(GC* gc) {    // 66 5A -- LDG Absolute
  gc->reg[GI].word = gc->mem[ReadWord(gc, gc->PC+1)];
  gc->PC += 3;
  return 0;
}

U8 LDAAS(GC* gc) {   // 66 65 -- LDA Absolute,S
  gc->reg[AX].word = gc->mem[ReadWord(gc, gc->PC+1)+gc->reg[SI].word];
  gc->PC += 3;
  return 0;
}

U8 LDBAS(GC* gc) {   // 66 66 -- LDB Absolute,S
  gc->reg[BX].word = gc->mem[ReadWord(gc, gc->PC+1)+gc->reg[SI].word];
  gc->PC += 3;
  return 0;
}

U8 LDCAS(GC* gc) {   // 66 67 -- LDC Absolute,S
  gc->reg[CX].word = gc->mem[ReadWord(gc, gc->PC+1)+gc->reg[SI].word];
  gc->PC += 3;
  return 0;
}

U8 LDDAS(GC* gc) {   // 66 68 -- LDD Absolute,S
  gc->reg[DX].word = gc->mem[ReadWord(gc, gc->PC+1)+gc->reg[SI].word];
  gc->PC += 3;
  return 0;
}

U8 LDSAS(GC* gc) {   // 66 69 -- LDS Absolute,S
  gc->reg[SI].word = gc->mem[ReadWord(gc, gc->PC+1)+gc->reg[SI].word];
  gc->PC += 3;
  return 0;
}

U8 LDGAS(GC* gc) {   // 66 6A -- LDG Absolute,S
  gc->reg[GI].word = gc->mem[ReadWord(gc, gc->PC+1)+gc->reg[SI].word];
  gc->PC += 3;
  return 0;
}

U8 LDAAG(GC* gc) {   // 66 75 -- LDA Absolute,G
  gc->reg[AX].word = gc->mem[ReadWord(gc, gc->PC+1)+gc->reg[SI].word];
  gc->PC += 3;
  return 0;
}

U8 LDBAG(GC* gc) {   // 66 76 -- LDB Absolute,G
  gc->reg[BX].word = gc->mem[ReadWord(gc, gc->PC+1)+gc->reg[SI].word];
  gc->PC += 3;
  return 0;
}

U8 LDCAG(GC* gc) {   // 66 77 -- LDC Absolute,G
  gc->reg[CX].word = gc->mem[ReadWord(gc, gc->PC+1)+gc->reg[SI].word];
  gc->PC += 3;
  return 0;
}

U8 LDDAG(GC* gc) {   // 66 78 -- LDD Absolute,G
  gc->reg[DX].word = gc->mem[ReadWord(gc, gc->PC+1)+gc->reg[SI].word];
  gc->PC += 3;
  return 0;
}

U8 LDSAG(GC* gc) {   // 66 79 -- LDS Absolute,G
  gc->reg[SI].word = gc->mem[ReadWord(gc, gc->PC+1)+gc->reg[SI].word];
  gc->PC += 3;
  return 0;
}

U8 LDGAG(GC* gc) {   // 66 7A -- LDG Absolute,G
  gc->reg[GI].word = gc->mem[ReadWord(gc, gc->PC+1)+gc->reg[SI].word];
  gc->PC += 3;
  return 0;
}

U8 LDA0S(GC* gc) {   // 66 85 -- LDA Immediate,S
  gc->reg[AX].word = ReadWord(gc, gc->PC+1)+gc->reg[SI].word;
  gc->PC += 3;
  return 0;
}

U8 LDB0S(GC* gc) {   // 66 86 -- LDB Immediate,S
  gc->reg[BX].word = ReadWord(gc, gc->PC+1)+gc->reg[SI].word;
  gc->PC += 3;
  return 0;
}

U8 LDC0S(GC* gc) {   // 66 87 -- LDC Immediate,S
  gc->reg[CX].word = ReadWord(gc, gc->PC+1)+gc->reg[SI].word;
  gc->PC += 3;
  return 0;
}

U8 LDD0S(GC* gc) {   // 66 88 -- LDD Immediate,S
  gc->reg[DX].word = ReadWord(gc, gc->PC+1)+gc->reg[SI].word;
  gc->PC += 3;
  return 0;
}

U8 LDS0S(GC* gc) {   // 66 89 -- LDS Immediate,S
  gc->reg[SI].word = ReadWord(gc, gc->PC+1)+gc->reg[SI].word;
  gc->PC += 3;
  return 0;
}

U8 LDG0S(GC* gc) {   // 66 8A -- LDG Immediate,S
  gc->reg[GI].word = ReadWord(gc, gc->PC+1)+gc->reg[SI].word;
  gc->PC += 3;
  return 0;
}

U8 LDA0G(GC* gc) {   // 66 95 -- LDA Immediate,G
  gc->reg[AX].word = ReadWord(gc, gc->PC+1)+gc->reg[GI].word;
  gc->PC += 3;
  return 0;
}

U8 LDB0G(GC* gc) {   // 66 96 -- LDB Immediate,G
  gc->reg[BX].word = ReadWord(gc, gc->PC+1)+gc->reg[GI].word;
  gc->PC += 3;
  return 0;
}

U8 LDC0G(GC* gc) {   // 66 97 -- LDC Immediate,G
  gc->reg[CX].word = ReadWord(gc, gc->PC+1)+gc->reg[GI].word;
  gc->PC += 3;
  return 0;
}

U8 LDD0G(GC* gc) {   // 66 98 -- LDD Immediate,G
  gc->reg[DX].word = ReadWord(gc, gc->PC+1)+gc->reg[GI].word;
  gc->PC += 3;
  return 0;
}

U8 LDS0G(GC* gc) {   // 66 99 -- LDS Immediate,G
  gc->reg[SI].word = ReadWord(gc, gc->PC+1)+gc->reg[GI].word;
  gc->PC += 3;
  return 0;
}

U8 LDG0G(GC* gc) {   // 66 9A -- LDG Immediate,G
  gc->reg[GI].word = ReadWord(gc, gc->PC+1)+gc->reg[GI].word;
  gc->PC += 3;
  return 0;
}

U8 LDA1(GC* gc) {   // 66 A5
  gc->reg[AX].word = *ReadReg(gc, gc->mem[gc->PC+1]);
  gc->PC += 2;
  return 0;
}

U8 LDB1(GC* gc) {   // 66 A6
  gc->reg[BX].word = *ReadReg(gc, gc->mem[gc->PC+1]);
  gc->PC += 2;
  return 0;
}

U8 LDC1(GC* gc) {   // 66 A7
  gc->reg[CX].word = *ReadReg(gc, gc->mem[gc->PC+1]);
  gc->PC += 2;
  return 0;
}

U8 LDD1(GC* gc) {   // 66 A8
  gc->reg[DX].word = *ReadReg(gc, gc->mem[gc->PC+1]);
  gc->PC += 2;
  return 0;
}

U8 LDS1(GC* gc) {   // 66 A9
  gc->reg[SI].word = *ReadReg(gc, gc->mem[gc->PC+1]);
  gc->PC += 2;
  return 0;
}

U8 LDG1(GC* gc) {   // 66 AA
  gc->reg[GI].word = *ReadReg(gc, gc->mem[gc->PC+1]);
  gc->PC += 2;
  return 0;
}

U8 LDSP1(GC* gc) {   // 66 AB
  gc->reg[SP].word = *ReadReg(gc, gc->mem[gc->PC+1]);
  gc->PC += 2;
  return 0;
}

U8 LDBP1(GC* gc) {   // 66 AC
  gc->reg[BP].word = *ReadReg(gc, gc->mem[gc->PC+1]);
  gc->PC += 2;
  return 0;
}

// 69 -- Compare *reg16 and imm16
U8 CMPpi(GC* gc) {
  if (gc->mem[*ReadReg(gc, gc->mem[gc->PC+1])] == ReadWord(gc, gc->PC+2)) SET_ZF(gc->PS);
  else RESET_ZF(gc->PS);
  if (((I16)(gc->mem[*ReadReg(gc, gc->mem[gc->PC+1])] - ReadWord(gc, gc->PC+2)) < 0)) SET_NF(gc->PS);
  else RESET_NF(gc->PS);
  gc->PC += 4;
  return 0;
}

// 88 - Exchange two registers
U8 XCHG4(GC* gc) {
  gcrc_t rc = ReadRegClust(gc->mem[gc->PC+1]);
  U16 temp = *ReadReg(gc, rc.x);
  *ReadReg(gc, rc.x) = *ReadReg(gc, rc.y);
  *ReadReg(gc, rc.y) = temp;
  gc->PC += 2;
  return 0;
}

// 8A - Load a word (16 bits) from [si] into ax
U8 LODSW(GC* gc) {
  gc->reg[AX].word = ReadWord(gc, gc->reg[SI].word);
  gc->PC++;
  return 0;
}

// 8B - Store a word (16 bits) into [si] from ax
U8 STOSW(GC* gc) {
  WriteWord(gc, gc->reg[SI].word, gc->reg[AX].word);
  gc->PC++;
  return 0;
}

// 90 - Decrement *m16
U8 DEXM(GC* gc) {   // 90
  gc->mem[ReadWord(gc, gc->PC+1)]--;
  gc->PC += 3;
  return 0;
}

U8 _PREF(GC* gc) {
  switch (gc->mem[gc->PC]) {
    case 0x24:
      puts("SIB prefix yayyy");
      gc->sib_prefix = 0x24;
      break;
    case 0x91 ... 0x9F:
      if (!gc->disp_prefix) {
        gc->disp_prefix = gc->mem[gc->PC++] - 0x90;
      }
      else {
        fatal("gc16x: \033[91merror:\033[0m Two prefixes of the same type: disp_prefix\n");
        old_st_legacy;
        exit(1);
      }
      break;
    default:
      printf("gc16x: \033[91merror:\033[0m Unhandled prefix %02X\n", gc->mem[gc->PC]);
  }
  return 0;
}

U8 ASL(GC* gc) {    // A0-A7
  U16* rgptr = ReadReg(gc, gc->mem[gc->PC]-0xA0);
  *rgptr = *rgptr << gc->mem[gc->PC+1];
  gc->PC += 2;
  return 0;
}

// D8-E7 -- Store *m8 to reg16
U8 STr0(GC* gc) {
  gc->mem[ReadWord(gc, gc->PC+1)] = *ReadReg(gc, gc->mem[gc->PC]-0xD8);
  gc->PC += 3;
  return 0;
}

// B0 - Increment *m16
U8 INXM(GC* gc) {
  gc->mem[ReadWord(gc, gc->PC+1)]++;
  gc->PC += 3;
  return 0;
}

// B1 -- Store byte into [%s] and increment %s
U8 STRb(GC* gc) {
  gc->mem[gc->reg[SI].word++] = gc->mem[gc->PC+1];
  gc->PC += 2;
  return 0;
}

// B8 - Jump to a label if (cx != 0)
U8 LOOP(GC* gc) {
  if (gc->reg[CX].word) {
    gc->reg[CX].word--;
    gc->PC = ReadWord(gc, gc->PC+1);
  }
  else {
    gc->PC += 3;
  }
  return 0;
}

// C0 - Change a value in memory to imm8
U8 MV26(GC* gc) {
  gc->mem[ReadWord(gc, gc->PC+1)] = gc->mem[gc->PC+3];
  gc->PC += 4;
  return 0;
}

// C7 - Call a subroutine/function
U8 CALL(GC* gc) {
  StackPush(gc, gc->PC+3);
  gc->PC = ReadWord(gc, gc->PC+1);
  return 0;
}

// D0 - Change a value in memory to imm8
U8 MW20(GC* gc) {
  WriteWord(gc, ReadWord(gc, gc->PC+1), gc->mem[gc->PC+3]);
  gc->PC += 4;
  return 0;
}

// D7 - Copy the value from top of the stack to a register
U8 COP1(GC* gc) {
  *ReadReg(gc, gc->mem[gc->PC+1]) = StackPop(gc);
  gc->reg[SP].word -= 2;
  gc->PC += 2;
  return 0;
}

// E0-E8 - Arithmetic shift right
U8 ASR(GC* gc) {
  *ReadReg(gc, gc->mem[gc->PC]-0xE0) >>= gc->mem[gc->PC+1];
  gc->PC += 2;
  return 0;
}

// EA - Do nothing
U8 NOP(GC* gc) {
  return 0;
}

// F9 - Load FLAGS into AX
U8 LFA(GC* gc) {
  gc->reg[AX].word &= 0b1111111100000000;
  gc->reg[AX].word |= gc->PS;
  return 0;
}

// FA - Load AX into FLAGS
U8 LAF(GC* gc) {
  gc->PS = (U8)gc->reg[AX].word;
  return 0;
}

U8 PG0F(GC*); // Page 0F - Stack operations
U8 PG10(GC*); // Page 10 - Register operations
U8 PG66(GC*); // Page 66 - Load/Store operations

// Zero page instructions
U8 (*INSTS[256])() = {
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &PG0F ,
  &PG10 , &LDRp , &LDRp , &LDRp , &LDRp , &LDRp , &LDRp , &LDRp , &LDRp , &LDRp , &LDRp , &LDRp , &LDRp , &LDRp , &LDRp , &LDRp ,
  &LDRp , &UNK  , &UNK  , &RC   , &_PREF, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &RE   , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &RET  , &STI  , &UNK  , &CLC  , &UNK  , &UNK  , &UNK  , &UNK  , &RNE  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &LDr0 , &LDr0 , &LDr0 , &LDr0 , &LDr0 , &LDr0 , &LDr0 , &LDr0 , &LDr0 , &LDr0 , &LDr0 , &LDr0 , &LDr0 , &LDr0 , &LDr0 , &LDr0 ,
  &UNK  , &HLT  , &CLI  , &LDAA , &LDBA , &LDCA , &LDDA , &LDSA , &LDGA , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &PG66 , &UNK  , &UNK  , &CMPpi, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LDA1 , &LDB1 , &LDC1 , &LDD1 , &LDS1 , &LDG1 , &LDSP1, &LDBP1, &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &XCHG4, &UNK  , &LODSW, &STOSW, &UNK  , &UNK  , &UNK  , &UNK  ,
  &DEXM , &_PREF, &_PREF, &_PREF, &_PREF, &_PREF, &_PREF, &_PREF, &_PREF, &_PREF, &_PREF, &_PREF, &_PREF, &_PREF, &_PREF, &_PREF,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &INXM , &STRb , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LOOP , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &MV26 , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &CALL , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &COP1 , &STr0 , &STr0 , &STr0 , &STr0 , &STr0 , &STr0 , &STr0 , &STr0 ,
  &STr0 , &STr0 , &STr0 , &STr0 , &STr0 , &STr0 , &STr0 , &STr0 , &UNK  , &UNK  , &NOP  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
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
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &JL0  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &INT0 , &INT1 , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &JG0  , &UNK  , &UNK  , &UNK  , &UNK  ,
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
  &INXr , &INXr , &INXr , &INXr , &INXr , &INXr , &INXr , &INXr , &INXr , &INXr , &INXr , &INXr , &INXr , &INXr , &INXr , &INXr ,
  &DEXr , &DEXr , &DEXr , &DEXr , &DEXr , &DEXr , &DEXr , &DEXr , &DEXr , &DEXr , &DEXr , &DEXr , &DEXr , &DEXr , &DEXr , &DEXr ,
  &UNK  , &ADD11, &OR11 , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &CMP10, &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &CMP11, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK
};

U8 (*INSTS_PG66[256])() = {
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LDAZ , &LDBZ , &LDCZ , &LDDZ , &LDSZ , &LDGZ , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LDAZS, &LDBZS, &LDCZS, &LDDZS, &LDSZS, &LDGZS, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &LDAZG, &LDBZG, &LDCZG, &LDDZG, &LDSZG, &LDGZG, &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
  &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  , &UNK  ,
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
  gc->PC++;
  return (INSTS_PG0F[gc->mem[gc->PC]])(gc);
}

U8 PG10(GC* gc) {   // 10H
  gc->PC++;
  return (INSTS_PG10[gc->mem[gc->PC]])(gc);
}

U8 PG66(GC* gc) {   // 66H
  gc->PC++;
  return (INSTS_PG66[gc->mem[gc->PC]])(gc);
}

U0 Reset(GC* gc) {
  gc->reg[SP].word = 0xF000;
  gc->reg[BP].word = 0xF000;
  gc->PC = 0x00000000;

  gc->PS = 0b01000000;
}

U0 PageDump(GC* gc, U8 page) {
  for (U16 i = (page*256); i < (page*256)+256; i++) {
    if (!(i % 16)) putchar(10);
    printf("%02X ", gc->mem[i]);
  }
}

U0 StackDump(GC* gc, U16 c) {
  printf("SP: %04X\n", gc->reg[SP].word);
  for (U16 i = 0xF000; i > 0xF000-c; i--) {
    printf("%04X: %02X\n", i, gc->mem[i]);
  }
}

U0 RegDump(GC* gc) {
  printf("\033[21A\033[10CAX %04X\n",   gc->reg[AX].word);
  printf("\033[10CBX %04X\n",           gc->reg[BX].word);
  printf("\033[10CCX %04X\n",           gc->reg[CX].word);
  printf("\033[10CDX %04X\n",           gc->reg[DX].word);
  printf("\033[10CSI %04X ASCII: %c\n", gc->reg[SI].word, gc->reg[SI].word);
  printf("\033[10CGI %04X ASCII: %c\n", gc->reg[GI].word, gc->reg[GI].word);
  printf("\033[10CEX %04X\n",           gc->reg[EX].word);
  printf("\033[10CFX %04X\n",           gc->reg[FX].word);
  printf("\033[10CHX %04X\n",           gc->reg[HX].word);
  printf("\033[10CLX %04X\n",           gc->reg[LX].word);
  printf("\033[10CX  %04X\n",           gc->reg[X].word);
  printf("\033[10CY  %04X\n",           gc->reg[Y].word);
  printf("\033[10CIX %04X ASCII: %c\n", gc->reg[IX].word, gc->reg[IX].word);
  printf("\033[10CIY %04X ASCII: %c\n", gc->reg[IY].word, gc->reg[IY].word);
  printf("\033[10CSP %04X\n",           gc->reg[SP].word);
  printf("\033[10CBP %04X\n",           gc->reg[BP].word);
  printf("\033[10CPC %08X\n",           gc->PC);
  printf("\033[10CPS %08b\n",           gc->PS);
  printf("\033[10C   -I---ZNC\033[0m\n");
}

U8 Exec(GC* gc, const U32 memsize) {
  U8 exc = 0;
  execloop:
    exc = (INSTS[gc->mem[gc->PC]])(gc);
    if (exc != 0) return errno;
    goto execloop;
  return exc;
}
