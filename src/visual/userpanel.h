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
// port headers included via controller/devicemanager.h
#include "controller/devicemanager.h"
#include "controller/tcucontroller.h"
#include "controller/lenscontroller.h"
#include "controller/lasercontroller.h"
#include "controller/rfcontroller.h"
#include "controller/scancontroller.h"
#include "controller/auxpanelmanager.h"

#include "widgets/mywidget.h"
#include "thread/joystick.h"
#include "util/threadpool.h"
#include "plugins/plugininterface.h"
#include "yolo/yolo_detector.h"
#include "pipeline/processingparams.h"
#include "pipeline/pipelineconfig.h"

QT_BEGIN_NAMESPACE
namespace Ui { class UserPanel; }
QT_END_NAMESPACE

class UserPanel;

class GrabThread : public QThread {
    Q_OBJECT
public:
    GrabThread(UserPanel *panel, int idx);
    ~GrabThread();

    void display_idx(bool read, int &idx);

protected:
    void run();

private:
    UserPanel *m_panel;
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

// Persistent frame-averaging state passed to extracted pipeline methods
struct FrameAverageState {
    cv::Mat seq[8], seq_sum, frame_a_sum, frame_b_sum;
    cv::Mat prev_img, prev_3d;
    int seq_idx = 0;
    bool frame_a = true;
    void release_avg_buffers() {
        seq_sum.release(); frame_a_sum.release(); frame_b_sum.release();
        for (auto& m : seq) m.release();
        seq_idx = 0;
    }
    void release_all() {
        prev_img.release(); prev_3d.release();
        release_avg_buffers();
    }
};

// Persistent ECC temporal denoising state
struct ECCState {
    std::deque<cv::Mat> fusion_buf, fusion_warps, fusion_warps_inv;
    cv::Mat fusion_last_warp;
    int prev_ecc_warp_mode = -1;
    void clear() {
        fusion_buf.clear(); fusion_warps.clear(); fusion_warps_inv.clear();
        fusion_last_warp.release();
    }
};

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

    int grab_thread_process(int *display_idx);
    void swap_grab_thread_display(int display_idx1, int display_idx2);
    bool is_maximized();

    // rename vid file in new thread
    static void move_to_dest(QString src, QString dst);

    // generate 3d image through scan result
    static void paint_3d();

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
    // TCU-related slots delegated to TCUController
    void update_light_speed(bool uw) { m_tcu_ctrl->update_light_speed(uw); }
    void setup_hz(int hz_unit) { m_tcu_ctrl->setup_hz(hz_unit); }
    void setup_stepping(int base_unit) { m_tcu_ctrl->setup_stepping(base_unit); }
    void setup_max_dist(float max_dist) { m_tcu_ctrl->setup_max_dist(max_dist); }
    void setup_max_dov(float max_dov) { m_tcu_ctrl->setup_max_dov(max_dov); }
    void update_delay_offset(float dist_offset) { m_tcu_ctrl->update_delay_offset(dist_offset); }
    void update_gate_width_offset(float dov_offset) { m_tcu_ctrl->update_gate_width_offset(dov_offset); }
    void update_laser_offset(float laser_offset) { m_tcu_ctrl->update_laser_offset(laser_offset); }
    void setup_laser(int laser_on) { m_tcu_ctrl->setup_laser(laser_on); }
    // Delegated to DeviceManager
    void set_baudrate(int idx, int baudrate) { m_device_mgr->set_baudrate(idx, baudrate); }
    void set_tcp_status_on_port(int idx, bool use_tcp) { m_device_mgr->set_tcp_status_on_port(idx, use_tcp); }
    void set_tcu_as_shared_port(bool share) { m_device_mgr->set_tcu_as_shared_port(share); }
    void com_write_data(int com_idx, QByteArray data) { m_device_mgr->com_write_data(com_idx, data); }
    void display_port_info(int idx) { m_device_mgr->display_port_info(idx); }
    void update_dev_ip();
    void set_dev_ip(int ip, int gateway);
    void change_pixel_format(int pixel_format, int display_idx = 0);
    void update_lower_3d_thresh();
    void reset_custom_3d_params();
    void export_current_video();
    void set_tcu_type(int idx) { m_tcu_ctrl->set_tcu_type(idx); }
    void update_ps_config(bool read, int idx, uint val) { m_tcu_ctrl->update_ps_config(read, idx, val); }
    void set_auto_mcp(bool auto_mcp) { m_tcu_ctrl->set_auto_mcp_slot(auto_mcp); }

    // signaled by joystick input
    void joystick_button_pressed(int btn);
    void joystick_button_released(int btn);
    void joystick_direction_changed(int direction);

