#ifndef USERPANEL_H
#define USERPANEL_H

#include "joystick.h"
#include "threadpool.h"
#include "imageproc.h"
#include "controlport.h"
#include "aliasing.h"

#include "preferences.h"
#include "scanconfig.h"
#include "lasersettings.h"
#include "distance3dview.h"
#include "plugininterface.h"

QT_BEGIN_NAMESPACE
namespace Ui { class UserPanel; }
QT_END_NAMESPACE

class GrabThread : public QThread {
    Q_OBJECT
public:
    GrabThread(void *info, int idx);

protected:
    void run();

private:
    void *p_info;
    int  display_idx;

signals:
    void stop_image_writing();
};

class UserPanel : public QMainWindow
{
    Q_OBJECT

// enumerations only
public:
     enum PREF_TYPE {
         WIN_PREF   = 0,
         TCU_PARAMS = 1,
         SCAN       = 2,
         IMG        = 3,
         TCU_PREF   = 4,
     };

public:
    UserPanel(QWidget *parent = nullptr);
    ~UserPanel();

    int grab_thread_process(int display_idx);
    bool is_maximized();

    // rename vid file in new thread
    static void move_to_dest(QString src, QString dst);

    // TODO movw image io to a new class
    // image i/o
    static void save_image_bmp(cv::Mat img, QString filename);
    static void save_image_tif(cv::Mat img, QString filename);
    static bool load_image_tif(cv::Mat &img, QString filename);

    // generate 3d image through scan result
    static void paint_3d();

public slots:
    // signaled by Titlebar button
    void set_theme();
    void switch_language();
    void screenshot();
    void clean();
    void export_config();
    void prompt_for_config_file();
    void load_config(QString config_name);
    void prompt_for_serial_file();
    void prompt_for_input_file();

    // signaled in settings ui
    void setup_hz(int hz_unit);
    void setup_stepping(int base_unit);
    void setup_max_dist(float max_dist);
    void update_delay_offset(float dist_offset);
    void update_gate_width_offset(float dov_offset);
    void update_laser_offset(float laser_offset);
    void setup_laser(int laser_on);
    void set_baudrate(int idx, int baudrate);
    void set_tcp_status_on_port(int idx, bool use_tcp);
    void set_tcu_as_shared_port(bool share);
    void com_write_data(int com_idx, QByteArray data);
    void display_port_info(int idx);
    void update_dev_ip();
    void set_dev_ip(int ip, int gateway);
    void change_pixel_format(int pixel_format, int display_idx = 0);
    void update_lower_3d_thresh();
    void reset_custom_3d_params();
    void save_current_video();
    void set_tcu_type(int idx);

    // signaled by joystick input
    void joystick_button_pressed(int btn);
    void joystick_button_released(int btn);
    void joystick_direction_changed(int direction);

    // signaled by ControlPort
    void append_data(QString str);
    void update_port_status(int connected_status);

    // signaled by Aliasing
    void set_distance_set(int id);

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

    void on_IRIS_OPEN_BTN_pressed();
    void on_IRIS_CLOSE_BTN_pressed();
    void on_IRIS_OPEN_BTN_released();
    void on_IRIS_CLOSE_BTN_released();
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
    void on_SCAN_CONFIG_BTN_clicked();

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

    // change misc. display
    void on_COM_DATA_RADIO_clicked();
    void on_ANALYSIS_RADIO_clicked();
    void on_PTZ_RADIO_clicked();

    // choose how mouse works in DISPLAY
    // TODO add a new exclusive button group
    void on_ZOOM_TOOL_clicked();
    void on_SELECT_TOOL_clicked();
    void on_PTZ_TOOL_clicked();

    // too much memory occupied (too many unfinished tasks in thread pool)
    void stop_image_writing();

    // hik cam binning mode 2x2
    void on_BINNING_CHECK_stateChanged(int arg1);

    // send customized data to ports
    void transparent_transmission_file(int id);

    // config optical gatewidth by file (serial number)
    void config_gatewidth(QString filename);

    void ptz_button_pressed(int id);
    void ptz_button_released(int id);
    void on_PTZ_SPEED_SLIDER_valueChanged(int value);
    void on_PTZ_SPEED_EDIT_editingFinished();
    void on_GET_ANGLE_BTN_clicked();
    void set_ptz_angle();
    void on_STOP_BTN_clicked();
    void point_ptz_to_target(QPoint target);

    void on_DUAL_LIGHT_BTN_clicked();

    void on_RESET_3D_BTN_clicked();

