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
  printf("$%04X  ", *pc);
  switch (bin[*pc]) {
  case 0x0F:
    (*pc)++;
    switch (bin[*pc]) {
      case 0x29:
        printf("jme $%04X\n", bc(bin[*pc+1], bin[*pc+2]));
        *pc += 3;
        break;
      case 0x2A:
        printf("jmne $%04X\n", bc(bin[*pc+1], bin[*pc+2]));
        *pc += 3;
        break;
      case 0x30:
        printf("jmp $%04X\n", bc(bin[*pc+1], bin[*pc+2]));
        *pc += 3;
        break;
      case 0x80:
        printf("pop %s\n", regname[bin[*pc+1]]);
        *pc += 2;
        break;
      case 0x82:
        printf("push *%s\n", regname[bin[*pc+1]]);
        *pc += 2;
        break;
      case 0x84:
        printf("push $%04X\n", bc(bin[*pc+1], bin[*pc+2]));
        *pc += 3;
        break;
      case 0x90:
        printf("push %s\n", regname[bin[*pc+1]]);
        *pc += 2;
        break;
      case 0xBB:
        printf("jl $%04X\n", bc(bin[*pc+1], bin[*pc+2]));
        *pc += 3;
        break;
      case 0xCB:
        printf("jg $%04X\n", bc(bin[*pc+1], bin[*pc+2]));
        *pc += 3;
        break;
      case 0xC2:
        printf("int $%02X\n", bin[*pc+1]);
        *pc += 2;
        break;
      case 0xE9:
        printf("cpuid\n");
        *pc += 1;
        break;
      default:
        printf("fp ...\n");
        return NULL;
    }
    break;
  case 0x10:
    (*pc)++;
    switch (bin[*pc]) {
      case 0x00:
        printf("add %s %s\n", regname[bin[*pc+1]/16], regname[bin[*pc+1]%16]);
        *pc += 2;
        break;
      case 0x01:
        printf("sub %s %s\n", regname[bin[*pc+1]/16], regname[bin[*pc+1]%16]);
        *pc += 2;
        break;
      case 0x02:
        printf("mul %s %s\n", regname[bin[*pc+1]/16], regname[bin[*pc+1]%16]);
        *pc += 2;
        break;
      case 0x03:
        printf("div %s %s\n", regname[bin[*pc+1]/16], regname[bin[*pc+1]%16]);
        *pc += 2;
        break;
      case 0x08 ... 0x0F:
        printf("add %s $%04X\n", regname[bin[*pc]-0x08], bc(bin[*pc+1], bin[*pc+2]));
        *pc += 3;
        break;
      case 0x18 ... 0x1F:
        printf("sub %s $%04X\n", regname[bin[*pc]-0x18], bc(bin[*pc+1], bin[*pc+2]));
        *pc += 3;
        break;
      case 0x28 ... 0x2F:
        printf("mul %s $%04X\n", regname[bin[*pc]-0x28], bc(bin[*pc+1], bin[*pc+2]));
        *pc += 3;
        break;
      case 0x38 ... 0x3F:
        printf("div %s $%04X\n", regname[bin[*pc]-0x38], bc(bin[*pc+1], bin[*pc+2]));
        *pc += 3;
        break;
      case 0x80:
        printf("storb %s\n", regname[bin[*pc+1]]);
        *pc += 2;
        break;
      case 0x81:
        printf("stgrb %s\n", regname[bin[*pc+1]]);
        *pc += 2;
        break;
      case 0x8B:
        printf("ldds\n");
        *pc += 1;
        break;
      case 0xC0 ... 0xCF:
        printf("inx %s\n", regname[bin[*pc]-0xC0]);
        *pc += 1;
        break;
      case 0xD0 ... 0xDF:
        printf("dex %s\n", regname[bin[*pc]-0xD0]);
        *pc += 1;
        break;
      case 0xEE:
        printf("cmp %s $%04X\n", regname[bin[*pc+1]], bc(bin[*pc+2], bin[*pc+3]));
        *pc += 4;
        break;
      case 0xF6:
        printf("cmp %s %s\n", regname[bin[*pc+1]/16], regname[bin[*pc+1]%16]);
        *pc += 2;
        break;
      default:
        printf("tp ...\n");
        return NULL;
    }
    break;
  case 0x11 ... 0x20:
    printf("ld%s *%s\n", regshort[bin[*pc]-0x11], regname[bin[*pc+1]]);
    *pc += 2;
    break;
  case 0x2B:
    printf("re\n");
    *pc += 1;
    break;
  case 0x33:
    printf("ret\n");
    *pc += 1;
    break;
  case 0x40 ... 0x4F:
    printf("ld%s $%04X\n", regshort[bin[*pc]-0x40], bc(bin[*pc+1], bin[*pc+2]));
    *pc += 3;
    break;
  case 0x69:
    printf("cmp *%s $%04X\n", regname[bin[*pc+1]], bc(bin[*pc+2], bin[*pc+3]));
    *pc += 4;
    break;
  case 0x77 ... 0x7E:
    printf("ld%s %s\n", regshort[bin[*pc]-0x77], regname[bin[*pc+1]]);
    *pc += 2;
    break;
  case 0xB8:
    printf("loop $%04X\n", bc(bin[*pc+1], bin[*pc+2]));
    *pc += 3;
    break;
  case 0xC7:
    printf("call $%04X\n", bc(bin[*pc+1], bin[*pc+2]));
    *pc += 3;
    break;
  default:
    printf("...\n");
    *pc += 1;
    // return NULL;
  }
  return "no shit\n";
}

U8 disasm(U8* bin, U32 size, FILE* out) {
  U16 pc = 0;
  while (pc < size) {
    if ((disasm_inst(bin, &pc, out) == NULL) || (pc > 0xFFF0)) {
      return 1;
    }
  }
  return 0;
}
