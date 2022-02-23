#ifndef DEMO_H
#define DEMO_H

#include <QSerialPort>
//#include <windows.h>

#include "joystick.h"
#include "threadpool.h"
#include "mywidget.h"
#include "imageproc.h"
#include "cam.h"
//#include "mvcam.h"
//#include "hqvscam.h"
//#include "euresyscam.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Demo; }
QT_END_NAMESPACE

class GrabThread : public QThread {
    Q_OBJECT
public:
    GrabThread(void *info);

protected:
    void run();

private:
    void *p_info;

signals:
    void stop_image_writing();
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

// enumerations only
public:
     enum PREF_TYPE {
         WIN_PREF = 0,
         TCU      = 1,
         SCAN     = 2,
         IMG      = 3,
         TCU_PREF = 4,
     };

public:
    Demo(QWidget *parent = nullptr);
    ~Demo();

    int grab_thread_process();
    bool is_maximized();

    // rename vid file in new thread
    static void move_to_dest(QString src, QString dst);
    static void save_image_bmp(cv::Mat img, QString filename);

public slots:
    // signaled by MouseThread
    void draw_cursor(QCursor c);

    // signaled by Titlebar button
    void switch_language();
    void screenshot();
    void clean();
    void export_config();
    void prompt_for_config_file();
    void load_config(QString config_name);

    // signaled in settings ui
    void setup_hz(int hz_unit);
    void setup_stepping(int base_unit);
    void setup_max_dist(int max_dist);
    void setup_laser(int laser_on);
    void set_serial_port_share(bool share);
    void set_baudrate(int idx, int baudrate);
    void com_write_data(int com_idx, QByteArray data);
    void display_baudrate(int idx);
    void set_auto_mcp(bool adaptive);
    void set_dev_ip(int ip, int gateway);
    void change_pixel_format(int pixel_format);

    // signaled by joystick input
    void joystick_button_pressed(int btn);
    void joystick_button_released(int btn);
    void joystick_direction_changed(int direction);

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
    // write original image
    void on_SAVE_BMP_BUTTON_clicked();
    // write result image
    void on_SAVE_RESULT_BUTTON_clicked();
    // record original video
    void on_SAVE_AVI_BUTTON_clicked();
    // record result video
    void on_SAVE_FINAL_BUTTON_clicked();

    // set & get curr device's params
    void on_SET_PARAMS_BUTTON_clicked();
    void on_GET_PARAMS_BUTTON_clicked();

    // customize the savepath through a file dialog
    void on_FILE_PATH_BROWSE_clicked();
    void on_FILE_PATH_EDIT_editingFinished();

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

    void on_GET_LENS_PARAM_BTN_clicked();
    // TODO rewrite auto focus function
    void on_AUTO_FOCUS_BTN_clicked();

    void on_LASER_ZOOM_IN_BTN_pressed();
    void on_LASER_ZOOM_OUT_BTN_pressed();
    void on_LASER_ZOOM_IN_BTN_released();
    void on_LASER_ZOOM_OUT_BTN_released();
    void laser_preset_reached();

    // slider-controlled slots
    void change_mcp(int val);
    void change_gain(int val);
    void change_delay(int val);
    void change_focus_speed(int val);

    // TODO add pause function
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

    // hide left parameter bar
    void on_HIDE_BTN_clicked();

    // change data display
    void on_COM_DATA_RADIO_clicked();
    void on_HISTOGRAM_RADIO_clicked();

    // choose how mouse works in DISPLAY
    void on_DRAG_TOOL_clicked();
    void on_SELECT_TOOL_clicked();

    // too much memory occupied (too many unfinished tasks in thread pool)
    void stop_image_writing();

    // hik cam binning mode 2x2
    void on_BINNING_CHECK_stateChanged(int arg1);

    // send customized data to ports
    void transparent_transmission_file(int id);

signals:
    // tell DATA_EXCHANGE (QTextEdit) to append data
    void append_text(QString text);

    // pause / continue scan
    void update_scan(bool show);

    // queue update_delay, mcp in thread
    void update_delay_in_thread();
    void update_mcp_in_thread(int new_mcp);

