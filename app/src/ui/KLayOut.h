#pragma once

#include <QWidget>

class QFrame;
class QLabel;
class QToolButton;

// Переиспользуемая «оконная» рамка с заголовком (реф. KLayOut @ctor 0x68bd20, base QWidget,
// setupUi @0x68c048). ВНИМАНИЕ: НЕ селектор раскладки экрана (имя обманчиво) — это chrome
// плавающей панели: styled-фон (QFrame#KFBackground) с верхней титул-полосой (QFrame#KFUpbar:
// label_title + btn_close QToolButton) над пустым спейсер-телом. KFUpbar/KFBackground — НЕ
// классы, просто objectName-стилизованные QFrame. 386×300. btn_close скрыт по умолчанию,
// иконка 2-состояний close.png/close_check.png. Сигнал clickBtnClose() (chained из clicked).
class KLayOut : public QWidget
{
    Q_OBJECT
public:
    explicit KLayOut(QWidget *parent = nullptr);

    void setTitle(const QString &title);   // реф. слот → label_title
    void setNoFrame();                     // реф.: KFBackground border 0px
    void setNoCloseBtn(bool noBtn);        // реф.: скрыть/показать btn_close

signals:
    void clickBtnClose();   // реф. — chained из btn_close.clicked

private:
    void setupUi();

    QFrame *m_background = nullptr;   // #KFBackground
    QFrame *m_upbar = nullptr;        // #KFUpbar
    QLabel *m_labelTitle = nullptr;
    QToolButton *m_btnClose = nullptr;
};
