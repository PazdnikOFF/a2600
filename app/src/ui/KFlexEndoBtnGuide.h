#pragma once

#include <QWidget>

#include <map>
#include <utility>

class QLabel;

// Оверлей-гайд кнопок ГИБКОГО эндоскопа (реф. KFlexEndoBtnGuide : QWidget, ctor @0x69ed20).
// UI-порт. В отличие от уже портированного жёсткого аналога KRigidEndoBtnGuide, у этого
// класса НЕТ Ui_-структуры Designer'а: все виджеты строятся кодом (InitWidgets @0x69ece0 →
// 4 × InitItem @0x69e860), а фонового изображения нет вовсе.
//
// Строка i (i = 0..3) = кнопка ПУЛЬТА ДУ i+1: иконка 28×28 в (0, 36·i) и подпись 150×28
// в (36, 36·i). Естественная канва контента — 186×136. Реф. хранит пары виджетов в
// unordered_map<int, pair<QLabel*,QLabel*>> (поле +0x30, единственное); в порте — std::map
// (порядок обхода 0→3 нужен нам для теста, семантика доступа та же).
//
// Геометрия САМОГО виджета в реф. задаётся не здесь, а владельцем KViewSoftEndo::InitUiConfig
// @0x46d7f0 из KDisplayOption::GetSoftEndoViewConf() (+ setStyleSheet("font-size:16px;"),
// objectName "widget_endobtnguide") — конфигурируемо, поэтому в порте берём канву 186×136.
//
// DEVICE-seam: ID функций кнопок читаются из user.ini [RemoteSwitch]Switch1..4 (реф. поля
// _KUserConf 0x00..0x0c, у нас — уже существующий KRemoteSwitchConfig), имя функции —
// KUserSet::GetFunctionName (портирована 1:1). Show() опирается на KEndoScope::IsEndoReady()
// и модель эндоскопа — на десктопе эндоскоп не готов, поэтому Show(true) корректно прячет.
class KFlexEndoBtnGuide : public QWidget
{
    Q_OBJECT
public:
    explicit KFlexEndoBtnGuide(QWidget *parent = nullptr);

    // Реф. @0x69e5b0: !bShow либо эндоскоп не готов → hide(); иначе show() и строки 0 и 3
    // видимы ТОЛЬКО если модель != "EUD.100S" (у неё две кнопки вместо четырёх).
    void Show(bool bShow);

    // Пара виджетов строки (для проверок; в реф. — просто доступ к map).
    std::pair<QLabel *, QLabel *> Item(int i) const;

public slots:
    // Реф. @0x69e6e8 (единственный слот класса, подключён к KUiMsgProxy::UpdateFlexEndoBtnGuide):
    // перечитать имена функций 4 кнопок и обновить ТОЛЬКО подписи (иконки не трогаются).
    void onSetEndoBtnFuncText();

private:
    void InitWidgets();        // реф. @0x69ece0: 4 × InitItem(i), затем onSetEndoBtnFuncText
    void InitItem(int index);  // реф. @0x69e860

    std::map<int, std::pair<QLabel *, QLabel *>> m_items;   // реф. поле +0x30
};
