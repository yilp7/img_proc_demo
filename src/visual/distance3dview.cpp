#include "distance3dview.h"
#include "ui_distance3dview.h"

#include <Q3DCamera>

Distance3DView::Distance3DView(QWidget *parent, QWidget *signal_sender, std::vector<cv::Rect> *list_roi) :
    QWidget(parent),
    m_ui(new Ui::Distance3DView),
    m_signal_sender(signal_sender),
    m_list_roi(list_roi),
    m_scatter_graph(nullptr),
    m_downsample(4),
    m_consecutive(false),
    m_get_single_frame(false)
{
    m_ui->setupUi(this);

    connect(m_ui->CONSECUTIVE_CHK, &QCheckBox::stateChanged, this,
            [this](int arg1){ m_consecutive = arg1; m_ui->REFRESH_BTN->setEnabled(!arg1); m_ui->DOWNSAMPLE_LST->setEnabled(!arg1); });
    connect(m_ui->REFRESH_BTN, &QPushButton::clicked, this, [this](){ m_get_single_frame = true; });

    m_ui->H_ANGLE_SPIN->setMinimum(-180);
    m_ui->H_ANGLE_SPIN->setMaximum( 180);
    m_ui->H_ANGLE_SPIN->setSingleStep(5);
    m_ui->H_ANGLE_SPIN->setValue(-90);
    connect(m_ui->H_ANGLE_SPIN, static_cast<void (QSpinBox::*)(int val)>(&QSpinBox::valueChanged), this,
            [this](int val){ m_scatter_graph->scene()->activeCamera()->setXRotation(val); });
    m_ui->V_ANGLE_SPIN->setMinimum(-180);
    m_ui->V_ANGLE_SPIN->setMaximum( 180);
    m_ui->V_ANGLE_SPIN->setValue(90);
    m_ui->V_ANGLE_SPIN->setSingleStep(5);
    connect(m_ui->V_ANGLE_SPIN, static_cast<void (QSpinBox::*)(int val)>(&QSpinBox::valueChanged), this,
            [this](int val){ m_scatter_graph->scene()->activeCamera()->setYRotation(val); });

    m_ui->SCALE_SLIDER->setMinimum(10);
    m_ui->SCALE_SLIDER->setMaximum(500);
    m_ui->SCALE_SLIDER->setValue(100);
    connect(m_ui->SCALE_SLIDER, &QSlider::sliderMoved, this, [this](int pos){ m_scatter_graph->scene()->activeCamera()->setZoomLevel(pos); });

    m_ui->PROJECTION_LST->addItem("3D");
    m_ui->PROJECTION_LST->addItem("2D");
    m_ui->PROJECTION_LST->addItem("1D-H");
    m_ui->PROJECTION_LST->addItem("1D-V");
    m_ui->PROJECTION_LST->setCurrentIndex(1);
    connect(m_ui->PROJECTION_LST, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), this,
            [this](int index){ m_scatter_graph->setOrthoProjection(index); load_data(); });

    m_ui->DOWNSAMPLE_LST->addItem("1");
    m_ui->DOWNSAMPLE_LST->addItem("2");
    m_ui->DOWNSAMPLE_LST->addItem("4");
    m_ui->DOWNSAMPLE_LST->setCurrentIndex(2);
    connect(m_ui->DOWNSAMPLE_LST, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), this,
            [this](int index){ m_downsample = 1 << index; load_data(); });

    m_scatter_graph = new QtDataVisualization::Q3DScatter();

    QLayout *l = new QVBoxLayout(m_ui->DISTANCE_DISPLAY);
    l->addWidget(QWidget::createWindowContainer(m_scatter_graph));
    m_ui->DISTANCE_DISPLAY->setLayout(l);

    QtDataVisualization::QValue3DAxis *axis_x = new QtDataVisualization::QValue3DAxis(m_scatter_graph);
    axis_x->setTitle("X");
    axis_x->setLabelFormat("%d");
    axis_x->setRange(0, 1104);
    m_scatter_graph->setAxisX(axis_x);
    QtDataVisualization::QValue3DAxis *axis_y = new QtDataVisualization::QValue3DAxis(m_scatter_graph);
    axis_y->setTitle("DISTANCE");
    axis_y->setRange(60, 160);
    m_scatter_graph->setAxisY(axis_y);
    QtDataVisualization::QValue3DAxis *axis_z = new QtDataVisualization::QValue3DAxis(m_scatter_graph);
    axis_z->setTitle("Y");
    axis_z->setLabelFormat("%d");
    axis_z->setRange(0, 1608);
    m_scatter_graph->setAxisZ(axis_z);

    QtDataVisualization::QScatter3DSeries *series = new QtDataVisualization::QScatter3DSeries(m_scatter_graph);
    m_scatter_graph->addSeries(series);
    series->setMesh(QtDataVisualization::QAbstract3DSeries::MeshPoint);

    m_scatter_graph->scene()->activeCamera()->setWrapXRotation(true);
    m_scatter_graph->scene()->activeCamera()->setWrapYRotation(true);

    load_data();

    QtDataVisualization::Q3DTheme *theme = new QtDataVisualization::Q3DTheme(QtDataVisualization::Q3DTheme::ThemeStoneMoss);
    theme->setBackgroundEnabled(false);
    theme->setLabelBackgroundEnabled(false);
    theme->setColorStyle(QtDataVisualization::Q3DTheme::ColorStyleRangeGradient);
    theme->setGridEnabled(true);
    QLinearGradient lg; // WideMatrix
    lg.setColorAt(1.00, QColor(0x3700c4));
    lg.setColorAt(0.86, QColor(0x4574fc));
    lg.setColorAt(0.71, QColor(0x84c6ff));
    lg.setColorAt(0.57, QColor(0x5fd9cb));
    lg.setColorAt(0.43, QColor(0x39fd6a));
    lg.setColorAt(0.29, QColor(0xfde22f));
    lg.setColorAt(0.14, QColor(0xfd602f));
    lg.setColorAt(0.00, QColor(0xb43a3a));
    QList<QLinearGradient> llg; llg << lg;
    theme->setBaseGradients(llg);
    m_scatter_graph->setActiveTheme(theme);

    m_scatter_graph->scene()->activeCamera()->setXRotation(-90);
    m_scatter_graph->scene()->activeCamera()->setYRotation( 90);
    m_scatter_graph->setOrthoProjection(true);

    qRegisterMetaType<cv::Mat>("cv::Mat");
    connect(signal_sender, SIGNAL(update_dist_mat(cv::Mat, double, double)), this, SLOT(update_dist_mat(cv::Mat, double, double)));

    m_ui->DISTANCE_DISPLAY->setFocus();
}

