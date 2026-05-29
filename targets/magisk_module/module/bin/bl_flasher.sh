#!/system/bin/sh
if [ -z "$MODDIR" ]; then
  MODDIR=$(CDPATH= cd -- "$(dirname "$0")/.." 2>/dev/null && pwd)
fi
if [ -z "$MODDIR" ]; then
  echo 'ERROR=MODDIR detection failed' >&2
  exit 1
fi

LANG=zh
if [ -f "$MODDIR/lang.txt" ]; then
  USER_LANG=$(cat "$MODDIR/lang.txt" | tr -d '[:space:]')
  if [ "$USER_LANG" = "en" ]; then
    LANG=en
  fi
fi

if [ "$LANG" = "zh" ]; then
  TEXT_IDLE="等待操作"
  TEXT_NO_SLOT="无法识别当前槽位"
  TEXT_NO_TARGET_SLOT="无法计算目标槽位"
  TEXT_FLASHING="正在将镜像刷写到槽位"
  TEXT_DEBUG_MODE="调试模式：仅处理不刷写"
  TEXT_DEBUG_DONE="调试完成，文件保存在"
  TEXT_DEBUG_ABL_DONE="调试完成，ABL 已提取到"
  TEXT_DEBUG_FAILED="调试过程中出错"
  TEXT_EXTRACT_FAILED="ABL 提取失败"
  TEXT_PATCH_FAILED="补丁应用失败"
  TEXT_EFISP_SET_RW_FAILED="efisp 分区设置可写失败"
  TEXT_EFISP_FLASH_FAILED="efisp 刷写失败"
  TEXT_EFISP_FLASH_OK="efisp 刷写完成"
  TEXT_GBL_VULN="检测到GBL漏洞，跳过BL刷写"
  TEXT_GBL_VULN_SKIP="已跳过BL刷写"
  TEXT_GBL_DETECT_FAILED="漏洞检测失败，继续流程"
  TEXT_NO_GBL_VULN="未检测到GBL漏洞"
  TEXT_INJECT_SFB="注入 superfastboot..."
  TEXT_NO_LOADER_ELF="loader.elf 不存在"
  TEXT_INJECT_FAILED="注入失败"
  TEXT_GENFW_FAILED="转换失败"
  TEXT_INJECT_OK="注入完成"
  TEXT_EFISP_WARN="efisp 刷写失败，继续刷入BL"
  TEXT_SET_RW_FAILED="分区设置可写失败"
  TEXT_FLASH_PART="刷写"
  TEXT_FLASH_OK="完成"
  TEXT_ALL_OK="全部完成（含efisp）"
  TEXT_ALL_OK_NO_EFISP="全部完成（不含efisp）"
  TEXT_BUSY="任务正在运行"
  TEXT_LOG_CLEARED="日志已清空"
else
  TEXT_IDLE="Waiting"
  TEXT_NO_SLOT="Cannot detect current slot"
  TEXT_NO_TARGET_SLOT="Cannot detect target slot"
  TEXT_FLASHING="Flashing to slot"
  TEXT_DEBUG_MODE="Debug Mode"
  TEXT_DEBUG_DONE="Debug done"
  TEXT_DEBUG_ABL_DONE="ABL extracted"
  TEXT_DEBUG_FAILED="Debug error"
  TEXT_EXTRACT_FAILED="ABL extract failed"
  TEXT_PATCH_FAILED="Patch failed"
  TEXT_EFISP_SET_RW_FAILED="efisp setrw failed"
  TEXT_EFISP_FLASH_FAILED="efisp flash failed"
  TEXT_EFISP_FLASH_OK="efisp flash ok"
  TEXT_GBL_VULN="GBL vuln detected, skip BL flash"
  TEXT_GBL_VULN_SKIP="Skipped BL flash"
  TEXT_GBL_DETECT_FAILED="Vuln check failed"
  TEXT_NO_GBL_VULN="No GBL vuln found"
  TEXT_INJECT_SFB="Injecting superfastboot"
  TEXT_NO_LOADER_ELF="loader.elf missing"
  TEXT_INJECT_FAILED="Inject failed"
  TEXT_GENFW_FAILED="Convert failed"
  TEXT_INJECT_OK="Injected"
  TEXT_EFISP_WARN="efisp failed, continue BL"
  TEXT_SET_RW_FAILED="setrw failed"
  TEXT_FLASH_PART="Flashing"
  TEXT_FLASH_OK="done"
  TEXT_ALL_OK="All done (with efisp)"
  TEXT_ALL_OK_NO_EFISP="All done (no efisp)"
  TEXT_BUSY="Task running"
  TEXT_LOG_CLEARED="Log cleared"
