#pragma once

#include "ui/KDialog.h"

class QLabel;
class QComboBox;
class QTextEdit;
class QPushButton;

// Диалог импорта правил logcheck с USB (реф. KImportRules : KDialog, ctor @0x8188c8).
// UI-порт. Ui_KImportRules отдельного символа НЕТ — setupUi заинлайнен в ctor; АБСОЛЮТНАЯ
// геометрия (без layout-менеджеров), setStyleSheet нигде НЕ вызывается (цвета — из глоб. qss).
// Логика чтения/записи файлов правил с USB (GetRulesFile/ReadFileToTextEdit/WriteTextEditToFile,
// KUsbDevice) — DEVICE, в порт не тянется (заглушки-слоты); порт — только разметка.
class KImportRules : public KDialog
{
    Q_OBJECT
public:
    explicit KImportRules(QWidget *parent = nullptr);

private slots:
    void ReadFileToTextEdit(const QString &name);   // реф.: файл правил с USB → textEdit (device)
    void OnClickImport();                            // реф.: textEdit → rulesfile (device)

private:
    void setupUi();

    QLabel      *label = nullptr;        // подсказка (китайский текст)
    QComboBox   *cb_rules = nullptr;     // список файлов правил (заполняется с USB)
    QTextEdit   *textEdit = nullptr;     // содержимое выбранного правила
    QPushButton *btn_import = nullptr;   // "Import"
};
