#include "mywidget.h"

Ruler::Ruler(QWidget *parent) :
    QLabel(parent),
    vertical(false),
    interval(10)
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
//    QFont temp = QFont(monaco);
//    temp.setPointSize(10);
//    temp.setWeight(QFont::Light);
    painter.setFont(monaco);
    draw_mark(&painter);

    painter.end();
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

Coordinate::Coordinate(QWidget *parent) : QFrame(parent), pair(0, 0), set_name(NULL), coord_x(NULL), coord_y(NULL) {}

void Coordinate::setup(QString name)
{
    if (set_name) {
        delete set_name;
        set_name = NULL;
    }
    if (coord_x) {
        delete coord_x;
        coord_x = NULL;
    }
    if (coord_y) {
        delete coord_y;
        coord_y = NULL;
    }
    set_name = new InfoLabel(this);
    set_name->setText(name);
    set_name->setFont(monaco);
//    set_name->setStyleSheet("color: #DEC4B0;");
    set_name->setGeometry(0, 0, this->fontMetrics().width(name) + 10, 13);
    coord_x = new InfoLabel(this);
    coord_x->setText("X: xxxx");
    coord_x->setFont(monaco);
//    coord_x->setStyleSheet("color: #DEC4B0;");
    coord_x->setGeometry(set_name->geometry().right() + 10, 0, this->fontMetrics().width("X: xxxx") + 10, 13);
    coord_y = new InfoLabel(this);
    coord_y->setText("Y: yyyy");
    coord_y->setFont(monaco);
//    coord_y->setStyleSheet("color: #DEC4B0;");
    coord_y->setGeometry(set_name->geometry().right() + 10, 17, this->fontMetrics().width("Y: yyyy") + 10, 13);

    this->resize(set_name->width() + 10 + coord_x->width(), 30);
}

void Coordinate::display_pos(QPoint p)
{
    pair.setX(p.x());
    pair.setY(p.y());
    coord_x->setText(QString::asprintf("X:% 05d", p.x()));
    coord_y->setText(QString::asprintf("Y:% 05d", p.y()));
}

IndexLabel::IndexLabel(QWidget *parent) : QLabel(parent) {}

void IndexLabel::setup(int idx, int pos_y)
{
    this->idx = idx;
    this->pos_y = pos_y;
}

void IndexLabel::mouseReleaseEvent(QMouseEvent *event)
{
    QLabel::mouseReleaseEvent(event);

    if(event->button() != Qt::LeftButton) return;
    emit index_label_clicked(pos_y);
}

MiscSelection::MiscSelection(QWidget *parent) : QComboBox(parent)
{
    this->view()->window()->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
}

void MiscSelection::mousePressEvent(QMouseEvent *event)
{
    clearFocus();
}

void MiscSelection::mouseReleaseEvent(QMouseEvent *event)
{
    switch (event->button()) {
    case Qt::LeftButton: emit selected(); break;
    case Qt::RightButton: showPopup(); break;
    default: break;
    }
    clearFocus();
}

void MiscSelection::wheelEvent(QWheelEvent *event)
{
    clearFocus();
}

void MiscSelection::paintEvent(QPaintEvent *event)
{
    QStylePainter painter(this);
    QStyleOptionComboBox opt;
    initStyleOption(&opt);
    opt.currentText = "";
    opt.subControls &= ~QStyle::SC_ComboBoxArrow;
    painter.drawComplexControl(QStyle::CC_ComboBox, opt);
    painter.drawControl(QStyle::CE_ComboBoxLabel, opt);
    QRect rect1 = style()->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxEditField, this);
    QRect rect2 = style()->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxArrow, this);
    painter.drawText(rect1.adjusted(0, 0, rect2.right() - rect1.right(), 0), Qt::AlignLeft | Qt::AlignVCenter, currentText());
}

//void MiscSelection::showPopup()
//{
//    QComboBox::showPopup();

//    QWidget *popup = this->findChild<QFrame*>();
//    if (!popup) return;
//    popup->move(popup->x(), popup->y() - this->height() - popup->height());
//}
