import { useI18n } from '../I18nContext';

export default function LangSwitcher() {
  const { lang, setLang, langs, label } = useI18n();

  if (langs.length <= 1) return null;

  const idx = langs.indexOf(lang);
  const next = langs[(idx + 1) % langs.length];

  return (
    <button
      className="lang-btn"
      onClick={() => setLang(next)}
      title={next}
    >
      {label(lang)}
    </button>
  );
}
