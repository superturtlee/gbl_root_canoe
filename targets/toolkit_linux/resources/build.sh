set -euo pipefail

SCRIPTDIR=$(dirname "$0")
cd "$SCRIPTDIR"
./bin/extractfv ./images/abl.img -o ./
mv ./LinuxLoader.efi ./ABL_original.efi
./bin/patch_abl ./ABL_original.efi ./ABL.efi > ./patch_log.txt
echo "Patching completed. Output: ABL.efi"