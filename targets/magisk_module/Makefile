clean:
	rm -rf build || true
copy_resources:
	mkdir -p build/module
	cp -r module/* build/module/
submodule_uefi: copy_resources
	cd ../../submodules/uefi && make build
	cp ../../submodules/uefi/build/loader.elf ./build/module/loader.elf
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
build: submodule_uefi submodule_patcher submodule_elflinker submodule_ablfvextractor
	# zip
	cd build/module && zip -r ../module_android.zip ./
