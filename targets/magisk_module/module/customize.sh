#!/system/bin/sh
SKIPUNZIP=1

ui_print "- Fake BL EFISP v5.0 / 假回锁 v5.0"
ui_print "- Device / 设备: $(getprop ro.product.model) ($(getprop ro.product.name))"

mkdir -p "$MODPATH/bin"
mkdir -p "$MODPATH/webroot"
mkdir -p "$MODPATH/tmp"

ui_print "- Extracting files / 正在解压..."
unzip -o "$ZIPFILE" 'module.prop' -d "$MODPATH" >&2
unzip -o "$ZIPFILE" 'uninstall.sh' -d "$MODPATH" >&2
unzip -o "$ZIPFILE" 'bin/*' -d "$MODPATH" >&2
unzip -o "$ZIPFILE" 'webroot/*' -d "$MODPATH" >&2

ui_print "- Setting permissions / 设置权限..."
set_perm_recursive "$MODPATH/bin" 0 0 0755 0755
set_perm_recursive "$MODPATH/webroot" 0 0 0755 0644
set_perm "$MODPATH/module.prop" 0 0 0644

ui_print "- Detecting current slot / 检测当前槽位..."
current_slot=$(getprop ro.boot.slot_suffix 2>/dev/null)
case "$current_slot" in
    _a|_b) ;;
    *) ui_print "! Failed to detect slot / 无法检测槽位"; abort "slot detection failed" ;;
esac

RUNTIME_DIR="$MODPATH/tmp"
BY_NAME_DIR="/dev/block/by-name"

ui_print "- Ensure kernel has no Baseband Guard and BL is unlocked"
ui_print "  确保内核没有 Baseband Guard，设备 BL 锁已解锁"
ui_print "- Ensure device is 8gen5 / 8elitegen5"
ui_print "  确保设备使用 8gen5 / 8elitegen5 处理器"

ui_print "============================================="
ui_print "  Fresh install or OTA reinstall?"
ui_print "  Vol+ = Fresh install (patches efisp) / 全新安装"
ui_print "  Vol- = OTA reinstall (skip efisp) / OTA重装"
ui_print "============================================="

while true; do
  keyevent=$(timeout 0.5 getevent -l 2>/dev/null)
  if echo "$keyevent" | grep -q "KEY_VOLUMEUP"; then
    ui_print "- Fresh install selected / 已选择全新安装"

    abl_part="$BY_NAME_DIR/abl$current_slot"
    ui_print "- Extracting ABL from slot $current_slot..."
    "$MODPATH/bin/extractfv" -o "$RUNTIME_DIR" -v "$abl_part" >> "$RUNTIME_DIR/extract.log" 2>&1

    ui_print "- Patching ABL / 正在修补 ABL..."
    "$MODPATH/bin/patch_abl" "$RUNTIME_DIR/LinuxLoader.efi" "$RUNTIME_DIR/patched.efi" >> "$RUNTIME_DIR/patch.log" 2>&1

    if [ ! -f "$RUNTIME_DIR/patched.efi" ]; then
      ui_print "! Patch failed / 修补失败"
      abort "patch failed"
    fi

    if grep -q "Warning: Failed to patch ABL GBL" "$RUNTIME_DIR/patch.log"; then
      ui_print "! No GBL exploit found / 未发现 GBL 漏洞"
      abort "no exploit"
    fi

    ui_print "- Flashing efisp / 正在刷写 efisp..."
    if ! blockdev --setrw "$BY_NAME_DIR/efisp" >> "$RUNTIME_DIR/flash.log" 2>&1; then
      ui_print "! Failed to set efisp writable / 设置可写失败"
      abort "setrw failed"
    fi

    if ! dd if="$RUNTIME_DIR/patched.efi" of="$BY_NAME_DIR/efisp" bs=4M conv=fsync >> "$RUNTIME_DIR/flash.log" 2>&1; then
      ui_print "! Failed to flash efisp / 刷写失败"
      abort "flash failed"
    fi
    sync

    rm -rf "$RUNTIME_DIR"
    ui_print "- Done. Reboot to recovery, format data, reinstall and select Vol-."
    ui_print "  完成。重启到 recovery 格式化 data，重装模块时选 Vol-。"

    break
  elif echo "$keyevent" | grep -q "KEY_VOLUMEDOWN"; then
    ui_print "- OTA reinstall selected / 已选择 OTA 重装"
    rm -rf "$RUNTIME_DIR"
    ui_print "- Done. After each OTA, reinstall and select Vol-, then use WebUI."
    ui_print "  完成。每次 OTA 后重装模块并选 Vol-，然后使用 WebUI 保护槽位。"
    break
  fi
done
