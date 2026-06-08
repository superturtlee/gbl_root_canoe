import { useState, useMemo, useSyncExternalStore } from 'react';
import { get, switchLang, available, t, langLabel, onLangChange } from './i18n';
import { I18nContext } from './I18nContext';
import { usePolling } from './hooks/usePolling';
import HeroCard from './components/HeroCard';
import SlotStatus from './components/SlotStatus';
import ImageMap from './components/ImageMap';
import LogPanel from './components/LogPanel';

function useLang() {
  const subscribe = (cb: () => void) => onLangChange(cb);
  const getSnapshot = () => get();
  return useSyncExternalStore(subscribe, getSnapshot);
}

export default function App() {
  const lang = useLang();
  const [_, forceRender] = useState(0);

  const i18nCtx = useMemo(() => ({
    lang,
    t,
    setLang: (l: string) => {
      switchLang(l);
      forceRender(n => n + 1);
    },
    langs: available(),
    label: langLabel,
  }), [lang]);

  const { status, log, refresh } = usePolling();

  return (
    <I18nContext.Provider value={i18nCtx}>
      <div className="page">
        <HeroCard status={status} />
        <main className="grid">
          <SlotStatus status={status} onRefresh={refresh} />
          <ImageMap status={status} />
          <LogPanel log={log} status={status} />
        </main>
      </div>
    </I18nContext.Provider>
  );
}
