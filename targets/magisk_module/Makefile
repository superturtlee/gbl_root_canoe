clean:
	rm -rf build || true
copy_resources:
	mkdir -p build/module
	cp -r module/* build/module/
submodule_patcher: copy_resources
	cd ../../submodules/patcher && make build_android
	cp ../../submodules/patcher/build/patch_abl_android ./build/module/bin/patch_abl
submodule_ablfvextractor: copy_resources
	cd ../../submodules/ablfvextractor && make build_android
	cp ../../submodules/ablfvextractor/build/extractfv_android ./build/module/bin/extractfv
build: submodule_patcher submodule_ablfvextractor
	# zip
	cd build/module && zip -r ../module_android.zip ./
