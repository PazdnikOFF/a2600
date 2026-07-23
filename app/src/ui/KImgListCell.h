#pragma once

#include <QFrame>

class QVBoxLayout;
class QLabel;

// Ячейка-превью снимка (реф. KImgListCell : QFrame, ctor @0x6820f8, sizeof 0x38). UI-порт.
// Композит, а НЕ QLabel-наследник, хотя API повторяет QLabel: внутри QVBoxLayout (spacing 0,
// поля 0) с двумя метками — label_img (картинка, SizePolicy Preferred/Expanding, AlignCenter)
// и label_txt (подпись, AlignCenter). ⚠️ label_txt в ctor СКРЫТ и показывается только из
// setText(); в прошивке setText не вызывается нигде ⇒ фактически всегда скрыт.
//
// Реф. ctor: resize(400,300), frameShape StyledPanel (6), frameShadow Raised (0x20),
// objectName "KImgListCell".
//
// ⚠️ КВИРК setMinimumSize/setMaximumSize: если подпись ВИДИМА, картинке ставится высота
// h - 20 (резерв 20 px под текст, `sub w2,w2,#0x14`), и только потом размер самому фрейму;
// если подпись скрыта — размер ставится напрямую, без правки картинки.
//
// Создаётся ровно в Ui_KImgList::setupUi @0x681018 — ЧЕТЫРЕ экземпляра (KlList_img0..3)
// под таблицей снимков.
class KImgListCell : public QFrame
{
    Q_OBJECT
public:
    explicit KImgListCell(QWidget *parent = nullptr);

    void clear();                                   // реф. @0x682680: img->clear(), txt->setText("")
    void setPixmap(const QPixmap &pm);              // реф. @0x682730
    void setText(const QString &text);              // реф. @0x682740: текст + показать подпись
    void setAlignment(Qt::Alignment a);             // реф. @0x682780: обеим меткам
    void setMaximumSize(const QSize &s);            // реф. @0x6827b8
    void setMaximumSize(int w, int h);              // реф. @0x682818
    void setMinimumSize(const QSize &s);            // реф. @0x682890
    void setMinimumSize(int w, int h);              // реф. @0x6828f0

    QLabel *ImgLabel() const { return m_labelImg; }
    QLabel *TextLabel() const { return m_labelTxt; }

private:
    QVBoxLayout *m_layout = nullptr;   // ui+0x00
    QLabel *m_labelImg = nullptr;      // ui+0x08
    QLabel *m_labelTxt = nullptr;      // ui+0x10
};
