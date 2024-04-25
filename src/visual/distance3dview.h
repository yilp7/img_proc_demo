#ifndef DISTANCE3DVIEW_H
#define DISTANCE3DVIEW_H

#include "util/util.h"

namespace Ui {
class Distance3DView;
}

class Distance3DView : public QWidget
{
    Q_OBJECT

public:
    explicit Distance3DView(QWidget *parent = nullptr, QWidget *signal_sender = nullptr, std::vector<cv::Rect> *list_roi = nullptr);
    ~Distance3DView();

public slots:
    void update_dist_mat(cv::Mat new_dist_mat, double d_min, double d_max);

private:
    void load_data();

private:
    Ui::Distance3DView    *ui;
    QWidget               *signal_sender;
    std::vector<cv::Rect> *list_roi;

    QtDataVisualization::Q3DScatter *scatter_graph;

    uint    downsample;

    QMutex  mat_mutex;
    cv::Mat dist_mat;
    double  dist_min;
    double  dist_max;
    bool    consecutive;
    bool    get_single_frame;
};

#endif // DISTANCE3DVIEW_H
