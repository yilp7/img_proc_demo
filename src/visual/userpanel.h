#ifndef USERPANEL_H
#define USERPANEL_H

//#include "cam.h"
#include "cam/mvcam.h"
#ifdef WIN32
#include "cam/ebuscam.h"
#endif
#ifdef USING_CAMERALINK
#include "cam/euresyscam.h"
#endif //USING_CAMERALINK
//#include "hqvscam.h"
//#include "euresyscam.h"

#include "image/imageio.h"
#include "image/imageproc.h"

#include "visual/preferences.h"
#include "visual/scanconfig.h"
#include "visual/lasercontrol.h"
#ifdef DISTANCE_3D_VIEW
#include "visual/distance3dview.h"
#endif //DISTANCE_3D_VIEW
#include "visual/aliasing.h"
#include "visual/presetpanel.h"
#include "visual/serialserver.h"
#include "util/config.h"
#include "automation/autoscan.h"

#include "port/tcu.h"
#include "port/lens.h"
#include "port/laser.h"
//#include "port/inclin.h"
#include "port/ptz.h"
#include "port/rangefinder.h"
#include "port/usbcan.h"
#include "port/udpptz.h"

#include "thread/joystick.h"
#include "thread/controlportthread.h"
#include "util/threadpool.h"
#include "plugins/plugininterface.h"

QT_BEGIN_NAMESPACE
namespace Ui { class UserPanel; }
QT_END_NAMESPACE

class GrabThread : public QThread {
    Q_OBJECT
public:
    GrabThread(void *info, int idx);
    ~GrabThread();

    void display_idx(bool read, int &idx);

protected:
    void run();

private:
    void *p_info;
    int  _display_idx;

signals:
    void stop_image_writing();
};

class TimedQueue : public QThread {
    Q_OBJECT
public:
    TimedQueue(double expiration_time);
    ~TimedQueue();

    void set_display_label(StatusIcon *lbl);
    void add_to_q();
    void empty_q();
    int count();
    void stop();

protected:
    void run();

private:
    QMutex mtx;
    bool _quit;
    double expiration_time;
    std::queue<struct timespec> q;
    StatusIcon* fps_status;
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

    void init();

    // Auto-scan support
    void set_command_line_args(const QStringList& args);
    void set_auto_scan_controller(class AutoScan* autoScan);

    int grab_thread_process(int *display_idx);
    void swap_grab_thread_display(int display_idx1, int display_idx2);
    bool is_maximized();

    // rename vid file in new thread
    static void move_to_dest(QString src, QString dst);

    // generate 3d image through scan result
    static void paint_3d();

    // 4-Camera System Methods
    bool initialize_camera(int cam_idx, int device_idx);  // Initialize camera at index
    bool start_camera(int cam_idx);                        // Start specific camera
    bool stop_camera(int cam_idx);                         // Stop specific camera
    bool get_camera_frame(int cam_idx, cv::Mat& frame);   // Get latest frame from camera queue
    void enable_four_camera_mode(bool enable);             // Enable/disable 4-camera composite mode
    bool is_four_camera_mode() const { return four_camera_mode; }

public slots:
    // signaled by Titlebar button
    void set_theme();
    void switch_language();
    void screenshot();
    void clean();
    void save_config_to_file();
    void prompt_for_config_file();
    void load_config(QString config_name);
    void generate_preset_data(int idx = -1);
    void apply_preset(nlohmann::json preset_data);
    void prompt_for_serial_file();
    void prompt_for_input_file();

    // signaled in preferences ui
    void search_for_devices();
    void rotate(int angle);
    void update_light_speed(bool uw);
    void setup_hz(int hz_unit);
    void setup_stepping(int base_unit);
    void setup_max_dist(float max_dist);
    void setup_max_dov(float max_dist);
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
    void export_current_video();
    void set_tcu_type(int idx);
    void update_ps_config(bool read, int idx, uint val);
    void set_auto_mcp(bool auto_mcp);

