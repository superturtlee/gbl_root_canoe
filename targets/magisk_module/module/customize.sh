#!/system/bin/sh
ui_print "============================================="
ui_print "  Please select language / 请选择语言"
ui_print "  Vol+ = Chinese  |  Vol- = English"
ui_print "============================================="

LANG="zh"
while true; do
  keyevent=$(timeout 0.5 getevent -l 2>/dev/null)
  if echo "$keyevent" | grep -q "KEY_VOLUMEUP"; then
    LANG="zh"
    ui_print "[已选择中文 / Chinese selected]"
    break
  elif echo "$keyevent" | grep -q "KEY_VOLUMEDOWN"; then
    LANG="en"
    ui_print "[English selected / 已选择英文]"
    break
  fi
done

echo "$LANG" > "$MODPATH/lang.txt"
ksud module config set user_lang "$LANG" 2>/dev/null
sleep 1

cat > "$MODPATH/module.prop" <<EOF
id=fake_bl_efisp
version=5.0
versionCode=12
author=zaomi
EOF

if [ "$LANG" = "zh" ]; then
  echo "name=假回锁" >> "$MODPATH/module.prop"
  echo "description=自动刷新bl相关分区到非活动槽位" >> "$MODPATH/module.prop"
else
  echo "name=Fake BL EFISP" >> "$MODPATH/module.prop"
  echo "description=Automatically flash BL-related partitions to inactive slot" >> "$MODPATH/module.prop"
fi

if [ "$LANG" = "zh" ]; then
  T_VERIFY="- 正在验证设备型号"
  T_DEVICE_OK="- 设备验证完成："
  T_PERM="- 正在设置权限"
  T_EFISP_TITLE="确保你的内核没有Baseband Guard，设备BL锁已经解锁"
  T_SOC="确保你的设备是8gen5/8elitegen5"
  T_CHECK_EXP="检测漏洞中..."
  T_INSTALL_CHOICE="请选择是否第一次安装假回锁"
  T_VOL_UP="音量上为是（全新安装，需要格式化）"
  T_VOL_DOWN="音量下为否（如果之前安装过一次假回锁或者刚刚首次安装并格式化，建议选否）"
  T_TIP_YES="如果选择是，将会安装包含补丁的efisp 然后重启recovery 进行格式化，格式化后请安装一次这个模块来完成安装，这时选否"
  T_TIP_NO="如果选择否，将会安装OTA更新补丁，每次OTA更新后都需要打开这个模块来安装补丁，来保留BL版本，安装完成后重启系统即可"
  T_SEL_YES="选择了是，正在安装包含补丁的efisp"
  T_NO_SLOT="无法识别当前槽位，已中止安装"
  T_PATCH_FAIL="补丁应用失败，已中止安装"
  T_NO_GBL="没有GBL漏洞，安装失败，已中止安装"
  T_SETRW_FAIL="efisp 分区设置可写失败"
  T_FLASH_FAIL="efisp 分区刷写失败"
  T_DONE_YES="安装完成，请重启到recovery进行格式化，格式化后请安装一次这个模块来完成安装，这时选否"
  T_SEL_NO="选择了否，正在安装OTA更新模块"
  T_DONE_NO="安装完成，请重启系统即可"
else
  T_VERIFY="- Verifying device model"
  T_DEVICE_OK="- Device verified:"
  T_PERM="- Setting permissions"
  T_EFISP_TITLE="Ensure kernel has no Baseband Guard and BL bootloader is unlocked"
  T_SOC="Ensure device is 8gen5 / 8elitegen5"
  T_CHECK_EXP="Detecting exploit..."
  T_INSTALL_CHOICE="Is this your first time installing Fake BL EFISP?"
  T_VOL_UP="Vol+ = YES (Fresh install, requires format)"
  T_VOL_DOWN="Vol- = NO (If installed before or just formatted)"
  T_TIP_YES="If YES: patched efisp will be installed, reboot to recovery and format data, then reinstall this module and select NO"
  T_TIP_NO="If NO: OTA patch will be installed, after each OTA, flash this module again to keep BL version"
  T_SEL_YES="Selected YES, installing patched efisp"
  T_NO_SLOT="Failed to detect current slot, abort"
  T_PATCH_FAIL="Failed to apply patch, abort"
  T_NO_GBL="No GBL exploit found, installation failed"
  T_SETRW_FAIL="Failed to set efisp to read-write"
  T_FLASH_FAIL="Failed to flash efisp"
  T_DONE_YES="Install complete. Reboot to recovery and format data, then reinstall module and choose NO"
  T_SEL_NO="Selected NO, installing OTA update patch"
  T_DONE_NO="Install complete, please reboot"
