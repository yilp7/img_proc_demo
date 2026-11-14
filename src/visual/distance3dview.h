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
    Ui::Distance3DView    *m_ui;
    QWidget               *m_signal_sender;
    std::vector<cv::Rect> *m_list_roi;

    QtDataVisualization::Q3DScatter *m_scatter_graph;

    uint    m_downsample;

    QMutex  m_mat_mutex;
    cv::Mat m_dist_mat;
    double  m_dist_min;
    double  m_dist_max;
    bool    m_consecutive;
    bool    m_get_single_frame;
};

#endif // DISTANCE3DVIEW_H
