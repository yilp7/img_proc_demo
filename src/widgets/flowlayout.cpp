#include "flowlayout.h"

#include <QWidget>

FlowLayout::FlowLayout(QWidget *parent, int margin, int hSpacing, int vSpacing)
    : QLayout(parent), m_h_space(hSpacing), m_v_space(vSpacing)
{
    setContentsMargins(margin, margin, margin, margin);
}

FlowLayout::FlowLayout(int margin, int hSpacing, int vSpacing)
    : m_h_space(hSpacing), m_v_space(vSpacing)
{
    setContentsMargins(margin, margin, margin, margin);
}

FlowLayout::~FlowLayout()
{
    while (QLayoutItem *item = takeAt(0)) delete item;
}

void FlowLayout::addItem(QLayoutItem *item) { m_items.append(item); }

int FlowLayout::horizontalSpacing() const
{
    if (m_h_space >= 0) return m_h_space;
    return const_cast<FlowLayout*>(this)->smartSpacing(QStyle::PM_LayoutHorizontalSpacing);
}

int FlowLayout::verticalSpacing() const
{
    if (m_v_space >= 0) return m_v_space;
    return const_cast<FlowLayout*>(this)->smartSpacing(QStyle::PM_LayoutVerticalSpacing);
}

int FlowLayout::count() const { return m_items.size(); }
QLayoutItem *FlowLayout::itemAt(int index) const { return m_items.value(index); }
QLayoutItem *FlowLayout::takeAt(int index)
{
    if (index >= 0 && index < m_items.size()) return m_items.takeAt(index);
    return nullptr;
}

Qt::Orientations FlowLayout::expandingDirections() const { return {}; }
bool FlowLayout::hasHeightForWidth() const { return true; }
int FlowLayout::heightForWidth(int width) const { return doLayout(QRect(0, 0, width, 0), true); }

void FlowLayout::setGeometry(const QRect &rect)
{
    QLayout::setGeometry(rect);
    doLayout(rect, false);
}

QSize FlowLayout::sizeHint() const
{
    // Natural size = all items laid out in a single row.
    // The actual height when wrapped is delivered via heightForWidth().
    int total_w = 0;
    int max_h = 0;
    for (int i = 0; i < m_items.size(); ++i) {
        const QSize s = m_items[i]->sizeHint();
        if (i > 0) total_w += qMax(0, horizontalSpacing());
        total_w += s.width();
        max_h = qMax(max_h, s.height());
    }
    const QMargins m = contentsMargins();
    return QSize(total_w + m.left() + m.right(), max_h + m.top() + m.bottom());
}

QSize FlowLayout::minimumSize() const
{
    // Minimum = widest single item (so the column can always shrink to one
    // item per row when the container is narrow).
    QSize size;
    for (QLayoutItem *item : m_items) {
        size = size.expandedTo(QSize(item->minimumSize().width(),
                                     item->sizeHint().height()));
    }
    const QMargins m = contentsMargins();
    size += QSize(m.left() + m.right(), m.top() + m.bottom());
    return size;
}

int FlowLayout::doLayout(const QRect &rect, bool testOnly) const
{
    int left, top, right, bottom;
    getContentsMargins(&left, &top, &right, &bottom);
    QRect effective = rect.adjusted(left, top, -right, -bottom);
    int x = effective.x();
    int y = effective.y();
    int line_h = 0;

    for (QLayoutItem *item : m_items) {
        QWidget *w = item->widget();
        int h_spc = horizontalSpacing();
        if (h_spc == -1 && w) h_spc = w->style()->layoutSpacing(QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Horizontal);
        int v_spc = verticalSpacing();
        if (v_spc == -1 && w) v_spc = w->style()->layoutSpacing(QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Vertical);

        int next_x = x + item->sizeHint().width() + h_spc;
        if (next_x - h_spc > effective.right() && line_h > 0) {
            x = effective.x();
            y = y + line_h + v_spc;
            next_x = x + item->sizeHint().width() + h_spc;
            line_h = 0;
        }
        if (!testOnly) item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));
        x = next_x;
        line_h = qMax(line_h, item->sizeHint().height());
    }
    return y + line_h - rect.y() + bottom;
}

int FlowLayout::smartSpacing(QStyle::PixelMetric pm) const
{
    QObject *p = parent();
    if (!p) return -1;
    if (p->isWidgetType()) {
        QWidget *pw = static_cast<QWidget*>(p);
        return pw->style()->pixelMetric(pm, nullptr, pw);
    }
    return static_cast<QLayout*>(p)->spacing();
}