    // signaled by joystick input
    void joystick_button_pressed(int btn);
    void joystick_button_released(int btn);
    void joystick_direction_changed(int direction);

    // signaled by ControlPort
    void update_port_status(ControlPort *port, QLabel *lbl = nullptr);
    void append_data(QString str);
    void update_port_status(int connected_status);
    void update_lens_params(qint32 lens_param, uint val);
    void update_ptz_params(qint32 ptz_param, double val);
    void update_distance(double distance);
    void update_usbcan_angle(float _h, float _v);
    void update_udpptz_angle(float _h, float _v);
    void update_ptz_status();

    // signaled by Aliasing
    void set_distance_set(int id);

    // ui switching
    void switch_ui();

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
    void on_FOCUS_FAR_BTN_pressed();
    void on_FOCUS_FAR_BTN_released();
    void on_FOCUS_NEAR_BTN_pressed();
    void on_FOCUS_NEAR_BTN_released();
    void on_RADIUS_INC_BTN_pressed();
    void on_RADIUS_INC_BTN_released();
    void on_RADIUS_DEC_BTN_pressed();
    void on_RADIUS_DEC_BTN_released();

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
    void change_gatewidth(int val);
    void change_focus_speed(int val);
    void change_lens_address(int val);

    // TODO add pause function
    // process scan
    void on_SCAN_BUTTON_clicked();
    void on_CONTINUE_SCAN_BUTTON_clicked();
    void on_RESTART_SCAN_BUTTON_clicked();
    void on_SCAN_CONFIG_BTN_clicked();

    // enable continue & restart button when scan paused
    void enable_scan_options(bool show);

    // update delay, rep_freq, gate width, laser_width and their widgets
    void update_laser_width();
    void update_delay();
    void update_gate_width();
    void update_tcu_params(qint32 tcu_param);

    // start laser & initialize laser stat
    void on_LASER_BTN_clicked();
    void start_laser();
    void init_laser();

    // hide left parameter bar
    void on_HIDE_BTN_clicked();
#if 0
    // change misc. display
    void on_COM_DATA_RADIO_clicked();
    void on_PTZ_RADIO_clicked();
    void on_PLUGIN_RADIO_clicked();
#endif
    // change alt display content
    void on_MISC_RADIO_1_clicked();
    void on_MISC_RADIO_2_clicked();
    void on_MISC_RADIO_3_clicked();
    void on_MISC_OPTION_1_currentIndexChanged(int index);
    void on_MISC_OPTION_2_currentIndexChanged(int index);
    void on_MISC_OPTION_3_currentIndexChanged(int index);

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

    void alt_display_control(int cmd);

    void on_DUAL_LIGHT_BTN_clicked();

    void on_RESET_3D_BTN_clicked();

    void display_fishnet_result(int result);

    void on_FIRE_LASER_BTN_clicked();

    void on_IMG_REGION_BTN_clicked();
    void on_SENSOR_TAPS_BTN_clicked();
    void on_SWITCH_TCU_UI_BTN_clicked();
    void on_SIMPLE_LASER_CHK_clicked();
    void on_AUTO_MCP_CHK_clicked();
    void on_delay_select_changed(int index);

    void on_PSEUDOCOLOR_CHK_stateChanged(int arg1);

signals:
    // tell DATA_EXCHANGE (QTextEdit) to append data
    void append_text(QString text);

    // tell SOURCE_DISPLAY to display an image
    void set_src_pixmap(QPixmap pm);
    void set_hist_pixmap(QPixmap pm);

    // pause / continue scan
    void update_scan(bool show);

    // signal that initialization is complete
    void initialization_complete();

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

    // send a preset data copy to preset panel
    void save_to_preset(int idx, nlohmann::json preset);

    // notify controlports
    void send_data(QByteArray data, uint read_size = 0, uint read_timeout = 100);
    void send_double_tcu_msg(qint32 tcu_param, double val);
    void send_uint_tcu_msg(qint32 tcu_param, uint val);
    void send_lens_msg(qint32 lens_param, uint val = 0);
    void set_lens_pos(qint32 lens_param, uint val);
    void send_laser_msg(QString msg);
    void send_ptz_msg(qint32 ptz_param, double val = 0);

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
    void update_delay_selector_ui();  // Update delay selector combo and slider based on four_camera_mode

