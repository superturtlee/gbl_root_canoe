REM change to the directory of this script
@echo off
cd /d %~dp0

set ABL_INPUT=images\abl.img
if not exist "%ABL_INPUT%" if exist "images\abl.elf" set ABL_INPUT=images\abl.elf
if not exist "%ABL_INPUT%" (
    echo Error: place your stock ABL image at images\abl.img or images\abl.elf
    exit /b 1
)

bin\extractfv "%ABL_INPUT%"
move extracted\LinuxLoader.efi ABL_original.efi
bin\patch_abl ABL_original.efi ABL.efi > patch_log.txt
echo "Patching completed. Output: ABL.efi"
