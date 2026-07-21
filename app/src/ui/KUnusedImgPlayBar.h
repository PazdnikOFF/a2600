#pragma once

#include <QWidget>

// Пейджер-бар браузера неиспользованных изображений (реф. KUnusedImgPlayBar : QWidget —
// НЕ диалог!, ctor @0x4ef598, Ui_KUnusedImgPlayBar::setupUi @0x4efa18). UI-порт. Встраиваемый
// QWidget-строка пагинации (реф. 1614×52); порт как самостоятельный QWidget. gridLayout 1
// ряд 8 колонок: label_total_num(TR_TotalNum) + btn_head/btn_pre(48×32) + edit_page(48×32,
// numeric-validator) + label_total_page(«/1») + btn_next/btn_tail + спейсер. Кастом
// KImgPushButton×4→QPushButton(глиф). Наружу сигнал PageChanged.
//
// DEVICE в порт не тянется: RefreshPageNums/CheckFirstOrLastPage (KUnusedImgModel — числа/
// enable-состояние), ClickBtn*/OneditingFinished (навигация модели) — заглушки.
class KUnusedImgPlayBar : public QWidget
{
    Q_OBJECT
public:
    explicit KUnusedImgPlayBar(QWidget *parent = nullptr);

private:
    void setupUi();
};
