#include "mywidget.h"

Display::Display(QWidget *parent) : QLabel(parent)
  , lefttop(0, 0)
  , center(0, 0)
  , grab(false)
  , drag(false)
  , curr_scale(0)
//  , scale{ QSize(640, 512), QSize(480, 384), QSize(320, 256), QSize(160, 128), QSize(80, 64)}
//  , scale{ QSize(640, 400), QSize(480, 300), QSize(320, 200), QSize(160, 100), QSize(80, 50)}
  , scale{ 1.0, 2.0 / 3, 1.0 / 2, 1.0 / 4, 1.0 / 8}
  , pressed(false)
{

}

void Display::update_roi(QPoint center)
{
    QPoint temp(scale[curr_scale] * this->width(), scale[curr_scale] * this->height());
    lefttop = center - temp / 2;
    if (lefttop.x() < 0) lefttop.setX(0);
    if (lefttop.y() < 0) lefttop.setY(0);
    if (lefttop.x() > this->width() - temp.x()) lefttop.setX(this->width() - temp.x());
    if (lefttop.y() > this->height() - temp.y()) lefttop.setY(this->height() - temp.y());

    display_region.x = lefttop.x();
    display_region.y = lefttop.y();
    display_region.width = scale[curr_scale] * this->width();
    display_region.height = scale[curr_scale] * this->height();
    this->center = lefttop + temp / 2;
}

void Display::mousePressEvent(QMouseEvent *event)
{
    QLabel::mousePressEvent(event);

    if(event->button() != Qt::LeftButton) return;

//    qDebug("pos: %d, %d\n", event->x(), event->y());
//    qDebug("pos: %d, %d\n", event->globalX(), event->globalY());
//    qDebug("%s pressed\n", qPrintable(this->objectName()));

    if (!grab) return;
    pressed = true;
    prev_pos = event->pos();
    if (drag) return;
    selection_v2.x = selection_v1.x = event->pos().x();
    selection_v2.y = selection_v1.y = event->pos().y();
    emit start_pos(event->pos());
}

void Display::mouseMoveEvent(QMouseEvent *event)
{
    QLabel::mouseMoveEvent(event);

    if (!grab) return;
    if (geometry().contains(event->pos())) emit curr_pos(event->pos());
    if (!pressed) return;
//    qDebug("pos: %d, %d\n", event->x(), event->y());
//    qDebug("pos: %d, %d\n", event->globalX(), event->globalY());
//    qDebug("%s selecting\n", qPrintable(this->objectName()));

    if (drag) update_roi(lefttop + QPoint((int)(scale[curr_scale] * this->width()), (int)(scale[curr_scale] * this->height())) / 2 - (event->pos() - prev_pos));
    prev_pos = event->pos();
    if (drag) return;
    selection_v2.x = event->pos().x();
    selection_v2.y = event->pos().y();
    emit shape_size(event->pos() - QPoint(selection_v1.x, selection_v1.y));
}

void Display::mouseReleaseEvent(QMouseEvent *event)
{
    QLabel::mouseReleaseEvent(event);

    if(event->button() != Qt::LeftButton) return;
    if (!grab) return;

    pressed = false;
    if (drag) return;
    selection_v2.x = event->pos().x();
    selection_v2.y = event->pos().y();
    emit shape_size(event->pos() - QPoint(selection_v1.x, selection_v1.y));
}

void Display::wheelEvent(QWheelEvent *event)
{
    QLabel::wheelEvent(event);

    if (!grab) return;
    QPoint center = lefttop + QPoint((int)(scale[curr_scale] * this->width() / 2), (int)(scale[curr_scale] * this->height() / 2));
    QLabel::wheelEvent(event);
    if(event->delta() > 0) {
        if (curr_scale >= 4) return;
        curr_scale++;
    }
    else {
        if (curr_scale <= 0) return;
        curr_scale--;
    }

//    if (curr_scale > 4) curr_scale = 4;
//    if (curr_scale < 0) curr_scale = 0;
    qDebug("current scale: %dx%d", (int)(scale[curr_scale] * this->width()), (int)(scale[curr_scale] * this->height()));

    update_roi(center);
}

