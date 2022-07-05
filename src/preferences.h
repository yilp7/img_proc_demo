#ifndef PREFERENCES_H
#define PREFERENCES_H

#include "utils.h"

namespace Ui {
class Preferences;
}

class Preferences : public QDialog
{
    Q_OBJECT

public:
    explicit Preferences(QWidget *parent = nullptr);
    ~Preferences();

private slots:
    void jump_to_content(int pos);

private:
    Ui::Preferences *ui;
};

#endif // PREFERENCES_H
