import { useEffect, useRef, useState, useCallback } from 'react';
import { getStatus, getLog, type Status } from '../api';

export function usePolling() {
  const [status, setStatus] = useState<Status | null>(null);
  const [log, setLog] = useState<string>('');
  const timerRef = useRef<ReturnType<typeof setTimeout> | null>(null);

  const refresh = useCallback(() => {
    try {
      const s = getStatus();
      setStatus(s);
      if (s.running) {
        try {
          setLog(getLog());
        } catch {}
      }
      const delay = s.running ? 3000 : 8000;
      timerRef.current = setTimeout(refresh, delay);
    } catch (e) {
      console.error('Poll error:', e);
      timerRef.current = setTimeout(refresh, 8000);
    }
  }, []);

  useEffect(() => {
    refresh();
    return () => {
      if (timerRef.current) clearTimeout(timerRef.current);
    };
  }, [refresh]);

  const forceRefresh = useCallback(() => {
    if (timerRef.current) clearTimeout(timerRef.current);
    refresh();
  }, [refresh]);

  return { status, log, refresh: forceRefresh };
}
