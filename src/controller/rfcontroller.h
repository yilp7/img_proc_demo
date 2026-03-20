#ifndef RFCONTROLLER_H
#define RFCONTROLLER_H

#include <QObject>

class RFController : public QObject
{
    Q_OBJECT

public:
    explicit RFController(QObject *parent = nullptr);

public slots:
    void update_distance(double distance);

signals:
    void distance_text_updated(QString text);
};

#endif // RFCONTROLLER_H
