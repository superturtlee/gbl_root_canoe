#!/system/bin/sh
# 系统更新后 保留bl版本
if [ -z "$MODDIR" ]; then
  MODDIR=$(CDPATH= cd -- "$(dirname "$0")/.." 2>/dev/null && pwd)
fi
if [ -z "$MODDIR" ]; then
  echo 'ERROR=MODDIR detection failed' >&2
  exit 1
fi
RUNTIME_DIR="$MODDIR/tmp"
BY_NAME_DIR="/dev/block/by-name"
BACKUP_DIR="/data/adb/abl_backup"
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
  [ -f "$MESSAGE_FILE" ] || printf '%s\n' '等待操作' > "$MESSAGE_FILE"
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

ensure_backup_dir() {
  if mkdir -p "$BACKUP_DIR" 2>/dev/null; then
    return 0
  fi
  write_log "无法创建备份目录: $BACKUP_DIR"
  return 1
}

file_hash() {
  _file="$1"
  if command -v sha256sum >/dev/null 2>&1; then
    _out=$(sha256sum "$_file" 2>/dev/null) || return 1
    printf '%s\n' "${_out%% *}"
    return 0
  fi
  if command -v md5sum >/dev/null 2>&1; then
    _out=$(md5sum "$_file" 2>/dev/null) || return 1
    printf '%s\n' "${_out%% *}"
    return 0
  fi
  if command -v cksum >/dev/null 2>&1; then
    _out=$(cksum "$_file" 2>/dev/null) || return 1
    printf '%s\n' "${_out%% *}"
    return 0
  fi
  return 1
}

verify_partition_backup_hash() {
  _source="$1"
  _backup="$2"

  _source_hash=$(file_hash "$_source" | tr -d '[:space:]') || {
    write_log "备份哈希校验失败：无法计算源分区哈希 $_source"
    return 1
  }
  _backup_hash=$(file_hash "$_backup" | tr -d '[:space:]') || {
    write_log "备份哈希校验失败：无法计算备份文件哈希 $_backup"
    return 1
  }

  if [ -z "$_source_hash" ] || [ -z "$_backup_hash" ]; then
    write_log "备份哈希校验失败：哈希为空 source=$_source backup=$_backup"
    return 1
  fi

  if [ "$_source_hash" != "$_backup_hash" ]; then
    write_log "备份哈希校验失败：源分区与备份文件不一致 source=$_source backup=$_backup"
    return 1
  fi

  write_log "备份哈希校验通过：$_backup"
  printf '%s\n' "$_backup_hash"
  return 0
}

backup_partition_image() {
  part_path="$1"
  part_name="$2"
  tag="$3"

  ensure_backup_dir || return 1

  ts=$(date '+%Y%m%d-%H%M%S')
  tmp_file="$BACKUP_DIR/${part_name}_${ts}_tmp_${tag}.img"

  write_log "备份 $part_name 到 $tmp_file"
  if ! dd if="$part_path" of="$tmp_file" bs=4M conv=fsync >> "$LOG_FILE" 2>&1; then
    write_log "备份失败: $part_name"
    rm -f "$tmp_file"
    return 1
  fi
  sync

  hash=$(verify_partition_backup_hash "$part_path" "$tmp_file" | tr -d '[:space:]' | cut -c1-16)
  if [ -z "$hash" ]; then
    write_log "备份失败：源分区与备份文件哈希校验未通过"
    rm -f "$tmp_file"
    return 1
  fi
  final_file="$BACKUP_DIR/${part_name}_${ts}_${hash}_${tag}.img"

  if ! mv "$tmp_file" "$final_file"; then
    write_log "备份重命名失败: $tmp_file"
    rm -f "$tmp_file"
    return 1
  fi

  write_log "备份完成: $final_file"
  printf '%s\n' "$final_file"
  return 0
}

latest_stored_abl() {
  ls -t "$BACKUP_DIR"/abl_*_backup.img "$BACKUP_DIR"/abl_*_saved.img 2>/dev/null | head -n 1
}

latest_backup_abl() {
  ls -t "$BACKUP_DIR"/abl_*_backup.img 2>/dev/null | head -n 1
}

latest_saved_abl() {
  ls -t "$BACKUP_DIR"/abl_*_saved.img 2>/dev/null | head -n 1
}

prune_old_saved_backups() {
  keep_file="$1"
  for old_file in "$BACKUP_DIR"/abl_*_saved.img; do
    [ -e "$old_file" ] || continue
    [ "$old_file" = "$keep_file" ] && continue
    rm -f "$old_file"
    write_log "已删除旧 saved 备份: $old_file"
  done
}

