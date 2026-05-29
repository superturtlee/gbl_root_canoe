clean:
	rm -rf edk2/Build || true
	rm -rf edk2/Conf || true
	rm -rf ./build || true

build: 
	cp -r ./Conf ./edk2/
	bash -c 'cd edk2 && . ./edksetup.sh && make BOARD_BOOTLOADER_PRODUCT_NAME=canoe TARGET_ARCHITECTURE=AARCH64 TARGET=RELEASE \
  		CLANG_BIN=/usr/bin/ CLANG_PREFIX=aarch64-linux-gnu- VERIFIED_BOOT_ENABLED=1 \
  		VERIFIED_BOOT_LE=0 AB_RETRYCOUNT_DISABLE=0 TARGET_BOARD_TYPE_AUTO=0 \
  		BUILD_USES_RECOVERY_AS_BOOT=0 DISABLE_PARALLEL_DOWNLOAD_FLASH=0 PVMFW_BCC_ENABLED=-DPVMFW_BCC\
  		REMOVE_CARVEOUT_REGION=1 QSPA_BOOTCONFIG_ENABLE=1 USER_BUILD_VARIANT=0 \
  		PREBUILT_HOST_TOOLS="BUILD_CC=clang BUILD_CXX=clang++ LDPATH=-fuse-ld=lld BUILD_AR=llvm-ar"' || true
	# test if the build is successful by checking the output file
	if [ ! -f edk2/Build/RELEASE_CLANG35/AARCH64/LinuxLoader.efi ]; then \
		echo "Build failed"; \
		exit 1; \
	fi
	mkdir -p build
	cp edk2/Build/RELEASE_CLANG35/AARCH64/QcomModulePkg/Application/LinuxLoader/LinuxLoader/DEBUG/LinuxLoader.dll ./build/loader.elf
	@echo "Loader built successfully: build/loader.elf"

build_superfbonly:
	cp -r ./Conf ./edk2/
	bash -c 'cd edk2 && . ./edksetup.sh && make BOARD_BOOTLOADER_PRODUCT_NAME=canoe TARGET_ARCHITECTURE=AARCH64 TARGET=RELEASE \
  		CLANG_BIN=/usr/bin/ CLANG_PREFIX=aarch64-linux-gnu- VERIFIED_BOOT_ENABLED=1 \
  		VERIFIED_BOOT_LE=0 AB_RETRYCOUNT_DISABLE=0 TARGET_BOARD_TYPE_AUTO=0 \
  		BUILD_USES_RECOVERY_AS_BOOT=0 DISABLE_PARALLEL_DOWNLOAD_FLASH=0 PVMFW_BCC_ENABLED=-DPVMFW_BCC\
  		REMOVE_CARVEOUT_REGION=1 QSPA_BOOTCONFIG_ENABLE=1 USER_BUILD_VARIANT=0 TEST_ADAPTER=1 \
  		PREBUILT_HOST_TOOLS="BUILD_CC=clang BUILD_CXX=clang++ LDPATH=-fuse-ld=lld BUILD_AR=llvm-ar"' || true
	# test if the build is successful by checking the output file
	if [ ! -f edk2/Build/RELEASE_CLANG35/AARCH64/LinuxLoader.efi ]; then \
		echo "Build failed"; \
		exit 1; \
	fi
	mkdir -p ./build
	cp edk2/Build/RELEASE_CLANG35/AARCH64/LinuxLoader.efi ./build/only_superfastboot.efi
	@echo "Only SuperFastBoot built successfully: build/only_superfastboot.efi"

build_generic:
	cp -r ./Conf ./edk2/
	bash -c 'cd edk2 && . ./edksetup.sh && make BOARD_BOOTLOADER_PRODUCT_NAME=canoe TARGET_ARCHITECTURE=AARCH64 TARGET=RELEASE \
  		CLANG_BIN=/usr/bin/ CLANG_PREFIX=aarch64-linux-gnu- VERIFIED_BOOT_ENABLED=1 \
  		VERIFIED_BOOT_LE=0 AB_RETRYCOUNT_DISABLE=0 TARGET_BOARD_TYPE_AUTO=0 \
  		BUILD_USES_RECOVERY_AS_BOOT=0 DISABLE_PARALLEL_DOWNLOAD_FLASH=0 PVMFW_BCC_ENABLED=-DPVMFW_BCC\
  		REMOVE_CARVEOUT_REGION=1 QSPA_BOOTCONFIG_ENABLE=1 USER_BUILD_VARIANT=0 AUTO_PATCH_ABL=1 DISABLE_PRINT=1\
  		PREBUILT_HOST_TOOLS="BUILD_CC=clang BUILD_CXX=clang++ LDPATH=-fuse-ld=lld BUILD_AR=llvm-ar"' || true
	# test if the build is successful by checking the output file
	if [ ! -f edk2/Build/RELEASE_CLANG35/AARCH64/LinuxLoader.efi ]; then \
		echo "Build failed"; \
		exit 1; \
	fi
	mkdir -p ./build
	cp edk2/Build/RELEASE_CLANG35/AARCH64/LinuxLoader.efi ./build/generic_superfastboot.efi
	@echo "Generic SuperFastBoot built successfully: build/generic_superfastboot.efi"

genfw: build
	cp ./edk2/Build/Source/C/bin/GenFw ./build

genfw_android:
	bash tools/build_android_genfw.sh

genfw_windows:
	x86_64-w64-mingw32-gcc -O2 -fshort-wchar -fno-strict-aliasing -fwrapv \
		-Iedk2/BaseTools/Source/C -Iedk2/BaseTools/Source/C/Include -Iedk2/BaseTools/Source/C/Include/Common -Iedk2/BaseTools/Source/C/Include/IndustryStandard -Iedk2/BaseTools/Source/C/Include/AArch64 -Iedk2/BaseTools/Source/C/Common -Iedk2/BaseTools/Source/C/GenFw \
		-o ./build/GenFw.exe \
		edk2/BaseTools/Source/C/GenFw/GenFw.c edk2/BaseTools/Source/C/GenFw/ElfConvert.c edk2/BaseTools/Source/C/GenFw/Elf32Convert.c edk2/BaseTools/Source/C/GenFw/Elf64Convert.c \
		edk2/BaseTools/Source/C/Common/BasePeCoff.c edk2/BaseTools/Source/C/Common/BinderFuncs.c edk2/BaseTools/Source/C/Common/CommonLib.c edk2/BaseTools/Source/C/Common/Crc32.c edk2/BaseTools/Source/C/Common/Decompress.c edk2/BaseTools/Source/C/Common/EfiCompress.c edk2/BaseTools/Source/C/Common/EfiUtilityMsgs.c edk2/BaseTools/Source/C/Common/FirmwareVolumeBuffer.c edk2/BaseTools/Source/C/Common/FvLib.c edk2/BaseTools/Source/C/Common/MemoryFile.c edk2/BaseTools/Source/C/Common/MyAlloc.c edk2/BaseTools/Source/C/Common/OsPath.c edk2/BaseTools/Source/C/Common/ParseGuidedSectionTools.c edk2/BaseTools/Source/C/Common/ParseInf.c edk2/BaseTools/Source/C/Common/PeCoffLoaderEx.c edk2/BaseTools/Source/C/Common/SimpleFileParsing.c edk2/BaseTools/Source/C/Common/StringFuncs.c edk2/BaseTools/Source/C/Common/TianoCompress.c \
		-Wl,-Bstatic -luuid -Wl,-Bdynamic