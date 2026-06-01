patch:
	rm -rf build || true
	cd ../../submodules/ablfvextractor && make build
	cd ../../submodules/patcher && make build
	mkdir -p build
	../../submodules/ablfvextractor/build/extractfv ./ablimg/abl.img -o ./build/
	mv ./build/LinuxLoader.efi ./build/ABL_original.efi
	../../submodules/patcher/build/patch_abl ./build/ABL_original.efi ./build/ABL.efi
