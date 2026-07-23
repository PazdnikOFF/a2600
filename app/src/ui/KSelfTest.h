#pragma once

#include "ui/KDialog.h"

#include <QStringList>

#include <functional>

class QGridLayout;
class QListWidget;
class QPushButton;

// Экран самодиагностики прибора (реф. KSelfTest @0x715240, X-2600; sizeof 0x60).
// Точка входа — свободная OpenKSelfTestDlg() @0x7157c8.
//
// Диалог НЕ интерактивен: весь отчёт собирается ОДИН раз в конструкторе
// (startCheck() → три проверки → displayInfo()), список строк показывается в
// QListWidget, единственная кнопка закрывает окно. Каждая проверка ДОБАВЛЯЕТ
// строку-ключ перевода ТОЛЬКО при проблеме — пустой список между заголовком и
// «завершено» означает «всё в порядке».
class KSelfTest : public KDialog
{
    Q_OBJECT
public:
    explicit KSelfTest(QWidget *parent = nullptr);

    void startCheck();       // реф. @0x7150f8
    void checkEndo();        // реф. @0x714690
    void checkLight();       // реф. @0x714c38
    void checkProcessor();   // реф. @0x714d00

    // Собранный отчёт (реф. поле +0x58).
    QStringList InfoList() const { return m_infoList; }

    // Не из реф.: ЕДИНСТВЕННЫЙ device-шов класса — счётчики наработки лампы
    // (в реф. `GetMainCtrlThread()->GetLampUsedTime()/GetLampTotalUsedTime()`).
    // Возвращает пару {наработка, суммарная наработка}; по умолчанию {0, 0}.
    static void SetLampTimeProvider(std::function<QPair<int, int>()> f);

public slots:
    void displayInfo();      // реф. @0x7145b8
    // ⚠️ Реф. @0x714600 — тело ровно `b displayInfo`, т.е. ПОЛНЫЙ синоним.
    // Ни одного подписчика/таймера в бинарнике не нашлось.
    void timerToUpdate() { displayInfo(); }

private:
    // Реф. поле +0x50 — указатель на Ui-структуру из 24 байт (три виджета).
    QGridLayout *m_pGridLayout = nullptr;
    QPushButton *m_pBtnOK = nullptr;
    QListWidget *m_pListWidget = nullptr;
    QStringList  m_infoList;                 // +0x58
};

// Реф. свободная функция @0x7157c8: LogPrintf, new на куче, exec(), delete.
void OpenKSelfTestDlg();