    // signaled by ControlPort (port_status, ptz delegated to DeviceManager)
    void append_data(QString str);
    // Lens params delegated to LensController
    void update_lens_params(qint32 lens_param, uint val) { m_lens_ctrl->update_lens_params(lens_param, val); }
    void update_distance(double distance) { m_rf_ctrl->update_distance(distance); }

    // signaled by Aliasing - delegated to TCUController
    void set_distance_set(int id) { m_tcu_ctrl->set_distance_set(id); }

    // ui switching
    void switch_ui();

    // VID_PAGE camera control - delegated to DeviceManager

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

    // lens control functions - delegated to LensController
    void on_ZOOM_IN_BTN_pressed() { m_lens_ctrl->on_ZOOM_IN_BTN_pressed(); }
    void on_ZOOM_IN_BTN_released() { m_lens_ctrl->on_ZOOM_IN_BTN_released(); }
    void on_ZOOM_OUT_BTN_pressed() { m_lens_ctrl->on_ZOOM_OUT_BTN_pressed(); }
    void on_ZOOM_OUT_BTN_released() { m_lens_ctrl->on_ZOOM_OUT_BTN_released(); }
    void on_FOCUS_FAR_BTN_pressed() { m_lens_ctrl->on_FOCUS_FAR_BTN_pressed(); }
    void on_FOCUS_FAR_BTN_released() { m_lens_ctrl->on_FOCUS_FAR_BTN_released(); }
    void on_FOCUS_NEAR_BTN_pressed() { m_lens_ctrl->on_FOCUS_NEAR_BTN_pressed(); }
    void on_FOCUS_NEAR_BTN_released() { m_lens_ctrl->on_FOCUS_NEAR_BTN_released(); }
    void on_RADIUS_INC_BTN_pressed() { m_lens_ctrl->on_RADIUS_INC_BTN_pressed(); }
    void on_RADIUS_INC_BTN_released() { m_lens_ctrl->on_RADIUS_INC_BTN_released(); }
    void on_RADIUS_DEC_BTN_pressed() { m_lens_ctrl->on_RADIUS_DEC_BTN_pressed(); }
    void on_RADIUS_DEC_BTN_released() { m_lens_ctrl->on_RADIUS_DEC_BTN_released(); }

    void on_GET_LENS_PARAM_BTN_clicked() { m_lens_ctrl->on_GET_LENS_PARAM_BTN_clicked(); }
    void on_AUTO_FOCUS_BTN_clicked() { m_lens_ctrl->on_AUTO_FOCUS_BTN_clicked(); }

    void on_IRIS_OPEN_BTN_pressed() { m_lens_ctrl->on_IRIS_OPEN_BTN_pressed(); }
    void on_IRIS_CLOSE_BTN_pressed() { m_lens_ctrl->on_IRIS_CLOSE_BTN_pressed(); }
    void on_IRIS_OPEN_BTN_released() { m_lens_ctrl->on_IRIS_OPEN_BTN_released(); }
    void on_IRIS_CLOSE_BTN_released() { m_lens_ctrl->on_IRIS_CLOSE_BTN_released(); }
    void laser_preset_reached() { m_laser_ctrl->laser_preset_reached(); }

    // slider-controlled slots (TCU-related delegated to TCUController)
    void change_mcp(int val) { m_tcu_ctrl->change_mcp(val); }
    void change_gain(int val);
    void change_delay(int val) { m_tcu_ctrl->change_delay(val); }
    void change_gatewidth(int val) { m_tcu_ctrl->change_gatewidth(val); }
    void change_focus_speed(int val) { m_lens_ctrl->change_focus_speed(val); }

    // Scan control - delegated to ScanController
    void on_SCAN_BUTTON_clicked() { m_scan_ctrl->on_SCAN_BUTTON_clicked(); }
    void on_CONTINUE_SCAN_BUTTON_clicked() { m_scan_ctrl->on_CONTINUE_SCAN_BUTTON_clicked(); }
    void on_RESTART_SCAN_BUTTON_clicked() { m_scan_ctrl->on_RESTART_SCAN_BUTTON_clicked(); }
    void on_SCAN_CONFIG_BTN_clicked() { m_scan_ctrl->on_SCAN_CONFIG_BTN_clicked(); }
    void enable_scan_options(bool show) { m_scan_ctrl->enable_scan_options(show); }

    // update delay, rep_freq, gate width, laser_width and their widgets - delegated to TCUController
    void update_laser_width() { m_tcu_ctrl->update_laser_width(); }
    void update_delay() { m_tcu_ctrl->update_delay(); }
    void update_gate_width() { m_tcu_ctrl->update_gate_width(); }
    void update_tcu_params(qint32 tcu_param) { m_tcu_ctrl->update_tcu_params(tcu_param); }