prune_old_backup_backups() {
  keep_file="$1"
  for old_file in "$BACKUP_DIR"/abl_*_backup.img; do
    [ -e "$old_file" ] || continue
    [ "$old_file" = "$keep_file" ] && continue
    rm -f "$old_file"
    write_log "已删除旧 backup 备份: $old_file"
  done
}

short_hash() {
  _file="$1"
  file_hash "$_file" | tr -d '[:space:]' | cut -c1-16
}

hash_from_filename() {
  _name=$(basename "$1")
  _name=${_name%.img}
  printf '%s\n' "$_name" | awk -F'_' '{print $(NF-1)}'
}

verify_image_hash() {
  _file="$1"
  _expected=$(hash_from_filename "$_file")
  _actual=$(short_hash "$_file")
  if [ -z "$_expected" ] || [ -z "$_actual" ]; then
    write_log "哈希校验失败（空值）: $_file"
    return 1
  fi
  if [ "$_expected" != "$_actual" ]; then
    write_log "哈希校验失败: $_file expected=$_expected actual=$_actual"
    return 1
  fi
  write_log "哈希校验通过: $_file ($_actual)"
  return 0
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
# extractfv and patch_abl from abl_dest
# 0 success, 1 failed 2 new ABL version with GBL vulnerability detected, skipping BL flash to preserve BL version
patch_efisp() {
  rm "$RUNTIME_DIR/patched.efi" 2>/dev/null || true
  rm "$RUNTIME_DIR/patch.log" 2>/dev/null || true
  rm "$RUNTIME_DIR/LinuxLoader.efi" 2>/dev/null || true
  $MODDIR/bin/extractfv -o "$RUNTIME_DIR" -v "$1" >> "$LOG_FILE" 2>&1
  #patch abl
  $MODDIR/bin/patch_abl "$RUNTIME_DIR/LinuxLoader.efi" "$RUNTIME_DIR/patched.efi" >> "$RUNTIME_DIR/patch.log" 2>&1
  cat "$RUNTIME_DIR/patch.log" >> "$LOG_FILE"
  if [ ! -f "$RUNTIME_DIR/patched.efi" ]; then
    write_log '补丁应用失败'
    return 1
  fi
  #flash
  if ! blockdev --setrw "/dev/block/by-name/efisp" >> "$LOG_FILE" 2>&1; then
    write_log 'efisp 分区设置可写失败'
    return 1
  fi
  if ! dd if="$RUNTIME_DIR/patched.efi" of=/dev/block/by-name/efisp bs=4M conv=fsync >> "$LOG_FILE" 2>&1; then
    write_log 'efisp dd 刷写失败'
    return 1
  fi
  sync
  write_log "efisp 分区刷写完成"
  #检查patch log 是否包含 "Warning: Failed to patch ABL GBL\n" 如果没有则新的ABL版本存在GBL漏洞，返回伪失败，绕过后续BL flash
  if ! grep -q "Warning: Failed to patch ABL GBL" "$RUNTIME_DIR/patch.log"; then
    write_log "检测到新的ABL版本，存在GBL漏洞，跳过后续BL刷写以保留BL版本"
    return 2
  fi
  return 0
}

# detect ABL GBL vulnerability without flashing efisp
# 0 vulnerable (should skip BL flash), 1 detect failed, 2 not vulnerable
detect_gbl_vulnerability() {
  rm "$RUNTIME_DIR/patched.efi" 2>/dev/null || true
  rm "$RUNTIME_DIR/patch.log" 2>/dev/null || true
  rm "$RUNTIME_DIR/LinuxLoader.efi" 2>/dev/null || true
  $MODDIR/bin/extractfv -o "$RUNTIME_DIR" -v "$1" >> "$LOG_FILE" 2>&1
  $MODDIR/bin/patch_abl "$RUNTIME_DIR/LinuxLoader.efi" "$RUNTIME_DIR/patched.efi" >> "$RUNTIME_DIR/patch.log" 2>&1
  cat "$RUNTIME_DIR/patch.log" >> "$LOG_FILE"
  if [ ! -f "$RUNTIME_DIR/patched.efi" ]; then
    write_log '漏洞检测失败：补丁文件未生成'
    return 1
  fi
  if ! grep -q "Warning: Failed to patch ABL GBL" "$RUNTIME_DIR/patch.log"; then
    write_log '检测到新的ABL版本，存在GBL漏洞，跳过后续BL刷写'
    return 0
  fi
  write_log '未检测到GBL漏洞，继续后续BL刷写'
  return 2
}

cleanup_lock() { rm -rf "$LOCK_DIR"; rm -f "$PID_FILE";  }

print_status() {
  ensure_runtime
  ensure_backup_dir >/dev/null 2>&1 || true
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
  latest_backup=''; latest_backup=$(latest_backup_abl)
  latest_saved=''; latest_saved=$(latest_saved_abl)
  has_backup='0'
  has_saved='0'
  [ -n "$latest_backup" ] && has_backup='1'
  [ -n "$latest_saved" ] && has_saved='1'

  _out="CURRENT_SLOT=${current_slot}
TARGET_SLOT=${target_slot}
RUNNING=${running}
PID=${pid}
STATE=${_state}
MESSAGE=${_msg}
UPDATED_AT=${_upd}
HAS_BACKUP=${has_backup}
LATEST_BACKUP=${latest_backup}
HAS_SAVED=${has_saved}
LATEST_SAVED=${latest_saved}"

  emit "$_out"
}

run_flash() {
  update_efisp="${1:-skip-efisp}"
  ensure_runtime
  if ! mkdir "$LOCK_DIR" 2>/dev/null; then
    write_log '已有刷写任务在执行，拒绝重复启动'
    exit 1
  fi
  printf '%s\n' "$$" > "$PID_FILE"
  trap cleanup_lock EXIT INT TERM HUP
  : > "$LOG_FILE"

  current_slot=$(detect_current_slot 2>/dev/null || true)
  if [ -z "$current_slot" ]; then
    write_state 'error' '无法识别当前槽位'; write_log '无法识别当前槽位'; exit 1
  fi
  target_slot=$(other_slot "$current_slot" 2>/dev/null || true)
  if [ -z "$target_slot" ]; then
    write_state 'error' '无法计算目标槽位'; write_log '无法计算目标槽位'; exit 1
  fi

  write_state 'running' "正在将镜像刷写到槽位 $target_slot"
  write_log "当前槽位: $current_slot  目标槽位: $target_slot"
  efisp_failed='0'
  abl_part=$(partition_path abl "$target_slot")
  source_abl_part=$(partition_path abl "$current_slot")

  new_backup_file=$(backup_partition_image "$source_abl_part" "abl" 'backup')
  if [ -z "$new_backup_file" ]; then
    write_state 'error' '自动备份当前槽位 abl 失败'
    write_log '自动备份当前槽位 abl 失败，已中止刷写'
    exit 1
  fi
  prune_old_backup_backups "$new_backup_file"

  if [ "$update_efisp" = 'update-efisp' ] || [ "$update_efisp" = '1' ] || [ "$update_efisp" = 'true' ]; then
    #patch abl and flash efisp first, since it's the only one that can brick the device if something goes wrong
    #0 success, 1 failed, 2 new ABL version with GBL vulnerability detected
    patch_efisp "$abl_part"
    ret=$?
    case $ret in
      0) ;; # 成功，继续
       1) efisp_failed='1'
         write_state 'running' 'efisp 分区刷写失败，继续刷写 BL 以保留旧版本'
         write_log '警告：新版本ABL补丁应用失败，但是继续刷写BL以保留BL版本，以加载旧版本efisp'
         write_log '理论可以保留BL版本成功引导，但不排除存在未知风险，请上传补丁日志以供分析'
         ;;
      2) write_state 'success' 'efisp 分区刷写完成，但跳过了BL刷写以保留BL版本'
         write_log 'efisp 分区刷写完成，但跳过了BL刷写（检测到GBL漏洞）'
         exit 0 ;;
      *) write_state 'error' '未知错误'
         exit 1 ;;
    esac
  else
    write_log '未勾选 efisp 更新，跳过 efisp 分区操作'
      detect_gbl_vulnerability "$abl_part"
      ret=$?
      case $ret in
        0) write_state 'success' '检测到GBL漏洞，已跳过BL刷写'
           write_log '未更新efisp且检测到GBL漏洞，已跳过BL刷写'
           exit 0 ;;
        1) write_log '漏洞检测失败，继续执行BL刷写流程' ;;
        2) ;;
        *) write_log '警告：未知检测结果，继续执行BL刷写流程' ;;
      esac
  fi

  for name in $IMAGE_NAMES; do
    part=$(partition_path "$name" "$target_slot")
    srcpart=$(partition_path "$name" "$current_slot")
    used_saved=''
    if [ "$name" = 'abl' ]; then
      saved_img=$(latest_saved_abl)
      if [ -n "$saved_img" ] && [ -f "$saved_img" ]; then
        if ! verify_image_hash "$saved_img"; then
          write_state 'error' 'saved 备份哈希校验失败，已中止刷写'
          exit 1
        fi
        srcpart="$saved_img"
        used_saved="$saved_img"
        write_log "刷写使用 saved 镜像: $srcpart"
      else
        write_log "未找到 saved 备份，回退使用当前槽位分区: $srcpart"
      fi
    fi

    write_log "blockdev --setrw $part"
    if ! blockdev --setrw "$part" >> "$LOG_FILE" 2>&1; then
      write_state 'error' "分区 $name 设置可写失败"; exit 1
    fi
    write_log "刷写 $name -> $part"
    if ! dd if="$srcpart" of="$part" bs=4M conv=fsync >> "$LOG_FILE" 2>&1; then
      write_state 'error' "分区 $name 刷写失败"; exit 1
    fi
    sync
    write_log "$name 完成"
    if [ "$name" = 'abl' ] && [ -n "$used_saved" ]; then
      rm -f "$used_saved"
      write_log "saved 刷写成功，已删除: $used_saved"
    fi
  done

  if [ "$efisp_failed" = '1' ]; then
    write_state 'warning' "BL 刷写完成，但 efisp 未更新"
    write_log 'BL 镜像刷写完成，但 efisp 未更新'
  elif [ "$update_efisp" = 'update-efisp' ] || [ "$update_efisp" = '1' ] || [ "$update_efisp" = 'true' ]; then
    write_state 'success' "全部刷写完成（含 efisp）"
    write_log '全部镜像与 efisp 刷写完成'
  else
    write_state 'success' "全部刷写完成（未更新 efisp）"
    write_log '全部镜像刷写完成（未更新 efisp）'
  fi
}

