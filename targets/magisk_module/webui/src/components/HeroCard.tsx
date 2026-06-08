import { useI18n } from '../I18nContext';
import type { Status } from '../api';

function stateClass(state: string): string {
  switch (state) {
    case 'success': return 'chip chip--success';
    case 'error': return 'chip chip--error';
    case 'warning': return 'chip chip--warn';
    case 'running': return 'chip chip--warn';
    default: return 'chip';
  }
}

export default function HeroCard({ status }: { status: Status | null }) {
  const { t } = useI18n();
  const running = status?.running;
  const st = status?.state ?? 'idle';
  const cur = status?.current_slot ?? '-';
  const tar = status?.target_slot ?? '-';
  const slotText = (cur !== '-' && tar !== '-')
    ? `${cur} → ${tar}`
    : t('chip.slotUnknown');

  return (
    <header className="hero">
      <div>
        <p className="eyebrow">{t('app.subtitle')}</p>
        <h1>{t('app.title')}</h1>
        <p className="hero-desc">{t('app.desc')}</p>
      </div>
      <div className="hero-meta">
        <span className={stateClass(st)}>
          {running ? t('chip.running') : `${st}`}
        </span>
        <span className={`chip ${running ? 'chip--warn' : ''}`}>{slotText}</span>
      </div>
    </header>
  );
}
