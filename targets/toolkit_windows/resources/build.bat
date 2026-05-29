REM change to the directory of this script
@echo off
cd /d %~dp0
bin\extractfv images/abl.img
move extracted\LinuxLoader.efi ABL_original.efi
bin\patch_abl ABL_original.efi ABL.efi > patch_log.txt
bin\elf_inject loader.elf ABL.efi ABL_with_superfastboot.dll
bin\GenFw -e UEFI_APPLICATION -o ABL_with_superfastboot.efi ABL_with_superfastboot.dll
del ABL_with_superfastboot.dll
echo "Patching completed. Output: ABL_with_superfastboot.efi"