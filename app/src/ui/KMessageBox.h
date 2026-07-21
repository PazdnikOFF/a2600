#pragma once

#include <QDialog>
#include <QMessageBox>

class QLabel;
class QToolButton;
class QVBoxLayout;

// Окно сообщения с кастомным хромом (реф. KMessageBox : QDialog, X-2600). UI-порт.
// НЕ наследует KDialog (в отличие от прочих диалогов) — свой безрамочный титул-бар
// (umessage_frame_upbar: label_dlgtitle + btn_close) поверх ВСТРОЕННОГО настоящего QMessageBox
// (иконка/текст/кнопки рисует он). Кнопки перестилизованы: TR-текст (Yes/No/OK/Ccl) + тёмный
// градиент 100×25 radius 7. Статические хелперы information/question/warning/confirm.
//
// Реф. раскладка: +0x30 QMessageBox* m_msgbox, +0x38 Ui*, +0x40 int m_result, +0x44 defBtn.
// Иконки close.png/warning.png — device-ассеты (KDisplayOption::GetIcon); у нас текст ✕ и
// встроенная QMessageBox-иконка (помечено).
class KMessageBox : public QDialog
{
    Q_OBJECT
public:
    explicit KMessageBox(QWidget *parent = nullptr);
    KMessageBox(QMessageBox::Icon icon, const QString &title, const QString &text,
                QMessageBox::StandardButtons buttons,
                QMessageBox::StandardButton defBtn = QMessageBox::NoButton,
                QWidget *parent = nullptr);

    int exec() override;                       // реф. @0x689c58: m_result если ≠0, иначе QDialog::exec
    void SetText(const QString &text);         // форвард к m_msgbox
    void SetIconPixmap(const QPixmap &pm);     // форвард к m_msgbox

    // Реф. статические хелперы (@0x689e58/0x68a008/0x689cb0/0x68a1b8) — единый паттерн:
    // new KMessageBox(Information, …) → setIconPixmap(warning.png) → exec → delete → код кнопки.
    static int information(QWidget *parent, const QString &title, const QString &text,
                           QMessageBox::StandardButtons buttons,
                           QMessageBox::StandardButton defBtn = QMessageBox::NoButton);
    static int question(QWidget *parent, const QString &title, const QString &text,
                        QMessageBox::StandardButtons buttons,
                        QMessageBox::StandardButton defBtn = QMessageBox::NoButton);
    static int warning(QWidget *parent, const QString &title, const QString &text,
                       QMessageBox::StandardButtons buttons,
                       QMessageBox::StandardButton defBtn = QMessageBox::NoButton);
    static int confirm(QWidget *parent, const QString &title, const QString &text,
                       QMessageBox::StandardButtons buttons,
                       QMessageBox::StandardButton defBtn = QMessageBox::NoButton);

private slots:
    void clickDlgBtn(int code);   // реф. @0x688ca8: m_result=code; close()

private:
    void setupUi();       // реф. Ui_KMessageBox::setupUi @0x68a3f8 (хром)
    void drawDialog();    // реф. @0x688cb0: встроить m_msgbox + перестилить кнопки
    void newMessageBox(QMessageBox::Icon icon, const QString &title, const QString &text,
                       QMessageBox::StandardButtons buttons,
                       QMessageBox::StandardButton defBtn);

    QMessageBox *m_msgbox = nullptr;
    QLabel      *label_dlgtitle = nullptr;
    QToolButton *btn_close = nullptr;
    QVBoxLayout *vLayout_box = nullptr;
    int m_result = 0;
    int m_defaultButton = 0;
};
