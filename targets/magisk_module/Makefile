NDK_PATH ?= $(ANDROID_NDK_HOME)
RUST_NDK_LINKER ?= $(NDK_PATH)/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android31-clang

clean:
	rm -rf build || true
	cd bl_flasher && cargo clean 2>/dev/null || true
	cd webui && rm -rf dist node_modules 2>/dev/null || true

copy_resources:
	mkdir -p build/module/bin
	mkdir -p build/module/webroot
	cp -r module/* build/module/

build_bl_flasher: copy_resources
	cd bl_flasher && CARGO_TARGET_AARCH64_LINUX_ANDROID_LINKER="$(RUST_NDK_LINKER)" \
	cargo build --target aarch64-linux-android --release
	cp bl_flasher/target/aarch64-linux-android/release/bl_flasher build/module/bin/

build_webui: copy_resources
	cd webui && npm ci && npm run build
	cp -r webui/dist/* build/module/webroot/

submodule_uefi: copy_resources
	cd ../../submodules/uefi && make build
	cp ../../submodules/uefi/build/loader.elf ./build/module/
	cd ../../submodules/uefi && make genfw_android
	cp ../../submodules/uefi/build/GenFw_android ./build/module/bin/GenFw

submodule_patcher: copy_resources
	cd ../../submodules/patcher && make build_android
	cp ../../submodules/patcher/build/patch_abl_android ./build/module/bin/patch_abl

submodule_elflinker: copy_resources
	cd ../../submodules/elflinker && make build_android
	cp ../../submodules/elflinker/build/elf_inject_android ./build/module/bin/elf_inject

submodule_ablfvextractor: copy_resources
	cd ../../submodules/ablfvextractor && make build_android
	cp ../../submodules/ablfvextractor/build/extractfv_android ./build/module/bin/extractfv

build: build_bl_flasher build_webui submodule_uefi submodule_patcher submodule_elflinker submodule_ablfvextractor
	cd build/module && zip -r ../module_android.zip ./
