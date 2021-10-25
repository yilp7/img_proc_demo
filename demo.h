#ifndef DEMO_H
#define DEMO_H

#include <QMainWindow>
#include <QComboBox>
#include <QThread>
#include <QMutex>
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDateTime>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QTimer>
#include <QDebug>
#include <QEvent>
#include <QTranslator>
#include <QTableWidget>
#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <windows.h>

#include "imageproc.h"
//#include "mvcam.h"
//#include "hqvscam.h"
#include "euresyscam.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Demo; }
QT_END_NAMESPACE

class GrabThread : public QThread {
public:
    GrabThread(void *info);

protected:
    void run();

private:
    void *p_info;
};

class MouseThread : public QThread {
    Q_OBJECT
public:
    MouseThread(void *info);

protected:
    void run();

private:
    void   *p_info;
    QTimer *t;

private slots:
    void draw_cursor();

signals:
    void set_cursor(QCursor);

};

class Demo : public QMainWindow
{
    Q_OBJECT

public:
    Demo(QWidget *parent = nullptr);
    ~Demo();

public:
    int grab_thread_process();
    bool is_maximized();

public slots:
    void draw_cursor(QCursor c);
    void switch_language();
    void screenshot();
    void clean();
    void setup_stepping(bool in_ns);

private slots:
    // on clicking enum btn: enumerate devices
    void on_ENUM_BUTTON_clicked();

    // start / shutdown the device
    void on_START_BUTTON_clicked();
    void on_SHUTDOWN_BUTTON_clicked();

    // set img grabbing mode
    void on_CONTINUOUS_RADIO_clicked();
    void on_TRIGGER_RADIO_clicked();
    void on_SOFTWARE_CHECK_stateChanged(int arg1);

    // manually grab one frame
    void on_SOFTWARE_TRIGGER_BUTTON_clicked();

    // start or end grabbing
    void on_START_GRABBING_BUTTON_clicked();
    void on_STOP_GRABBING_BUTTON_clicked();

    // save the current image / start recording videos
    void on_SAVE_BMP_BUTTON_clicked();
    void on_SAVE_RESULT_BUTTON_clicked();
    void on_SAVE_AVI_BUTTON_clicked();
    void on_SAVE_FINAL_BUTTON_clicked();

    // set & get curr device's params
    void on_SET_PARAMS_BUTTON_clicked();
    void on_GET_PARAMS_BUTTON_clicked();
    void on_GAIN_EDIT_textEdited(const QString &arg1);

    // customize the savepath through a file dialog
    void on_FILE_PATH_BROWSE_clicked();

    // update distance, repeated freq, delay and gate width
    void on_DIST_BTN_clicked();

    // start to build 3d images and update display region
    void on_IMG_3D_CHECK_stateChanged(int arg1);
    void on_FRAME_AVG_CHECK_stateChanged(int arg1);

    // lens control functions
    void on_ZOOM_IN_BTN_pressed();
    void on_ZOOM_IN_BTN_released();
    void on_ZOOM_OUT_BTN_pressed();
    void on_ZOOM_OUT_BTN_released();
    void on_FOCUS_NEAR_BTN_pressed();
    void on_FOCUS_NEAR_BTN_released();
    void on_FOCUS_FAR_BTN_pressed();
    void on_FOCUS_FAR_BTN_released();
    void on_FOCUS_SPEED_EDIT_textEdited(const QString &arg1);

    void on_GET_LENS_PARAM_BTN_clicked();
    void on_AUTO_FOCUS_BTN_clicked();

    void on_LASER_ZOOM_IN_BTN_pressed();
    void on_LASER_ZOOM_OUT_BTN_pressed();
    void on_LASER_ZOOM_IN_BTN_released();
    void on_LASER_ZOOM_OUT_BTN_released();

    // slider-controlled slots
    void change_mcp(int val);
    void change_gain(int val);
    void change_delay(int val);
    void change_focus_speed(int val);

    // process scan
    void on_SCAN_BUTTON_clicked();
    void on_CONTINUE_SCAN_BUTTON_clicked();
    void on_RESTART_SCAN_BUTTON_clicked();

    // append data to DATA_EXCHANGE
    void append_data(QString text);

    // enable continue & restart button when scan paused
    void enable_scan_options(bool show);

    // update delay, rep_freq
    void update_delay();

    // start laser & initialize laser stat
    void on_LASER_BTN_clicked();
    void start_laser();
    void init_laser();

    void on_HIDE_BTN_clicked();

    void on_COM_DATA_RADIO_clicked();
    void on_HISTOGRAM_RADIO_clicked();

    void on_DRAG_TOOL_clicked();
    void on_SELECT_TOOL_clicked();

    void on_ENHANCE_OPTIONS_currentIndexChanged(int index);

signals:
    // tell DATA_EXCHANGE (QTextEdit) to append data
    void append_text(QString text);

    // pause / continue scan
    void update_scan(bool show);

