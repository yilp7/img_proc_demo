#ifndef ALIASING_H
#define ALIASING_H

#include "util/util.h"

struct AliasingData {
    float rep_freq;
    float distance;
    int num_period;

    friend QDebug& operator << (QDebug& qd, const AliasingData& ad) {
        qd << "PRF: " << ad.rep_freq << ", distance: " << ad.distance << ", # period: " << ad.num_period;
        return qd;
    }
};

class UserPanel;

class Aliasing : public QWidget
{
    Q_OBJECT
public:
    explicit Aliasing(QWidget *parent = nullptr);
    ~Aliasing();

    void update_distance(float distance);
    void show_ui();
    AliasingData retrieve_data(int id);

signals:
    void delay_set_selected(int selection_id);

public:
    float original_distance;
    int   current_selection;

    QMap<int, struct AliasingData> recommended_parameters; // key: #period, value: recommand parameters

protected:
    void closeEvent(QCloseEvent *event);

private:
    QTableView         *inference;
    QStandardItemModel *inference_model;
    QButtonGroup       *apply_btn_grp;
};

#endif // ALIASING_H
