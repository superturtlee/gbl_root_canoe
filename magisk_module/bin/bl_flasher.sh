#!/system/bin/sh
if [ -z "$MODDIR" ]; then
  MODDIR=$(CDPATH= cd -- "$(dirname "$0")/.." 2>/dev/null && pwd)
fi
if [ -z "$MODDIR" ]; then
  echo 'ERROR=MODDIR detection failed' >&2
  exit 1
fi

LANG="zh"
if [ -f "$MODDIR/lang.txt" ]; then
  USER_LANG=$(cat "$MODDIR/lang.txt" | tr -d '[:space:]')
  if [ "$USER_LANG" = "en" ]; then
    LANG="en"
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
  TEXT_EFISP_FLASH_FAILED="efisp dd 刷写失败"
  TEXT_EFISP_FLASH_OK="efisp 分区刷写完成"
  TEXT_GBL_VULN="检测到新的ABL版本，存在GBL漏洞，跳过后续BL刷写以保留BL版本"
  TEXT_GBL_VULN_SKIP="检测到GBL漏洞，已跳过BL刷写"
  TEXT_GBL_DETECT_FAILED="漏洞检测失败，继续执行BL刷写流程"
  TEXT_NO_GBL_VULN="未检测到GBL漏洞，继续后续BL刷写"
  TEXT_INJECT_SFB="正在注入 superfastboot loader..."
  TEXT_NO_LOADER_ELF="loader.elf 不存在，无法安装 superfastboot"
  TEXT_INJECT_FAILED="elf_inject 注入失败"
  TEXT_GENFW_FAILED="GenFw 转换失败"
  TEXT_INJECT_OK="superfastboot loader 注入完成"
  TEXT_EFISP_WARN="efisp 分区刷写失败，继续刷写 BL 以保留旧版本"
  TEXT_EFISP_WARN2="警告：新版本ABL补丁应用失败，但是继续刷写BL以保留BL版本，以加载旧版本efisp"
  TEXT_EFISP_WARN3="理论可以保留BL版本成功引导，但不排除存在未知风险，请上传补丁日志以供分析"
  TEXT_SET_RW_FAILED="分区 设置可写失败"
  TEXT_FLASH_PART="刷写"
  TEXT_FLASH_OK="完成"
  TEXT_ALL_OK="全部刷写完成（含 efisp）"
  TEXT_ALL_OK_NO_EFISP="全部刷写完成（未更新 efisp）"
  TEXT_BUSY="已有刷写任务在执行，拒绝重复启动"
  TEXT_LOG_CLEARED="日志已清空"
else
  TEXT_IDLE="Waiting"
  TEXT_NO_SLOT="Cannot detect current slot"
  TEXT_NO_TARGET_SLOT="Cannot detect target slot"
  TEXT_FLASHING="Flashing images to slot"
  TEXT_DEBUG_MODE="Debug Mode: Process only, no flash"
  TEXT_DEBUG_DONE="Debug done, files saved to"
  TEXT_DEBUG_ABL_DONE="Debug done, ABL extracted to"
  TEXT_DEBUG_FAILED="Error during debug"
  TEXT_EXTRACT_FAILED="ABL extract failed"
  TEXT_PATCH_FAILED="Patch failed"
  TEXT_EFISP_SET_RW_FAILED="efisp setrw failed"
  TEXT_EFISP_FLASH_FAILED="efisp dd flash failed"
  TEXT_EFISP_FLASH_OK="efisp flash done"
  TEXT_GBL_VULN="New ABL with GBL vuln detected, skip BL flash to keep version"
  TEXT_GBL_VULN_SKIP="GBL vuln detected, skip BL flash"
  TEXT_GBL_DETECT_FAILED="Vuln detect failed, continue flashing"
  TEXT_NO_GBL_VULN="No GBL vuln, continue flashing"
  TEXT_INJECT_SFB="Injecting superfastboot loader..."
  TEXT_NO_LOADER_ELF="loader.elf missing, cannot install superfastboot"
  TEXT_INJECT_FAILED="elf_inject failed"
  TEXT_GENFW_FAILED="GenFw convert failed"
  TEXT_INJECT_OK="superfastboot loader injected"
  TEXT_EFISP_WARN="efisp flash failed, continue BL to keep old version"
  TEXT_EFISP_WARN2="Warning: ABL patch failed, but flash BL anyway to keep version"
  TEXT_EFISP_WARN3="May boot normally, but risk exists. Please upload logs"
  TEXT_SET_RW_FAILED="partition setrw failed"
  TEXT_FLASH_PART="Flashing"
  TEXT_FLASH_OK="done"
  TEXT_ALL_OK="All done (with efisp)"
  TEXT_ALL_OK_NO_EFISP="All done (no efisp)"
  TEXT_BUSY="Flash task already running"
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
read_line() { [ -f "$1" ] && head -n 1 "$1"; }
emit() { printf '%s' "$1" | tr '\n' '\t'; }

