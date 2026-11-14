#include "mywidget.h"

Ruler::Ruler(QWidget *parent) :
    QLabel(parent),
    m_vertical(false),
    m_interval(10)
{

}

void Ruler::draw_mark(QPainter *painter)
{
    float start = 0;
    if (m_vertical) {
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
            start += m_interval;
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
            start += m_interval;
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

AnimationLabel::AnimationLabel(QWidget *parent, QString path, QSize size, int interval) : QLabel(parent)
{
    connect(&m_t, SIGNAL(timeout()), SLOT(draw()));
    setup_animation(path, size, interval);
}

void AnimationLabel::setup_animation(QString path, QSize size, int interval)
{
    m_source = QPixmap(path);
    m_partition_size = size;
    m_curr_pos = QPoint();
    m_t.stop();
    m_t.start(interval);
    draw();
}

void AnimationLabel::draw()
{
    if (m_curr_pos.x() > m_source.width() - 1) m_curr_pos -= QPoint(m_source.width(), -m_partition_size.height());
    if (m_curr_pos.y() > m_source.height() - 1) m_curr_pos = QPoint();
    QPixmap partition = m_source.copy(m_curr_pos.x(), m_curr_pos.y(), m_partition_size.width(), m_partition_size.height());
    this->setPixmap(partition.scaled(this->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_curr_pos += QPoint(m_partition_size.width(), 0);
}

Tools::Tools(QWidget *parent) : QRadioButton(parent) {}

Coordinate::Coordinate(QWidget *parent) : QFrame(parent), m_pair(0, 0), m_set_name(nullptr), m_coord_x(nullptr), m_coord_y(nullptr) {}

void Coordinate::setup(QString name)
{
    if (m_set_name) {
        delete m_set_name;
        m_set_name = nullptr;
    }
    if (m_coord_x) {
        delete m_coord_x;
        m_coord_x = nullptr;
    }
    if (m_coord_y) {
        delete m_coord_y;
        m_coord_y = nullptr;
    }
    m_set_name = new InfoLabel(this);
    m_set_name->setText(name);
    m_set_name->setFont(monaco);
    m_set_name->setGeometry(0, 0, this->fontMetrics().width(name) + 10, 13);
    m_coord_x = new InfoLabel(this);
    m_coord_x->setText("X: xxxx");
    m_coord_x->setFont(monaco);
    m_coord_x->setGeometry(m_set_name->geometry().right() + 10, 0, this->fontMetrics().width("X: xxxx") + 10, 13);
    m_coord_y = new InfoLabel(this);
    m_coord_y->setText("Y: yyyy");
    m_coord_y->setFont(monaco);
    m_coord_y->setGeometry(m_set_name->geometry().right() + 10, 17, this->fontMetrics().width("Y: yyyy") + 10, 13);

    this->resize(m_set_name->width() + 10 + m_coord_x->width(), 30);
}

void Coordinate::display_pos(QPoint p)
{
    m_pair.setX(p.x());
    m_pair.setY(p.y());
    m_coord_x->setText(QString::asprintf("X:% 05d", p.x()));
    m_coord_y->setText(QString::asprintf("Y:% 05d", p.y()));
}

IndexLabel::IndexLabel(QWidget *parent) : QLabel(parent) {}

void IndexLabel::setup(int idx, int pos_y)
{
    this->m_idx = idx;
    this->m_pos_y = pos_y;
}

void IndexLabel::mouseReleaseEvent(QMouseEvent *event)
{
    QLabel::mouseReleaseEvent(event);

    if(event->button() != Qt::LeftButton) return;
    emit index_label_clicked(m_pos_y);
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
