.PHONY: \
	submodule_patcher_clean \
	submodule_ablfvextractor_clean \
	clean_submodules \
	target_toolkit_windows_clean \
	target_toolkit_linux_clean \
	target_magisk_module_clean \
	tools_vbmetafixer_clean \
	targets_clean \
	clean \
	target_toolkit_windows \
	target_toolkit_linux \
	target_magisk_module \
	dev_target_extract_and_patch \
	tools_vbmetafixer_linux \
	tools_vbmetafixer_windows

submodule_patcher_clean:
	cd submodules/patcher && make clean
submodule_ablfvextractor_clean:
	cd submodules/ablfvextractor && make clean
clean_submodules: submodule_patcher_clean submodule_ablfvextractor_clean

target_toolkit_windows_clean:
	cd targets/toolkit_windows && make clean
target_toolkit_linux_clean:
	cd targets/toolkit_linux && make clean
target_magisk_module_clean:
	cd targets/magisk_module && make clean
tools_vbmetafixer_clean:
	cd tools/vbmetafixer && make clean
targets_clean: target_toolkit_windows_clean target_toolkit_linux_clean target_magisk_module_clean tools_vbmetafixer_clean

clean: clean_submodules targets_clean

target_toolkit_windows:
	cd targets/toolkit_windows && make build 
target_toolkit_linux:
	cd targets/toolkit_linux && make build
target_magisk_module:
	cd targets/magisk_module && make build

dev_target_extract_and_patch:
	cd dev_targets/extract_and_patch && make patch

# tools not dependency of main project, build separately
tools_vbmetafixer_linux:
	cd tools/vbmetafixer && make build
tools_vbmetafixer_windows:
	cd tools/vbmetafixer && make build_windows
