# CGovnOS - Govno Core 16X

CGovnOS is a virtual machine for the Govno Core 16X processor.

## Compile and run GovnASM
1. At first, you need to compile gc16, there is a script for it: `./build_gc16`
2. Compile your GovnASM code with kasm: `./kasm test.asm test.bin`
3. Run the compiled binary: `./gc16 test.bin`

## Load and run GovnOS
To compile and run GovnOS on GC16X, type:
```
./ball                         # compile all binaries
./newdisk disk.img 64K         # create new disk image
./rom-load-govnos disk.img     # load boot image
./gc16 disk disk.img           # run govnos
```

