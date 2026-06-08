import {
  Hct,
  TonalPalette,
  hexFromArgb,
} from '@material/material-color-utilities';

type TokenMap = Record<string, string>;

function tone(t: TonalPalette, n: number): string {
  return hexFromArgb(t.tone(n));
}

function sys(light: number, dark: number, token: string, map: TokenMap) {
  map[token] = `var(--md-ref-palette-${light})`;
}

export function generateTheme(seedOrHue: string | number): TokenMap {
  const hct = typeof seedOrHue === 'string'
    ? Hct.fromInt(parseInt(seedOrHue.replace('#', ''), 16))
    : Hct.from(seedOrHue, 48, 50);

  const hue = hct.hue;
  const chroma = Math.min(hct.chroma || 48, 48);

  const primary     = TonalPalette.fromHueAndChroma(hue, chroma);
  const secondary   = TonalPalette.fromHueAndChroma(hue + 30, Math.round(chroma * 0.5));
  const tertiary    = TonalPalette.fromHueAndChroma(hue + 60, Math.round(chroma * 0.35));
  const neutral     = TonalPalette.fromHueAndChroma(hue, Math.round(chroma * 0.10));
  const neutralV    = TonalPalette.fromHueAndChroma(hue, Math.round(chroma * 0.16));

  // Reference palette tokens
  const toneAt = (p: TonalPalette, n: number) => hexFromArgb(p.tone(n));
  const t = (p: TonalPalette) => ({
    0: toneAt(p, 0), 10: toneAt(p, 10), 20: toneAt(p, 20),
    30: toneAt(p, 30), 40: toneAt(p, 40), 50: toneAt(p, 50),
    60: toneAt(p, 60), 70: toneAt(p, 70), 80: toneAt(p, 80),
    90: toneAt(p, 90), 92: toneAt(p, 92), 94: toneAt(p, 94),
    95: toneAt(p, 95), 96: toneAt(p, 96), 98: toneAt(p, 98),
    99: toneAt(p, 99), 100: toneAt(p, 100),
  });

  const pri = t(primary);
  const sec = t(secondary);
  const ter = t(tertiary);
  const n   = t(neutral);
  const nv  = t(neutralV);

  // Error: fixed MD3 palette
  const err   = TonalPalette.fromHueAndChroma(25, 84);
  const errT  = t(err);

  const map: TokenMap = {
    // Source
    '--md-source': typeof seedOrHue === 'string' ? seedOrHue : `hsl(${hue}, ${chroma}%, 50%)`,

    // Reference — primary
    '--md-ref-palette-primary0':   pri[0],
    '--md-ref-palette-primary10':  pri[10],
    '--md-ref-palette-primary20':  pri[20],
    '--md-ref-palette-primary30':  pri[30],
    '--md-ref-palette-primary40':  pri[40],
    '--md-ref-palette-primary50':  pri[50],
    '--md-ref-palette-primary60':  pri[60],
    '--md-ref-palette-primary70':  pri[70],
    '--md-ref-palette-primary80':  pri[80],
    '--md-ref-palette-primary90':  pri[90],
    '--md-ref-palette-primary95':  pri[95],
    '--md-ref-palette-primary99':  pri[99],
    '--md-ref-palette-primary100': pri[100],

    // Reference — secondary
    '--md-ref-palette-secondary0':   sec[0],
    '--md-ref-palette-secondary10':  sec[10],
    '--md-ref-palette-secondary20':  sec[20],
    '--md-ref-palette-secondary30':  sec[30],
    '--md-ref-palette-secondary40':  sec[40],
    '--md-ref-palette-secondary50':  sec[50],
    '--md-ref-palette-secondary60':  sec[60],
    '--md-ref-palette-secondary70':  sec[70],
    '--md-ref-palette-secondary80':  sec[80],
    '--md-ref-palette-secondary90':  sec[90],
    '--md-ref-palette-secondary95':  sec[95],
    '--md-ref-palette-secondary99':  sec[99],
    '--md-ref-palette-secondary100': sec[100],

    // Reference — tertiary
    '--md-ref-palette-tertiary0':   ter[0],
    '--md-ref-palette-tertiary10':  ter[10],
    '--md-ref-palette-tertiary20':  ter[20],
    '--md-ref-palette-tertiary30':  ter[30],
    '--md-ref-palette-tertiary40':  ter[40],
    '--md-ref-palette-tertiary50':  ter[50],
    '--md-ref-palette-tertiary60':  ter[60],
    '--md-ref-palette-tertiary70':  ter[70],
    '--md-ref-palette-tertiary80':  ter[80],
    '--md-ref-palette-tertiary90':  ter[90],
    '--md-ref-palette-tertiary95':  ter[95],
    '--md-ref-palette-tertiary99':  ter[99],
    '--md-ref-palette-tertiary100': ter[100],

    // Reference — error (fixed)
    '--md-ref-palette-error0':   errT[0],
    '--md-ref-palette-error10':  errT[10],
    '--md-ref-palette-error20':  errT[20],
    '--md-ref-palette-error30':  errT[30],
    '--md-ref-palette-error40':  errT[40],
    '--md-ref-palette-error50':  errT[50],
    '--md-ref-palette-error60':  errT[60],
    '--md-ref-palette-error70':  errT[70],
    '--md-ref-palette-error80':  errT[80],
    '--md-ref-palette-error90':  errT[90],
    '--md-ref-palette-error95':  errT[95],
    '--md-ref-palette-error99':  errT[99],
    '--md-ref-palette-error100': errT[100],

    // Reference — neutral
    '--md-ref-palette-neutral0':   n[0],
    '--md-ref-palette-neutral10':  n[10],
    '--md-ref-palette-neutral20':  n[20],
    '--md-ref-palette-neutral30':  n[30],
    '--md-ref-palette-neutral40':  n[40],
    '--md-ref-palette-neutral50':  n[50],
    '--md-ref-palette-neutral60':  n[60],
    '--md-ref-palette-neutral70':  n[70],
    '--md-ref-palette-neutral80':  n[80],
    '--md-ref-palette-neutral90':  n[90],
    '--md-ref-palette-neutral95':  n[95],
    '--md-ref-palette-neutral99':  n[99],
    '--md-ref-palette-neutral100': n[100],

    // Reference — neutral-variant
    '--md-ref-palette-neutral-variant0':   nv[0],
    '--md-ref-palette-neutral-variant10':  nv[10],
    '--md-ref-palette-neutral-variant20':  nv[20],
    '--md-ref-palette-neutral-variant30':  nv[30],
    '--md-ref-palette-neutral-variant40':  nv[40],
    '--md-ref-palette-neutral-variant50':  nv[50],
    '--md-ref-palette-neutral-variant60':  nv[60],
    '--md-ref-palette-neutral-variant70':  nv[70],
    '--md-ref-palette-neutral-variant80':  nv[80],
    '--md-ref-palette-neutral-variant90':  nv[90],
    '--md-ref-palette-neutral-variant95':  nv[95],
    '--md-ref-palette-neutral-variant99':  nv[99],
    '--md-ref-palette-neutral-variant100': nv[100],
  };

  // System tokens — light theme
  const sysLight: TokenMap = {
    '--md-sys-color-primary':                pri[40],
    '--md-sys-color-on-primary':             pri[100],
    '--md-sys-color-primary-container':       pri[90],
    '--md-sys-color-on-primary-container':    pri[10],
    '--md-sys-color-secondary':              sec[40],
    '--md-sys-color-on-secondary':           sec[100],
    '--md-sys-color-secondary-container':     sec[90],
    '--md-sys-color-on-secondary-container':  sec[10],
    '--md-sys-color-tertiary':               ter[40],
    '--md-sys-color-on-tertiary':            ter[100],
    '--md-sys-color-tertiary-container':      ter[90],
    '--md-sys-color-on-tertiary-container':   ter[10],
    '--md-sys-color-error':                  errT[40],
    '--md-sys-color-on-error':               errT[100],
    '--md-sys-color-error-container':         errT[90],
    '--md-sys-color-on-error-container':      errT[10],
    '--md-sys-color-surface':                n[98],
    '--md-sys-color-on-surface':             n[10],
    '--md-sys-color-surface-variant':        nv[90],
    '--md-sys-color-on-surface-variant':     nv[30],
    '--md-sys-color-surface-container-lowest': n[100],
    '--md-sys-color-surface-container-low':  n[96],
    '--md-sys-color-surface-container':      n[94],
    '--md-sys-color-surface-container-high': n[92],
    '--md-sys-color-surface-container-highest': n[90],
    '--md-sys-color-inverse-surface':        n[20],
    '--md-sys-color-inverse-on-surface':     n[95],
    '--md-sys-color-inverse-primary':        pri[80],
    '--md-sys-color-outline':                nv[50],
    '--md-sys-color-outline-variant':        nv[80],
  };

  Object.assign(map, sysLight);
  return map;
}

export function applyTheme(seed: string | number): void {
  const tokens = generateTheme(seed);
  const root = document.documentElement;
  for (const [key, val] of Object.entries(tokens)) {
    root.style.setProperty(key, val);
  }
}
