#include "KScopeStaus.h"
#include "endo/KEndoScope.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QPixmap>

// ── KScopeValue (ребёнок) ────────────────────────────────────────────────────

KScopeValue::KScopeValue(QWidget *parent)
    : QWidget(parent)
{
    // Реф. ctor @0x68d578: ~5 QLabel, AlignCenter, «font-size: 12px;».
    setObjectName(QStringLiteral("KScopeValue"));
    QVBoxLayout *v = new QVBoxLayout(this);
    v->setContentsMargins(2, 2, 2, 2);
    v->setSpacing(2);
    auto mk = [&](const char *name) {
        QLabel *l = new QLabel(this);
        l->setObjectName(QString::fromLatin1(name));
        l->setAlignment(Qt::AlignCenter);
        l->setStyleSheet(QStringLiteral("font-size: 12px;"));
        v->addWidget(l);
        return l;
    };
    m_model = mk("label_model");
    m_sn = mk("label_sn");
    m_channel = mk("label_channel");
    m_distal = mk("label_distal");
    m_bending = mk("label_bending");
}

void KScopeValue::SetModel(const QString &model)
{
    // Реф. @SetModel: спец-кейсы ENL-110/ENL-X20 (в реф. влияют на раскладку); показываем модель.
    m_model->setText(model);
}

void KScopeValue::SetInstrumentSN(const QString &sn)
{
    m_sn->setText(sn);
}

void KScopeValue::SetInstrumentChannel(double v)
{
    // Подпись-префикс — инференс (точный caption-литерал не извлекался).
    m_channel->setText(QStringLiteral("Ch %1").arg(v, 0, 'f', 1));
}

void KScopeValue::SetDistalEnd(double v)
{
    m_distal->setText(QStringLiteral("DE %1").arg(v, 0, 'f', 1));
}

void KScopeValue::SetBendingSection(double v)
{
    m_bending->setText(QStringLiteral("BS %1").arg(v, 0, 'f', 1));
}

// ── KScopeStaus (панель) ─────────────────────────────────────────────────────

KScopeStaus::KScopeStaus(QWidget *parent)
    : QFrame(parent)
{
    // Реф. ctor @0x68cf70: 3 стек-ребёнка, adjustSize, SetPicCheck, чтение сцена (device —
    // опущено), связка 3 сигналов → слоты KScopeValue.
    setObjectName(QStringLiteral("KScopeStaus"));

    m_pValue = new KScopeValue(this);
    m_pValue->resize(105, 120);

    m_pIconLabel = new QLabel(this);
    m_pIconLabel->setObjectName(QStringLiteral("label_icon"));
    m_pIconLabel->resize(105, 88);
    m_pIconLabel->setAlignment(Qt::AlignCenter);

    m_pModelLabel = new QLabel(this);
    m_pModelLabel->setObjectName(QStringLiteral("label_model"));
    m_pModelLabel->resize(105, 40);
    m_pModelLabel->setGeometry(0, 215, 105, 40);
    m_pModelLabel->setWordWrap(true);
    m_pModelLabel->setAlignment(Qt::AlignCenter);
    m_pModelLabel->setStyleSheet(QStringLiteral("font-size:17px;color: rgb(0, 153, 153);"));
    m_pModelLabel->hide();   // реф.: скрыт до подключения

    resize(105, 260);
    SetPicCheck();

    // Реф.: сигналы панели пробрасываются в одноимённые слоты KScopeValue.
    connect(this, &KScopeStaus::SetInstrumentChannel, m_pValue, &KScopeValue::SetInstrumentChannel);
    connect(this, &KScopeStaus::SetDistalEnd, m_pValue, &KScopeValue::SetDistalEnd);
    connect(this, &KScopeStaus::SetBendingSection, m_pValue, &KScopeValue::SetBendingSection);
}

void KScopeStaus::SetPicCheck()
{
    // Реф. @0x68cab0: репозиция трёх детей (стек сверху вниз).
    m_pIconLabel->move(0, 0);
    m_pValue->move(0, 92);
    m_pModelLabel->move(0, 215);
}

void KScopeStaus::SetScopeModel(const QString &model)
{
    // Реф. @0x68cc00: путь биопсии + пиксмап (или очистка, если у модели есть канал) +
    // display-модель в KScopeValue.
    if (model.isEmpty()) {
        m_pIconLabel->clear();
        m_pModelLabel->clear();
        return;
    }
    const QString path = m_encStyle.getBiopsyImg(model);
    if (KEndoScope::IsEndoHasChannel(model)) {
        m_pIconLabel->setPixmap(QPixmap());   // есть канал — иконку очищаем
    } else {
        QPixmap pix(path);
        if (!pix.isNull())
            m_pIconLabel->setPixmap(pix.scaled(m_pIconLabel->size(),
                                               Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    const QString disp = m_encStyle.GetEndoDisplayModel(model);
    m_pValue->SetModel(disp);
    m_pModelLabel->setText(disp);
    m_pModelLabel->show();
}

void KScopeStaus::SetScopeSN(const QString &sn)
{
    m_pValue->SetInstrumentSN(sn);   // реф. @0x68cb08
}

void KScopeStaus::SetScopeConnect(bool connected)
{
    // Реф. @0x68d258: true → device GetEndoInfo (опущено); false → очистка+скрыть.
    if (connected) {
        m_pValue->show();
        m_pModelLabel->show();
    } else {
        SetScopeModel(QString());
        m_pValue->hide();
        m_pModelLabel->hide();
    }
}

void KScopeStaus::mouseReleaseEvent(QMouseEvent *e)
{
    // Реф. @0x68cbc0: клик по панели → clicked().
    QFrame::mouseReleaseEvent(e);
    if (rect().contains(e->pos()))
        emit clicked();
}
