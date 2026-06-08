#!/system/bin/sh
SKIPUNZIP=1

ui_print "- Fake BL EFISP v5.0"
ui_print "- Device: $(getprop ro.product.model) ($(getprop ro.product.name))"

mkdir -p "$MODPATH/bin"
mkdir -p "$MODPATH/webroot"
mkdir -p "$MODPATH/tmp"

ui_print "- Extracting files..."
unzip -o "$ZIPFILE" 'module.prop' -d "$MODPATH" >&2
unzip -o "$ZIPFILE" 'uninstall.sh' -d "$MODPATH" >&2
unzip -o "$ZIPFILE" 'bin/*' -d "$MODPATH" >&2
unzip -o "$ZIPFILE" 'webroot/*' -d "$MODPATH" >&2

ui_print "- Setting permissions..."
set_perm_recursive "$MODPATH/bin" 0 0 0755 0755
set_perm_recursive "$MODPATH/webroot" 0 0 0755 0644
set_perm "$MODPATH/module.prop" 0 0 0644

ui_print "- Detecting current slot..."
current_slot=$(getprop ro.boot.slot_suffix 2>/dev/null)
case "$current_slot" in
    _a|_b) ;;
    *) ui_print "! Failed to detect slot"; abort "slot detection failed" ;;
esac

RUNTIME_DIR="$MODPATH/tmp"
BY_NAME_DIR="/dev/block/by-name"

ui_print "============================================="
ui_print "  Fresh install or OTA reinstall?"
ui_print "  Vol+ = Fresh install (patches efisp)"
ui_print "  Vol- = OTA reinstall (skip efisp)"
ui_print "============================================="

while true; do
  keyevent=$(timeout 0.5 getevent -l 2>/dev/null)
  if echo "$keyevent" | grep -q "KEY_VOLUMEUP"; then
    ui_print "- Fresh install selected"

    abl_part="$BY_NAME_DIR/abl$current_slot"
    ui_print "- Extracting ABL from slot $current_slot..."
    "$MODPATH/bin/extractfv" -o "$RUNTIME_DIR" -v "$abl_part" >> "$RUNTIME_DIR/extract.log" 2>&1

    ui_print "- Patching ABL..."
    "$MODPATH/bin/patch_abl" "$RUNTIME_DIR/LinuxLoader.efi" "$RUNTIME_DIR/patched.efi" >> "$RUNTIME_DIR/patch.log" 2>&1

    if [ ! -f "$RUNTIME_DIR/patched.efi" ]; then
      ui_print "! Patch failed"
      abort "patch failed"
    fi

    if grep -q "Warning: Failed to patch ABL GBL" "$RUNTIME_DIR/patch.log"; then
      ui_print "! No GBL exploit found"
      abort "no exploit"
    fi

    ui_print "- Flashing efisp..."
    if ! blockdev --setrw "$BY_NAME_DIR/efisp" >> "$RUNTIME_DIR/flash.log" 2>&1; then
      ui_print "! Failed to set efisp writable"
      abort "setrw failed"
    fi

    if ! dd if="$RUNTIME_DIR/patched.efi" of="$BY_NAME_DIR/efisp" bs=4M conv=fsync >> "$RUNTIME_DIR/flash.log" 2>&1; then
      ui_print "! Failed to flash efisp"
      abort "flash failed"
    fi
    sync

    rm -rf "$RUNTIME_DIR"
    ui_print "- Done. Reboot to recovery and format data, then reinstall and select Vol-."

    break
  elif echo "$keyevent" | grep -q "KEY_VOLUMEDOWN"; then
    ui_print "- OTA reinstall selected"
    rm -rf "$RUNTIME_DIR"
    ui_print "- Done. After each OTA, reinstall and select Vol-, then use WebUI for slot protection."
    break
  fi
done
