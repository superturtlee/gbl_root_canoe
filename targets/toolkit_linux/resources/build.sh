set -euo pipefail

SCRIPTDIR=$(dirname "$0")
cd "$SCRIPTDIR"

ABL_INPUT="./images/abl.img"
if [ ! -f "$ABL_INPUT" ] && [ -f "./images/abl.elf" ]; then
  ABL_INPUT="./images/abl.elf"
fi

if [ ! -f "$ABL_INPUT" ]; then
  echo "Error: place your stock ABL image at ./images/abl.img or ./images/abl.elf" >&2
  exit 1
fi

./bin/extractfv "$ABL_INPUT" -o ./
mv ./LinuxLoader.efi ./ABL_original.efi
./bin/patch_abl ./ABL_original.efi ./ABL.efi > ./patch_log.txt
echo "Patching completed. Output: ABL.efi"
