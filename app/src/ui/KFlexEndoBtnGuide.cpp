#include "ui/KFlexEndoBtnGuide.h"

#include <QLabel>
#include <QPixmap>

#include "endo/KEndoScope.h"
#include "sys/KRemoteSwitchConfig.h"
#include "sys/KUserSet.h"
#include "ui/Theme.h"

// Реф. константы InitItem @0x69e860 (сверены по инструкциям, не округлены):
//   иконка  setFixedSize(0x1c, 0x1c) = 28×28, move(0, w20)
//   подпись setFixedSize(0x96, 0x1c) = 150×28, move(0x24, w20), setAlignment(0x81)
//   w20 = (i + (i << 3)) << 2  ==  36·i          (@0x69e8a0 + @0x69e8b4)
static const int kIconSize = 28;
static const int kTextW = 150;
static const int kTextH = 28;
static const int kTextX = 36;
static const int kRowPitch = 36;
static const int kItemCount = 4;   // реф. InitWidgets: строго i = 0..3

KFlexEndoBtnGuide::KFlexEndoBtnGuide(QWidget *parent)
    : QWidget(parent)
{
    // Реф. ctor @0x69ed20: QWidget(parent, 0) → пустая map → InitWidgets().
    InitWidgets();
}

void KFlexEndoBtnGuide::InitWidgets()
{
    // Реф. @0x69ece0: цикл 0..3 и хвостовой вызов слота. Ничего больше — ни objectName,
    // ни stylesheet, ни resize на самом виджете (их ставит владелец KViewSoftEndo).
    for (int i = 0; i < kItemCount; ++i)
        InitItem(i);
    onSetEndoBtnFuncText();
}

void KFlexEndoBtnGuide::InitItem(int index)
{
    const int y = (index + (index << 3)) << 2;   // реф. арифметика 1:1 (= 36·index)

    QLabel *icon = new QLabel(this);
    icon->setFixedSize(kIconSize, kIconSize);
    icon->move(0, y);
    // Реф. строка @0x867840 (len 47) — дословно, вместе с пробелами оригинала.
    icon->setStyleSheet(QStringLiteral("QLabel { background:transparent; border: none }"));
    // Реф.: KSystem::RootPath() + "qss/black/icon/endobtn/btn%1.png".arg(index) —
    // в порте тот же файл через theme::asset (asset добавляет "qss/" сам).
    const QPixmap pm(theme::asset(QStringLiteral("black/icon/endobtn/btn%1.png").arg(index)));
    if (!pm.isNull()) {
        // Реф. @0x69e99c: QSize(28,28), Qt::IgnoreAspectRatio, Qt::FastTransformation
        // (исходники 28×29 — масштабирование НАМЕРЕННО с искажением).
        icon->setPixmap(pm.scaled(QSize(kIconSize, kIconSize),
                                  Qt::IgnoreAspectRatio, Qt::FastTransformation));
    }

    QLabel *text = new QLabel(this);
    text->setFixedSize(kTextW, kTextH);
    text->move(kTextX, y);
    text->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);   // реф. 0x81
    // Текст здесь НЕ ставится — только в onSetEndoBtnFuncText (реф. так же).

    m_items.emplace(index, std::make_pair(icon, text));
}

void KFlexEndoBtnGuide::onSetEndoBtnFuncText()
{
    // Реф. @0x69e6e8: GetKUserSet()->GetVideoParamConfig(), поля +0x00..+0x0c —
    // это [RemoteSwitch]Switch1..4 из user.ini (ключи прочитаны в ReadVideoParamConfig
    // @0x663168: "RemoteSwitch/Switch1".."4"). У нас эти ключи уже читает
    // KRemoteSwitchConfig, поэтому берём их оттуда: строка i ↔ Switch(i+1).
    const KRemoteSwitchConfig &rs = KRemoteSwitchConfig::GetInstance();
    for (int i = 0; i < kItemCount; ++i) {
        auto it = m_items.find(i);
        if (it == m_items.end() || !it->second.second)
            continue;   // реф. разыменовал бы nullptr из operator[] — в норме недостижимо
        it->second.second->setText(KUserSet::GetFunctionName(rs.GetRemoteSwitchFunctionId(i + 1)));
    }
}

void KFlexEndoBtnGuide::Show(bool bShow)
{
    // Реф. @0x69e5b0.
    if (!bShow || !GetEndoScope()->IsEndoReady()) {
        hide();
        return;
    }
    show();

    // Реф.: сравнение модели с "EUD.100S" (@0x862990) — у этой модели строки 0 и 3
    // скрываются (порядок вызовов реф.: 0.first, 0.second, 3.first, 3.second).
    const _EndoInfoStruct *info = GetEndoScope()->EndoInfo();
    const bool visible = !info || info->sModel.compare(QStringLiteral("EUD.100S")) != 0;
    for (int i : {0, 3}) {
        auto it = m_items.find(i);
        if (it == m_items.end())
            continue;
        if (it->second.first)  it->second.first->setVisible(visible);
        if (it->second.second) it->second.second->setVisible(visible);
    }
}

std::pair<QLabel *, QLabel *> KFlexEndoBtnGuide::Item(int i) const
{
    auto it = m_items.find(i);
    return it == m_items.end() ? std::make_pair<QLabel *, QLabel *>(nullptr, nullptr)
                               : it->second;
}
