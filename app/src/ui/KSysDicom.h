#pragma once

#include "ui/KDialog.h"

// Диалог настроек DICOM (реф. KSysDicom : KDialog, ctor @0x5c0dd8, Ui_KSysDicom::setupUi
// @0x5c2e18). UI-порт. Большой диалог (реф. setGeometry 1909×1059), НЕ использует SetKStyle/
// SetTitle — визуальный заголовок это внутренний label_7 (TR_DSetting). Три группы:
//   • Basic settings: Station Name / Port / Local AE / Timeout (4 QLineEdit w200);
//   • Storage settings: 2 чекбокса (sync upload, PDF report sync);
//   • Server list: QTableWidget + кнопки Add/Edit/Del/Ping/Verification/Default/Save/Exit.
// setStyleSheet нет (глобальный theme qss). DCMTK-сеть (C-ECHO/Ping/Store SCU), JSON-конфиг
// KDICOMConf, наполнение таблицы серверов — DEVICE, в порт не тянется (заглушки-слоты; Exit→close).
class KSysDicom : public KDialog
{
    Q_OBJECT
public:
    explicit KSysDicom(QWidget *parent = nullptr);

private:
    void setupUi();
};
