#include "panoramapanel.h"

#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPointF>
#include <QPixmap>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QWidget>
#include <QWheelEvent>

namespace {
const int kResizeHandleSize = 18;
}

PanoramaPanel::PanoramaPanel(QWidget *parent)
    : FloatingPanel(parent),
      m_scroll(nullptr),
      m_preview(nullptr),
      m_status(nullptr),
      m_reset_button(nullptr),
      m_resize_handle(nullptr),
      m_interactive(false),
      m_panning(false),
      m_resizing(false),
      m_zoom(1.0),
      m_pan(),
      m_last_pan_pos(),
      m_resize_press_global(),
      m_resize_press_geometry()
{
    setTitle("Panorama");
    setMinimumSize(360, 130);

    QWidget *header = findChild<QWidget*>("FloatingPanelHeader");
    QHBoxLayout *header_layout = header ? qobject_cast<QHBoxLayout*>(header->layout()) : nullptr;
    m_status = new QLabel("", header ? header : this);
    m_status->setObjectName("PANORAMA_STATUS");
    m_status->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_status->setFixedHeight(18);
    m_status->setVisible(false);

    m_reset_button = new QPushButton("reset", header ? header : this);
    m_reset_button->setObjectName("FloatingPanelReset");
    m_reset_button->setFixedSize(40, 20);
    m_reset_button->setFocusPolicy(Qt::NoFocus);
    m_reset_button->setToolTip("Reset panorama panel");
    connect(m_reset_button, &QPushButton::clicked,
            this, &PanoramaPanel::resetRequested);

    if (header_layout) {
        if (QLabel *title_label = header->findChild<QLabel*>("FloatingPanelTitle")) {
            title_label->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        }
        header_layout->setStretch(0, 0);
        header_layout->insertWidget(1, m_status, 0, Qt::AlignVCenter);
        header_layout->insertStretch(2, 1);
        header_layout->insertWidget(qMax(0, header_layout->count() - 2),
                                    m_reset_button, 0, Qt::AlignVCenter);
    } else {
        contentLayout()->addWidget(m_status);
        contentLayout()->addWidget(m_reset_button);
    }

    m_scroll = new QScrollArea(this);
    m_scroll->setObjectName("PANORAMA_SCROLL");
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setWidgetResizable(true);
    m_scroll->setAlignment(Qt::AlignCenter);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_preview = new QLabel(m_scroll);
    m_preview->setObjectName("PANORAMA_PREVIEW");
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setMinimumSize(1, 1);
    m_preview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_preview->setStyleSheet("background: #03070A;");
    m_scroll->setWidget(m_preview);
    m_scroll->viewport()->installEventFilter(this);
    m_preview->installEventFilter(this);

    contentLayout()->addWidget(m_scroll, 1);

    m_resize_handle = new QWidget(this);
    m_resize_handle->setObjectName("PANORAMA_RESIZE_HANDLE");
    m_resize_handle->setAttribute(Qt::WA_StyledBackground, true);
    m_resize_handle->setFixedSize(kResizeHandleSize, kResizeHandleSize);
    m_resize_handle->setCursor(Qt::SizeFDiagCursor);
    m_resize_handle->setToolTip("Resize panorama panel");
    m_resize_handle->installEventFilter(this);
    m_resize_handle->raise();

    clearPreview();
}

void PanoramaPanel::clearPreview()
{
    resetView();
    QPixmap blank(960, 120);
    blank.fill(Qt::black);
    setPreviewPixmap(blank);
}

void PanoramaPanel::setPreviewPixmap(const QPixmap &pixmap)
{
    if (pixmap.isNull()) return;
    m_source_pixmap = pixmap;
    updatePreviewPixmap();
}

void PanoramaPanel::setGenerating()
{
    m_interactive = false;
    m_panning = false;
    resetView();
    if (m_status) {
        m_status->setText("panorama generating");
        m_status->setVisible(true);
    }
    if (m_preview) m_preview->setCursor(Qt::ArrowCursor);
    if (m_scroll && m_scroll->viewport()) m_scroll->viewport()->setCursor(Qt::ArrowCursor);
    updatePreviewPixmap();
}

void PanoramaPanel::setGenerated()
{
    m_interactive = true;
    m_panning = false;
    resetView();
    if (m_status) {
        m_status->setText("panorama generated");
        m_status->setVisible(true);
    }
    if (m_preview) m_preview->setCursor(Qt::OpenHandCursor);
    if (m_scroll && m_scroll->viewport()) m_scroll->viewport()->setCursor(Qt::OpenHandCursor);
    updatePreviewPixmap();
}

