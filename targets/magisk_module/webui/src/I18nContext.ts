import { createContext, useContext } from 'react';
import { get, switchLang, t, langLabel, available } from './i18n';

export interface I18nCtx {
  lang: string;
  t: (key: string) => string;
  setLang: (lang: string) => void;
  langs: string[];
  label: (lang: string) => string;
}

export const I18nContext = createContext<I18nCtx>({
  lang: get(),
  t,
  setLang: () => {},
  langs: available(),
  label: langLabel,
});

export function useI18n(): I18nCtx {
  return useContext(I18nContext);
}