ensure_runtime() {
  mkdir -p "$RUNTIME_DIR"
  [ -f "$LOG_FILE" ]     || : > "$LOG_FILE"
  [ -f "$STATE_FILE" ]   || printf '%s\n' 'idle' > "$STATE_FILE"
  [ -f "$MESSAGE_FILE" ] || printf '%s\n' "$TEXT_IDLE" > "$MESSAGE_FILE"
  [ -f "$UPDATED_FILE" ] || timestamp > "$UPDATED_FILE"
}

write_state() {
  ensure_runtime
  printf '%s\n' "$1" > "$STATE_FILE"
  printf '%s\n' "$2" > "$MESSAGE_FILE"
  timestamp > "$UPDATED_FILE"
}

write_log() {
  ensure_runtime
  printf '[%s] %s\n' "$(timestamp)" "$*" >> "$LOG_FILE"
}

detect_current_slot() {
  case "$(getprop ro.boot.slot_suffix 2>/dev/null)" in
    _a) printf '%s\n' '_a' ;;
    _b) printf '%s\n' '_b' ;;
    *)  return 1 ;;
  esac
}

other_slot() {
  case "$1" in
    _a) printf '%s\n' '_b' ;;
    _b) printf '%s\n' '_a' ;;
    *)  return 1 ;;
  esac
}

partition_path() { printf '%s\n' "$BY_NAME_DIR/${1}${2}"; }

current_pid() {
  if [ -f "$PID_FILE" ]; then
    pid=$(tr -d '[:space:]' < "$PID_FILE")
    if [ -n "$pid" ] && kill -0 "$pid" 2>/dev/null; then
      printf '%s\n' "$pid"; return 0
    fi
    rm -f "$PID_FILE"
  fi
  return 1
}