bool PanoramaPanel::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_resize_handle) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto *mouse = static_cast<QMouseEvent*>(event);
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
            auto *mouse = static_cast<QMouseEvent*>(event);
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
            auto *mouse = static_cast<QMouseEvent*>(event);
            if (m_resizing && mouse->button() == Qt::LeftButton) {
                m_resizing = false;
                if (m_resize_handle) m_resize_handle->releaseMouse();
                emit resizedByUser();
                return true;
            }
        }

        return FloatingPanel::eventFilter(obj, event);
    }

    const bool from_preview = (obj == m_preview);
    const bool from_viewport = (m_scroll && obj == m_scroll->viewport());
    if (!from_preview && !from_viewport) return FloatingPanel::eventFilter(obj, event);

    if (!m_interactive) return FloatingPanel::eventFilter(obj, event);

    if (event->type() == QEvent::Wheel) {
        auto *wheel = static_cast<QWheelEvent*>(event);
        const int delta = wheel->angleDelta().y();
        if (delta == 0) return true;

        const double old_zoom = m_zoom;
        const double factor = delta > 0 ? 1.15 : (1.0 / 1.15);
        m_zoom = qBound(1.0, m_zoom * factor, 8.0);
        if (qFuzzyCompare(old_zoom, m_zoom)) return true;

        const QSize target = m_scroll ? m_scroll->viewport()->size() : QSize();
        if (target.width() > 0 && target.height() > 0) {
            const QPoint cursor = eventPointInViewport(obj, wheel->pos());
            const QPointF center(target.width() / 2.0, target.height() / 2.0);
            const QPointF relative = QPointF(cursor) - center - QPointF(m_pan);
            const QPointF adjusted = QPointF(cursor) - center - relative * (m_zoom / old_zoom);
            m_pan = QPoint(qRound(adjusted.x()), qRound(adjusted.y()));
        }

        updatePreviewPixmap();
        return true;
    }

    if (event->type() == QEvent::MouseButtonPress) {
        auto *mouse = static_cast<QMouseEvent*>(event);
        if (mouse->button() == Qt::LeftButton) {
            m_panning = true;
            m_last_pan_pos = eventPointInViewport(obj, mouse->pos());
            if (m_preview) {
                m_preview->setCursor(Qt::ClosedHandCursor);
                m_preview->grabMouse();
            }
            if (m_scroll && m_scroll->viewport()) m_scroll->viewport()->setCursor(Qt::ClosedHandCursor);
            return true;
        }
    }

    if (event->type() == QEvent::MouseMove) {
        auto *mouse = static_cast<QMouseEvent*>(event);
        if (m_panning) {
            const QPoint pos = eventPointInViewport(obj, mouse->pos());
            m_pan += pos - m_last_pan_pos;
            m_last_pan_pos = pos;
            updatePreviewPixmap();
            return true;
        }
    }

    if (event->type() == QEvent::MouseButtonRelease) {
        auto *mouse = static_cast<QMouseEvent*>(event);
        if (m_panning && mouse->button() == Qt::LeftButton) {
            m_panning = false;
            if (m_preview) {
                m_preview->releaseMouse();
                m_preview->setCursor(Qt::OpenHandCursor);
            }
            if (m_scroll && m_scroll->viewport()) m_scroll->viewport()->setCursor(Qt::OpenHandCursor);
            return true;
        }
    }

    return FloatingPanel::eventFilter(obj, event);
}

void PanoramaPanel::resizeEvent(QResizeEvent *event)
{
    FloatingPanel::resizeEvent(event);
    if (m_resize_handle) {
        m_resize_handle->move(qMax(0, width() - kResizeHandleSize),
                              qMax(0, height() - kResizeHandleSize));
        m_resize_handle->raise();
    }
    updatePreviewPixmap();
}

QPoint PanoramaPanel::eventPointInViewport(QObject *obj, const QPoint &pos) const
{
    if (obj == m_preview && m_scroll) {
        return m_preview->mapTo(m_scroll->viewport(), pos);
    }
    return pos;
}

void PanoramaPanel::resetView()
{
    m_zoom = 1.0;
    m_pan = QPoint(0, 0);
    m_last_pan_pos = QPoint();
}

void PanoramaPanel::updatePreviewPixmap()
{
    if (!m_preview || m_source_pixmap.isNull()) return;

    QSize target = m_scroll ? m_scroll->viewport()->size() : m_preview->size();
    if ((target.width() <= 0 || target.height() <= 0)) target = m_preview->size();
    if (target.width() <= 0 || target.height() <= 0) return;

    QPixmap canvas(target);
    canvas.fill(Qt::black);

    QSize scaled_size = m_source_pixmap.size().scaled(target, Qt::KeepAspectRatio);
    if (m_interactive) {
        scaled_size = QSize(qMax(1, qRound(scaled_size.width() * m_zoom)),
                            qMax(1, qRound(scaled_size.height() * m_zoom)));

        auto clamp_axis = [](int pan, int view, int image) {
            if (image <= view) return 0;
            const int limit = (image - view) / 2;
            return qBound(-limit, pan, limit);
        };
        m_pan.setX(clamp_axis(m_pan.x(), target.width(), scaled_size.width()));
        m_pan.setY(clamp_axis(m_pan.y(), target.height(), scaled_size.height()));
    } else {
        m_pan = QPoint(0, 0);
    }

    const QPixmap scaled = m_source_pixmap.scaled(scaled_size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    const int x = (target.width() - scaled.width()) / 2 + (m_interactive ? m_pan.x() : 0);
    const int y = (target.height() - scaled.height()) / 2 + (m_interactive ? m_pan.y() : 0);

    QPainter painter(&canvas);
    painter.drawPixmap(x, y, scaled);
    painter.end();

    m_preview->setPixmap(canvas);
}
