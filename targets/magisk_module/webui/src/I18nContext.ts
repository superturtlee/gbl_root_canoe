import { createContext, useContext } from 'react';
import { type Lang, get, switchLang, t as translate } from './i18n';

export interface I18nCtx {
  lang: Lang;
  t: (key: string) => string;
  setLang: (lang: Lang) => void;
  langs: Lang[];
}

export const I18nContext = createContext<I18nCtx>({
  lang: get(),
  t: translate,
  setLang: () => {},
  langs: ['zh', 'en'],
});

export function useI18n(): I18nCtx {
  return useContext(I18nContext);
}
