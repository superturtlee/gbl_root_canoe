import { useState, useEffect, useRef } from 'react';
import { useI18n } from '../I18nContext';

export default function LangSwitcher() {
  const { lang, setLang, langs, label } = useI18n();
  const [open, setOpen] = useState(false);
  const ref = useRef<HTMLDivElement>(null);

  useEffect(() => {
    if (!open) return;
    const handler = (e: MouseEvent) => {
      if (ref.current && !ref.current.contains(e.target as Node)) setOpen(false);
    };
    document.addEventListener('click', handler);
    return () => document.removeEventListener('click', handler);
  }, [open]);

  if (langs.length <= 1) return null;

  return (
    <div className="lang-picker" ref={ref}>
      <button className="lang-btn" onClick={() => setOpen(!open)}>
        {label(lang)}
      </button>
      {open && (
        <ul className="lang-dropdown">
          {langs.map(l => (
            <li key={l}>
              <button
                className={`lang-item${l === lang ? ' lang-item--active' : ''}`}
                onClick={() => { setLang(l); setOpen(false); }}
              >
                {label(l)}
              </button>
            </li>
          ))}
        </ul>
      )}
    </div>
  );
}