fi

ui_print "$T_VERIFY"
_model=$(getprop ro.product.model 2>/dev/null)
_name=$(getprop ro.product.name 2>/dev/null)
_inc=$(getprop ro.build.version.incremental 2>/dev/null)
ui_print "$T_DEVICE_OK $_model / $_name / $_inc"
ui_print "$T_PERM"

set_perm_recursive "$MODPATH/bin" 0 0 0755 0755
set_perm_recursive "$MODPATH/webroot" 0 0 0755 0644
set_perm "$MODPATH/module.prop" 0 0 0644
set_perm "$MODPATH/customize.sh" 0 0 0755

detect_current_slot() {
  case "$(getprop ro.boot.slot_suffix 2>/dev/null)" in
    _a) echo _a ;;
    _b) echo _b ;;
    *) return 1 ;;
  esac
}

BY_NAME_DIR=/dev/block/by-name
RUNTIME_DIR=$MODPATH/tmp
mkdir -p $RUNTIME_DIR

ui_print "$T_EFISP_TITLE"
ui_print "$T_SOC"
ui_print "$T_CHECK_EXP"
current_slot=$(detect_current_slot)

ui_print "$T_INSTALL_CHOICE"
ui_print "$T_VOL_UP"
ui_print "$T_VOL_DOWN"
ui_print "$T_TIP_YES"
ui_print "$T_TIP_NO"

while true; do
  keyevent=$(timeout 0.5 getevent -l 2>/dev/null)
  if echo "$keyevent" | grep -q "KEY_VOLUMEUP"; then
    ui_print "$T_SEL_YES"
    if [ -z "$current_slot" ]; then
      ui_print "$T_NO_SLOT"
      abort "slot detection failed"
    fi
    abl_part="$BY_NAME_DIR/abl$current_slot"
    $MODPATH/bin/extractfv -o $RUNTIME_DIR -v "$abl_part" >> $RUNTIME_DIR/extract.log 2>&1
    $MODPATH/bin/patch_abl $RUNTIME_DIR/LinuxLoader.efi $RUNTIME_DIR/patched.efi >> $RUNTIME_DIR/patch.log 2>&1
    if [ ! -f $RUNTIME_DIR/patched.efi ]; then
      ui_print "$T_PATCH_FAIL"
      abort "patch failed"
    fi
    if grep -q "Warning: Failed to patch ABL GBL" $RUNTIME_DIR/patch.log; then
      ui_print "$T_NO_GBL"
      abort "no exploit"
    fi
    if ! blockdev --setrw $BY_NAME_DIR/efisp >> $RUNTIME_DIR/flash.log 2>&1; then
      ui_print "$T_SETRW_FAIL"
      abort "setrw failed"
    fi
    if ! dd if=$RUNTIME_DIR/patched.efi of=$BY_NAME_DIR/efisp bs=4M conv=fsync >> $RUNTIME_DIR/flash.log 2>&1; then
      ui_print "$T_FLASH_FAIL"
      abort "flash failed"
    fi
    sync
    ui_print "$T_DONE_YES"
    rm -rf $RUNTIME_DIR
    break
  elif echo "$keyevent" | grep -q "KEY_VOLUMEDOWN"; then
    ui_print "$T_SEL_NO"
    ui_print "$T_DONE_NO"
    rm -rf $RUNTIME_DIR
    break
  fi
done
