#include "floatingpanel.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
const int kHeaderHeight = 28;
const int kButtonSize   = 20;
}

FloatingPanel::FloatingPanel(QWidget *parent) :
    QWidget(parent),
    m_header(nullptr),
    m_title_label(nullptr),
    m_pin_button(nullptr),
    m_hide_button(nullptr),
    m_root_layout(nullptr),
    m_content_layout(nullptr),
    m_pinned(false),
    m_dragging(false),
    m_press_offset()
{
    setObjectName("FloatingPanel");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumSize(160, 80);

    m_header = new QWidget(this);
    m_header->setObjectName("FloatingPanelHeader");
    m_header->setFixedHeight(kHeaderHeight);
    m_header->setAttribute(Qt::WA_StyledBackground, true);
    m_header->setCursor(Qt::OpenHandCursor);

    m_title_label = new QLabel(m_header);
    m_title_label->setObjectName("FloatingPanelTitle");

    m_pin_button = new QPushButton(QStringLiteral("pin"), m_header);
    m_pin_button->setObjectName("FloatingPanelPin");
    m_pin_button->setCheckable(true);
    m_pin_button->setFixedSize(kButtonSize * 2, kButtonSize);
    m_pin_button->setFocusPolicy(Qt::NoFocus);

    m_hide_button = new QPushButton(QStringLiteral("X"), m_header);
    m_hide_button->setObjectName("FloatingPanelHide");
    m_hide_button->setFixedSize(kButtonSize, kButtonSize);
    m_hide_button->setFocusPolicy(Qt::NoFocus);

    QHBoxLayout *header_layout = new QHBoxLayout(m_header);
    header_layout->setContentsMargins(6, 2, 2, 2);
    header_layout->setSpacing(4);
    header_layout->addWidget(m_title_label, 1);
    header_layout->addWidget(m_pin_button);
    header_layout->addWidget(m_hide_button);

    m_content_layout = new QVBoxLayout();
    m_content_layout->setContentsMargins(4, 4, 4, 4);

    m_root_layout = new QVBoxLayout(this);
    m_root_layout->setContentsMargins(0, 0, 0, 0);
    m_root_layout->setSpacing(0);
    m_root_layout->addWidget(m_header);
    m_root_layout->addLayout(m_content_layout, 1);

    connect(m_pin_button,  &QPushButton::clicked, this, &FloatingPanel::onPinClicked);
    connect(m_hide_button, &QPushButton::clicked, this, &FloatingPanel::onHideClicked);
}

void FloatingPanel::setTitle(const QString &title)
{
    m_title_label->setText(title);
    setWindowTitle(title);
}

QString FloatingPanel::title() const
{
    return m_title_label->text();
}

void FloatingPanel::setPinned(bool pinned)
{
    if (m_pinned == pinned) return;
    m_pinned = pinned;
    m_pin_button->setChecked(pinned);
    m_header->setCursor(pinned ? Qt::ArrowCursor : Qt::OpenHandCursor);
    if (pinned && m_dragging) m_dragging = false;
    emit pinToggled(pinned);
}

QPoint FloatingPanel::clampToParent(const QPoint &desiredTopLeft,
                                    const QSize &panelSize,
                                    const QSize &parentSize)
{
    // Parent smaller than panel on an axis: clamp to 0 on that axis (no
    // negative coords; caller's layout decides how to handle oversized
    // panels, which should not occur in practice).
    const int max_x = qMax(0, parentSize.width()  - panelSize.width());
    const int max_y = qMax(0, parentSize.height() - panelSize.height());
    const int x = qBound(0, desiredTopLeft.x(), max_x);
    const int y = qBound(0, desiredTopLeft.y(), max_y);
    return QPoint(x, y);
}

void FloatingPanel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    raise();

    // Drag only when the press is on the header band and not on a button.
    // Use the fixed header height rather than m_header->geometry(), so the
    // hit-test works before the layout has been activated.
    if (!m_pinned && event->pos().y() >= 0 && event->pos().y() < kHeaderHeight) {
        const QWidget *child = m_header->childAt(m_header->mapFrom(this, event->pos()));
        const bool on_button = (child == m_pin_button || child == m_hide_button);
        if (!on_button) {
            m_dragging = true;
            m_press_offset = event->pos();
            m_header->setCursor(Qt::ClosedHandCursor);
        }
    }

    QWidget::mousePressEvent(event);
}

void FloatingPanel::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && parentWidget()) {
        const QPoint global = event->globalPos();
        const QPoint parent_local = parentWidget()->mapFromGlobal(global);
        const QPoint desired = parent_local - m_press_offset;
        const QPoint clamped = clampToParent(desired, size(), parentWidget()->size());
        if (pos() != clamped) {
            move(clamped);
            emit dragged(clamped);
        }
    }

    QWidget::mouseMoveEvent(event);
}

void FloatingPanel::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_dragging && event->button() == Qt::LeftButton) {
        m_dragging = false;
        m_header->setCursor(m_pinned ? Qt::ArrowCursor : Qt::OpenHandCursor);
    }
    QWidget::mouseReleaseEvent(event);
}

void FloatingPanel::onPinClicked()
{
    setPinned(m_pin_button->isChecked());
}

void FloatingPanel::onHideClicked()
{
    emit hideRequested();
    hide();
}
