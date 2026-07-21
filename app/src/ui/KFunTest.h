#pragma once

#include "ui/KDialog.h"

class QPushButton;
class QListView;
class QComboBox;
class QSpinBox;

// Диалог функционального теста (реф. KFunTest : KDialog, ctor @0x836c30). UI-порт.
// Ui_KFunTest::setupUi @0x8379a8 — АБСОЛЮТНАЯ геометрия (Qt Designer), setStyleSheet НЕТ,
// тексты — литеральный английский. Диалог 640×480, SetKStyle(KDLG_W640), титул "Function Test".
// Списки кейсов (QListView + QStringListModel), кнопки перекладки >>/<<, Count/Speed, checklog.
//
// DEVICE/автотест в порт НЕ тянется (только разметка): загрузка /tmp/testcaselist.txt и
// autotest/casefile в модели, движок автотеста (KFuncTask/симулятор/input), слоты Click* —
// заглушки. Модели списков пусты в превью.
class KFunTest : public KDialog
{
    Q_OBJECT
public:
    explicit KFunTest(QWidget *parent = nullptr);

private slots:
    void ClickStart();            // реф.: запуск выбранных тест-кейсов (device)
    void ClickPresureTest();      // реф.: стресс-тест (device)
    void ClickToTest();           // реф.: перенести кейс библиотека→исполнение
    void ClickToCaseLibrary();    // реф.: перенести обратно
    void OpenImportRules();       // реф.: открыть диалог импорта правил (KImportRules)

private:
    void setupUi();
    void initWidget();

    QListView   *table_caselibrary = nullptr;
    QListView   *table_testcase = nullptr;
    QPushButton *btn_start = nullptr;
    QPushButton *btn_presure = nullptr;
    QPushButton *btn_totest = nullptr;
    QPushButton *btn_tolibrary = nullptr;
    QPushButton *btn_import = nullptr;
    QSpinBox    *spinBox_count = nullptr;
    QComboBox   *cmb_speed = nullptr;
    QComboBox   *comboBox_cheklogstatus = nullptr;
};
