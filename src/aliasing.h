#ifndef ALIASING_H
#define ALIASING_H

#include "utils.h"

struct AliasingData {
    float rep_freq;
    float distance;
    int num_period;
};

class UserPanel;
class Aliasing : public QObject
{
    Q_OBJECT
public:
    explicit Aliasing(QObject *parent = nullptr);

    void update_distance(float distance);
    void show_ui();
    AliasingData retrieve_data(int id);

signals:
    void delay_set_selected(int selection_id);

public:
    float original_distance;
    int   current_selection;

private:
    QTableView         *inference;
    QStandardItemModel *inference_model;
    QButtonGroup       *apply_btn_grp;
};

#endif // ALIASING_H