    // start laser & initialize laser stat
    // Laser control delegated to LaserController
    void on_LASER_BTN_clicked() { m_laser_ctrl->on_LASER_BTN_clicked(); }
    void start_laser() { m_laser_ctrl->start_laser(); }
    void init_laser() { m_laser_ctrl->init_laser(); }

    // hide left parameter bar
    void on_HIDE_BTN_clicked();
    // Alt display panel - delegated to AuxPanelManager
    void on_MISC_RADIO_1_clicked() { m_aux_panel->on_MISC_RADIO_1_clicked(); }
    void on_MISC_RADIO_2_clicked() { m_aux_panel->on_MISC_RADIO_2_clicked(); }
    void on_MISC_RADIO_3_clicked() { m_aux_panel->on_MISC_RADIO_3_clicked(); }
    void on_MISC_OPTION_1_currentIndexChanged(int index) { m_aux_panel->on_MISC_OPTION_1_currentIndexChanged(index); }
    void on_MISC_OPTION_2_currentIndexChanged(int index) { m_aux_panel->on_MISC_OPTION_2_currentIndexChanged(index); }
    void on_MISC_OPTION_3_currentIndexChanged(int index) { m_aux_panel->on_MISC_OPTION_3_currentIndexChanged(index); }

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

    // PTZ button handlers - delegated to DeviceManager

    void alt_display_control(int cmd);

    void on_DUAL_LIGHT_BTN_clicked();

    void on_RESET_3D_BTN_clicked();

    void display_fishnet_result(int result);

    void on_FIRE_LASER_BTN_clicked() { m_laser_ctrl->on_FIRE_LASER_BTN_clicked(); }

    void on_IMG_REGION_BTN_clicked();
    void on_SENSOR_TAPS_BTN_clicked();
    void on_SWITCH_TCU_UI_BTN_clicked() { m_tcu_ctrl->on_SWITCH_TCU_UI_BTN_clicked(); }
    void on_SIMPLE_LASER_CHK_clicked() { m_tcu_ctrl->on_SIMPLE_LASER_CHK_clicked(); }
    void on_AUTO_MCP_CHK_clicked() { m_tcu_ctrl->on_AUTO_MCP_CHK_clicked(); }

    void on_PSEUDOCOLOR_CHK_stateChanged(int arg1);

signals:
    // tell DATA_EXCHANGE (QTextEdit) to append data
    void append_text(QString text);

    // tell SOURCE_DISPLAY to display an image
    void set_src_pixmap(QPixmap pm);
    void set_hist_pixmap(QPixmap pm);

    // pause / continue scan
    void update_scan(bool show);

    // queue update_delay, mcp in thread
    void update_delay_in_thread();
    void update_mcp_in_thread(int new_mcp);

    // task queue full in thread pool
    void task_queue_full();

    // scan finished in grab thread
    void finish_scan_signal();

    // screenshot request from grab thread
    void save_screenshot_signal(QString path);

#ifdef LVTONG
    // enable/disable model list from grab thread
    void set_model_list_enabled(bool enabled);
#endif

    // packet loss status update from grab thread
    void packet_lost_updated(int packets_lost);

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
    

    // shut the cam down
    int shut_down();

    // update activated / deactivated buttons
    void enable_controls(bool cam_rdy);

    // Port init/setup delegated to DeviceManager

    // Lens operations delegated to LensController
    void lens_stop() { m_lens_ctrl->lens_stop(); }
    void set_zoom() { m_lens_ctrl->set_zoom_from_ui(); }
    void set_focus() { m_lens_ctrl->set_focus_from_ui(); }
    void focus_far() { m_lens_ctrl->focus_far(); }
    void focus_near() { m_lens_ctrl->focus_near(); }

    // Laser preset delegated to LaserController
    void goto_laser_preset(char target) { m_laser_ctrl->goto_laser_preset(target); }

    // save img in buffer to file; or save imgs while scanning (declarations moved to pipeline section)

    // convert data to be sent to HEX buffer - delegated to TCUController
    QByteArray convert_to_send_tcu(uchar num, unsigned int send) { return m_tcu_ctrl->convert_to_send_tcu(num, send); }

    // update data to data-display; fb: whether reading feedback from com
    QByteArray generate_ba(uchar *data, int len);
    QByteArray communicate_display(int id, QByteArray write, int write_size, int read_size, bool fb);

    // filter_scan stays (uses img_mem[0])
    void filter_scan();
    void auto_scan_for_target() { m_scan_ctrl->auto_scan_for_target(); }

    // change current in laser - delegated to TCUController
    void update_current() { m_tcu_ctrl->update_current(); }

    // write/read params to config file
    void convert_write(QDataStream &out, const int TYPE);
    bool convert_read(QDataStream &out, const int TYPE);

    // read device config by config
    void read_gatewidth_lookup_table(QFile *fp);
    
