#ifndef DEVELOPEROPTIONS_H
#define DEVELOPEROPTIONS_H

#include <QDialog>

namespace Ui {
class DeveloperOptions;
}

class DeveloperOptions : public QDialog
{
    Q_OBJECT

public:
    explicit DeveloperOptions(QWidget *parent = nullptr);
    ~DeveloperOptions();

private:
    Ui::DeveloperOptions *ui;
};

#endif // DEVELOPEROPTIONS_H