fi

RUNTIME_DIR="$MODDIR/tmp"
BY_NAME_DIR="/dev/block/by-name"
IMAGE_NAMES="abl"
LOG_FILE="$RUNTIME_DIR/flash.log"
STATE_FILE="$RUNTIME_DIR/state"
MESSAGE_FILE="$RUNTIME_DIR/message"
UPDATED_FILE="$RUNTIME_DIR/updated"
PID_FILE="$RUNTIME_DIR/flash.pid"
LOCK_DIR="$RUNTIME_DIR/flash.lock"
export PATH=/data/adb/ksu/bin:/system/bin:/system/xbin:$PATH

timestamp() { date '+%Y-%m-%d %H:%M:%S'; }
read_line() { [ -f "$1" ] && head -n1 "$1"; }
emit() { echo -n "$1" | tr '\n' '\t'; }

ensure_runtime() {
  mkdir -p "$RUNTIME_DIR"
  [ -f "$LOG_FILE" ] || : > "$LOG_FILE"
  [ -f "$STATE_FILE" ] || echo idle > "$STATE_FILE"
  [ -f "$MESSAGE_FILE" ] || echo "$TEXT_IDLE" > "$MESSAGE_FILE"
  [ -f "$UPDATED_FILE" ] || timestamp > "$UPDATED_FILE"
}

write_state() {
  ensure_runtime
  echo "$1" > "$STATE_FILE"
  echo "$2" > "$MESSAGE_FILE"
  timestamp > "$UPDATED_FILE"
}

write_log() {
  ensure_runtime
  echo "[$(timestamp)] $*" >> "$LOG_FILE"
}

detect_current_slot() {
  case "$(getprop ro.boot.slot_suffix 2>/dev/null)" in
    _a) echo _a ;;
    _b) echo _b ;;
    *) return 1 ;;
  esac
}

other_slot() {
  case "$1" in
    _a) echo _b ;;
    _b) echo _a ;;
    *) return 1 ;;
  esac
}

partition_path() { echo "$BY_NAME_DIR/$1$2"; }

current_pid() {
  [ -f "$PID_FILE" ] || return 1
  pid=$(cat "$PID_FILE" | tr -d '[:space:]')
  kill -0 "$pid" 2>/dev/null && echo "$pid" && return 0
  rm -f "$PID_FILE"
  return 1
}

