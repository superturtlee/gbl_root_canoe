export type Lang = 'zh' | 'en';

type Dict = Record<string, string>;
const registry = new Map<Lang, Dict>();

export function register(lang: Lang, dict: Dict): void {
  registry.set(lang, { ...registry.get(lang), ...dict });
}

const STORAGE_KEY = 'bl_flasher_lang';

export function get(): Lang {
  const stored = localStorage.getItem(STORAGE_KEY);
  if (stored === 'zh' || stored === 'en') return stored;
  if (navigator.language.startsWith('zh')) return 'zh';
  return 'en';
}

export function set(lang: Lang): void {
  localStorage.setItem(STORAGE_KEY, lang);
}

export function t(key: string): string {
  return registry.get(get())?.[key] ?? key;
}

export function available(): Lang[] {
  return [...registry.keys()];
}

register('en', {
  'app.title': 'BL Flasher',
  'app.subtitle': 'KernelSU Module WebUI',
  'app.desc': 'Auto-detect active slot. Skip BL flash if new build has GBL exploit. Flash BL images to inactive slot and patch ABL to efisp as needed.',
  'slot.status': 'Slot Status',
  'slot.current': 'Current Slot',
  'slot.target': 'Target Slot',
  'slot.images': 'Image Count',
  'slot.task': 'Task Status',
  'btn.refresh': 'Refresh',
  'btn.flash': 'Flash To Other Slot',
  'btn.clearLog': 'Clear Log',
  'opt.updateEfisp': 'Update efisp',
  'opt.superfastboot': 'Install superfastboot (requires Update efisp)',
  'opt.debugMode': 'Debug Mode (process only, no flash)',
  'warn.text': 'Flashing bootloader partitions is high risk. Verify images match your device.',
  'img.map': 'Image Mapping',
  'tbl.partition': 'Partition',
  'tbl.source': 'Source (Current)',
  'tbl.target': 'Target',
  'tbl.action': 'Action',
  'tbl.waiting': 'Waiting for module status',
  'log.title': 'Live Log',
  'log.auto': 'Auto polling last 200 lines',
  'log.waiting': 'Waiting for log...',
  'modal.risk': 'HIGH RISK',
  'modal.title': 'Confirm Flash',
  'modal.confirmText': 'About to start flashing. Confirm target and operation.',
  'modal.step1': '1st Confirm: Copy BL from current slot to',
  'modal.step1debug': 'Debug Mode: All processing without flashing. Files saved to tmp.',
  'modal.step2': '2nd Confirm: High-risk write. Wrong action may brick target slot.',
  'modal.withEfisp': 'with efisp update',
  'modal.withSfb': 'with efisp update + superfastboot',
  'modal.noEfisp': 'efisp not updated',
  'modal.confirmSlot': 'Verify slot is correct.',
  'btn.cancel': 'Cancel',
  'btn.continue': 'Continue',
  'btn.confirmFlash': 'Confirm Flash',
  'btn.startDebug': 'Start Debug',
  'chip.waiting': 'Waiting',
  'chip.slotUnknown': 'Slot Unknown',
  'chip.running': 'Task Running',
  'toast.needEfisp': 'superfastboot requires Update efisp enabled',
  'toast.running': 'Flash task already running',
  'toast.startDebug': 'Debug task started',
  'toast.startFlash': 'Flash task started',
  'toast.debugDone': 'Debug completed',
  'toast.flashDone': 'Flash completed',
  'toast.blDone': 'BL flashed, efisp not updated',
  'toast.failed': 'Task finished (failed)',
  'toast.startError': 'Failed to start task',
  'toast.logBusy': 'Cannot clear log while task running',
  'toast.logCleared': 'Log cleared',
  'toast.needEfispMsg': 'Install superfastboot requires Update efisp',
  'status.skipped': 'Skipped BL flash',
  'status.success': 'Success',
  'status.error': 'Error',
  'status.warning': 'Warning',
  'status.running': 'Running',
  'status.idle': 'Idle',
});

register('zh', {
  'app.title': '假回锁 - BL Flasher',
  'app.subtitle': 'KernelSU Module WebUI',
  'app.desc': '自动识别当前活动槽位，如果新版本存在GBL漏洞则跳过BL刷写，同时将BL相关镜像刷写到另一个槽位，并按需修补新版ABL到efisp',
  'slot.status': '槽位状态',
  'slot.current': '当前槽位',
  'slot.target': '目标槽位',
  'slot.images': '镜像数量',
  'slot.task': '任务状态',
  'btn.refresh': '刷新',
  'btn.flash': '刷写到另一槽位',
  'btn.clearLog': '清空日志',
  'opt.updateEfisp': '更新 efisp',
  'opt.superfastboot': '安装 superfastboot（需要更新 efisp）',
  'opt.debugMode': '调试模式（仅处理不刷写）',
  'warn.text': '刷写对象是 bootloader 相关分区，风险较高。开始前请确认镜像与机型严格匹配。',
  'img.map': '镜像映射',
  'tbl.partition': '分区名',
  'tbl.source': '源分区 (当前槽位)',
  'tbl.target': '目标分区',
  'tbl.action': '操作',
  'tbl.waiting': '等待读取模块状态',
  'log.title': '实时日志',
  'log.auto': '自动轮询最近 200 行',
  'log.waiting': '等待日志输出...',
  'modal.risk': '高风险操作',
  'modal.title': '确认刷写',
  'modal.confirmText': '即将开始刷写，请确认操作对象和槽位。',
  'modal.step1': '第一次确认: 将把当前槽位的 BL 分区拷贝到槽位',
  'modal.step1debug': '调试模式：将执行所有处理流程但不刷写分区，生成的文件保存在 tmp 目录。',
  'modal.step2': '第二次确认: 这是高风险写入操作，错误操作可能导致目标槽位无法启动。确认后将立即开始刷写。',
  'modal.withEfisp': '，并更新 efisp',
  'modal.withSfb': '，并更新 efisp（包含 superfastboot loader）',
  'modal.noEfisp': '，不更新 efisp',
  'modal.confirmSlot': '请确认槽位无误。',
  'btn.cancel': '取消',
  'btn.continue': '继续',
  'btn.confirmFlash': '确认刷写',
  'btn.startDebug': '开始调试',
  'chip.waiting': '等待检测',
  'chip.slotUnknown': '槽位未知',
  'chip.running': '任务运行中',
  'toast.needEfisp': '安装 superfastboot 需要同时勾选更新 efisp',
  'toast.running': '已有刷写任务在运行',
  'toast.startDebug': '调试任务已启动',
  'toast.startFlash': '刷写任务已启动',
  'toast.debugDone': '调试完成',
  'toast.flashDone': '刷写已完成',
  'toast.blDone': 'BL 刷写完成，但 efisp 未更新',
  'toast.failed': '任务已结束（失败）',
  'toast.startError': '任务启动失败',
  'toast.logBusy': '任务运行中，暂时不能清空日志',
  'toast.logCleared': '日志已清空',
  'toast.needEfispMsg': '安装 superfastboot 需要同时勾选更新 efisp',
  'status.skipped': '已跳过BL刷写',
  'status.success': '成功',
  'status.error': '失败',
  'status.warning': '警告',
  'status.running': '运行中',
  'status.idle': '空闲',
});

export function onLangChange(cb: () => void): () => void {
  const handler = () => cb();
  window.addEventListener('langchange', handler);
  return () => window.removeEventListener('langchange', handler);
}

export function switchLang(lang: Lang): void {
  set(lang);
  window.dispatchEvent(new Event('langchange'));
}
