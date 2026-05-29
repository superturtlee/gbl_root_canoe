#!/system/bin/sh

MODDIR=${0%/*}

rm -rf "$MODDIR/tmp"
ui_print "卸载完成,仅卸载OTA更新辅助，假回锁请自行卸载（因为需要清数据）"