    void display_fishnet_result(int result);

    void on_FIRE_LASER_BTN_clicked();

    void on_IMG_REGION_BTN_clicked();

    void on_SWITCH_TCU_UI_BTN_clicked();

signals:
    // tell DATA_EXCHANGE (QTextEdit) to append data
    void append_text(QString text);

    // tell SOURCE_DISPLAY to display an image
    void set_pixmap(QPixmap pm);

    // pause / continue scan
    void update_scan(bool show);

    // queue update_delay, mcp in thread
    void update_delay_in_thread();
    void update_mcp_in_thread(int new_mcp);

    // task queue full in thread pool
    void task_queue_full();

    // local video (or stream) stopped playing
    void video_stopped();

    // update device list in preferences ui
    void update_device_list(int, QStringList);
    void device_connection_status_changed(int, QStringList);

    // update distance matrix in 3d view
    void update_dist_mat(cv::Mat, double, double);

#ifdef LVTONG
    // update fishnet result in thread
    void update_fishnet_result(int res);
#endif

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
    void setup_serial_port(QSerialPort **port, int id, QString port_num, int baud_rate);
    void setup_tcp_port(QTcpSocket **port);

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
    QByteArray communicate_display(int id, QByteArray write, int write_size, int read_size, bool fb);

    // update gate width
    void update_laser_width();
    void update_gate_width();

    // configure scan
    void filter_scan();
    void auto_scan_for_target();

    // change current in laser
    void update_current();

    // write/read params to config file
    void convert_write(QDataStream &out, const int TYPE);
    bool convert_read(QDataStream &out, const int TYPE);

    // read device config by config
    void read_gatewidth_lookup_table(QFile *fp);

    // connect to serial port using tcp socket
    void connect_to_serial_server_tcp();
    void disconnect_from_serial_server_tcp();

    // send ptz control cmd
    void send_ctrl_cmd(uchar dir);

    // static image display (drag & drop)
    void start_static_display(int width, int height, bool is_color, int display_idx = 0, int pixel_depth = 8, int device_type = -1);
    bool load_image_file(QString filename, bool init);
    int load_video_file(QString filename, bool format_gray = false, void (*process_frame)(cv::Mat &frame, void *ptr) = NULL,
                        void *ptr = NULL, int display_idx = 0, bool display = true);

    // status display
    void update_pixel_depth(int depth, int display_idx = 0);

    // tcu_type
    void update_tcu_param_pos(QLabel *u_unit, QLineEdit *n_input, QLabel *n_unit, QLineEdit *p_input);
    uint get_width_in_us(float val);
    uint get_width_in_ns(float val);
    uint get_width_in_ps(float val);

public:
    bool                    mouse_pressed;
    std::vector<cv::Rect>   list_roi;                   // user-selected roi

private:
    Ui::UserPanel*          ui;

    StatusBar*              status_bar;
    Preferences*            pref;
    ScanConfig*             scan_config;
    LaserSettings*          laser_settings;
    Distance3DView*         view_3d;
    Aliasing*               aliasing;
    FloatingWindow*         fw[2];

    int                     calc_avg_option;            // a: 4 frames; b: 8 frames
    bool                    trigger_by_software;        // whether the device gets trigger signal from sw

    QMutex                  image_mutex[3];             // img handle lock
    QMutex                  port_mutex;                 // port handle lock
    QMutex                  display_mutex[3];           // display handle lock
    Cam*                    curr_cam;                   // current camera
    float                   time_exposure_edit;
    float                   gain_analog_edit;
    float                   frame_rate_edit;
    QString                 save_location;              // where to save the image
    QString                 TEMP_SAVE_LOCATION;         // temp location to save the image
    cv::VideoWriter         vid_out[3];                 // video writer for ORI/RES/export
    QString                 current_video_filename;     // name of the imported video file (if not a stream)
    QString                 output_filename;            // target output name when exporting video
    QString                 temp_output_filename;       // temp save location of target output file

    TCU*                    ptr_tcu;
    Inclin*                 ptr_inc;
    Lens*                   ptr_lens;
    Laser*                  ptr_laser;
    PTZ*                    ptr_ptz;

