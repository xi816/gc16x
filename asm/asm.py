#!/usr/bin/python3

import sys;

T_INS = 0x00;
T_INT = 0x01;
T_BYT = 0x02;
T_LAB = 0x03;
T_REG = 0x04;
T_0ID = 0x05;
T_CHR = 0x06;
T_EOL = 0x07;
T_EOF = 0xFF;

# Lexer:
def Lex(prog: str):
  prog += "\n\0";
  toks = [];
  pos = 0;
  cpos = 0;
  proglen = len(prog);
  basemode = 10;
  buf = "";
  bytesmode = 1;
  while (True):
    if (prog[pos] == "\0"):
      toks.append((T_EOL,));
      toks.append((T_EOF,));
      return toks, 0;
    elif (prog[pos] == ";"):
      pos += 1;
      while (prog[pos] != "\n"):
        pos += 1;
    elif (prog[pos] == "\""):
      pos += 1;
      while (prog[pos] != "\""):
        if (prog[pos] == "$"):
          buf += "\n";
        elif (prog[pos] == "^"):
          pos += 1;
          if (ord(prog[pos]) in range(65, 91)):
            buf += chr(ord(prog[pos])-64);
          elif (prog[pos] == "@"):
            buf += "\0";
          elif (prog[pos] == "$"):
            buf += "$";
        else:
          buf += prog[pos];
        pos += 1;
        cpos += 1;
      pos += 1;
      toks.append((T_CHR, buf, cpos));
      buf = "";
    elif (prog[pos] in WHI):
      pos += 1;
    elif (prog[pos] == "%"):
      basemode = 16;
      pos += 1;
    elif (prog[pos] == "#"):
      basemode = 8;
      pos += 1;
    elif (prog[pos] == "\n"):
      bytesmode = 1;
      toks.append((T_EOL,));
      pos += 1;
    elif (prog[pos] in INT):
      while (prog[pos] in INTEXT):
        buf += prog[pos];
        pos += 1;
      toks.append((T_INT, int(buf, base=basemode)));
      basemode = 10;
      buf = "";
      cpos += 1+bytesmode;
    elif (prog[pos] in CAP):
      while (prog[pos] in CAP+INT+"-"):
        buf += prog[pos];
        pos += 1;
      if (prog[pos] == ":"):
        toks.append((T_LAB, buf, cpos));
        pos += 1;
      else:
        if (buf in KEY):
          toks.append((T_INS, buf, cpos));
          cpos += KSZ[buf];
        elif (buf == "BYTES"):
          toks.append((T_BYT, cpos));
          bytesmode = 0;
        else:
          toks.append((T_0ID, buf));
          cpos += 2;
      buf = "";
    else:
      print(f"\033[31m    Unknown\033[0m character {hex(ord(prog[pos]))[2:].upper():0>2}");
      return [], 1;

  return [], 1;

def FetchLabels(prog: list):
  labs = dict();
  for i in prog:
    if (i[0] == T_LAB):
      labs[i[1]] = i[2];
  return labs;

def RemEmpty(prog: str):
  return "\n".join([i for i in prog.split("\n") if i]);

