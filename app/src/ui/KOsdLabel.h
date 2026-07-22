#pragma once

#include "KFrame.h"
#include <QString>
#include <QPoint>
#include <functional>

class QLabel;
class KOsdSubMenu;

// Конфиг строки-метки OSD-меню (реф. _KOsdLabelConfig, 16 байт): текст + указатель на
// «action»-функцию (реф. Xxx::EnterMenu(_KPoint,int,KOsdSubMenu*) — открывает под-подменю).
// nav-vs-inert кодируется НАЛИЧИЕМ указателя (null → инертная строка). Иконок/msg-полей НЕТ.
struct KOsdLabelConfig
{
    QString text;
    std::function<void(QPoint, int, KOsdSubMenu *)> action;   // реф. fn-ptr @+0x8 (DEVICE-seam)
};

// Строка-метка OSD-меню (реф. KOsdLabel : KFrame, ctor @0x480a50, size 0x60). UI-порт.
// QHBoxLayout(spacing 0, margins 9,0,0,0) + label_title(QLabel), фикс 250×32. Текст-only
// (ни иконки, ни стрелки). ConfirmKeyAct(key) → зовёт сохранённый action(startPoint, key,
// locatedMenu) — реф. открытие под-подменю (KIrisSetting/KOperationModeSetting::EnterMenu).
// DEVICE-SEAM = только этот action-колбэк (в порте — инъект std::function, дефолт no-op);
// вся геометрия/метка — чистый Qt. 100% PORT (кроме seam).
class KOsdLabel : public KFrame
{
    Q_OBJECT
public:
    explicit KOsdLabel(const KOsdLabelConfig &cfg, QWidget *parent = nullptr);

    void ConfirmKeyAct(int key) override;   // реф.: action(GetStartPoint(), key, GetLocatedMenu())

private:
    QString m_text;                                              // +0x50
    std::function<void(QPoint, int, KOsdSubMenu *)> m_action;    // +0x58 (DEVICE-seam)
    QLabel *m_labelTitle = nullptr;                              // Ui[1]
};
