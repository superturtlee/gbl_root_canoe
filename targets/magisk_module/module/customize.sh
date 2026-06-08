#!/system/bin/sh
SKIPUNZIP=1

ui_print "- Fake BL EFISP v5.1 / 假回锁 v5.1"

mkdir -p "$MODPATH/bin"
mkdir -p "$MODPATH/webroot"
mkdir -p "$MODPATH/tmp"

ui_print "- Extracting files / 正在解压..."
unzip -o "$ZIPFILE" 'module.prop' -d "$MODPATH" >&2
unzip -o "$ZIPFILE" 'uninstall.sh' -d "$MODPATH" >&2
unzip -o "$ZIPFILE" 'bin/*' -d "$MODPATH" >&2
unzip -o "$ZIPFILE" 'webroot/*' -d "$MODPATH" >&2
[ -f "$MODPATH/loader.elf" ] || unzip -o "$ZIPFILE" 'loader.elf' -d "$MODPATH" >&2

ui_print "- Setting permissions / 正在设置权限..."
set_perm_recursive "$MODPATH/bin" 0 0 0755 0755
set_perm_recursive "$MODPATH/webroot" 0 0 0755 0644
set_perm "$MODPATH/module.prop" 0 0 0644

ui_print "- Verifying device model / 正在验证设备型号..."
_model=$(getprop ro.product.model 2>/dev/null)
_name=$(getprop ro.product.name 2>/dev/null)
_inc=$(getprop ro.build.version.incremental 2>/dev/null)
ui_print "- Device verified / 设备验证完成: $_model / $_name / $_inc"

ui_print "- Ensure kernel has no Baseband Guard and BL bootloader is unlocked"
ui_print "  确保你的内核没有Baseband Guard，设备BL锁已经解锁"
ui_print "- Ensure device is 8gen5 / 8elitegen5"
ui_print "  确保你的设备是8gen5/8elitegen5"

current_slot=$(getprop ro.boot.slot_suffix 2>/dev/null)
case "$current_slot" in
    _a|_b) ;;
    *) ui_print "Failed to detect current slot, abort / 无法识别当前槽位，已中止安装"; abort "slot detection failed" ;;
esac

RUNTIME_DIR="$MODPATH/tmp"
BY_NAME_DIR="/dev/block/by-name"

ui_print "- Detecting exploit / 检测漏洞中..."

ui_print "============================================="
ui_print "  Is this your first time installing Fake BL EFISP?"
ui_print "  请选择是否第一次安装假回锁"
ui_print ""
ui_print "  Vol+ = YES (Fresh install, requires format)"
ui_print "        音量上为是（全新安装，需要格式化）"
ui_print "  Vol- = NO (If installed before or just formatted)"
ui_print "        音量下为否（如果之前安装过一次假回锁或者刚刚首次安装并格式化，建议选否）"
ui_print ""
ui_print "  If YES: patched efisp will be installed, reboot to recovery"
ui_print "          and format data, then reinstall this module and select NO"
ui_print "          如果选择是，将会安装包含补丁的efisp"
ui_print "          然后重启recovery进行格式化"
ui_print "          格式化后请安装一次这个模块来完成安装，这时选否"
ui_print "  If NO: OTA patch will be installed, after each OTA,"
ui_print "         flash this module again to keep BL version"
ui_print "         如果选择否，将会安装OTA更新补丁"
ui_print "         每次OTA更新后都需要打开这个模块来安装补丁"
ui_print "         来保留BL版本，安装完成后重启系统即可"
ui_print "============================================="

while true; do
  keyevent=$(timeout 0.5 getevent -l 2>/dev/null)
  if echo "$keyevent" | grep -q "KEY_VOLUMEUP"; then
    ui_print "- Selected YES, installing patched efisp / 选择了是，正在安装包含补丁的efisp"

    abl_part="$BY_NAME_DIR/abl$current_slot"
    ui_print "- Extracting ABL from slot $current_slot / 正在提取ABL..."
    "$MODPATH/bin/extractfv" -o "$RUNTIME_DIR" -v "$abl_part" >> "$RUNTIME_DIR/extract.log" 2>&1

    ui_print "- Patching ABL / 正在修补ABL..."
    "$MODPATH/bin/patch_abl" "$RUNTIME_DIR/LinuxLoader.efi" "$RUNTIME_DIR/patched.efi" >> "$RUNTIME_DIR/patch.log" 2>&1

    if [ ! -f "$RUNTIME_DIR/patched.efi" ]; then
      ui_print "Failed to apply patch, abort / 补丁应用失败，已中止安装"
      abort "patch failed"
    fi

    if grep -q "Warning: Failed to patch ABL GBL" "$RUNTIME_DIR/patch.log"; then
      ui_print "No GBL exploit found, installation failed / 没有GBL漏洞，安装失败，已中止安装"
      abort "no exploit"
    fi

    ui_print "- Flashing efisp / 正在刷写efisp..."
    if ! blockdev --setrw "$BY_NAME_DIR/efisp" >> "$RUNTIME_DIR/flash.log" 2>&1; then
      ui_print "Failed to set efisp to read-write / efisp 分区设置可写失败"
      abort "setrw failed"
    fi

    if ! dd if="$RUNTIME_DIR/patched.efi" of="$BY_NAME_DIR/efisp" bs=4M conv=fsync >> "$RUNTIME_DIR/flash.log" 2>&1; then
      ui_print "Failed to flash efisp / efisp 分区刷写失败"
      abort "flash failed"
    fi
    sync

    rm -rf "$RUNTIME_DIR"
    ui_print "- Install complete. Reboot to recovery and format data, then reinstall module and choose NO."
    ui_print "  安装完成。请重启到recovery进行格式化，格式化后请安装一次这个模块来完成安装，这时选否。"

    break
  elif echo "$keyevent" | grep -q "KEY_VOLUMEDOWN"; then
    ui_print "- Selected NO, installing OTA update patch / 选择了否，正在安装OTA更新模块"
    rm -rf "$RUNTIME_DIR"
    ui_print "- Install complete, please reboot / 安装完成，请重启系统即可"
    break
  fi
done
