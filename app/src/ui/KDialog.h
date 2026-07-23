#pragma once

#include <QDialog>

class QFrame;
class QLabel;
class QToolButton;
class QVBoxLayout;

// Базовый класс всех диалогов X-2600 (реф. KDialog : QDialog @0x68ad60). Даёт кастомный
// хром: тёмный фон-рамку (KFBackground), титул-бар высотой 35 (KFUpbar) с заголовком и
// close-кнопкой, безрамочное окно поверх остальных. Подклассы кладут контент в ContentArea().
//
// Реф. хром вынесен в отдельный виджет KLayOut (Ui_KLayOut::setupUi @0x68c048) + отдельный
// overlay-лейбл m_titleLabel поверх. У нас хром встроен прямо в KDialog (проще, визуально
// эквивалентно), заголовок пишется в label_title. objectName-ы (KFBackground/KFUpbar/
// label_title/btn_close) сохранены 1:1 — по ним цепляется прошивочный qss/black/style_kdialog.qss;
// у нас стили заданы значениями из style.qss (KFBackground rgba(26,26,26)+border rgba(83,83,83),
// KFUpbar #0f1218) — не зависим от наличия файла.
//
// _KDLG_STYLE (реф. SetKStyle @0x68b910): 0=как есть; 1=fullscreen/без рамки; 2=позиц. шир.460;
// 3=320; 4=480; 5=640; 6=700; 7=1024 (фикс. ширины окна).
enum _KDLG_STYLE {
    KDLG_DEFAULT = 0, KDLG_FULLSCREEN = 1, KDLG_W460 = 2, KDLG_W320 = 3,
    KDLG_W480 = 4, KDLG_W640 = 5, KDLG_W700 = 6, KDLG_W1024 = 7
};

class KDialog : public QDialog
{
    Q_OBJECT
public:
    // Реф. ctor(QWidget*, bool subscribeStatus). Второй аргумент (подписка на смену
    // системного статуса → авто-закрытие) — device; здесь опущен.
    explicit KDialog(QWidget *parent = nullptr, bool subscribeStatus = false);

    void SetTitle(const QString &title);   // реф. @0x68b408: windowTitle + label_title
    void SetKStyle(_KDLG_STYLE style);     // реф. @0x68b910: пресеты ширины окна
    // Реф. @0x68b768 — ровно `b QDialog::exec` (виртуальный слот vptr+0x1c8). Отдельное имя
    // существует только ради единообразия вызовов из слоя навигации (OpenThesaurusSaveDlg).
    virtual int DoModal() { return exec(); }
    void SetBtnCloseAbort(bool abort) { m_btnCloseAbort = abort; }   // реф. @0x68bac8
    void SetBtnCloseVisible(bool v);

    // Область для контента подкласса (под титул-баром). Реф. подкласс кладёт layout на сам
    // диалог; у нас — в выделенный контент-виджет (визуально то же: контент под титулом).
    QWidget *ContentArea() const { return m_content; }

signals:
    void readyToClose();   // реф. @0x82a080

protected:
    void keyPressEvent(QKeyEvent *event) override;   // реф. @0x68bb60: Esc → close

private:
    void buildChrome();

    _KDLG_STYLE  m_style = KDLG_DEFAULT;   // +0x30
    QFrame      *m_background = nullptr;    // KFBackground
    QFrame      *m_upbar = nullptr;         // KFUpbar (h=35)
    QLabel      *m_titleLabel = nullptr;    // label_title
    QToolButton *m_btnClose = nullptr;      // btn_close
    QWidget     *m_content = nullptr;       // область контента подкласса
    bool         m_btnCloseEnable = true;   // +0x40
    bool         m_btnCloseVisible = true;  // +0x41
    bool         m_btnCloseAbort = false;   // +0x42
};
