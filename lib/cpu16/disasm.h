// Govno Core 16X disassembler

U8* regname[16] = {
  "%ax", "%bx", "%cx", "%dx", "%si", "%gi", "%sp", "%bp",
  "%ex", "%fx", "%hx", "%lx", "%x" , "%y" , "%ix", "%iy"
};
U8* regshort[16] = {
  "a", "b", "c", "d", "s", "g", "sp", "bp",
  "e", "f", "h", "l", "x", "y", "ix", "iy"
};

U16 bc(U8 low, U8 high) {
  return (U16)((high << 8) + low);
}

U8* disasm_inst(U8* bin, U16* pc, FILE* out) {
  switch (bin[*pc]) {
  case 0x40 ... 0x4F:
    printf("ld%s $%04X\n", regshort[bin[*pc]-0x40], bc(bin[*pc+1], bin[*pc+2]));
    *pc += 3;
    break;
  case 0x11 ... 0x20:
    printf("ld%s *%s\n", regshort[bin[*pc]-0x11], regname[bin[*pc+1]]);
    *pc += 2;
    break;
  default:
    printf("...\n");
    return NULL;
  }
  return "no shit\n";
}

U8 disasm(U8* bin, U32 size, FILE* out) {
  U16 pc = 0;
  while (pc < size) {
    if (disasm_inst(bin, &pc, out) == NULL) {
      return 1;
    }
  }
  return 0;
}