    // config sync functions
    void syncConfigToPreferences();
    void syncPreferencesToConfig();

    // TCP server connection delegated to DeviceManager

    // send ptz control cmd
    void send_ctrl_cmd(uchar dir);

    // static image display (drag & drop)
    void start_static_display(int width, int height, bool is_color, int display_idx = 0, int pixel_depth = 8, int device_type = -1);
    bool load_image_file(QString filename, bool init);
    int load_video_file(QString filename, bool format_gray = false, void (*process_frame)(cv::Mat &frame, void *ptr) = NULL,
                        void *ptr = NULL, int display_idx = 0, bool display = true);

    // status display
    void update_pixel_depth(int depth, int display_idx = 0);

public:
    bool                  mouse_pressed;
    std::vector<cv::Rect> list_roi;                   // user-selected roi

private:
    Ui::UserPanel*  ui;
    StatusBar*      status_bar;
    Preferences*    pref;
    DeviceManager*  m_device_mgr;
    TCUController*  m_tcu_ctrl;
    LensController* m_lens_ctrl;
    LaserController* m_laser_ctrl;
    RFController*    m_rf_ctrl;
    ScanController*  m_scan_ctrl;
    AuxPanelManager* m_aux_panel;
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
    Cam*            curr_cam;                   // current camera
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
    
    std::queue<cv::Mat>        q_img[3];                   // image queue in grab_thread
    std::queue<int>            q_frame_info;
    TimedQueue                 q_fps_calc;
//    bool                    updated[3];                 // whether the program get a new image from stream
    // TODO add other scan features
//    std::vector<TCUDataGroup>  q_scan;                   // targets' tcu param found while scanning

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
    bool                    ptz_adjust_ongoing;

    uchar                   lang;                       // 0: en_us, 1: zh_cn
    QTranslator             trans;

    ThreadPool              tp;

    QButtonGroup*           alt_ctrl_grp;               // alt display control button group

//    QMediaPlayer*           video_input;
//    VideoSurface*           video_surface;

    // TEMP ONLY
    // TODO move to addons
    PluginInterface*        pluginInterface;            // for ir with visible light

    // YOLO Detection
    YoloDetector*           m_yolo_detector[3] = {nullptr, nullptr, nullptr};
    QMutex                  m_yolo_init_mutex;
    int                     m_yolo_last_model[3] = {-1, -1, -1};  // Track loaded model for each thread
    void draw_yolo_boxes(cv::Mat& image, const std::vector<YoloResult>& results);

    // Thread-safe snapshots of UI widget state and Config for grab_thread_process
    QMutex                  m_params_mutex;
    ProcessingParams        m_processing_params;
    PipelineConfig          m_pipeline_config;
    void update_processing_params();

    // Pipeline methods extracted from grab_thread_process
    // Returns false if iteration should be skipped (LVTONG empty queue)
    bool acquire_frame(int thread_idx, cv::Mat& prev_img, bool& updated, int& packets_lost);
    void preprocess_frame(int thread_idx, const ProcessingParams& params,
        const PipelineConfig& pcfg,
        FrameAverageState& avg_state, int& _w, int& _h, uint hist[256]);
    std::vector<YoloResult> detect_yolo(int thread_idx, bool updated,
        const PipelineConfig& pcfg, YoloDetector*& yolo);
#ifdef LVTONG
    double detect_fishnet(int thread_idx, const PipelineConfig& pcfg, cv::dnn::Net& net);
#endif
    void frame_average_and_3d(int thread_idx, bool updated, const ProcessingParams& params,
        const PipelineConfig& pcfg,
        FrameAverageState& avg_state, ECCState& ecc_state, int _w, int _h, int _pixel_depth,
        int& ww, int& hh);
    void enhance_frame(int thread_idx, const ProcessingParams& params,
        const PipelineConfig& pcfg);
    void render_and_display(int thread_idx, int display_idx,
        const ProcessingParams& params, const PipelineConfig& pcfg,
        const std::vector<YoloResult>& yolo_results,
        double is_net, int ww, int hh, int _w, int _h, float weight,
        uint hist[256], const QString& info_tcu, const QString& info_time);
    void advance_scan(int thread_idx, bool updated, const PipelineConfig& pcfg,
        int& scan_img_count,
        QString& scan_save_path_a, QString& scan_save_path);
    void record_frame(int thread_idx, int display_idx, bool updated,
        const ProcessingParams& params, const PipelineConfig& pcfg,
        double is_net,
        const QString& info_tcu, const QString& info_time,
        int ww, int hh, float weight);
    void save_to_file(bool save_result, int idx, const PipelineConfig& pcfg);
    void save_scan_img(QString path, QString name, const PipelineConfig& pcfg);

public:

};
#endif // USERPANEL_H