run_restore() {
  ensure_runtime
  if ! mkdir "$LOCK_DIR" 2>/dev/null; then
    write_log '已有任务在执行，拒绝重复启动恢复'
    exit 1
  fi
  printf '%s\n' "$$" > "$PID_FILE"
  trap cleanup_lock EXIT INT TERM HUP
  : > "$LOG_FILE"

  ensure_backup_dir || { write_state 'error' '无法创建备份目录'; exit 1; }

  current_slot=$(detect_current_slot 2>/dev/null || true)
  if [ -z "$current_slot" ]; then
    write_state 'error' '无法识别当前槽位'; write_log '无法识别当前槽位'; exit 1
  fi

  current_part=$(partition_path abl "$current_slot")
  write_state 'running' "正在恢复当前槽位 $current_slot 的 abl"
  write_log "恢复目标分区: $current_part"

  restore_img=$(latest_backup_abl)
  if [ -z "$restore_img" ] || [ ! -f "$restore_img" ]; then
    write_state 'error' '未找到可恢复的 abl 备份'
    write_log '未找到可恢复的 abl 备份'
    exit 1
  fi

  if ! verify_image_hash "$restore_img"; then
    write_state 'error' 'backup 备份哈希校验失败，已中止恢复'
    write_log 'backup 备份哈希校验失败，已中止恢复'
    exit 1
  fi

  saved_file=$(backup_partition_image "$current_part" "abl" 'saved')
  if [ -z "$saved_file" ]; then
    write_state 'error' '恢复前保存当前 abl 失败'
    write_log '恢复前保存当前 abl 失败'
    exit 1
  fi
  prune_old_saved_backups "$saved_file"

  if ! blockdev --setrw "$current_part" >> "$LOG_FILE" 2>&1; then
    write_state 'error' '当前槽位 abl 设置可写失败'
    write_log '当前槽位 abl 设置可写失败'
    exit 1
  fi

  write_log "开始恢复: $restore_img -> $current_part"
  if ! dd if="$restore_img" of="$current_part" bs=4M conv=fsync >> "$LOG_FILE" 2>&1; then
    write_state 'error' '恢复 abl 失败'
    write_log '恢复 abl 失败'
    exit 1
  fi

  sync
  write_state 'success' 'abl 恢复完成'
  write_log "恢复完成，当前槽位已写入: $restore_img"
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

start_restore() {
  ensure_runtime
  if pid=$(current_pid 2>/dev/null); then
    emit "ALREADY_RUNNING=${pid}"; return 0
  fi
  if command -v nohup >/dev/null 2>&1; then
    nohup sh "$0" restore >/dev/null 2>&1 &
  else
    sh "$0" restore </dev/null >/dev/null 2>&1 &
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
  write_state 'idle' '日志已清空'
  emit 'CLEARED=1'
}

case "$1" in
  status)    print_status ;;
  flash)     run_flash "$2" ;;
  start)     start_flash "$2" ;;
  restore)   run_restore ;;
  start-restore) start_restore ;;
  log)       print_log ;;
  tail)      tail_log "$2" ;;
  clear-log) clear_log ;;
  *)         printf 'Usage: %s {status|flash|start|restore|start-restore|log|tail [lines]|clear-log}\n' "$0" >&2; exit 1 ;;
esac