patch_efisp() {
  install_superfastboot="${2:-no-superfastboot}"
  debug_mode="${3:-no-debug}"
  rm "$RUNTIME_DIR/patched.efi" 2>/dev/null || true
  rm "$RUNTIME_DIR/patch.log" 2>/dev/null || true
  rm "$RUNTIME_DIR/LinuxLoader.efi" 2>/dev/null || true
  $MODDIR/bin/extractfv -o "$RUNTIME_DIR" -v "$1" >> "$LOG_FILE" 2>&1
  $MODDIR/bin/patch_abl "$RUNTIME_DIR/LinuxLoader.efi" "$RUNTIME_DIR/patched.efi" >> "$RUNTIME_DIR/patch.log" 2>&1
  cat "$RUNTIME_DIR/patch.log" >> "$LOG_FILE"
  if [ ! -f "$RUNTIME_DIR/patched.efi" ]; then
    write_log "$TEXT_PATCH_FAILED"
    return 1
  fi

  if [ "$install_superfastboot" = "with-superfastboot" ]; then
    write_log "$TEXT_INJECT_SFB"
    if [ ! -f "$MODDIR/loader.elf" ]; then
      write_log "$TEXT_NO_LOADER_ELF"
      return 1
    fi
    $MODDIR/bin/elf_inject "$MODDIR/loader.elf" "$RUNTIME_DIR/patched.efi" "$RUNTIME_DIR/injected.dll" >> "$LOG_FILE" 2>&1
    if [ ! -f "$RUNTIME_DIR/injected.dll" ]; then
      write_log "$TEXT_INJECT_FAILED"
      return 1
    fi
    $MODDIR/bin/GenFw -e UEFI_APPLICATION -o "$RUNTIME_DIR/patched.efi" "$RUNTIME_DIR/injected.dll" >> "$LOG_FILE" 2>&1
    if [ ! -f "$RUNTIME_DIR/patched.efi" ]; then
      write_log "$TEXT_GENFW_FAILED"
      return 1
    fi
    write_log "$TEXT_INJECT_OK"
  fi

  if [ "$debug_mode" = "debug" ]; then
    write_log "$TEXT_DEBUG_MODE: $TEXT_DEBUG_DONE $RUNTIME_DIR/patched.efi"
    return 0
  fi

  if ! blockdev --setrw "/dev/block/by-name/efisp" >> "$LOG_FILE" 2>&1; then
    write_log "$TEXT_EFISP_SET_RW_FAILED"
    return 1
  fi
  if ! dd if="$RUNTIME_DIR/patched.efi" of=/dev/block/by-name/efisp bs=4M conv=fsync >> "$LOG_FILE" 2>&1; then
    write_log "$TEXT_EFISP_FLASH_FAILED"
    return 1
  fi
  sync
  write_log "$TEXT_EFISP_FLASH_OK"
  if ! grep -q "Warning: Failed to patch ABL GBL" "$RUNTIME_DIR/patch.log"; then
    write_log "$TEXT_GBL_VULN"
    return 2
  fi
  return 0
}

detect_gbl_vulnerability() {
  rm "$RUNTIME_DIR/patched.efi" 2>/dev/null || true
  rm "$RUNTIME_DIR/patch.log" 2>/dev/null || true
  rm "$RUNTIME_DIR/LinuxLoader.efi" 2>/dev/null || true
  $MODDIR/bin/extractfv -o "$RUNTIME_DIR" -v "$1" >> "$LOG_FILE" 2>&1
  $MODDIR/bin/patch_abl "$RUNTIME_DIR/LinuxLoader.efi" "$RUNTIME_DIR/patched.efi" >> "$RUNTIME_DIR/patch.log" 2>&1
  cat "$RUNTIME_DIR/patch.log" >> "$LOG_FILE"
  if [ ! -f "$RUNTIME_DIR/patched.efi" ]; then
    write_log "$TEXT_PATCH_FAILED"
    return 1
  fi
  if ! grep -q "Warning: Failed to patch ABL GBL" "$RUNTIME_DIR/patch.log"; then
    write_log "$TEXT_GBL_VULN"
    return 0
  fi
  write_log "$TEXT_NO_GBL_VULN"
  return 2
}

cleanup_lock() { rm -rf "$LOCK_DIR"; rm -f "$PID_FILE";  }

print_status() {
  ensure_runtime
  current_slot=$(getprop ro.boot.slot_suffix 2>/dev/null)
  case "$current_slot" in _a|_b) ;; *) current_slot='' ;; esac
  target_slot=''
  case "$current_slot" in _a) target_slot='_b' ;; _b) target_slot='_a' ;; esac

  running='0'; pid=''
  if [ -f "$PID_FILE" ]; then
    pid=$(tr -d '[:space:]' < "$PID_FILE")
    if [ -n "$pid" ] && kill -0 "$pid" 2>/dev/null; then
      running='1'
    else
      pid=''; rm -f "$PID_FILE"
    fi
  fi

  _state=''; [ -f "$STATE_FILE" ] && read -r _state < "$STATE_FILE"
  _msg=''; [ -f "$MESSAGE_FILE" ] && read -r _msg < "$MESSAGE_FILE"
  _upd=''; [ -f "$UPDATED_FILE" ] && read -r _upd < "$UPDATED_FILE"

  _out="CURRENT_SLOT=${current_slot}
