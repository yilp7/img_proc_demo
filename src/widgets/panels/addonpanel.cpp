#include "addonpanel.h"

#include <QEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QWidget>

namespace {
const int kResizeHandleSize = 18;
}

AddonPanel::AddonPanel(QWidget *parent)
    : FloatingPanel(parent),
      m_host(nullptr),
      m_resize_handle(nullptr),
      m_resizing(false),
      m_resize_press_global(),
      m_resize_press_geometry()
{
    setTitle(tr("Addon"));
    setMinimumSize(220, 160);

    m_host = new QWidget(this);
    m_host->setObjectName("ADDON_PLUGIN_HOST");
    m_host->setAttribute(Qt::WA_StyledBackground, true);
    m_host->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    contentLayout()->addWidget(m_host, 1);

    m_resize_handle = new QWidget(this);
    m_resize_handle->setObjectName("ADDON_RESIZE_HANDLE");
    m_resize_handle->setAttribute(Qt::WA_StyledBackground, true);
    m_resize_handle->setFixedSize(kResizeHandleSize, kResizeHandleSize);
    m_resize_handle->setCursor(Qt::SizeFDiagCursor);
    m_resize_handle->installEventFilter(this);
    m_resize_handle->raise();
}

void AddonPanel::fitPluginChildren()
{
    if (!m_host) return;
    const QList<QWidget*> children =
        m_host->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    for (QWidget *child : children) {
        if (!child) continue;
        child->setGeometry(m_host->rect());
    }
}

bool AddonPanel::eventFilter(QObject *obj, QEvent *event)
{
    if (obj != m_resize_handle) return FloatingPanel::eventFilter(obj, event);

    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouse = static_cast<QMouseEvent*>(event);
        if (mouse->button() == Qt::LeftButton) {
            raise();
            m_resizing = true;
            m_resize_press_global = mouse->globalPos();
            m_resize_press_geometry = geometry();
            if (m_resize_handle) m_resize_handle->grabMouse();
            emit resizedByUser();
            return true;
        }
    }

    if (event->type() == QEvent::MouseMove && m_resizing) {
        QMouseEvent *mouse = static_cast<QMouseEvent*>(event);
        const QPoint delta = mouse->globalPos() - m_resize_press_global;
        const QSize min_size = minimumSize();
        int max_w = qMax(min_size.width(), m_resize_press_geometry.width() + delta.x());
        int max_h = qMax(min_size.height(), m_resize_press_geometry.height() + delta.y());

        if (parentWidget()) {
            max_w = qMax(min_size.width(),
                         parentWidget()->width() - m_resize_press_geometry.x());
            max_h = qMax(min_size.height(),
                         parentWidget()->height() - m_resize_press_geometry.y());
        }

        const int next_w = qBound(min_size.width(),
                                  m_resize_press_geometry.width() + delta.x(), max_w);
        const int next_h = qBound(min_size.height(),
                                  m_resize_press_geometry.height() + delta.y(), max_h);
        if (size() != QSize(next_w, next_h)) {
            resize(next_w, next_h);
            emit resizedByUser();
        }
        return true;
    }

    if (event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *mouse = static_cast<QMouseEvent*>(event);
        if (m_resizing && mouse->button() == Qt::LeftButton) {
            m_resizing = false;
            if (m_resize_handle) m_resize_handle->releaseMouse();
            emit resizedByUser();
            return true;
        }
    }

    return FloatingPanel::eventFilter(obj, event);
}

void AddonPanel::resizeEvent(QResizeEvent *event)
{
    FloatingPanel::resizeEvent(event);
    if (m_resize_handle) {
        m_resize_handle->move(qMax(0, width() - kResizeHandleSize),
                              qMax(0, height() - kResizeHandleSize));
        m_resize_handle->raise();
    }
    fitPluginChildren();
}
