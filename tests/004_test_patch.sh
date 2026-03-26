
cd "$(dirname "$0")"
# Clean previous outputs
rm -rf ./extracted || true
# Run the extraction script
python ../tools/extractfv.py 001_myron_abl.elf -o ./extracted >> /dev/null 2>&1
# Run the patching script with disabled Patch 2-5
gcc -o patch_abl ../tools/patch_abl.c -D DISABLE_PATCH_2 -D DISABLE_PATCH_3 -D DISABLE_PATCH_4 -D DISABLE_PATCH_5 -D DISABLE_PATCH_6
#run the patching tool
mkdir ./patch_output || true
./patch_abl ./extracted/LinuxLoader.efi ./patch_output/patch1.efi >> /dev/null 2>&1
EXPECTED_HASH="e550bac6ce39e58b37d51883218c654d"
ACTUAL_HASH=$(md5sum ./patch_output/patch1.efi | awk '{print $1}')
if [ "$EXPECTED_HASH" = "$ACTUAL_HASH" ]; then
    echo "Test passed: Final patched1 file matches expected hash."
else
    echo "Test failed: Final patched1 file hash does not match expected."
    echo "Expected: $EXPECTED_HASH"
    echo "Actual:   $ACTUAL_HASH"
    rm -rf ./extracted
    rm -rf ./patch_output
    rm ./patch_abl
    exit 1
fi
#enable Patch 2 and run the patching tool again
gcc -o patch_abl ../tools/patch_abl.c -D DISABLE_PATCH_1 -D DISABLE_PATCH_3 -D DISABLE_PATCH_4 -D DISABLE_PATCH_5 -D DISABLE_PATCH_6
./patch_abl ./extracted/LinuxLoader.efi ./patch_output/patch2.efi >> /dev/null 2>&1
EXPECTED_HASH="469c873732971372f9e4bf6cea91b17e"
ACTUAL_HASH=$(md5sum ./patch_output/patch2.efi | awk '{print $1}')
if [ "$EXPECTED_HASH" = "$ACTUAL_HASH" ]; then
    echo "Test passed: Final patched2 file matches expected hash."
else
    echo "Test failed: Final patched2 file hash does not match expected."
    echo "Expected: $EXPECTED_HASH"
    echo "Actual:   $ACTUAL_HASH"
    rm -rf ./extracted
    rm -rf ./patch_output
    rm ./patch_abl
    exit 1
fi
#enable Patch 3 and run the patching tool again
gcc -o patch_abl ../tools/patch_abl.c -D DISABLE_PATCH_1 -D DISABLE_PATCH_2 -D DISABLE_PATCH_4 -D DISABLE_PATCH_5 -D DISABLE_PATCH_6
./patch_abl ./extracted/LinuxLoader.efi ./patch_output/patch3.efi >> /dev/null 2>&1
EXPECTED_HASH="c59d105bae89e5c55cfe43ae72ea950a"
ACTUAL_HASH=$(md5sum ./patch_output/patch3.efi | awk '{print $1}')
if [ "$EXPECTED_HASH" = "$ACTUAL_HASH" ]; then
    echo "Test passed: Final patched3 file matches expected hash."
else
    echo "Test failed: Final patched3 file hash does not match expected."
    echo "Expected: $EXPECTED_HASH"
    echo "Actual:   $ACTUAL_HASH"
    rm -rf ./extracted
    rm -rf ./patch_output
    rm ./patch_abl
    exit 1
fi
#enable Patch 4 and run the patching tool again
gcc -o patch_abl ../tools/patch_abl.c -D DISABLE_PATCH_1 -D DISABLE_PATCH_2 -D DISABLE_PATCH_3 -D DISABLE_PATCH_5 -D DISABLE_PATCH_6
./patch_abl ./extracted/LinuxLoader.efi ./patch_output/patch4.efi >> /dev/null 2>&1
EXPECTED_HASH="d91a0611d7a23a8809b95bb7764fc901"
ACTUAL_HASH=$(md5sum ./patch_output/patch4.efi | awk '{print $1}')
if [ "$EXPECTED_HASH" = "$ACTUAL_HASH" ]; then
    echo "Test passed: Final patched4 file matches expected hash."
else
    echo "Test failed: Final patched4 file hash does not match expected."
    echo "Expected: $EXPECTED_HASH"
    echo "Actual:   $ACTUAL_HASH"
    rm -rf ./extracted
    rm -rf ./patch_output
    rm ./patch_abl
    exit 1
fi
#enable Patch 5 and run the patching tool again
gcc -o patch_abl ../tools/patch_abl.c -D DISABLE_PATCH_1 -D DISABLE_PATCH_2 -D DISABLE_PATCH_3 -D DISABLE_PATCH_4 -D DISABLE_PATCH_6
./patch_abl ./extracted/LinuxLoader.efi ./patch_output/patch5.efi >> /dev/null 2>&1
#check sum of the final patched file 1-5
EXPECTED_HASH="9a18127e8bfd10e18a53911298f000cb"
ACTUAL_HASH=$(md5sum ./patch_output/patch5.efi | awk '{print $1}')
if [ "$EXPECTED_HASH" = "$ACTUAL_HASH" ]; then
    echo "Test passed: Final patched5 file matches expected hash."
else
    echo "Test failed: Final patched5 file hash does not match expected."
    echo "Expected: $EXPECTED_HASH"
    echo "Actual:   $ACTUAL_HASH"
    rm -rf ./extracted
    rm -rf ./patch_output
    rm ./patch_abl
    exit 1
fi
rm -rf ./extracted
rm -rf ./patch_output
rm ./patch_abl
