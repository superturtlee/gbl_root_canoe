clean:
	rm -rf build || true
build:
	mkdir -p build/toolkit/bin
	gcc -O2 -o build/toolkit/bin/vbmetabackup vbmetabackup.c
	gcc -O2 -o build/toolkit/bin/vbmetaport vbmetaport.c
	gcc -O2 -o build/toolkit/vbmetaflasher vbmetaflasher.c

build_windows:
	mkdir -p build/toolkit_win/bin
	x86_64-w64-mingw32-gcc -o ./build/toolkit_win/bin/vbmetabackup.exe vbmetabackup.c
	x86_64-w64-mingw32-gcc -o ./build/toolkit_win/bin/vbmetaport.exe vbmetaport.c
	x86_64-w64-mingw32-gcc -o ./build/toolkit_win/vbmetaflasher.exe vbmetaflasher.c -lkernel32
