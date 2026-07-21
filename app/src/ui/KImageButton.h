#pragma once

#include <QPushButton>
#include <QString>

// Картиночная кнопка на СТИЛЯХ (реф. KImageButton, ctor @0x3e9218 / 4-QString @0x3e9050, base
// QPushButton). UI-порт. ОТЛИЧИЕ от KImgPushButton/KPagePushButton: НЕ paintEvent, а QSS —
// UpdateQSS собирает stylesheet с псевдо-состояниями :hover/:pressed/:disabled и зовёт
// setStyleSheet (Qt рендерит сам). 4 состояния: normal/hover/selected(:pressed)/invalid
// (:disabled). Текст — штатный QPushButton::setText + per-state цвета (#ffffff/#00badb/#4f4f4f).
// Дефолт 211×46. url = BasePath()+"/"+имя; BasePath = CustomBasePath или «02-data/rw-common/
// picture». Сигнал — штатный clicked(). 100% PORT.
class KImageButton : public QPushButton
{
    Q_OBJECT
public:
    explicit KImageButton(QWidget *parent = nullptr);
    KImageButton(const QString &normal, const QString &hover, const QString &selected,
                 const QString &invalid, QWidget *parent = nullptr);

    void SetImage(const QString &normal, const QString &hover,
                  const QString &selected, const QString &invalid);
    void SetNormalImage(const QString &s) { m_normal = s; UpdateQSS(); }
    void SetHoverImage(const QString &s) { m_hover = s; UpdateQSS(); }
    void SetSelectedImage(const QString &s) { m_selected = s; UpdateQSS(); }
    void SetInvalidImage(const QString &s) { m_invalid = s; UpdateQSS(); }
    void SetCustomBasePath(const QString &s) { m_customBase = s; UpdateQSS(); }
    QString CustomBasePath() const { return m_customBase; }
    QString DefaultBasePath() const { return QStringLiteral("02-data/rw-common/picture"); }
    QString BasePath() const { return m_customBase.isEmpty() ? DefaultBasePath() : m_customBase; }

    void SetWidth(int w) { m_width = w; setFixedWidth(w); UpdateQSS(); }
    void SetHeight(int h) { m_height = h; setFixedHeight(h); UpdateQSS(); }
    void SetSize(int w, int h) { m_width = w; m_height = h; setFixedSize(w, h); UpdateQSS(); }
    void SetStrech(bool s) { m_strech = s; UpdateQSS(); }
    void SetTextXPos(int x) { m_textX = x; UpdateQSS(); }
    void SetTextYPos(int y) { m_textY = y; UpdateQSS(); }

private:
    void UpdateQSS();
    QString imgUrl(const QString &name) const;

    bool m_strech = true;   // +0x30
    int m_width = 211;      // +0x34
    int m_height = 46;      // +0x38
    int m_textX = -1;       // +0x3c
    int m_textY = -1;       // +0x40
    QString m_normal, m_hover, m_selected, m_invalid, m_customBase;
};
