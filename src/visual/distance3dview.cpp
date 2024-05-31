#include "distance3dview.h"
#include "ui_distance3dview.h"

#include <Q3DCamera>

Distance3DView::Distance3DView(QWidget *parent, QWidget *signal_sender, std::vector<cv::Rect> *list_roi) :
    QWidget(parent),
    ui(new Ui::Distance3DView),
    signal_sender(signal_sender),
    list_roi(list_roi),
    scatter_graph(nullptr),
    downsample(4),
    consecutive(false),
    get_single_frame(false)
{
    ui->setupUi(this);

    connect(ui->CONSECUTIVE_CHK, &QCheckBox::stateChanged, this,
            [this](int arg1){ consecutive = arg1; ui->REFRESH_BTN->setEnabled(!arg1); ui->DOWNSAMPLE_LST->setEnabled(!arg1); });
    connect(ui->REFRESH_BTN, &QPushButton::clicked, this, [this](){ get_single_frame = true; });

    ui->H_ANGLE_SPIN->setMinimum(-180);
    ui->H_ANGLE_SPIN->setMaximum( 180);
    ui->H_ANGLE_SPIN->setSingleStep(5);
    ui->H_ANGLE_SPIN->setValue(-90);
    connect(ui->H_ANGLE_SPIN, static_cast<void (QSpinBox::*)(int val)>(&QSpinBox::valueChanged), this,
            [this](int val){ scatter_graph->scene()->activeCamera()->setXRotation(val); });
    ui->V_ANGLE_SPIN->setMinimum(-180);
    ui->V_ANGLE_SPIN->setMaximum( 180);
    ui->V_ANGLE_SPIN->setValue(90);
    ui->V_ANGLE_SPIN->setSingleStep(5);
    connect(ui->V_ANGLE_SPIN, static_cast<void (QSpinBox::*)(int val)>(&QSpinBox::valueChanged), this,
            [this](int val){ scatter_graph->scene()->activeCamera()->setYRotation(val); });

    ui->SCALE_SLIDER->setMinimum(10);
    ui->SCALE_SLIDER->setMaximum(500);
    ui->SCALE_SLIDER->setValue(100);
    connect(ui->SCALE_SLIDER, &QSlider::sliderMoved, this, [this](int pos){ scatter_graph->scene()->activeCamera()->setZoomLevel(pos); });

    ui->PROJECTION_LST->addItem("3D");
    ui->PROJECTION_LST->addItem("2D");
    ui->PROJECTION_LST->addItem("1D-H");
    ui->PROJECTION_LST->addItem("1D-V");
    ui->PROJECTION_LST->setCurrentIndex(1);
    connect(ui->PROJECTION_LST, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), this,
            [this](int index){ scatter_graph->setOrthoProjection(index); load_data(); });

    ui->DOWNSAMPLE_LST->addItem("1");
    ui->DOWNSAMPLE_LST->addItem("2");
    ui->DOWNSAMPLE_LST->addItem("4");
    ui->DOWNSAMPLE_LST->setCurrentIndex(2);
    connect(ui->DOWNSAMPLE_LST, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), this,
            [this](int index){ downsample = 1 << index; load_data(); });

    scatter_graph = new QtDataVisualization::Q3DScatter();

    QLayout *l = new QVBoxLayout(ui->DISTANCE_DISPLAY);
    l->addWidget(QWidget::createWindowContainer(scatter_graph));
    ui->DISTANCE_DISPLAY->setLayout(l);

    QtDataVisualization::QValue3DAxis *axis_x = new QtDataVisualization::QValue3DAxis(scatter_graph);
    axis_x->setTitle("X");
    axis_x->setLabelFormat("%d");
    axis_x->setRange(0, 1104);
    scatter_graph->setAxisX(axis_x);
    QtDataVisualization::QValue3DAxis *axis_y = new QtDataVisualization::QValue3DAxis(scatter_graph);
    axis_y->setTitle("DISTANCE");
    axis_y->setRange(60, 160);
    scatter_graph->setAxisY(axis_y);
    QtDataVisualization::QValue3DAxis *axis_z = new QtDataVisualization::QValue3DAxis(scatter_graph);
    axis_z->setTitle("Y");
    axis_z->setLabelFormat("%d");
    axis_z->setRange(0, 1608);
    scatter_graph->setAxisZ(axis_z);

    QtDataVisualization::QScatter3DSeries *series = new QtDataVisualization::QScatter3DSeries(scatter_graph);
    scatter_graph->addSeries(series);
    series->setMesh(QtDataVisualization::QAbstract3DSeries::MeshPoint);

    scatter_graph->scene()->activeCamera()->setWrapXRotation(true);
    scatter_graph->scene()->activeCamera()->setWrapYRotation(true);

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
    scatter_graph->setActiveTheme(theme);

    scatter_graph->scene()->activeCamera()->setXRotation(-90);
    scatter_graph->scene()->activeCamera()->setYRotation( 90);
    scatter_graph->setOrthoProjection(true);

    qRegisterMetaType<cv::Mat>("cv::Mat");
    connect(signal_sender, SIGNAL(update_dist_mat(cv::Mat, double, double)), this, SLOT(update_dist_mat(cv::Mat, double, double)));

    ui->DISTANCE_DISPLAY->setFocus();
}

Distance3DView::~Distance3DView()
{
    delete scatter_graph;
    delete ui;
}

void Distance3DView::update_dist_mat(cv::Mat new_dist_mat, double d_min, double d_max)
{
    if (!consecutive && !get_single_frame) return;
    if (mat_mutex.tryLock(10)) return;
    dist_mat = new_dist_mat.clone();
    mat_mutex.unlock();
    if (consecutive) {
        static QElapsedTimer t;
        if (t.isValid()) {
            if (t.elapsed() < 1000) return;
            t.restart();
        }
        else t.start();
    }
    dist_min = d_min;
    dist_max = d_max;
    load_data();
    if (get_single_frame) get_single_frame = false;
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
//        if (line_num++ & (downsample - 1)) continue;
//        item = line.split(',');
//        for (int i = 0; i < item.size(); i += downsample) array->append(QVector3D(line_num, item[i].toFloat(), i));
//    }
//    dist_csv.close();

    cv::Mat result_dist_mat;
    cv::Mat mask = cv::Mat::zeros(dist_mat.size(), CV_8UC1);
    switch (ui->PROJECTION_LST->currentIndex()) {
    case 0:
    case 1:
        mask = 255; break;
    case 2:
        for (cv::Rect roi: *list_roi) mask(cv::Rect(roi.x, roi.y + roi.height / 2, roi.width, 1)).setTo(255);
        break;
    case 3:
        for (cv::Rect roi: *list_roi) mask(cv::Rect(roi.x + roi.width / 2, roi.y, 1, roi.height)).setTo(255);
        break;
    default: break;
    }
    dist_mat.copyTo(result_dist_mat, mask);

    mat_mutex.lock();
    float *ptr = (float*)result_dist_mat.data;
    for (int i = 0; i < result_dist_mat.rows; i += downsample) {
        for (int j = 0; j < result_dist_mat.cols; j += downsample) {
            array->append(QVector3D(i, ptr[i * result_dist_mat.cols + j], j));
        }
    }
    scatter_graph->axisX()->setRange(0, result_dist_mat.rows);
    scatter_graph->axisY()->setRange(dist_min * 0.95, dist_max * 1.05);
    scatter_graph->axisZ()->setRange(0, result_dist_mat.cols);
    mat_mutex.unlock();

    scatter_graph->seriesList().at(0)->dataProxy()->resetArray(array);
}