patch_efisp() {
  is_sfb=$2
  is_debug=$3
  rm -f $RUNTIME_DIR/*
  $MODDIR/bin/extractfv -o $RUNTIME_DIR -v "$1" >> "$LOG_FILE" 2>&1
  $MODDIR/bin/patch_abl $RUNTIME_DIR/LinuxLoader.efi $RUNTIME_DIR/patched.efi >> $RUNTIME_DIR/patch.log 2>&1
  cat $RUNTIME_DIR/patch.log >> "$LOG_FILE"
  [ -f $RUNTIME_DIR/patched.efi ] || { write_log "$TEXT_PATCH_FAILED"; return 1; }

  if [ "$is_sfb" = "with-superfastboot" ]; then
    write_log "$TEXT_INJECT_SFB"
    [ -f "$MODDIR/loader.elf" ] || { write_log "$TEXT_NO_LOADER_ELF"; return 1; }
    $MODDIR/bin/elf_inject "$MODDIR/loader.elf" $RUNTIME_DIR/patched.efi $RUNTIME_DIR/injected.dll >> "$LOG_FILE" 2>&1
    [ -f $RUNTIME_DIR/injected.dll ] || { write_log "$TEXT_INJECT_FAILED"; return 1; }
    $MODDIR/bin/GenFw -e UEFI_APPLICATION -o $RUNTIME_DIR/patched.efi $RUNTIME_DIR/injected.dll >> "$LOG_FILE" 2>&1
    write_log "$TEXT_INJECT_OK"
  fi

  if [ "$is_debug" = "debug" ]; then
    write_log "$TEXT_DEBUG_MODE"
    return 0
  fi

  if ! blockdev --setrw "$BY_NAME_DIR/efisp" >> "$LOG_FILE" 2>&1; then
    write_log "$TEXT_EFISP_SET_RW_FAILED"
    return 1
  fi
  if ! dd if=$RUNTIME_DIR/patched.efi of="$BY_NAME_DIR/efisp" bs=4M conv=fsync >> "$LOG_FILE" 2>&1; then
    write_log "$TEXT_EFISP_FLASH_FAILED"
    return 1
  fi
  sync
  write_log "$TEXT_EFISP_FLASH_OK"
  if ! grep -q "Warning: Failed to patch ABL GBL" $RUNTIME_DIR/patch.log; then
    write_log "$TEXT_GBL_VULN"
    return 2
  fi
  return 0
}

detect_gbl_vulnerability() {
  rm -f $RUNTIME_DIR/*
  $MODDIR/bin/extractfv -o $RUNTIME_DIR -v "$1" >> "$LOG_FILE" 2>&1
  $MODDIR/bin/patch_abl $RUNTIME_DIR/LinuxLoader.efi $RUNTIME_DIR/patched.efi >> $RUNTIME_DIR/patch.log 2>&1
  cat $RUNTIME_DIR/patch.log >> "$LOG_FILE"
  [ -f $RUNTIME_DIR/patched.efi ] || { write_log "$TEXT_PATCH_FAILED"; return 1; }
  if ! grep -q "Warning: Failed to patch ABL GBL" $RUNTIME_DIR/patch.log; then
    write_log "$TEXT_GBL_VULN"
    return 0
  fi
  write_log "$TEXT_NO_GBL_VULN"
  return 2
}

cleanup_lock() { rm -rf "$LOCK_DIR" "$PID_FILE"; }

print_status() {
  ensure_runtime
  current_slot=$(detect_current_slot)
  target_slot=$(other_slot "$current_slot")
  running=0
  pid=$(current_pid)
  [ -n "$pid" ] && running=1
  _state=$(read_line "$STATE_FILE")
  _msg=$(read_line "$MESSAGE_FILE")
  _upd=$(read_line "$UPDATED_FILE")

  out="CURRENT_SLOT=$current_slot
TARGET_SLOT=$target_slot
RUNNING=$running
PID=$pid
STATE=$_state
MESSAGE=$_msg
UPDATED_AT=$_upd
USER_LANG=$LANG"
  emit "$out"
}

run_flash() {
  mode=$1
  sfb=no
  debug=no
  if [ "$mode" = "update-efisp-with-superfastboot" ]; then
    mode=update-efisp
    sfb=with-superfastboot
  fi
  if [ "$mode" = "debug" ]; then
    debug=yes
    mode=skip-efisp
  fi
  if [ "$mode" = "debug-with-superfastboot" ]; then
    debug=yes
    mode=update-efisp
    sfb=with-superfastboot
  fi

  ensure_runtime
  mkdir "$LOCK_DIR" 2>/dev/null || { write_log "$TEXT_BUSY"; exit 1; }
  echo $$ > "$PID_FILE"
  trap cleanup_lock EXIT INT TERM HUP
  : > "$LOG_FILE"

  current_slot=$(detect_current_slot)
  target_slot=$(other_slot "$current_slot")
  [ -z "$current_slot" ] && { write_state error "$TEXT_NO_SLOT"; exit 1; }
  [ -z "$target_slot" ] && { write_state error "$TEXT_NO_TARGET_SLOT"; exit 1; }
  write_state running "$TEXT_FLASHING $target_slot"

  if [ "$debug" = "yes" ]; then
    abl=$(partition_path abl "$target_slot")
    if [ "$mode" = "update-efisp" ]; then
      patch_efisp "$abl" $sfb yes
      if [ $? -eq 0 ]; then
        write_state success "$TEXT_DEBUG_DONE"
      else
        write_state error "$TEXT_DEBUG_FAILED"
      fi
    else
      $MODDIR/bin/extractfv -o $RUNTIME_DIR -v "$abl" >> "$LOG_FILE" 2>&1
      if [ -f $RUNTIME_DIR/LinuxLoader.efi ]; then
        write_state success "$TEXT_DEBUG_ABL_DONE"
      else
        write_state error "$TEXT_EXTRACT_FAILED"
      fi
    fi
    exit 0
  fi

  efisp_fail=0
  abl=$(partition_path abl "$target_slot")
  if [ "$mode" = "update-efisp" ]; then
    patch_efisp "$abl" $sfb no
    res=$?
    if [ $res -eq 1 ]; then
      efisp_fail=1
      write_state running "$TEXT_EFISP_WARN"
    elif [ $res -eq 2 ]; then
      write_state success "$TEXT_GBL_VULN_SKIP"
      exit 0
    fi
  else
    detect_gbl_vulnerability "$abl"
    res=$?
    [ $res -eq 0 ] && { write_state success "$TEXT_GBL_VULN_SKIP"; exit 0; }
  fi

  for part in $IMAGE_NAMES; do
    dst=$(partition_path "$part" "$target_slot")
    src=$(partition_path "$part" "$current_slot")
    blockdev --setrw "$dst" >> "$LOG_FILE" 2>&1 || { write_state error "$TEXT_SET_RW_FAILED"; exit 1; }
    dd if="$src" of="$dst" bs=4M conv=fsync >> "$LOG_FILE" 2>&1 || { write_state error "$TEXT_FLASH_PART failed"; exit 1; }
    sync
    write_log "$TEXT_FLASH_PART $part -> $dst $TEXT_FLASH_OK"
  done

  if [ $efisp_fail -eq 1 ]; then
    write_state warning "BL done, efisp failed"
  elif [ "$mode" = "update-efisp" ]; then
    write_state success "$TEXT_ALL_OK"
  else
    write_state success "$TEXT_ALL_OK_NO_EFISP"
  fi
}

start_flash() {
  ensure_runtime
  [ -n "$(current_pid)" ] && { emit "ALREADY_RUNNING=1"; return; }
  nohup sh "$0" flash "$1" >/dev/null 2>&1 &
  sleep 1
  if [ -n "$(current_pid)" ]; then
    emit "STARTED=1"
  else
    st=$(read_line "$STATE_FILE")
    [ -n "$st" ] && emit "FINISHED=$st" || emit "STARTED=0"
  fi
}

print_log() { cat "$LOG_FILE" | tr '\n' '\t'; }
tail_log() { tail -n200 "$LOG_FILE" | tr '\n' '\t'; }

clear_log() {
  ensure_runtime
  [ -n "$(current_pid)" ] && { emit "BUSY=1"; return; }
  : > "$LOG_FILE"
  write_state idle "$TEXT_LOG_CLEARED"
  emit "CLEARED=1"
}

case "$1" in
  status) print_status ;;
  flash) run_flash "$2" ;;
  start) start_flash "$2" ;;
  log) print_log ;;
  tail) tail_log ;;
  clear-log) clear_log ;;
  *) exit 1 ;;
esac