    // shut the cam down
    int shut_down();

    // update activated / deactivated buttons
    void enable_controls(bool cam_rdy);

    // attempt to communicate with 4 COMs
    void init_control_port();
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
    void save_to_file(bool save_result, int idx);
    void save_scan_img(QString path, QString name);

    // convert data to be sent to HEX buffer
    QByteArray convert_to_send_tcu(uchar num, unsigned int send);

    // update data to data-display; fb: whether reading feedback from com
    QByteArray generate_ba(uchar *data, int len);
    QByteArray communicate_display(int id, QByteArray write, int write_size, int read_size, bool fb);

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

    // config sync functions
    void syncConfigToPreferences();
    void syncPreferencesToConfig();

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
    void update_tcu_param_pos(int dir, QLabel *u_unit, QLineEdit *n_input, QLabel *n_unit, QLineEdit *p_input);
    void split_value_by_unit(float val, uint &us, uint &ns, uint &ps, int idx = -1);
//    uint get_width_in_us(float val);
//    uint get_width_in_ns(float val);
//    uint get_width_in_ps(float val, int idx = -1);

public:
    bool                  mouse_pressed;
    std::vector<cv::Rect> list_roi;                   // user-selected roi

private:
    Ui::UserPanel*  ui;
    StatusBar*      status_bar;
    Preferences*    pref;
    ScanConfig*     scan_config;
    LaserControl*   laser_settings;
    Config*         config;
#ifdef DISTANCE_3D_VIEW
    Distance3DView* view_3d;
#endif //DISTANCE_3D_VIEW
    Aliasing*       aliasing;
    PresetPanel*    preset;
    FloatingWindow* fw_display[2];
    FloatingWindow* secondary_display;

    bool simple_ui;

    int             calc_avg_option;            // a: 4 frames; b: 8 frames
    bool            trigger_by_software;        // whether the device gets trigger signal from sw
    QMutex          image_mutex[3];             // img handle lock
    QMutex          port_mutex;                 // port handle lock
    QMutex          display_mutex[3];           // display handle lock
    QMutex          frame_info_mutex;           // q_frame_info queue lock (shared across threads)
    // 4-Camera Management
    Cam*            cameras[4];                 // array of 4 camera pointers (cam-1 to cam-4)
    std::queue<cv::Mat> camera_queues[4];       // camera-specific image queues
    std::queue<int>     camera_frame_info[4];   // camera-specific frame info
    QMutex          camera_mutexes[4];          // camera-specific mutexes
    QMutex          camera_frame_mutexes[4];    // camera frame info mutexes
    bool            camera_active[4];           // whether camera is active
    struct main_ui_info cam_unions[4];          // camera callback structures
    cv::Mat         composite_frame;            // Composite frame for 4-camera display
    cv::Mat         camera_latest[4];           // Latest frame from each camera
    int             composite_gap_size;         // Gap size between camera views in pixels
    bool            four_camera_mode;           // Whether 4-camera composite mode is active

    Cam*            curr_cam;                   // current camera (kept for compatibility)
    float           time_exposure_edit;
    float           gain_analog_edit;
    float           frame_rate_edit;
    QString         save_location;              // where to save the image
    QString         TEMP_SAVE_LOCATION;         // temp location to save the image
    cv::VideoWriter vid_out[4];                 // video writer for ORI/RES/alt-display1/2
    cv::VideoWriter vid_out_proc;               // video writer for export
    QString         current_video_filename;     // name of the imported video file (if not a stream)
    QString         output_filename;            // target output name when exporting video
    QString         temp_output_filename;       // temp save location of target output file

