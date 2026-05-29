# Test script for extracting FV from ABL image
DIR="$(dirname "$0")"
cd "$(dirname "$0")"

# Clean previous outputs
rm -rf ./extracted || true
# Run the extraction script
cd ../
make build
cd $DIR
../build/extractfv 001_myron_abl.elf -o ./extracted >> /dev/null 2>&1
md5sum extracted/LinuxLoader.efi > extracted/LinuxLoader.efi.md5
#compare with expected hash (replace with actual expected hash)
EXPECTED_HASH="993388c4312750c9bfd0df8aae8f1e11"
ACTUAL_HASH=$(cat extracted/LinuxLoader.efi.md5 | awk '{print $1}')
rm -rf ./extracted
if [ "$EXPECTED_HASH" = "$ACTUAL_HASH" ]; then
    echo "Test passed: Extracted FV matches expected hash."
else
    echo "Test failed: Extracted FV hash does not match expected."
    echo "Expected: $EXPECTED_HASH"
    echo "Actual:   $ACTUAL_HASH"
    exit 1
fi