    // task queue full in thread pool
    void task_queue_full();

protected:
//  overload
    void keyPressEvent(QKeyEvent *event);
    void closeEvent(QCloseEvent *event);
    void resizeEvent(QResizeEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void showEvent(QShowEvent *event);

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
    // inline function for auto-focus
    void focus_far();
    void focus_near();

    // laser preset
    void set_laser_preset_target(int *pos);
    void goto_laser_preset(char target);

    // save img in buffer to file; or save imgs while scanning
    void save_to_file(bool save_result);
    void save_scan_img();

    // convert data to be sent to HEX buffer
    QByteArray convert_to_send_tcu(uchar num, unsigned int send);

    // update data to data-display; fb: whether reading feedback from com
    QByteArray generate_ba(uchar *data, int len);
    QByteArray communicate_display(QSerialPort *com, QByteArray write, int write_size, int read_size, bool fb);

    // update gate width
    void update_gate_width();

    // filter mat by brightness
    void filter_scan();

    // change current in laser
    void update_current();

    // write/read params to config file
    void convert_write(QDataStream &out, const int TYPE);
    bool convert_read(QDataStream &out, const int TYPE);

    // read device config by config
    void read_gatewidth_lookup_table(QFile *fp);
public:
    bool                    mouse_pressed;

private:
    Ui::Demo*               ui;

    int                     calc_avg_option;            // a: 5 frames; b: 10 frames
    double                  range_threshold;            // range threshold for building 3d images
    bool                    trigger_by_software;        // whether the device gets trigger signal from sw

    QMutex                  image_mutex;                // img handle lock
    QMutex                  port_mutex;                 // port handle lock
    Cam*                    curr_cam;                   // current camera
    float                   time_exposure_edit;
    float                   gain_analog_edit;
    float                   frame_rate_edit;
    QString                 save_location;              // where to save the image
    QString                 TEMP_SAVE_LOCATION;         // temp location to save the image
    cv::VideoWriter         vid_out[2];                 // video writer for ORI/RES

    QSerialPort*            com[4];                     // 0: tcu, 1: rangefinder, 2: lens, 3: laser
    bool                    share_serial_port;          // whether using a single comm for serial communication
    uchar                   out_buffer[7];              // write buffer for serial communication
    uchar                   in_buffer[7];               // read buffer for serial communication
    float                   rep_freq;
    int                     laser_width_u, laser_width_n;
    int                     gate_width_a_u, gate_width_a_n;
    int                     delay_a_u, delay_a_n, delay_b_u, delay_b_n, delay_n_n;
    float                   stepping;
    int                     hz_unit;
    int                     base_unit;
    int                     fps;
    int                     duty;
    int                     mcp;
    int                     laser_on;
    int                     zoom;
    int                     focus;
    int                     distance;                   // dist read from rangefinder
    float                   max_dist;
    int                     laser_width;
    float                   delay_dist;                 // estimated distance calculated from delay
    float                   depth_of_view;
    // TODO rewrite auto focus function
    int                     focus_direction;
    int                     clarity[3];
    char                    curr_laser_idx;

    int                     display_option;             // data display option: 1: com data; 2: histogram
    QButtonGroup            *display_grp;

    std::queue<cv::Mat>     img_q;                      // image queue in grab_thread
    // TODO add other scan features
    std::deque<float>       scan_q;                     // objects' distance found while scanning

    int                     device_type;                // 1: hik gige
    bool                    device_on;                  // whether curr device is on
    bool                    trigger_mode_on;            // whether img grabbing is on trigger mode
    bool                    start_grabbing;             // whether the device is rdy to grab imgs
    bool                    record_original;            // whether recording original stream
    bool                    record_modified;            // whether recording modified stream
    bool                    save_original;              // saving original bmp
    bool                    save_modified;              // saving modified bmp
    bool                    save_scan;
    bool                    image_3d;                   // whether to build a 3d image
    int                     trigger_source;             // where the device gets the trigger signal
    bool                    is_color;                   // display in mono8 or rgb8

    int                     w;                          // image width
    int                     h;                          // image height

    GrabThread*             h_grab_thread;              // img-grab thread handle
    bool                    grab_thread_state;          // whether thread is created
    MouseThread*            h_mouse_thread;             // draw the mouse icon
    JoystickThread*         h_joystick_thread;          // process joystick input

    cv::Mat                 img_mem;                    // right-side img display source (stream)
    cv::Mat                 modified_result;            // right-side img display modified (stream)
    cv::Mat                 cropped_img;                // right-side img display cropped (stream)
    cv::Mat                 prev_img;                   // previous original image
    cv::Mat                 prev_3d;                    // previous 3d image
    cv::Mat                 seq[10];                    // for frame average
    cv::Mat                 seq_sum;
    uint                    hist[256];                  // display histogram
    int                     seq_idx;                    // frame-average current index

    QLabel*                 com_label[4];               // for com communication
    QLineEdit*              com_edit[4];

    bool                    scan;                       // auto-scan for object detection
    float                   scan_distance;              // curr distance of scanning
    float                   scan_step;                  // stepping size when scanning
    float                   scan_stopping_delay;              // upper limit for scanning
    QString                 scan_name;

    float                   c;                          // light speed
    float                   dist_ns;                    // dist of light per ns
    bool                    frame_a_3d;                 // skip every second frame
    bool                    auto_mcp;                   // adaptive mcp

    bool                    hide_left;                  // whether left bar is hidden
    int                     resize_place;               // mouse position when resizing or at border
    QPoint                  prev_pos;                   // previous window position

    bool                    joybtn_A;                   // joystick button states
    bool                    joybtn_B;
    bool                    joybtn_X;
    bool                    joybtn_Y;
    bool                    joybtn_L1;
    bool                    joybtn_L2;
    bool                    joybtn_R1;
    bool                    joybtn_R2;

    bool                    en;                         // for language switching
    QTranslator             trans;

    ThreadPool              tp;

    int                     gw_lut[1000];               // lookup table for gatewidth config by serial

};
#endif // DEMO_H
