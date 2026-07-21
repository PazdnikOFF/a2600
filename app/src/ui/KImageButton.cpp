#include "KImageButton.h"

KImageButton::KImageButton(QWidget *parent)
    : KImageButton(QString(), QString(), QString(), QString(), parent)   // реф. @0x3e9218 делегирует
{
}

KImageButton::KImageButton(const QString &normal, const QString &hover, const QString &selected,
                           const QString &invalid, QWidget *parent)
    : QPushButton(parent)
    , m_normal(normal)
    , m_hover(hover)
    , m_selected(selected)
    , m_invalid(invalid)
{
    // Реф. 4-QString ctor @0x3e9050: дефолт 211×46 через SetWidth/SetHeight (+UpdateQSS).
    setFixedSize(m_width, m_height);
    UpdateQSS();
}

void KImageButton::SetImage(const QString &normal, const QString &hover,
                            const QString &selected, const QString &invalid)
{
    m_normal = normal; m_hover = hover; m_selected = selected; m_invalid = invalid;
    UpdateQSS();
}

QString KImageButton::imgUrl(const QString &name) const
{
    if (name.isEmpty())
        return QString();
    return BasePath() + QStringLiteral("/") + name;
}

void KImageButton::UpdateQSS()
{
    // Реф. UpdateQSS: strech → border-image (масштаб), иначе → background-image center.
    QString textPad;
    if (m_textX >= 0)
        textPad += QStringLiteral("text-align:left;padding-left:%1px;min-width:%2px;")
                       .arg(m_textX).arg(m_width - m_textX);
    if (m_textY >= 0)
        textPad += QStringLiteral("padding-top:%1px;").arg(m_textY);

    QString qss;
    if (m_strech) {
        qss = QStringLiteral(
                  "KImageButton{color:#ffffff;border:0px;border-image:url(%1);%5}"
                  "KImageButton:hover{color:#ffffff;border:0px;border-image:url(%2);%5}"
                  "KImageButton:pressed{color:#00badb;border:0px;border-image:url(%3);%5}"
                  "KImageButton:disabled{color:#4f4f4f;border:0px;border-image:url(%4);%5}")
                  .arg(imgUrl(m_normal), imgUrl(m_hover), imgUrl(m_selected), imgUrl(m_invalid), textPad);
    } else {
        const QString bg = QStringLiteral("background-repeat:no-repeat;background-position:center;"
                                          "min-width:%1px;min-height:%2px;").arg(m_width).arg(m_height);
        qss = QStringLiteral(
                  "KImageButton{color:#ffffff;border:0px;background-image:url(%1);%5%6}"
                  "KImageButton:hover{color:#ffffff;background-image:url(%2);%5%6}"
                  "KImageButton:pressed{color:#00badb;background-image:url(%3);%5%6}"
                  "KImageButton:disabled{color:#4f4f4f;background-image:url(%4);%5%6}")
                  .arg(imgUrl(m_normal), imgUrl(m_hover), imgUrl(m_selected), imgUrl(m_invalid), bg, textPad);
    }
    setStyleSheet(qss);
}
