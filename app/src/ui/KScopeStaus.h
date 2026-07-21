#pragma once

#include <QFrame>
#include <QString>

#include "sys/KEncStyle.h"

class QLabel;

// Дочерний виджет: спецификации подключённого эндоскопа (реф. KScopeValue @ctor 0x68d578,
// base QWidget). 5 меток (Model / Instrument SN / Channel / Distal End / Bending Section),
// все AlignCenter, stylesheet «font-size: 12px;». Числовые спеки приходят слотами.
// Подписи-префиксы — инференс (точные caption-литералы из бинарника не извлекались).
class KScopeValue : public QWidget
{
    Q_OBJECT
public:
    explicit KScopeValue(QWidget *parent = nullptr);

    void SetModel(const QString &model);          // спец-кейсы ENL-110/ENL-X20 (реф.)
    void SetInstrumentSN(const QString &sn);

public slots:
    void SetInstrumentChannel(double v);
    void SetDistalEnd(double v);
    void SetBendingSection(double v);

private:
    QLabel *m_model = nullptr;
    QLabel *m_sn = nullptr;
    QLabel *m_channel = nullptr;
    QLabel *m_distal = nullptr;
    QLabel *m_bending = nullptr;
};

// Кликабельная панель статуса эндоскопа (реф. KScopeStaus @ctor 0x68cf70, base QFrame —
// имя действительно с опечаткой «Staus»). UI-порт РЕАЛЬНОГО кастом-виджета — ранее
// подставлялся заглушкой-QFrame. 3 составных ребёнка: KScopeValue (105×120) + label иконки
// биопсийного канала (105×88) + teal-метка модели (105×40 @ (0,215), «font-size:17px;
// color: rgb(0,153,153);», скрыта до подключения). Панель целиком — кнопка (clicked()).
//
// Использует РЕАЛЬНЫЕ чистые методы в дереве: KEncStyle::getBiopsyImg/GetEndoDisplayModel
// (сборка пути/маппинг, ini не читают), KEndoScope::IsEndoHasChannel (static). DEVICE-STUB:
// живое чтение сцена (GetEndoModel в ctor, GetEndoScope()->GetEndoInfo() в SetScopeConnect)
// не тянется — данные подаёт вызывающий через SetScopeModel/SetScopeSN.
class KScopeStaus : public QFrame
{
    Q_OBJECT
public:
    explicit KScopeStaus(QWidget *parent = nullptr);

    void SetScopeConnect(bool connected);        // реф. @0x68d258
    void SetScopeModel(const QString &model);    // реф. @0x68cc00
    void SetScopeSN(const QString &sn);          // реф. @0x68cb08
    void SetShowSpareFlag(bool v) { m_bShowSpare = v; }   // реф. @0x68cbb8
    void SetPicCheck();                          // реф. @0x68cab0: репозиция детей

signals:
    void clicked();                              // реф. @0x82a920
    void SetInstrumentChannel(double v);         // реф. сигналы → слоты KScopeValue
    void SetDistalEnd(double v);
    void SetBendingSection(double v);

protected:
    void mouseReleaseEvent(QMouseEvent *) override;   // реф. @0x68cbc0

private:
    KScopeValue *m_pValue = nullptr;   // +0x30
    QLabel *m_pIconLabel = nullptr;    // +0x38
    QLabel *m_pModelLabel = nullptr;   // +0x40
    bool m_bShowSpare = true;          // +0x48
    KEncStyle m_encStyle;
};
