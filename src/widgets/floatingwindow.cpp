#include "floatingwindow.h"

FloatingWindow::FloatingWindow() :
    QWidget(),
    m_pressed(false),
    m_disp(nullptr),
    m_frame(nullptr),
    m_w(1920),
    m_h(1080)
{
    setObjectName("FloatingWindow");
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint | Qt::WindowStaysOnTopHint);
    setMouseTracking(true);
    this->setMinimumSize(240, 135);
    this->resize(480, 270);

    m_frame = new QFrame(this);
    m_frame->setObjectName("FLOATING_WINDOW_CHROME");
    m_frame->setAttribute(Qt::WA_StyledBackground, true);
    m_frame->resize(this->size());
    QGridLayout *layout = new QGridLayout(m_frame);
    layout->setContentsMargins(0, 0, 0, 0);

    layout->addWidget(new QSizeGrip(m_frame), 1, 1, Qt::AlignBottom | Qt::AlignRight);
//    layout->addWidget(new QSizeGrip(this), 0, 0, Qt::AlignTop    | Qt::AlignRight);
//    layout->addWidget(new QSizeGrip(this), 0, 0, Qt::AlignBottom | Qt::AlignLeft);
//    layout->addWidget(new QSizeGrip(this), 0, 0, Qt::AlignTop    | Qt::AlignLeft);

//    QPushButton *capture = new QPushButton(QIcon(":/tools/dark/capture"), "", m_frame);
//    connect(capture, &QPushButton::clicked, this, [this](){ hide(); });
//    capture->setFixedSize(20, 20);
//    capture->setIconSize(QSize(16, 16));
//    layout->addWidget(capture, 0, 7, Qt::AlignTop | Qt::AlignRight);

    QPushButton *exit = new QPushButton(QIcon(":/tools/exit"), "", m_frame);
    exit->setObjectName("FLOATING_WINDOW_EXIT");
    connect(exit, &QPushButton::clicked, this, [this](){ hide(); });
    exit->setFixedSize(20, 20);
    exit->setIconSize(QSize(16, 16));
    layout->addWidget(exit, 0, 1, Qt::AlignTop | Qt::AlignRight);

    m_disp = new Display(this);
//    m_disp->setObjectName("DISP");
    this->resize(this->size());
    m_disp->resize(this->size());
    m_disp->setMouseTracking(true);
    m_disp->lower();

    QTimer::singleShot(5000, [this](){ m_frame->hide(); });
}

Display *FloatingWindow::get_display_widget()
{
    return m_disp;
}

void FloatingWindow::resize_display(int width, int height)
{
    m_w = width, m_h = height;
    this->resize(m_w * this->height() / m_h, m_h);
    QResizeEvent e(this->size(), this->size());
    resizeEvent(&e);
}

void FloatingWindow::mousePressEvent(QMouseEvent *event)
{
    if(event->button() != Qt::LeftButton) return;
    m_pressed = true;
    m_prev_pos = event->globalPos();
    m_timer_mouse.restart();

    QWidget::mousePressEvent(event);
}

void FloatingWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_pressed) {
        // use globalPos instead of pos to prevent window shaking
        window()->move(window()->pos() + event->globalPos() - m_prev_pos);
        m_prev_pos = event->globalPos();
    }
    else {
        setCursor(cursor_curr_pointer);
    }
    QWidget::mouseMoveEvent(event);
}

void FloatingWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() != Qt::LeftButton) return;
    m_pressed = false;
    if (m_timer_mouse.elapsed() < 500) {
        m_frame->show();
        QTimer::singleShot(5000, [this](){ m_frame->hide(); });
    }

    QWidget::mouseReleaseEvent(event);
}

void FloatingWindow::resizeEvent(QResizeEvent *event)
{
    this->resize(this->width(), this->width() * m_h / m_w);
    QPoint center = m_disp->m_center;
    center = center * this->width() / m_disp->width();
    m_disp->update_roi(center);

    m_disp->resize(this->size());
    m_frame->resize(this->size());
}
