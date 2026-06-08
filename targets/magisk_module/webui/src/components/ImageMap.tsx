import { useI18n } from '../I18nContext';
import { IMAGE_NAMES, type Status } from '../api';

export default function ImageMap({ status }: { status: Status | null }) {
  const { t } = useI18n();
  const cur = status?.current_slot;
  const tar = status?.target_slot;

  if (!cur || !tar || cur === '-' || tar === '-') {
    return (
    <section className="panel panel-medium map-panel">
        <div className="panel-head">
          <h2>{t('img.map')}</h2>
        </div>
        <div className="table-wrap">
          <table>
            <tbody>
              <tr><td className="empty">{t('tbl.waiting')}</td></tr>
            </tbody>
          </table>
        </div>
      </section>
    );
  }

  return (
    <section className="panel panel--outlined map-panel">
      <div className="panel-head">
        <h2>{t('img.map')}</h2>
        <span className="caption">{status?.updated_at ?? '-'}</span>
      </div>
      <div className="table-wrap">
        <table>
          <thead>
            <tr>
              <th>{t('tbl.partition')}</th>
              <th>{t('tbl.source')}</th>
              <th>{t('tbl.target')}</th>
              <th>{t('tbl.action')}</th>
            </tr>
          </thead>
          <tbody>
            {IMAGE_NAMES.map(name => (
              <tr key={name}>
                <td>{name}</td>
                <td className="caption">/dev/block/by-name/{name}{cur}</td>
                <td>/dev/block/by-name/{name}{tar}</td>
                <td><span className="pill pill--ok">{t('status.copyPart')}</span></td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </section>
  );
}
