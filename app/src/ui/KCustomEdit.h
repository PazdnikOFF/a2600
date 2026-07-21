#pragma once

#include "ui/KDialog.h"

class QFrame;
class QPushButton;

// Диалог редактора параметров изображения/видео (реф. KCustomEdit : KDialog, ctor
// @0x633e20, Ui_KCustomEdit::setupUi @0x635630). UI-порт. Модальный, 578×1080,
// SetKStyle(W460), титул TR_PSet. Достижим из KSystemSetDlg (OpenCustomEdit).
// Две страницы (frame_page1 видна, frame_page2 скрыта; btn_nextpage/KParamSetBtn
// переключают — SwitchShowPage):
//   page1: усиление (тип + L1/L2/L3 ImgEnh/ColorEnh комбо) + баланс RGB (radio R/B/C
//          + спины) + зум (L1 read-only 1.0, L2/L3 double 1.1–4.0) + кнопки эндоскопа
//          (0/1/2/3 комбо) + педали (Lt/Rt комбо);
//   page2: прочее — IRIS/шумоподавление/яркостный баланс (комбо) + удаление дымки/HDR
//          (KOptionListButton→QComboBox).
// Кастом-виджеты заменены: KLineH→QFrame(HLine); KOptionListButton→QComboBox;
// KParamSetBtn→панель Default/Save/Exit.
//
// DEVICE в порт не тянется: 17 слотов Change* (image/color enh, RGB, zoom, IRIS,
// denoise, brightEq, dehaze, HDR — видеопроцессор), входящие фиды UpdateVideoParam/
// UpdateButtonConf/SystemStatusChangeAct/EndoScopeStatusChangedAct, LoadVideoParam/
// GetVideoParam/GetKUserSet — заглушки; значения комбо/спинов = дефолт. Exit→close.
class KCustomEdit : public KDialog
{
    Q_OBJECT
public:
    explicit KCustomEdit(QWidget *parent = nullptr);

private:
    void setupUi();

    QFrame *m_page1 = nullptr;
    QFrame *m_page2 = nullptr;
};