    // TODO: implement by inheriting QThread
//    TCUThread*    ptr_tcu;
//    InclinThread* ptr_inc;
//    LensThread*   ptr_lens;
//    LaserThread*  ptr_laser;
//    PTZThread*    ptr_ptz;
    // implemented thriugh moveToThread
    TCU     *p_tcu;
    Lens    *p_lens;
    Laser   *p_laser;
    PTZ     *p_ptz;
    RangeFinder *p_rf;
    USBCAN  *p_usbcan;
    UDPPTZ  *p_udpptz;
    QThread *th_tcu;
    QThread *th_lens;
    QThread *th_laser;
//    QThread *th_inclin;
    QThread *th_ptz;
    QThread *th_rf;
    QThread *th_usbcan;
    QThread *th_udpptz;

    // WARNING port communication mostly moved to new class ControlPort
    QSerialPort*  serial_port[5];           // 0: tcu, 1: rangefinder, 2: lens, 3: laser, 4: PTZ
    bool          serial_port_connected[5];
    QTcpSocket*   tcp_port[5];              // 0-1: 232, 2-4: 485
    bool          use_tcp[5];
    bool          share_serial_port;        // whether using a single comm for serial communication
    float         stepping;
    int           hz_unit;
    int           base_unit;
    int           laser_on;
    uint          zoom;
    uint          focus;
    int           distance;                 // dist read from rangefinder (or manually set)
    float         rep_freq;
    float         laser_width;
    float         delay_dist;               // estimated distance calculated from delay
    float         depth_of_view;            // depth of view from gate width
    int           mcp_max;
    bool          aliasing_mode;
    int           aliasing_level;
    int           focus_direction;
    int           clarity[3];
    char          curr_laser_idx;
    int           alt_display_option;         // data display option: 1: com data; 2: histogram; 3: PTZ; 4: addons
    QButtonGroup* display_grp;

    std::queue<cv::Mat>        q_img[3];                   // image queue in grab_thread
    std::queue<int>            q_frame_info;
    TimedQueue                 q_fps_calc;
//    bool                    updated[3];                 // whether the program get a new image from stream
    // TODO add other scan features
    std::vector<TCUDataGroup>  q_scan;                   // targets' tcu param found while scanning

    int  device_type;        // 1: hik gige
    bool device_on;          // whether curr device is on
    bool trigger_mode_on;    // whether img grabbing is on trigger mode
    bool start_grabbing;     // whether the device is rdy to grab imgs
    bool record_original[3]; // whether recording original stream
    bool record_modified[3]; // whether recording modified stream
    bool save_original[3];   // saving original bmp
    bool save_modified[3];   // saving modified bmp
    bool image_3d[3];        // whether to build a 3d image
    int  image_rotate[3];    // rotate image ccw
    int  trigger_source;     // where the device gets the trigger signal
    bool is_color[3];        // source in grayscale or color
    bool pseudocolor[3];     // apply pseudocolor
    int  w[3];               // image width
    int  h[3];               // image height
    int  pixel_format[3];    // for hik cam, use mono 8 for others
    int  pixel_depth[3];     // pixel depth

    Display*        displays[3];                // 3 set of display widgets
    int             display_thread_idx[3];      // idx of the thread displaying in display_i
    bool            grab_image[3];              // whether thread should continue grabbing image
    GrabThread*     h_grab_thread[3];           // img-grab thread handle
    bool            grab_thread_state[3];       // whether grabbing thread is on
    bool            video_thread_state;         // whether video reading thread is on
    JoystickThread* h_joystick_thread;          // process joystick input

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
    std::vector<std::pair<float, float>> scan_ptz_route;
    std::vector<float>      scan_tcu_route;
    int                     scan_ptz_idx;
    int                     scan_tcu_idx;

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

    QButtonGroup*           alt_ctrl_grp;               // alt display control button group

//    QMediaPlayer*           video_input;
//    VideoSurface*           video_surface;

    // TEMP ONLY
    // TODO move to addons
    PluginInterface*        pluginInterface;            // for ir with visible light

    // Auto-scan support
    QStringList             m_command_line_args;
    class AutoScan*         m_auto_scan_controller;

};
#endif // USERPANEL_H
