clean:
	rm -rf build || true
build: 
	mkdir -p build
	gcc -O2 -o build/patch_abl patch_abl.c

build_android:
	mkdir -p build
	$(NDK_PATH)/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android31-clang patch_abl.c -o build/patch_abl_android

build_windows:
	mkdir -p build
	x86_64-w64-mingw32-gcc -o ./build/patch_abl.exe patch_abl.c
