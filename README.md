# CGovnOS - Govno Core 16X

CGovnOS is a virtual machine for the Govno Core 16X processor.

## Compile and run GovnASM
1. At first, you need to compile gc16, there is a script for it: `./build`
2. Compile GovnASM with kasm: `./kasm test.asm test.bin`
3. Run the compiled binary: `./gc16 test.bin`

## Load and run GovnOS
To compile and run GovnOS on GC16X, type:
```
./ball                     # compile all binaries
./newdisk name.img 16K     # create new disk image
./rom-load-govnos name.img # load boot image
./gc16 disk name.img       # run govnos
```

