.PHONY: build build-all ball clean

build-all: bin/gc16 bin/gasman bin/gboot bin/ugovnfs bin/govnos
	printf "\nCompilation finished!\n"

bin/gc16: bin/
	printf "Building $@...\n"
	gcc core/main.c -Ilib -lSDL2 -o $@

bin/gasman: bin/
	printf "Building $@...\n"
	gcc gasman/main.c -Ilib -o $@

bin/gboot: bin/
	printf "Building $@...\n"
	gcc gboot/main.c -o $@

bin/ugovnfs: bin/
	printf "Building $@...\n"
	gcc ugovnfs/main.c -Ilib -o $@

bin/govnos: kasm/kasm bin/
	printf "Building $@...\n"
	./$< govnos/boot.asm $@

bin/:
	mkdir -p $@

clean: bin/
	rm -rf $<*

build: bin/gc16
ball: build-all