    // WARNING port communication mostly moved to new class ControlPort
    QSerialPort*            serial_port[5];             // 0: tcu, 1: rangefinder, 2: lens, 3: laser, 4: PTZ
    bool                    serial_port_connected[5];
    QTcpSocket*             tcp_port[5];                // 0-1: 232, 2-4: 485
    bool                    use_tcp[5];
    bool                    share_serial_port;          // whether using a single comm for serial communication
    float                   stepping;
    int                     hz_unit;
    int                     base_unit;
    int                     laser_on;
    uint                    zoom;
    uint                    focus;
    int                     distance;                   // dist read from rangefinder
    float                   rep_freq;
    float                   laser_width;
    float                   delay_dist;                 // estimated distance calculated from delay
    float                   depth_of_view;              // depth of view from gate width
    bool                    aliasing_mode;
    int                     aliasing_level;
    int                     focus_direction;
    int                     clarity[3];
    char                    curr_laser_idx;

    int                     display_option;             // data display option: 1: com data; 2: histogram
    QButtonGroup*           display_grp;

    std::queue<cv::Mat>     q_img[3];                   // image queue in grab_thread
    std::queue<int>         q_frame_info;
//    bool                    updated[3];                 // whether the program get a new image from stream
    // TODO add other scan features
    std::vector<TCUDataGroup> q_scan;                   // targets' tcu param found while scanning

    int                     device_type;                // 1: hik gige
    bool                    device_on;                  // whether curr device is on
    bool                    trigger_mode_on;            // whether img grabbing is on trigger mode
    bool                    start_grabbing;             // whether the device is rdy to grab imgs
    bool                    record_original[3];         // whether recording original stream
    bool                    record_modified[3];         // whether recording modified stream
    bool                    save_original[3];           // saving original bmp
    bool                    save_modified[3];           // saving modified bmp
    bool                    image_3d[3];                // whether to build a 3d image
    int                     trigger_source;             // where the device gets the trigger signal
    bool                    is_color[3];                // display in mono8 or rgb8

    int                     w[3];                       // image width
    int                     h[3];                       // image height
    int                     pixel_format[3];            // for hik cam, use mono 8 for others
    int                     pixel_depth[3];             // pixel depth

    Display*                displays[3];                // 3 set of display widgets
    bool                    grab_image[3];              // whether thread should continue grabbing image
    GrabThread*             h_grab_thread[3];           // img-grab thread handle
    bool                    grab_thread_state[3];       // whether grabbing thread is on
    bool                    video_thread_state;         // whether video reading thread is on
    JoystickThread*         h_joystick_thread;          // process joystick input

    cv::Mat                 img_mem[3];                 // right-side img display source (stream)
    cv::Mat                 modified_result[3];         // right-side img display modified (stream)
//    cv::Mat                 img_display[3];             // right-side img display cropped (stream)
//    cv::Mat                 prev_img[3];                // previous original image
//    cv::Mat                 prev_3d[3];                 // previous 3d image
//    cv::Mat                 seq[3][8];                  // for frame average
//    cv::Mat                 seq_sum[3];
//    cv::Mat                 frame_a_sum[3];
//    cv::Mat                 frame_b_sum[3];
//    uint                    hist[3][256];               // display histogram
//    cv::Mat                 hist_mat[3];
//    int                     seq_idx[3];                 // frame-average current index
//    cv::Mat                 dist_mat[3];
    cv::Mat                 user_mask[3];

    QLabel*                 com_label[5];               // for com communication
    QLineEdit*              com_edit[5];

    bool                    auto_scan_mode;             // search for target
    bool                    scan;                       // auto-scan for object detection
    float                   scan_distance;              // curr distance of scanning
    float                   scan_step;                  // stepping size when scanning
    float                   scan_stopping_delay;        // upper limit for scanning
    QString                 scan_name;
    cv::Mat                 scan_3d;
    cv::Mat                 scan_sum;
    int                     scan_idx;

    float                   c;                          // light speed
    float                   dist_ns;                    // dist of light per ns
    bool                    frame_a_3d;                 // skip every second frame
    bool                    auto_mcp;                   // adaptive mcp
    bool                    multi_laser_lenses;         // send additional data to lens com if true

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
    uint                    lens_adjust_ongoing;
    bool                    ptz_adjust_ongoing;

    uchar                   lang;                       // 0: en_us, 1: zh_cn
    QTranslator             trans;

    ThreadPool              tp;

    QButtonGroup*           ptz_grp;                    // ptz button group
    int                     ptz_speed;
    float                   angle_h;
    float                   angle_v;

//    QMediaPlayer*           video_input;
//    VideoSurface*           video_surface;

    // TEMP ONLY
    // TODO move to addons
    PluginInterface*        pluginInterface;            // for ir with visible light

};
#endif // USERPANEL_H