# Compiler:
def Govnbin(prog: list, labs: dict):
  code = bytearray();
  pos = 0;
  while (prog[pos][0] != T_EOF):
    if (prog[pos][0] == T_LAB):
      pos += 1;
    elif (prog[pos][0] == T_BYT):
      pos += 1;
      while (prog[pos][0] != T_EOL):
        if (prog[pos][0] == T_INT):
          code.append(prog[pos][1] % 256);
        elif (prog[pos][0] == T_CHR):
          for i in prog[pos][1]:
            code.append(ord(i) % 256);
        else:
          print(f"  ERROR: Unknown byte {prog[pos][0]}");
          return [], 1;
        pos += 1;
      pos += 1;
    elif (prog[pos][0] == T_EOL):
      pos += 1;
    elif (prog[pos][0] == T_INS):
      if (prog[pos][1] == "PUSH"):
        pos += 1;
        if (prog[pos][0] == T_INT):
          code.append(0x19);
          code.append(prog[pos][1] >> 8);
          code.append(prog[pos][1] % 256);
        elif (prog[pos][0] == T_0ID):
          val = labs[prog[pos][1]];
          code.append(0x19);
          code.append(val >> 8);
          code.append(val % 256);
        else:
          printf("  ERROR: PUSH instruction can only take immediate values or labels");
          return 1;
        pos += 1;
      elif (prog[pos][1] == "RDDB"):
        pos += 1;
        if (prog[pos][0] == T_INT):
          code.append(0x20);
          code.append(prog[pos][1] >> 8);
          code.append(prog[pos][1] % 256);
        elif (prog[pos][0] == T_0ID):
          val = labs[prog[pos][1]];
          code.append(0x20);
          code.append(val >> 8);
          code.append(val % 256);
        else:
          print("  ERROR: RDDB instruction can only take immediate values or labels");
          return [], 1;
        pos += 1;
      elif (prog[pos][1] == "REAB"):
        pos += 1;
        if (prog[pos][0] == T_INT):
          code.append(0x21);
          code.append(prog[pos][1] >> 8);
          code.append(prog[pos][1] % 256);
        elif (prog[pos][0] == T_0ID):
          val = labs[prog[pos][1]];
          code.append(0x21);
          code.append(val >> 8);
          code.append(val % 256);
        else:
          print("  ERROR: REAB instruction can only take immediate values or labels");
          return [], 1;
        pos += 1;
      elif (prog[pos][1] == "REAW"):
        pos += 1;
        if (prog[pos][0] == T_INT):
          code.append(0x22);
          code.append(prog[pos][1] >> 8);
          code.append(prog[pos][1] % 256);
        elif (prog[pos][0] == T_0ID):
          val = labs[prog[pos][1]];
          code.append(0x22);
          code.append(val >> 8);
          code.append(val % 256);
        else:
          print("  ERROR: REAW instruction can only take immediate values or labels");
          return [], 1;
        pos += 1;
      elif (prog[pos][1] == "RDDW"):
        pos += 1;
        if (prog[pos][0] == T_INT):
          code.append(0x10);
          code.append(prog[pos][1] >> 8);
          code.append(prog[pos][1] % 256);
        elif (prog[pos][0] == T_0ID):
          val = labs[prog[pos][1]];
          code.append(0x10);
          code.append(val >> 8);
          code.append(val % 256);
        else:
          print("  ERROR: RDDW instruction can only take immediate values or labels");
          return [], 1;
        pos += 1;
      elif (prog[pos][1] == "RDDWR"):
        pos += 1;
        if (prog[pos][0] == T_INT):
          code.append(0x25);
          code.append(prog[pos][1] >> 8);
          code.append(prog[pos][1] % 256);
        elif (prog[pos][0] == T_0ID):
          val = labs[prog[pos][1]];
          code.append(0x25);
          code.append(val >> 8);
          code.append(val % 256);
        else:
          print("  ERROR: RDDW instruction can only take immediate values or labels");
          return [], 1;
        pos += 1;
      elif (prog[pos][1] == "POP"):
        code.append(0x1A);
        pos += 1;
      elif (prog[pos][1] == "LSP"):
        code.append(0x4A);
        pos += 1;
      elif (prog[pos][1] == "LCS"):
        code.append(0x4B);
        pos += 1;
      elif (prog[pos][1] == "SSP"):
        code.append(0x4C);
        pos += 1;
      elif (prog[pos][1] == "SCS"):
        code.append(0x4D);
        pos += 1;
      elif (prog[pos][1] == "SSP"):
        code.append(0x4E);
        pos += 1;
      elif (prog[pos][1] == "SHL"):
        code.append(0x03);
        code.append(prog[pos+1][1] >> 8);
        code.append(prog[pos+1][1] % 256);
        pos += 3;
      elif (prog[pos][1] == "SHR"):
        code.append(0x04);
        code.append(prog[pos+1][1] >> 8);
        code.append(prog[pos+1][1] % 256);
        pos += 3;
      elif (prog[pos][1] == "JE"):
        code.append(0x08);
        code.append(0x00);
        code.append(labs[prog[pos+1][1]] >> 8);
        code.append(labs[prog[pos+1][1]] % 256);
        pos += 2;
      elif (prog[pos][1] == "CALE"):
        code.append(0x0A);
        code.append(0x00);
        code.append(labs[prog[pos+1][1]] >> 8);
        code.append(labs[prog[pos+1][1]] % 256);
        pos += 2;
      elif (prog[pos][1] == "CALNE"):
        code.append(0x0A);
        code.append(0x01);
        code.append(labs[prog[pos+1][1]] >> 8);
        code.append(labs[prog[pos+1][1]] % 256);
        pos += 2;
      elif (prog[pos][1] == "CMP"):
        code.append(0x11);
        pos += 1;
      elif (prog[pos][1] == "DUP"):
        code.append(0x1B);
        pos += 1;
      elif (prog[pos][1] == "JNE"):
        code.append(0x08);
        code.append(0x01);
        code.append(labs[prog[pos+1][1]] >> 8);
        code.append(labs[prog[pos+1][1]] % 256);
        pos += 2;
      elif (prog[pos][1] == "CPUID"):
        code.append(0x01);
        pos += 1;
      elif (prog[pos][1] == "CSP"):
        code.append(0x02);
        pos += 1;
      elif (prog[pos][1] == "ADD"):
        code.append(0x1D);
        pos += 1;
      elif (prog[pos][1] == "SUB"):
        code.append(0x1E);
        pos += 1;
      elif (prog[pos][1] == "MUL"):
        code.append(0x1F);
        pos += 1;
      elif (prog[pos][1] == "DIV"):
        code.append(0x24);
        pos += 1;
      elif (prog[pos][1] == "SWAP"):
        code.append(0x1C);
        pos += 1;
      elif (prog[pos][1] == "JMP"):
        code.append(0x07);
        code.append(labs[prog[pos+1][1]] >> 8);
        code.append(labs[prog[pos+1][1]] % 256);
        pos += 2;
      elif (prog[pos][1] == "CALL"):
        code.append(0x06);
        code.append(labs[prog[pos+1][1]] >> 8);
        code.append(labs[prog[pos+1][1]] % 256);
        pos += 2;
      elif (prog[pos][1] == "RET"):
        code.append(0x09);
        pos += 1;
      elif (prog[pos][1] == "INT"):
        code.append(0x23);
        pos += 1;
      else:
        print(f"\033[31m    Unknown\033[0m instruction {prog[pos][1]}");
        return code, 1;
    else:
      print(f"\033[31m    Unknown\033[0m token {prog[pos]}");
      print(f"      At position {pos}");
      return code, 1;

  return code, 0;

def main(argc: int, argv: list) -> int:
  if (argc == 1):
    print("No arguments given");
    return 1;
  elif (argc == 2):
    print("No binary filename given");
    return 1;
  progname = argv[1];
  outname = argv[2];

  with open(progname, "r") as fl:
    src = fl.read();
  src = RemEmpty(src)+"\0";
  tokens, exitcode = Lex(src);

  return 0;

sys.exit(main(len(sys.argv), sys.argv));

