# CGovnOS - Govno Core 16X

CGovnOS is a virtual machine for the Govno Core 16X processor.

## Start

1. Compile the source code\
   The `./ball` script compiles all of the programs included in this repo: CGovnOS-GC16X, mkfs.govnfs, gboot and GASMAN.\
   For only compiling CGovnOS-GC16X you can use the `./build` script. (`kasm` assembler will not be built as it is written in python).

2. Write assembly code (GovnASM) and compile it to binary\
   Now you can start writing assembly code. For example, create a file called `test.asm`:

```asm
main:
  push #50h
  int #00h
```

and compile it with `kasm`: `./kasm test.asm test.bin`.

3. Running binary on a CGovnOS-GC16X virtual machine\
   Now, you can run it using `./gc16 test.bin`. Congratulations, you compiled GovnASM assembly for the Govno Core 16X processor and ran it on a CGovnOS-GC16X virtual machine.

## Compile GovnOS

To compile and run GovnOS on GC16X, type:

```
newdisk name.img
rom-load-govnos name.img
run-govnos name.img
```
