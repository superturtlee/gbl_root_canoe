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

export function getStatus(): Status {
  const raw = run('status');
  return JSON.parse(raw);
}

export function getLog(): string[] {
  const raw = run('log-json');
  try {
    return JSON.parse(raw);
  } catch {
    return [raw];
  }
}

export function getLogTail(n: number = 200): string {
  return run(`tail ${n}`);
}

export function startFlash(mode: string): StartResult {
  const raw = run(`start ${mode}`);
  return JSON.parse(raw);
}

export function clearLog(): ClearResult {
  const raw = run('clear-log');
  return JSON.parse(raw);
}

export const FLASH_MODES = {
  SKIP_EFISP: 'skip-efisp',
  UPDATE_EFISP: 'update-efisp',
  UPDATE_EFISP_SFB: 'update-efisp-with-superfastboot',
  DEBUG: 'debug',
  DEBUG_SFB: 'debug-with-superfastboot',
} as const;

export const IMAGE_NAMES = ['abl'] as const;
