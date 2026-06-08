import { exec, getModuleDir } from './bridge';

export interface Status {
  current_slot: string;
  target_slot: string;
  running: boolean;
  pid: number | null;
  state: string;
  message: string;
  updated_at: string;
}

export interface StartResult {
  started?: boolean;
  already_running?: boolean;
  finished?: string;
  error?: string;
}

export interface ClearResult {
  cleared?: boolean;
  busy?: boolean;
}

function run(args: string): string {
  const dir = getModuleDir();
  return exec(`MODDIR=${shellQuote(dir)} ${shellQuote(dir + '/bin/bl_flasher')} ${args}`);
}

function shellQuote(s: string): string {
  return `'${s.replace(/'/g, `'\\''`)}'`;
}

const FALLBACK_STATUS: Status = {
  current_slot: '-',
  target_slot: '-',
  running: false,
  pid: null,
  state: 'error',
  message: 'Binary communication failed',
  updated_at: '',
};

export function getStatus(): Status {
  try {
    const raw = run('status');
    return JSON.parse(raw);
  } catch {
    return FALLBACK_STATUS;
  }
}

export function getLog(): string {
  try {
    return run('log');
  } catch {
    return '';
  }
}

export function getLogTail(n: number = 200): string {
  return run(`tail ${n}`);
}

export function startFlash(mode: string): StartResult {
  try {
    const raw = run(`start ${mode}`);
    return JSON.parse(raw);
  } catch {
    return { started: false, error: 'Binary communication failed' };
  }
}

export function clearLog(): ClearResult {
  try {
    const raw = run('clear-log');
    return JSON.parse(raw);
  } catch {
    return {};
  }
}

export const FLASH_MODES = {
  SKIP_EFISP: 'skip-efisp',
  UPDATE_EFISP: 'update-efisp',
  UPDATE_EFISP_SFB: 'update-efisp-with-superfastboot',
  DEBUG: 'debug',
  DEBUG_SFB: 'debug-with-superfastboot',
} as const;

export const IMAGE_NAMES = ['abl'] as const;
