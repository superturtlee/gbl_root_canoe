import { useState, useCallback } from 'react';
import { useI18n } from '../I18nContext';
import { startFlash, clearLog, FLASH_MODES, IMAGE_NAMES, type Status } from '../api';
import { toast } from '../bridge';
import LangSwitcher from './LangSwitcher';
import ConfirmModal from './ConfirmModal';

interface Props {
  status: Status | null;
  onRefresh: () => void;
}

export default function SlotStatus({ status, onRefresh }: Props) {
  const { t } = useI18n();
  const [showModal, setShowModal] = useState(false);
  const [updateEfisp, setUpdateEfisp] = useState(false);
  const [superfastboot, setSuperfastboot] = useState(false);
  const [debugMode, setDebugMode] = useState(false);

  const running = status?.running ?? false;
  const cur = status?.current_slot ?? '-';
  const tar = status?.target_slot ?? '-';
  const disabled = running || cur === '-' || tar === '-';

  const handleFlash = useCallback(() => {
    if (superfastboot && !updateEfisp) {
      toast(t('toast.needEfisp'));
      return;
    }
    setShowModal(true);
  }, [superfastboot, updateEfisp, t]);

  const handleConfirm = useCallback(() => {
    setShowModal(false);
    let mode: string = FLASH_MODES.SKIP_EFISP;
    if (debugMode) {
      mode = superfastboot ? FLASH_MODES.DEBUG_SFB : FLASH_MODES.DEBUG;
    } else if (updateEfisp) {
      mode = superfastboot ? FLASH_MODES.UPDATE_EFISP_SFB : FLASH_MODES.UPDATE_EFISP;
    }

    try {
      const res = startFlash(mode);
      if (res.already_running) toast(t('toast.running'));
      else if (res.started) toast(debugMode ? t('toast.startDebug') : t('toast.startFlash'));
      else if (res.finished === 'success') toast(debugMode ? t('toast.debugDone') : t('toast.flashDone'));
      else if (res.finished === 'warning') toast(t('toast.blDone'));
      else if (res.finished === 'error') toast(t('toast.failed'));
      else toast(t('toast.startError'));
    } catch (e) {
      toast(`${t('toast.startError')}: ${e}`);
    }
    onRefresh();
  }, [updateEfisp, superfastboot, debugMode, onRefresh, t]);

  const handleClearLog = useCallback(() => {
    try {
      const res = clearLog();
      if (res.busy) toast(t('toast.logBusy'));
      else toast(t('toast.logCleared'));
    } catch (e) {
      toast(`${e}`);
    }
    onRefresh();
  }, [onRefresh, t]);

  return (
    <>
      <section className="panel panel-high stats-panel">
        <div className="panel-head">
          <h2>{t('slot.status')}</h2>
          <div className="panel-head-actions">
            <LangSwitcher />
            <button className="btn--text" onClick={onRefresh}>{t('btn.refresh')}</button>
          </div>
        </div>

        <div className="stats-grid">
          <div className="stat-card">
            <span className="stat-label">{t('slot.current')}</span>
            <strong className="stat-value">{cur}</strong>
          </div>
          <div className="stat-card stat-card--accent">
            <span className="stat-label">{t('slot.target')}</span>
            <strong className="stat-value">{tar}</strong>
          </div>
          <div className="stat-card">
            <span className="stat-label">{t('slot.images')}</span>
            <strong className="stat-value">{IMAGE_NAMES.length}</strong>
          </div>
          <div className="stat-card">
            <span className="stat-label">{t('slot.task')}</span>
            <strong className="stat-value stat-small">{status?.message ?? t('chip.waiting')}</strong>
          </div>
        </div>

        <div className="actions">
          <button className="btn--filled" disabled={disabled} onClick={handleFlash}>
            {t('btn.flash')}
          </button>
          <button className="btn--outlined" disabled={running} onClick={handleClearLog}>
            {t('btn.clearLog')}
          </button>
        </div>

        <label className="option">
          <input type="checkbox" checked={updateEfisp} onChange={e => setUpdateEfisp(e.target.checked)} />
          <span>{t('opt.updateEfisp')}</span>
        </label>
        <label className="option">
          <input type="checkbox" checked={superfastboot} onChange={e => setSuperfastboot(e.target.checked)} />
          <span>{t('opt.superfastboot')}</span>
        </label>
        <label className="option">
          <input type="checkbox" checked={debugMode} onChange={e => setDebugMode(e.target.checked)} />
          <span>{t('opt.debugMode')}</span>
        </label>

        <p className="warn">{t('warn.text')}</p>
      </section>

      {showModal && (
        <ConfirmModal
          targetSlot={tar}
          updateEfisp={updateEfisp}
          superfastboot={superfastboot}
          debugMode={debugMode}
          onCancel={() => setShowModal(false)}
          onConfirm={handleConfirm}
        />
      )}
    </>
  );
}
