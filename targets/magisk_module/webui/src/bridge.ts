interface KsuBridge {
  moduleInfo?: () => string | Record<string, unknown>;
  exec?: (cmd: string) => string | { stdout: string; stderr: string };
  toast?: (msg: string) => void;
}

declare global {
  interface Window {
    ksu?: KsuBridge;
  }
}

function getBridge(): KsuBridge | null {
  return window.ksu ?? null;
}

function extractStdout(raw: unknown): string {
  if (raw == null) return '';
  if (typeof raw === 'object' && raw !== null) {
    const obj = raw as Record<string, unknown>;
    if (typeof obj.stdout === 'string') return obj.stdout;
    if (typeof obj.out === 'string') return obj.out;
  }
  return String(raw);
}

export function exec(cmd: string): string {
  const bridge = getBridge();
  if (!bridge?.exec) return '';
  return extractStdout(bridge.exec(cmd));
}

export function toast(msg: string): void {
  getBridge()?.toast?.(msg);
}

export function moduleInfo(): { moduleDir: string } {
  const bridge = getBridge();
  if (!bridge) throw new Error('No WebUI bridge');

  if (bridge.moduleInfo) {
    const raw = bridge.moduleInfo();
    const parsed = typeof raw === 'string' ? JSON.parse(raw) : raw;
    if (parsed && typeof parsed === 'object') {
      const obj = parsed as Record<string, unknown>;
      if (typeof obj.moduleDir === 'string') return { moduleDir: obj.moduleDir };
    }
  }

  if (bridge.exec) {
    const found = exec(
      'for d in /data/adb/modules/*/; do [ -f "${d}bin/bl_flasher" ] && printf "%s" "${d%/}" && break; done'
    ).trim();
    if (found) return { moduleDir: found };
  }

  throw new Error('Module not found');
}

export function getModuleDir(): string {
  try {
    return moduleInfo().moduleDir;
  } catch {
    return '/data/adb/modules/fake_bl_efisp';
  }
}
