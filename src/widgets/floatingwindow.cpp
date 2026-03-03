#include "floatingwindow.h"

FloatingWindow::FloatingWindow() :
    QWidget(),
    pressed(false),
    disp(NULL),
    frame(NULL),
    w(1920),
    h(1080)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint | Qt::WindowStaysOnTopHint);
    setMouseTracking(true);
    this->setMinimumSize(240, 135);
    this->resize(480, 270);

    frame = new QFrame(this);
//    frame->setObjectName("FRAME");
    frame->resize(this->size());
    QGridLayout *layout = new QGridLayout(frame);
    layout->setContentsMargins(0, 0, 0, 0);

    layout->addWidget(new QSizeGrip(frame), 1, 1, Qt::AlignBottom | Qt::AlignRight);
//    layout->addWidget(new QSizeGrip(this), 0, 0, Qt::AlignTop    | Qt::AlignRight);
//    layout->addWidget(new QSizeGrip(this), 0, 0, Qt::AlignBottom | Qt::AlignLeft);
//    layout->addWidget(new QSizeGrip(this), 0, 0, Qt::AlignTop    | Qt::AlignLeft);

//    QPushButton *capture = new QPushButton(QIcon(":/tools/dark/capture"), "", frame);
//    connect(capture, &QPushButton::clicked, this, [this](){ hide(); });
//    capture->setFixedSize(20, 20);
//    capture->setIconSize(QSize(16, 16));
//    capture->setStyleSheet("background-color: transparent; border: none;");
//    layout->addWidget(capture, 0, 7, Qt::AlignTop | Qt::AlignRight);

    QPushButton *exit = new QPushButton(QIcon(":/tools/exit"), "", frame);
    connect(exit, &QPushButton::clicked, this, [this](){ hide(); });
    exit->setFixedSize(20, 20);
    exit->setIconSize(QSize(16, 16));
    exit->setStyleSheet("background-color: transparent; border: none;");
    layout->addWidget(exit, 0, 1, Qt::AlignTop | Qt::AlignRight);

    disp = new Display(this);
//    disp->setObjectName("DISP");
    this->resize(this->size());
    disp->resize(this->size());
    disp->setMouseTracking(true);
    disp->lower();

    QTimer::singleShot(5000, [this](){ frame->hide(); });
}

Display *FloatingWindow::get_display_widget()
{
    return disp;
}

void FloatingWindow::resize_display(int width, int height)
{
    w = width, h = height;
    this->resize(w * this->height() / h, h);
    QResizeEvent e(this->size(), this->size());
    resizeEvent(&e);
}

void FloatingWindow::mousePressEvent(QMouseEvent *event)
{
    if(event->button() != Qt::LeftButton) return;
    pressed = true;
    prev_pos = event->globalPos();
    timer_mouse.restart();

    QWidget::mousePressEvent(event);
}

void FloatingWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (pressed) {
        // use globalPos instead of pos to prevent window shaking
        window()->move(window()->pos() + event->globalPos() - prev_pos);
        prev_pos = event->globalPos();
    }
    else {
        setCursor(cursor_curr_pointer);
    }
    QWidget::mouseMoveEvent(event);
}

void FloatingWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() != Qt::LeftButton) return;
    pressed = false;
    if (timer_mouse.elapsed() < 500) {
        frame->show();
        QTimer::singleShot(5000, [this](){ frame->hide(); });
    }

    QWidget::mouseReleaseEvent(event);
}

void FloatingWindow::resizeEvent(QResizeEvent *event)
{
    this->resize(this->width(), this->width() * h / w);
    QPoint center = disp->center;
    center = center * this->width() / disp->width();
    disp->update_roi(center);

    disp->resize(this->size());
    frame->resize(this->size());
}
