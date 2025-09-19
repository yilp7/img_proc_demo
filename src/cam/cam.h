#ifndef CAM_H
#define CAM_H

#include <stddef.h>
#include <queue>
#include <opencv2/opencv.hpp>
#include <QtCore>

class Cam {
public:
    int device_type;
    int curr_idx;

public:
    Cam() {};
    ~Cam() {};

    virtual int search_for_devices() = 0;

    virtual int start(int idx = 0) = 0;
    virtual int shut_down() = 0;

    virtual int set_user_pointer(void *user) = 0;

    virtual int start_grabbing() = 0;
    virtual int stop_grabbing() = 0;

    virtual void get_max_frame_size(int *w, int *h) = 0;
    virtual void frame_size(bool read, int *w, int *h, int *inc_w = NULL, int *inc_h = NULL) = 0;
    virtual void frame_offset(bool read, int *x, int *y, int *inc_x = NULL, int *inc_y = NULL) = 0;
    virtual void time_exposure(bool read, float *val) = 0;
    virtual void frame_rate(bool read, float *val) = 0;
    virtual void gain_analog(bool read, float *val) = 0;
    virtual void trigger_mode(bool read, bool *val) = 0;
    virtual void trigger_source(bool read, bool *val) = 0;
    virtual int ip_config(bool read, int *val) = 0;
    virtual int ip_address(bool read, int idx, int *ip, int *gateway, int *nic_address = NULL) = 0;
    virtual int pixel_type(bool read, int *val) = 0;
    virtual void trigger_once() = 0;

    virtual QStringList get_device_list() = 0;
    virtual int gige_device_num() = 0;
    virtual int usb3_device_num() = 0;

};
#if 1
struct main_ui_info {
    std::queue<int> *frame_info_q;
    std::queue<cv::Mat> *img_q;
    QMutex *frame_info_mutex;  // Mutex to protect frame_info_q
    QMutex *img_mutex;          // Mutex to protect img_q
};
#endif
#endif // CAM_H
