#include "KUpdateMng.h"
#include "KUiNavigation.h"

KUpdateMng::KUpdateMng(QWidget *parent)
    : KDialog(parent, /*subscribeStatus=*/false)
{
    // Реф. ctor @0x716938, порядок сохранён. Никаких виджетов — только окно и таймер.
    setObjectName(QStringLiteral("KUpdateMng"));
    resize(400, 300);
    setWindowTitle(tr("TR_Dlg"));
    SetKStyle(KDLG_FULLSCREEN);      // реф. SetKStyle(1)

    connect(&m_timer, &QTimer::timeout, this, &KUpdateMng::OpenModule);
    m_timer.start(50);               // реф. 50 мс
}

int KUpdateMng::exec()
{
    // Реф. @0x716bf8: результат QDialog::exec() отбрасывается.
    KDialog::exec();
    return m_result;
}

int KUpdateMng::OpenUpdateView(int moduleId)
{
    // Реф. @0x7168a0: `sub w2,id,#0xe; cmp w2,#1; b.hi` — «всё, кроме 14 и 15, наружу как есть».
    if (unsigned(moduleId - 14) > 1u)
        return moduleId;

    if (moduleId == 14) {
        // Цикл: подготовка может вернуть 14 (повторить) — тогда открываем её снова.
        while (true) {
            moduleId = OpenUpdatePrepare(this);
            if (unsigned(moduleId - 14) > 1u)
                return moduleId;   // код вне {14,15} — наружу
            if (moduleId != 14)
                break;             // 15 — переходим к прошивке
        }
    }
    OpenUpdateAction(this);
    return 0;                      // реф.: после прошивки ВСЕГДА 0
}

void KUpdateMng::OpenModule()
{
    // Реф. @0x716900.
    m_timer.stop();
    m_result = OpenUpdateView(m_moduleId);
    close();
}
