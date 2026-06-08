import { useState } from 'react';
import { useI18n } from '../I18nContext';

interface Props {
  targetSlot: string;
  updateEfisp: boolean;
  superfastboot: boolean;
  debugMode: boolean;
  onCancel: () => void;
  onConfirm: () => void;
}

export default function ConfirmModal({
  targetSlot,
  updateEfisp,
  superfastboot,
  debugMode,
  onCancel,
  onConfirm,
}: Props) {
  const { t } = useI18n();
  const [step, setStep] = useState(1);

  const handleNext = () => {
    if (step === 1 && !debugMode) {
      setStep(2);
    } else {
      onConfirm();
    }
  };

  let msg: string;
  if (debugMode) {
    msg = t('modal.step1debug');
  } else if (step === 1) {
    msg = `${t('modal.step1')} ${targetSlot}`;
    if (updateEfisp) msg += superfastboot ? t('modal.withSfb') : t('modal.withEfisp');
    else msg += t('modal.noEfisp');
    msg += t('modal.confirmSlot');
  } else {
    msg = t('modal.step2');
  }

  const btnText = (debugMode || step === 2)
    ? (debugMode ? t('btn.startDebug') : t('btn.confirmFlash'))
    : t('btn.continue');

  return (
    <div className="modal-backdrop" onClick={onCancel}>
      <div className="modal" onClick={e => e.stopPropagation()}>
        <p className="eyebrow">{t('modal.risk')}</p>
        <h2>{t('modal.title')}</h2>
        <p className="modal-text">{msg}</p>
        <div className="modal-actions">
          <button className="btn--text" onClick={onCancel}>{t('btn.cancel')}</button>
          <button className="btn--filled" onClick={handleNext}>{btnText}</button>
        </div>
      </div>
    </div>
  );
}
