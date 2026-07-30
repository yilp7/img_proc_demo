#include "sidedisplaywidget.h"

#include "widgets/display.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
QLabel* make_section_label(const QString &text, QWidget *parent)
{
    auto *lbl = new QLabel(text, parent);
    lbl->setObjectName("SIDE_DISPLAY_SECTION_LBL");
    return lbl;
}

QPushButton* make_swap_button(QWidget *parent)
{
    auto *btn = new QPushButton(parent);
    btn->setObjectName("SIDE_DISPLAY_SWAP_BTN");
    btn->setFixedSize(20, 18);
// Use the same icon as ui->SWITCH_TCU_UI_BTN (set in UserPanel::toggle_color_mode
    // and the init path). Dark theme is the boot default; UserPanel re-applies
    // the correct variant on theme switch.
    btn->setIcon(QIcon(":/tools/dark/switch"));
    btn->setIconSize(QSize(14, 14));
    btn->setToolTip(SideDisplayWidget::tr("Swap with main display"));
    btn->setCursor(Qt::PointingHandCursor);
    btn->setFocusPolicy(Qt::NoFocus);
    return btn;
}

QLabel* make_thumb(QWidget *parent)
{
    auto *thumb = new QLabel(parent);
    thumb->setObjectName("SIDE_DISPLAY_THUMB");
    thumb->setAttribute(Qt::WA_StyledBackground, true);
    thumb->setMinimumHeight(120);
    thumb->setAlignment(Qt::AlignCenter);
    return thumb;
}

QWidget* make_header_row(QLabel *label, QPushButton *swap, QWidget *parent)
{
    auto *row = new QWidget(parent);
    auto *l = new QHBoxLayout(row);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(4);
    l->addWidget(label);
    l->addStretch(1);
    l->addWidget(swap);
    return row;
}
} // namespace

SideDisplayWidget::SideDisplayWidget(QWidget *parent)
    : QFrame(parent),
      m_layout(nullptr),
      m_alt1_label(nullptr), m_alt1_swap(nullptr), m_alt1_thumb(nullptr),
      m_alt2_label(nullptr), m_alt2_swap(nullptr), m_alt2_thumb(nullptr)
{
    setObjectName("SIDE_DISPLAY_WIDGET");
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(4, 4, 4, 4);
    m_layout->setSpacing(4);

    m_alt1_label = make_section_label(tr("ALT 1"), this);
    m_alt1_swap  = make_swap_button(this);
    m_alt1_thumb = make_thumb(this);
    m_alt2_label = make_section_label(tr("ALT 2"), this);
    m_alt2_swap  = make_swap_button(this);
    m_alt2_thumb = make_thumb(this);

    m_layout->addWidget(make_header_row(m_alt1_label, m_alt1_swap, this));
    m_layout->addWidget(m_alt1_thumb, 1);
    m_layout->addWidget(make_header_row(m_alt2_label, m_alt2_swap, this));
    m_layout->addWidget(m_alt2_thumb, 1);

    connect(m_alt1_swap, &QPushButton::clicked,
            this, [this]() { emit swap_requested(1); });
    connect(m_alt2_swap, &QPushButton::clicked,
            this, [this]() { emit swap_requested(2); });
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

void SideDisplayWidget::apply_color_mode(bool light_mode)
{
    const QIcon icon(QStringLiteral(":/tools/%1/switch")
                     .arg(light_mode ? "light" : "dark"));
    if (m_alt1_swap) m_alt1_swap->setIcon(icon);
    if (m_alt2_swap) m_alt2_swap->setIcon(icon);
}
