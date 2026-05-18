#include "sidedisplaywidget.h"

#include "widgets/display.h"

#include <QLabel>
#include <QVBoxLayout>

namespace {
QLabel* make_section_label(const QString &text, QWidget *parent)
{
    auto *lbl = new QLabel(text, parent);
    lbl->setObjectName("SIDE_DISPLAY_SECTION_LBL");
    lbl->setStyleSheet("font-weight: 700; color: #E0924A; background: transparent;");
    return lbl;
}

QLabel* make_thumb(QWidget *parent)
{
    auto *thumb = new QLabel(parent);
    thumb->setObjectName("SIDE_DISPLAY_THUMB");
    thumb->setMinimumHeight(120);
    thumb->setAlignment(Qt::AlignCenter);
    thumb->setStyleSheet("background-color: #040303; border: 1px solid #382D25;");
    return thumb;
}
} // namespace

SideDisplayWidget::SideDisplayWidget(QWidget *parent)
    : QFrame(parent),
      m_layout(nullptr),
      m_alt1_label(nullptr), m_alt1_thumb(nullptr),
      m_alt2_label(nullptr), m_alt2_thumb(nullptr)
{
    setObjectName("SIDE_DISPLAY_WIDGET");
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(4, 4, 4, 4);
    m_layout->setSpacing(4);

    m_alt1_label = make_section_label("ALT 1", this);
    m_alt1_thumb = make_thumb(this);
    m_alt2_label = make_section_label("ALT 2", this);
    m_alt2_thumb = make_thumb(this);

    m_layout->addWidget(m_alt1_label);
    m_layout->addWidget(m_alt1_thumb, 1);
    m_layout->addWidget(m_alt2_label);
    m_layout->addWidget(m_alt2_thumb, 1);
}

void SideDisplayWidget::setup(Display *alt1, Display *alt2)
{
    auto scaler = [](QLabel *thumb) {
        return [thumb](const QPixmap &pm) {
            if (!thumb || pm.isNull()) return;
            thumb->setPixmap(pm.scaled(thumb->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        };
    };
    if (alt1 && m_alt1_thumb)
        connect(alt1, &Display::set_pixmap, m_alt1_thumb, scaler(m_alt1_thumb), Qt::QueuedConnection);
    if (alt2 && m_alt2_thumb)
        connect(alt2, &Display::set_pixmap, m_alt2_thumb, scaler(m_alt2_thumb), Qt::QueuedConnection);
}
