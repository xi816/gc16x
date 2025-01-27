# CGovnOS - Govno Core 16X

CGovnOS is a virtual machine for the Govno Core 16X processor.

## Compile and run GovnASM
1. At first, you need to compile gc16: `make bin/gc16`
2. Compile your GovnASM code with kasm: `./kasm/kasm /path/to/govn.asm /path/to/result.bin`
3. Run the compiled binary file: `./gc16 /path/to/result.bin`

## Load and run GovnOS
To compile and run GovnOS on GC16X, type:
```bash
make build-all                    # compile all binaries
./scripts/newdisk disk.img 64K    # create new disk image
./scripts/setup-govnos disk.img   # load boot image
./bin/gc16 disk disk.img          # run govnos
```

