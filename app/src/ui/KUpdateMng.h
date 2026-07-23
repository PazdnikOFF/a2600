#pragma once

#include "ui/KDialog.h"

#include <QTimer>

// Менеджер обновления ПО (реф. KUpdateMng : KDialog, ctor @0x716938, sizeof 0x80). UI-порт.
//
// ⚠️ ЭТО НЕ ЭКРАН, А РОУТЕР: дерева виджетов НЕТ вообще (ни одного QLabel/layout, ни одного
// setStyleSheet). Невидимый диалог 400×300 со SetKStyle(1) показывается, через **50 мс**
// таймер дёргает OpenModule(), тот прогоняет цепочку под-диалогов и закрывает роутер.
//
// Цепочка обновления (id — та же нумерация _KModuleId, что у сервисного меню):
//   13 = сам KUpdateMng, **14 = KUpdatePrepare** (распаковка пакета), **15 = KUpdateAction**
//   (прошивка разделов). Стартовый модуль — 14 (поле +0x5c).
class KUpdateMng : public KDialog
{
    Q_OBJECT
public:
    explicit KUpdateMng(QWidget *parent = nullptr);

    // Реф. @0x716bf8: код QDialog::exec() ИГНОРИРУЕТСЯ, наружу отдаётся поле +0x58
    // (так же устроен KUpdatePrepare::exec @0x6e2310).
    int exec() override;

    // Реф. @0x7168a0. Всё, что не 14 и не 15, возвращается КАК ЕСТЬ (диалоги не открываются):
    //   id == 14 → цикл `id = OpenUpdatePrepare(this)`, пока подготовка снова просит 14;
    //              как только код вне {14,15} — вернуть его наружу;
    //   id == 15 → OpenUpdateAction(this) и ВСЕГДА вернуть 0.
    int OpenUpdateView(int moduleId);

    int Result() const { return m_result; }        // +0x58
    int ModuleId() const { return m_moduleId; }    // +0x5c

public slots:
    void OpenModule();   // реф. @0x716900: стоп таймера → OpenUpdateView(m_moduleId) → close()

private:
    int m_result = 0;        // +0x58 — код результата (его отдаёт exec/OpenUpdateMng)
    int m_moduleId = 14;     // +0x5c — стартовый модуль цепочки (реф. 14 = KUpdatePrepare)
    QTimer m_timer;          // +0x60 — член-объект, интервал 50 мс
};