TARGET_SLOT=${target_slot}
RUNNING=${running}
PID=${pid}
STATE=${_state}
MESSAGE=${_msg}
UPDATED_AT=${_upd}
USER_LANG=${LANG}"

  emit "$_out"
}

run_flash() {
  update_efisp="${1:-skip-efisp}"
  install_superfastboot="no-superfastboot"
  debug_mode="no-debug"
  case "$update_efisp" in
    "update-efisp-with-superfastboot")
      update_efisp="update-efisp"
      install_superfastboot="with-superfastboot"
      ;;
    "debug")
      debug_mode="debug"
      update_efisp="skip-efisp"
      ;;
    "debug-with-superfastboot")
      debug_mode="debug"
      update_efisp="update-efisp"
      install_superfastboot="with-superfastboot"
      ;;
  esac

  ensure_runtime
  if ! mkdir "$LOCK_DIR" 2>/dev/null; then
    write_log "$TEXT_BUSY"
    exit 1
  fi
  printf '%s\n' "$$" > "$PID_FILE"
  trap cleanup_lock EXIT INT TERM HUP
  : > "$LOG_FILE"

  current_slot=$(detect_current_slot 2>/dev/null || true)
  if [ -z "$current_slot" ]; then
    write_state 'error' "$TEXT_NO_SLOT"; write_log "$TEXT_NO_SLOT"; exit 1
  fi
  target_slot=$(other_slot "$current_slot" 2>/dev/null || true)
  if [ -z "$target_slot" ]; then
    write_state 'error' "$TEXT_NO_TARGET_SLOT"; write_log "$TEXT_NO_TARGET_SLOT"; exit 1
  fi

  write_state 'running' "$TEXT_FLASHING $target_slot"
  write_log "Current slot: $current_slot  Target slot: $target_slot"

  if [ "$debug_mode" = "debug" ]; then
    write_log "$TEXT_DEBUG_MODE"
    abl_part=$(partition_path abl "$target_slot")
    if [ "$update_efisp" = 'update-efisp' ] || [ "$update_efisp" = '1' ] || [ "$update_efisp" = 'true' ]; then
      patch_efisp "$abl_part" "$install_superfastboot" "$debug_mode"
      ret=$?
      if [ $ret -eq 0 ]; then
        write_state 'success' "$TEXT_DEBUG_DONE $RUNTIME_DIR"
        write_log "$TEXT_DEBUG_DONE $RUNTIME_DIR/patched.efi"
      else
        write_state 'error' "$TEXT_DEBUG_FAILED"
        write_log "$TEXT_DEBUG_FAILED"
      fi
    else
      write_log "$TEXT_DEBUG_MODE, no efisp, extract ABL only"
      rm "$RUNTIME_DIR/LinuxLoader.efi" 2>/dev/null || true
      $MODDIR/bin/extractfv -o "$RUNTIME_DIR" -v "$abl_part" >> "$LOG_FILE" 2>&1
      if [ -f "$RUNTIME_DIR/LinuxLoader.efi" ]; then
        write_state 'success' "$TEXT_DEBUG_ABL_DONE $RUNTIME_DIR/LinuxLoader.efi"
        write_log "$TEXT_DEBUG_ABL_DONE $RUNTIME_DIR/LinuxLoader.efi"
      else
        write_state 'error' "$TEXT_EXTRACT_FAILED"
        write_log "$TEXT_EXTRACT_FAILED"
      fi
    fi
    exit 0
  fi

  efisp_failed='0'
  abl_part=$(partition_path abl "$target_slot")

  if [ "$update_efisp" = 'update-efisp' ] || [ "$update_efisp" = '1' ] || [ "$update_efisp" = 'true' ]; then
    patch_efisp "$abl_part" "$install_superfastboot" "$debug_mode"
    ret=$?
    case $ret in
      0) ;;
      1) efisp_failed='1'
         write_state 'running' "$TEXT_EFISP_WARN"
         write_log "$TEXT_EFISP_WARN2"
         write_log "$TEXT_EFISP_WARN3"
         ;;
      2) write_state 'success' "$TEXT_EFISP_FLASH_OK, $TEXT_GBL_VULN_SKIP"
         write_log "$TEXT_EFISP_FLASH_OK, $TEXT_GBL_VULN_SKIP"
         exit 0 ;;
      *) write_state 'error' "Unknown error"
         exit 1 ;;
    esac
  else
    write_log "Skip efisp update"
    detect_gbl_vulnerability "$abl_part"
    ret=$?
    case $ret in
      0) write_state 'success' "$TEXT_GBL_VULN_SKIP"
         write_log "$TEXT_GBL_VULN_SKIP"
         exit 0 ;;
      1) write_log "$TEXT_GBL_DETECT_FAILED" ;;
      2) ;;
      *) write_log "Unknown detect result, continue" ;;
    esac
  fi

  for name in $IMAGE_NAMES; do
    part=$(partition_path "$name" "$target_slot")
    srcpart=$(partition_path "$name" "$current_slot")

    write_log "blockdev --setrw $part"
    if ! blockdev --setrw "$part" >> "$LOG_FILE" 2>&1; then
      write_state 'error' "$name $TEXT_SET_RW_FAILED"; exit 1
    fi
    write_log "$TEXT_FLASH_PART $name -> $part"
    if ! dd if="$srcpart" of="$part" bs=4M conv=fsync >> "$LOG_FILE" 2>&1; then
      write_state 'error' "$name flash failed"; exit 1
    fi
    sync
    write_log "$name $TEXT_FLASH_OK"
  done

  if [ "$efisp_failed" = '1' ]; then
    write_state 'warning' "BL flashed, efisp not updated"
    write_log "BL done, efisp not updated"
  elif [ "$update_efisp" = 'update-efisp' ] || [ "$update_efisp" = '1' ] || [ "$update_efisp" = 'true' ]; then
    write_state 'success' "$TEXT_ALL_OK"
    write_log "$TEXT_ALL_OK"
  else
    write_state 'success' "$TEXT_ALL_OK_NO_EFISP"
    write_log "$TEXT_ALL_OK_NO_EFISP"
  fi
}

