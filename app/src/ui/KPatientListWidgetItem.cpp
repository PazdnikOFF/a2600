#include "KPatientListWidgetItem.h"

#include <QEvent>
#include <QFontMetrics>
#include <QLabel>
#include <QPixmap>
#include <QToolTip>

// ------------------------------------------------------------------ KHoverEventFilter ---

KHoverEventFilter::KHoverEventFilter(QWidget *w, const QString &fullText, int offset)
    : QObject(w)
    , m_widget(w)
    , m_text(fullText)
    , m_offset(offset)
{
}

bool KHoverEventFilter::eventFilter(QObject *obj, QEvent *e)
{
    // Реф. @0x833678: подсказка с ПОЛНЫМ текстом рядом с виджетом.
    if (e->type() == QEvent::ToolTip && m_widget) {
        const QPoint base = m_widget->mapToGlobal(QPoint(0, 0));
        QToolTip::showText(base + QPoint(-10, -2 * m_widget->height() + m_offset), m_text);
        return true;
    }
    // Реф.: ветки Enter/Leave живут под условием m_offset <= 39 — при 150 недостижимы.
    return QObject::eventFilter(obj, e);
}

// ------------------------------------------------------------ KPatientListWidgetItem ---

KPatientListWidgetItem::KPatientListWidgetItem(const QMap<QString, QString> &icons,
                                               const QString &text, QWidget *parent)
    : QFrame(parent)
{
    // Реф. ctor @0x79e9c0 (setupUi заинлайнен).
    if (objectName().isEmpty())
        setObjectName(QStringLiteral("KPatientListWidgetItem"));
    resize(211, 80);

    m_labelImg = new QLabel(this);
    m_labelImg->setObjectName(QStringLiteral("label_img"));
    m_labelImg->setGeometry(0, 0, 211, 80);     // реф. QRect(x1=0,y1=0,x2=210,y2=79)
    m_labelImg->setAlignment(Qt::AlignCenter);

    m_labelText = new QLabel(this);
    m_labelText->setObjectName(QStringLiteral("label_text"));
    m_labelText->setGeometry(23, 50, 165, 28);  // реф. QRect(x1=23,y1=50,x2=187,y2=77)

    m_labelImg->setText(tr("img"));             // реф. retranslateUi — плейсхолдер
    m_labelText->setText(QString());

    // Реф. порядок чтения ключей карты.
    m_selectIcon  = icons.value(QStringLiteral("selectIcon"));
    m_normalIcon  = icons.value(QStringLiteral("normalIcon"));
    m_hoverIcon   = icons.value(QStringLiteral("hoverIcon"));
    m_disableIcon = icons.value(QStringLiteral("disableIcon"));

    // Реф.: размер фрейма берётся из ПИКСМАПА обычного состояния, а не из resize выше.
    const QPixmap pm(m_normalIcon);
    setFixedSize(pm.width(), pm.height());
    m_labelImg->setPixmap(pm);
    m_labelText->setAlignment(Qt::AlignCenter);
    m_labelText->setText(text);
}

void KPatientListWidgetItem::SetFontSize(int px)
{
    // Реф. @0x79e8a0: строка-шаблон @0x8a17f8 (len 24).
    m_labelText->setStyleSheet(QStringLiteral("QLabel{font-size: %1px;}").arg(px));
}

void KPatientListWidgetItem::Select()   { m_labelImg->setPixmap(QPixmap(m_selectIcon)); }
void KPatientListWidgetItem::UnSelect() { m_labelImg->setPixmap(QPixmap(m_normalIcon)); }
void KPatientListWidgetItem::Hover()    { m_labelImg->setPixmap(QPixmap(m_hoverIcon)); }
void KPatientListWidgetItem::Disable()  { m_labelImg->setPixmap(QPixmap(m_disableIcon)); }

// ------------------------------------------------------------------- setElidedFrame ---

void setElidedFrame(KPatientListWidgetItem *item, int n, const QString &text,
                    const QString &suffix)
{
    // Реф. @0x7a3468. Аргумент n не используется (реф. зовёт с 6) — сохранён ради сигнатуры.
    Q_UNUSED(n);
    if (!item)
        return;
    QLabel *lbl = item->getLabel();
    if (!lbl)
        return;
    const QFontMetrics fm(lbl->font());
    const int textWidth = fm.horizontalAdvance(lbl->text());
    if (item->width() - 10 > textWidth)
        return;   // помещается — ни фильтра, ни обрезки

    item->installEventFilter(new KHoverEventFilter(item, text));
    const int w = lbl->width() - fm.horizontalAdvance(suffix) + 17;   // реф. +17
    lbl->setText(fm.elidedText(text, Qt::ElideRight, w) + suffix);
}
