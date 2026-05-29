clean:
	rm -rf build || true
copy_resources:
	mkdir -p build/toolkit
	mkdir -p build/toolkit/bin
	cp -r resources/* build/toolkit/
submodule_uefi: copy_resources
	cd ../../submodules/uefi && make build
	cp ../../submodules/uefi/build/loader.elf ./build/toolkit/loader.elf
	cd ../../submodules/uefi && make genfw
	cp ../../submodules/uefi/build/GenFw ./build/toolkit/bin/GenFw
submodule_patcher: copy_resources
	cd ../../submodules/patcher && make build
	cp ../../submodules/patcher/build/patch_abl ./build/toolkit/bin/patch_abl
submodule_elflinker: copy_resources
	cd ../../submodules/elflinker && make build
	cp ../../submodules/elflinker/build/elf_inject ./build/toolkit/bin/elf_inject
submodule_ablfvextractor: copy_resources
	cd ../../submodules/ablfvextractor && make build
	cp ../../submodules/ablfvextractor/build/extractfv ./build/toolkit/bin/extractfv
build: submodule_uefi submodule_patcher submodule_elflinker submodule_ablfvextractor
	# zip
	cd build/toolkit && zip -r ../toolkit_linux.zip ./