Ruler::Ruler(QWidget *parent) : QLabel(parent)
  , vertical(false)
  , interval(10)
{

}

void Ruler::draw_mark(QPainter *painter)
{
    float start = 0;
    if (vertical) {
        for (int i = 0; i <= 10000; i += 10) {
            if (i % 50) painter->drawRect(QRectF(5, start, 5, 0.1));
            else if (i % 100) painter->drawRect(QRectF(3, start, 7, 0.1));
            else if (i) {
                painter->drawRect(QRectF(0, start, 10, 0.1));
                QString str = QString::number(i);
                painter->rotate(90);
                painter->drawText(QPointF(start - painter->fontMetrics().width(str) / 2, -13), str);
                painter->rotate(-90);
//                painter->drawText(QPointF(13, start + fontMetrics().height() / 4), str);
            }
            start += interval;
        }

    }
    else {
        for (int i = 0; i <= 10000; i += 10) {
            if (i % 50) painter->drawRect(QRectF(start, 5, 0.1, 5));
            else if (i % 100) painter->drawRect(QRectF(start, 3, 0.1, 7));
            else if (i) {
                painter->drawRect(QRectF(start, 0, 0.1, 10));
                QString str = QString::number(i);
                painter->drawText(QPointF(start - painter->fontMetrics().width(str) / 2, 22), str);
            }
            start += interval;
        }
    }
}

void Ruler::paintEvent(QPaintEvent *event)
{
    QPainter painter;
    painter.begin(this);

    painter.setRenderHint(QPainter::Antialiasing);
    painter.setFont(QFont("Consolas", 10, QFont::Light));
    draw_mark(&painter);

    painter.end();
}

InfoLabel::InfoLabel(QWidget *parent) : QLabel(parent) {}

TitleButton::TitleButton(QString icon, QWidget *parent) : QPushButton(QIcon(icon), "", parent) {}

void TitleButton::mouseMoveEvent(QMouseEvent *event) {}

TitleBar::TitleBar(QWidget *parent) : QFrame(parent)
  , is_maximized(false)
  , pressed(false)
{
    icon = new InfoLabel(this);
    icon->setGeometry(60, 5, 20, 20);
    icon->setObjectName("ICON");
    title = new InfoLabel(this);
    title->setGeometry(90, 7, 60, 16);
    title->setObjectName("NAME");
    min = new TitleButton(":/tools/min.png", this);
    min->setObjectName("MIN_BTN");
    max = new TitleButton(":/tools/max.png", this);
    max->setObjectName("MAX_BTN");
    exit = new TitleButton(":/tools/exit.png", this);
    exit->setObjectName("EXIT_BTN");
    connect(min, SIGNAL(clicked()), window(), SLOT(showMinimized()));
    connect(max, SIGNAL(clicked()), SLOT(process_maximize()));
    connect(exit, SIGNAL(clicked()), window(), SLOT(close()));

    prog_settings = new ProgSettings();

    settings = new TitleButton("", this);
    settings->setObjectName("SETTINGS_BTN");
    QMenu *settings_menu = new QMenu();
    QAction *options = new QAction("-", this);
    settings_menu->addAction(options);
    settings->setMenu(settings_menu);
    connect(options, SIGNAL(triggered()), prog_settings, SLOT(show()));

    capture = new TitleButton("", this);
    capture->setObjectName("CAPTURE_BTN");
    connect(capture, SIGNAL(clicked()), this->parent()->parent(), SLOT(screenshot()));

    cls = new TitleButton("", this);
    cls->setObjectName("CLS_BTN");
    connect(cls, SIGNAL(clicked()), this->parent()->parent(), SLOT(clean()));

    lang = new TitleButton("", this);
    lang->setObjectName("LANGUAGE_BTN");
    connect(lang, SIGNAL(clicked()), this->parent()->parent(), SLOT(switch_language()));
}

void TitleBar::process_maximize()
{
    if (!is_maximized) normal_stat = window()->geometry();
    max->setIcon(is_maximized ? QIcon(":/tools/max.png") : QIcon(":/tools/restore.png"));
    is_maximized ? window()->setGeometry(normal_stat) : window()->setGeometry(QApplication::desktop()->availableGeometry());
    is_maximized ^= 1;
}

