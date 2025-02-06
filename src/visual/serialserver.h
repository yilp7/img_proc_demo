#ifndef SERIALSERVER_H
#define SERIALSERVER_H

#include "util/util.h"

namespace Ui {
class SerialServer;
}

class SerialServer : public QDialog
{
    Q_OBJECT

public:
    explicit SerialServer(QWidget *parent = nullptr);
    ~SerialServer();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    Ui::SerialServer *ui;

    QComboBox *port_type_lists[4];
};

#endif // SERIALSERVER_H
