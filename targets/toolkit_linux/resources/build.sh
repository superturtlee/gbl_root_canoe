set -euo pipefail

SCRIPTDIR=$(dirname "$0")
cd "$SCRIPTDIR"
./bin/extractfv ./images/abl.img -o ./
mv ./LinuxLoader.efi ./ABL_original.efi
./bin/patch_abl ./ABL_original.efi ./ABL.efi > ./patch_log.txt
./bin/elf_inject ./loader.elf ./ABL.efi ./ABL_with_superfastboot.dll
./bin/GenFw -e UEFI_APPLICATION -o ./ABL_with_superfastboot.efi ./ABL_with_superfastboot.dll
rm ./ABL_with_superfastboot.dll
cat ./patch_log.txt
echo "Patching completed. Output: ABL_with_superfastboot.efi"