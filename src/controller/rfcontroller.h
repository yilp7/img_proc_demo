#ifndef RFCONTROLLER_H
#define RFCONTROLLER_H

#include <QObject>

QT_BEGIN_NAMESPACE
namespace Ui { class UserPanel; }
QT_END_NAMESPACE

class RFController : public QObject
{
    Q_OBJECT

public:
    explicit RFController(QObject *parent = nullptr);

    void init(Ui::UserPanel *ui);

public slots:
    void update_distance(double distance);

private:
    Ui::UserPanel* m_ui;
};

#endif // RFCONTROLLER_H
