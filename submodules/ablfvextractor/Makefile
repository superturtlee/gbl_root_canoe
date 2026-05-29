clean:
	rm -rf build || true

build:
	mkdir -p build
	gcc -o ./build/extractfv ./extractfv.c -llzma

build_android:
	mkdir -p build
	bash ./tools/build_extractfv_android.sh

build_windows:
	mkdir -p build
	bash ./tools/build_extractfv_windows.sh