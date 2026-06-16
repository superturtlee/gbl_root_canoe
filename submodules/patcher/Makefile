SRCS := $(shell find src -name '*.c')
CFLAGS := -O2 -Iinclude

clean:
	rm -rf build || true

build:
	mkdir -p build
	gcc $(CFLAGS) -o build/patch_abl $(SRCS)

build_android:
	mkdir -p build
	$(NDK_PATH)/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android31-clang $(CFLAGS) -o build/patch_abl_android $(SRCS)

build_windows:
	mkdir -p build
	x86_64-w64-mingw32-gcc $(CFLAGS) -o build/patch_abl.exe $(SRCS)
