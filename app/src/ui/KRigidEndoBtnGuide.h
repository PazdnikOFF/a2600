#pragma once

#include <QWidget>

// Оверлей-гайд кнопок жёсткого эндоскопа (реф. KRigidEndoBtnGuide : QWidget — НЕ диалог!,
// ctor @0x69d768, Ui_KRigidEndoBtnGuide::setupUi @0x69d7e8). UI-порт. Встраиваемый
// QWidget-оверлей (реф. 730×513, контент 540×370, абсолютная геометрия — 9 QLabel).
// Порт как самостоятельный QWidget (базовый класс совпал). Центральное фото эндоскопа
// (rigidendo_btn_guide.png) на фоне + подписи функций кнопок A/B (лево/право, короткое/
// долгое нажатие) + M (мультифункция, центр) + легенда сокращений + подсказка выхода.
//
// Кастом-виджетов нет (все 9 — QLabel). Фон-пиксмап — theme::asset (локальная сверка).
//
// DEVICE в порт не тянется: GetEndoBtnFuncText (KUserOsdSet::GetFunctionName/
// GetButtonFunctionId — имена функций кнопок), onSetEndoBtnFuncText (обновление 6 меток),
// SetVisible (глоб. флаг GetSystemStatus) — заглушки; имена функций = фолбэк TR_EEOMenu.
class KRigidEndoBtnGuide : public QWidget
{
    Q_OBJECT
public:
    explicit KRigidEndoBtnGuide(QWidget *parent = nullptr);

private:
    void setupUi();
};
