clean:
	rm -rf build || true
copy_resources:
	mkdir -p build/toolkit
	mkdir -p build/toolkit/bin
	cp -r resources/* build/toolkit/
submodule_uefi: copy_resources
	cd ../../submodules/uefi && make build
	cp ../../submodules/uefi/build/loader.elf ./build/toolkit/loader.elf
	cd ../../submodules/uefi && make genfw_windows
	cp ../../submodules/uefi/build/GenFw.exe ./build/toolkit/bin/GenFw.exe
submodule_patcher: copy_resources
	cd ../../submodules/patcher && make build_windows
	cp ../../submodules/patcher/build/patch_abl.exe ./build/toolkit/bin/patch_abl.exe
submodule_elflinker: copy_resources
	cd ../../submodules/elflinker && make build_windows
	cp ../../submodules/elflinker/build/elf_inject.exe ./build/toolkit/bin/elf_inject.exe
submodule_ablfvextractor: copy_resources
	cd ../../submodules/ablfvextractor && make build_windows
	cp ../../submodules/ablfvextractor/build/extractfv.exe ./build/toolkit/bin/extractfv.exe
build: submodule_uefi submodule_patcher submodule_elflinker submodule_ablfvextractor
	# zip
	cd build/toolkit && zip -r ../toolkit_windows.zip ./
