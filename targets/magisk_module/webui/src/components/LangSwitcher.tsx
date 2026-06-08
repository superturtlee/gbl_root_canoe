import { useI18n } from '../I18nContext';
import { type Lang } from '../i18n';

const LANG_LABELS: Record<Lang, string> = {
  zh: '中',
  en: 'EN',
};

export default function LangSwitcher() {
  const { lang, setLang, langs } = useI18n();

  if (langs.length <= 1) return null;

  const next = langs[(langs.indexOf(lang) + 1) % langs.length];

  return (
    <button
      className="lang-btn"
      onClick={() => setLang(next)}
      title={next}
    >
      {LANG_LABELS[lang]}
    </button>
  );
}