    // queue update_delay in thread
    void update_delay_in_thread();

protected:
//  overload
    void keyPressEvent(QKeyEvent *event);
    void closeEvent(QCloseEvent *event);
    void resizeEvent(QResizeEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);

// control functions
private:
    void data_exchange(bool read);

    // shut the cam down
    int shut_down();

    // update activated / deactivated buttons
    void enable_controls(bool cam_rdy);

    // attempt to communicate with 4 COMs
    void setup_com(QSerialPort **com, int id, QString port_num, int baud_rate);

    // stop lens operation (focus / zoom)
    void lens_stop();

    // set zoom & focus value
    void set_zoom();
    void set_focus();
    inline void focus_far();
    inline void focus_near();

// helper function
private:
    // save img in buffer to file; or save imgs while scanning
    void save_to_file(bool save_result);
    void save_scan_img();

    // convert data to be sent to HEX buffer
    void convert_to_send_tcu(uchar num, unsigned int send);

    // update data to data-display; fb: whether feedback is needed
    void communicate_display(QSerialPort *com, int receive_size, int send_size, bool fb);

    // update gate width
    void update_gate_width();

    // filter mat by brightness
    void filter_scan();

    // change current in laser
    void update_current();


private:
    Ui::Demo *ui;

// controlling variables
private:
    int                     calc_avg_option;
    double                  range_threshold;            // range threshold for building 3d images
    bool                    trigger_by_software;        // whether the device gets trigger signal from sw

    QMutex                  save_img_mux;               // img handle lock
    Cam*                    curr_cam;                   // current camera
//    MvCam*                  curr_cam;                   // current camera
    float                   time_exposure_edit;
    float                   gain_analog_edit;
    float                   frame_rate_edit;
    QString                 save_location;              // where to save the image
    QString                 TEMP_SAVE_LOCATION;      // temp location to save the image
    cv::VideoWriter         vid_out[2];

    QSerialPort*            com[4];                     // 0: tcu, 1: range, 2: focus, 3: thermo
    uchar                   out_buffer[7];
    uchar                   in_buffer[7];
    int                     rep_freq;
    int                     laser_width_u, laser_width_n;
    int                     gate_width_a_u, gate_width_a_n;
    int                     delay_a_u, delay_a_n, delay_b_u, delay_b_n, delay_n_n;
    float                   stepping;
    bool                    stepping_in_ns;
    int                     fps;
    int                     duty;
    int                     mcp;
    int                     zoom;
    int                     focus;
    int                     distance;
    float                   delay_dist;
    float                   depth_of_vision;
    int                     focus_direction;
    int                     clarity[3];

    int                     display_option;

    std::queue<cv::Mat>     img_q;                      // image queue in grab_thread
    std::deque<float>       scan_q;                     // objects' distance found while scanning

// info variables
private:
    bool                    device_on;                  // whether curr device is on
    bool                    trigger_mode_on;            // whether img grabbing is on trigger mode
    bool                    start_grabbing;             // whether the device is rdy to grab imgs
    bool                    record_original;            // whether recording original stream
    bool                    record_modified;            // whether recording modified stream
    bool                    save_original;              // saving original bmp
    bool                    save_modified;              // saving modified bmp
    bool                    image_3d;                   // whether to build a 3d image
    int                     trigger_source;             // where the device gets the trigger signal

    int                     w;
    int                     h;

    GrabThread*             h_grab_thread;              // img-grab thread handle
    bool                    grab_thread_state;          // whether thread is created
    MouseThread*            h_mouse_thread;             // process mouse img

    cv::Mat                 img_mem;                    // right-side img display source (stream)
    cv::Mat                 modified_result;            // right-side img display modified (stream)
    cv::Mat                 cropped_img;                // right-side img display cropped (stream)
    cv::Mat                 prev_img;                   // previous original image
    cv::Mat                 prev_3d;                    // previous 3d image
    cv::Mat                 seq[10];                    // for frame average
    cv::Mat                 seq_sum;
    cv::Mat                 accu[5];                    // for accumulation process
    cv::Mat                 accu_sum;
    uint                    hist[256];                  // display histogram
    int                     seq_idx;                    // frame-average current index
    int                     accu_idx;                   // accumulation current index

    QLabel*                 com_label[4];               // com communication
    QLineEdit*              com_edit[4];

    bool                    scan;                       // auto-scan for object detection
    float                   scan_distance;              // curr distance of scanning
    float                   scan_step;                  // stepping size when scanning
    float                   scan_farthest;              // upper limit for scanning
    QString                 scan_name;

    float                   c;                          // light speed
    float                   dist_ns;                    // dist of light per ns
    bool                    frame_a_3d;                 // skip every second frame

    bool                    hide_left;
    int                     resize_place;
    QPoint                  prev_pos;

    bool                    en;
    QTranslator             trans;

public:
    bool                    mouse_pressed;

};
#endif // DEMO_H
