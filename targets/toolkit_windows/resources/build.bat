REM change to the directory of this script
@echo off
cd /d %~dp0
bin\extractfv images/abl.img
move extracted\LinuxLoader.efi ABL_original.efi
bin\patch_abl ABL_original.efi ABL.efi > patch_log.txt
echo "Patching completed. Output: ABL.efi"