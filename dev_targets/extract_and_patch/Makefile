patch:
	rm -rf build || true
	cd ../../submodules/ablfvextractor && make build
	cd ../../submodules/patcher && make build
	mkdir -p build
	../../submodules/ablfvextractor/build/extractfv ./ablimg/abl.img -o ./build/
	mv ./LinuxLoader.efi ./ABL_original.efi
	../../submodules/patcher/build/patch_abl ./ABL_original.efi ./ABL.efi