Distance3DView::~Distance3DView()
{
    delete m_scatter_graph;
    delete m_ui;
}

void Distance3DView::update_dist_mat(cv::Mat new_dist_mat, double d_min, double d_max)
{
    if (!m_consecutive && !m_get_single_frame) return;
    if (m_mat_mutex.tryLock(10)) return;
    m_dist_mat = new_dist_mat.clone();
    m_mat_mutex.unlock();
    if (m_consecutive) {
        static QElapsedTimer t;
        if (t.isValid()) {
            if (t.elapsed() < 1000) return;
            t.restart();
        }
        else t.start();
    }
    m_dist_min = d_min;
    m_dist_max = d_max;
    load_data();
    if (m_get_single_frame) m_get_single_frame = false;
}

void Distance3DView::load_data()
{
    QtDataVisualization::QScatterDataArray *array = new QtDataVisualization::QScatterDataArray();
//    QStringList item;
//    QString line;

//    QFile dist_csv("../dist.csv");
//    dist_csv.open(QIODevice::ReadOnly | QIODevice::Text);
//    QTextStream stream(&dist_csv);
//    int line_num = 0;
//    while (!stream.atEnd()) {
//        line = stream.readLine().trimmed();
//        if (line.isEmpty()) break;
//        if (line_num++ & (m_downsample - 1)) continue;
//        item = line.split(',');
//        for (int i = 0; i < item.size(); i += m_downsample) array->append(QVector3D(line_num, item[i].toFloat(), i));
//    }
//    dist_csv.close();

    cv::Mat result_dist_mat;
    cv::Mat mask = cv::Mat::zeros(m_dist_mat.size(), CV_8UC1);
    switch (m_ui->PROJECTION_LST->currentIndex()) {
    case 0:
    case 1:
        mask = 255; break;
    case 2:
        for (cv::Rect roi: *m_list_roi) mask(cv::Rect(roi.x, roi.y + roi.height / 2, roi.width, 1)).setTo(255);
        break;
    case 3:
        for (cv::Rect roi: *m_list_roi) mask(cv::Rect(roi.x + roi.width / 2, roi.y, 1, roi.height)).setTo(255);
        break;
    default: break;
    }
    m_dist_mat.copyTo(result_dist_mat, mask);

    m_mat_mutex.lock();
    float *ptr = (float*)result_dist_mat.data;
    for (int i = 0; i < result_dist_mat.rows; i += m_downsample) {
        for (int j = 0; j < result_dist_mat.cols; j += m_downsample) {
            float temp = ptr[i * result_dist_mat.cols + j];
            if (temp > m_dist_min && temp < m_dist_max) array->append(QVector3D(i, temp, j));
        }
    }
    m_scatter_graph->axisX()->setRange(0, result_dist_mat.rows);
    m_scatter_graph->axisY()->setRange(m_dist_min * 0.95, m_dist_max * 1.05);
    m_scatter_graph->axisZ()->setRange(0, result_dist_mat.cols);
    m_mat_mutex.unlock();

    m_scatter_graph->seriesList().at(0)->dataProxy()->resetArray(array);
}