void TitleBar::resizeEvent(QResizeEvent *event)
{
    settings->setGeometry(this->width() - 310, 5, 20, 20);
    capture->setGeometry(this->width() - 270, 5, 20, 20);
    cls->setGeometry(this->width() - 230, 5, 20, 20);
    lang->setGeometry(this->width() - 190, 5, 20, 20);
    min->setGeometry(this->width() - 120, 0, 40, 30);
    max->setGeometry(this->width() - 80, 0, 40, 30);
    exit->setGeometry(this->width() - 40, 0, 40, 30);

    QFrame::resizeEvent(event);
}

void TitleBar::mousePressEvent(QMouseEvent *event)
{
    QFrame::mousePressEvent(event);

    if(event->button() != Qt::LeftButton) return;
    pressed = true;
    prev_pos = event->globalPos();
}

void TitleBar::mouseMoveEvent(QMouseEvent *event)
{
    QFrame::mouseMoveEvent(event);

    if (is_maximized || !pressed) return;
    // use globalPos instead of pos to prevent window shaking
    window()->move(window()->pos() + event->globalPos() - prev_pos);
    prev_pos = event->globalPos();
}

void TitleBar::mouseReleaseEvent(QMouseEvent *event)
{
    QFrame::mouseReleaseEvent(event);

    if(event->button() != Qt::LeftButton) return;
    pressed = false;
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    QFrame::mouseDoubleClickEvent(event);

    if (event->button() != Qt::LeftButton) return;
    process_maximize();
}

AnimationLabel::AnimationLabel(QWidget *parent, QString path, QSize size, int interval) : QLabel(parent) {
    connect(&t, SIGNAL(timeout()), SLOT(draw()));
    setup_animation(path, size, interval);
}

void AnimationLabel::setup_animation(QString path, QSize size, int interval)
{
    source = QPixmap(path);
    partition_size = size;
    curr_pos = QPoint();
    t.stop();
    t.start(interval);
    draw();
}

void AnimationLabel::draw()
{
    if (curr_pos.x() > source.width() - 1) curr_pos -= QPoint(source.width(), -partition_size.height());
    if (curr_pos.y() > source.height() - 1) curr_pos = QPoint();
    QPixmap partition = source.copy(curr_pos.x(), curr_pos.y(), partition_size.width(), partition_size.height());
    this->setPixmap(partition.scaled(this->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    curr_pos += QPoint(partition_size.width(), 0);
}

Tools::Tools(QWidget *parent) : QRadioButton(parent) {}

Coordinate::Coordinate(QWidget *parent) : QFrame(parent), set_name(NULL), coord_x(NULL), coord_y(NULL) {}

void Coordinate::setup(QString name)
{
    if (set_name) delete set_name;
    if (coord_x) delete coord_x;
    if (coord_y) delete coord_y;
    set_name = new InfoLabel(this);
    set_name->setText(name);
    set_name->setFont(QFont("Consolas", 10));
    set_name->setStyleSheet("color: #DEC4B0;");
    set_name->setGeometry(0, 0, this->fontMetrics().width(name) + 10, 13);
    coord_x = new InfoLabel(this);
    coord_x->setText("X: xxxx");
    coord_x->setFont(QFont("Consolas", 10));
    coord_x->setStyleSheet("color: #DEC4B0;");
    coord_x->setGeometry(set_name->geometry().right() + 10, 0, this->fontMetrics().width("X: xxxx") + 10, 13);
    coord_y = new InfoLabel(this);
    coord_y->setText("Y: yyyy");
    coord_y->setFont(QFont("Consolas", 10));
    coord_y->setStyleSheet("color: #DEC4B0;");
    coord_y->setGeometry(set_name->geometry().right() + 10, 17, this->fontMetrics().width("Y: yyyy") + 10, 13);

    this->resize(set_name->width() + 10 + coord_x->width(), 30);
}

void Coordinate::display_pos(QPoint p)
{
    coord_x->setText(QString::asprintf("X: %04d", abs(p.x())));
    coord_y->setText(QString::asprintf("Y: %04d", abs(p.y())));
}
