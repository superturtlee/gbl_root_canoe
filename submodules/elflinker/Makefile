clean:
	rm -rf build || true
build: 
	mkdir -p build
	gcc -O2 -o build/elf_inject elf_inject.c

build_android:
	mkdir -p build
	$(NDK_PATH)/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android31-clang elf_inject.c -o build/elf_inject_android

build_windows:
	mkdir -p build
	x86_64-w64-mingw32-gcc -o ./build/elf_inject.exe elf_inject.c