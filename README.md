# GC16X - Govno Core 16X

GC16X is a virtual machine for the Govno Core 16X processor.

> [!NOTE]
> The new version of the CPU called GC24 is avaliable at https://github.com/xi816/gc24/

## Start
1. Compile the source code\
The `./ball` script compiles all of the programs included in this repo: GC16X, mkfs.govnfs, gboot and GASMAN.\
For only compiling GC16X you can use the `./build` script. (`kasm` assembler will not be built as it is written in python).

2. Write assembly code (GovnASM) and compile it to binary\
Now you can start writing assembly code. For example, create a file called `test.asm`:
```asm
main:
  push $50 ; $ is hex
  int 0 ; exit
```
and compile it with `kasm`: `./kasm test.asm test.bin`.

3. Running binary on a CGovnOS-GC16X virtual machine\
Now, you can run it using `./gc16 test.bin`. Congratulations, you wrote & compiled your assembly program for the Govno Core 16X processor and ran it on a GC16X virtual machine.

## Compile GovnOS
To compile and run GovnOS on GC16X, type:
```shell
./prepare-disk name.img
./gc16 disk name.img
```
