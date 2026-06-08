import { useRef, useEffect } from 'react';
import { useI18n } from '../I18nContext';
import type { Status } from '../api';

interface Props {
  log: string;
  status: Status | null;
}

export default function LogPanel({ log, status }: Props) {
  const { t } = useI18n();
  const preRef = useRef<HTMLPreElement>(null);

  useEffect(() => {
    if (preRef.current) {
      preRef.current.scrollTop = preRef.current.scrollHeight;
    }
  }, [log]);

  const display = log || t('log.waiting');

  return (
    <section className="panel panel-low log-panel">
      <div className="panel-head">
        <h2>{t('log.title')}</h2>
        <span className="caption">{status?.running ? t('log.auto') : ''}</span>
      </div>
      <pre ref={preRef} className="log-output">{display}</pre>
    </section>
  );
}