start_flash() {
  update_efisp="${1:-skip-efisp}"
  ensure_runtime
  if pid=$(current_pid 2>/dev/null); then
    emit "ALREADY_RUNNING=${pid}"; return 0
  fi
  if command -v nohup >/dev/null 2>&1; then
    nohup sh "$0" flash "$update_efisp" >/dev/null 2>&1 &
  else
    sh "$0" flash "$update_efisp" </dev/null >/dev/null 2>&1 &
  fi
  sleep 1
  if pid=$(current_pid 2>/dev/null); then
    emit "STARTED=1
PID=${pid}"
  else
    _st=''; [ -f "$STATE_FILE" ] && read -r _st < "$STATE_FILE"
    case "$_st" in
      success|error|warning) emit "FINISHED=${_st}" ;;
      *) emit 'STARTED=0' ;;
    esac
  fi
}

print_log() { ensure_runtime; tr '\n' '\t' < "$LOG_FILE"; }
tail_log()  { ensure_runtime; tail -n "${1:-200}" "$LOG_FILE" | tr '\n' '\t'; }

clear_log() {
  ensure_runtime
  if current_pid >/dev/null 2>&1; then emit 'BUSY=1'; return 1; fi
  : > "$LOG_FILE"
  write_state 'idle' "$TEXT_LOG_CLEARED"
  emit 'CLEARED=1'
}

case "$1" in
  status)    print_status ;;
  flash)     run_flash "$2" ;;
  start)     start_flash "$2" ;;
  log)       print_log ;;
  tail)      tail_log "$2" ;;
  clear-log) clear_log ;;
  *)         printf 'Usage: %s {status|flash|start|log|tail [lines]|clear-log}\n' "$0" >&2; exit 1 ;;
esac
