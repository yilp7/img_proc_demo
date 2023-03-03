#ifndef MVCAM_H
#define MVCAM_H

#include "MvCameraControl.h"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include <queue>
#include <QtCore>

class Cam
{
private:
    void* dev_handle;

public:
    // 0: no device; 1: MvCam; 2: HqvCam
    int                    device_type;
    bool                   cameralink;
    MV_CC_DEVICE_INFO_LIST gige_dev_list;
    MV_CC_DEVICE_INFO_LIST usb3_dev_list;
    int                    curr_idx;

public:
    Cam();
    ~Cam();

    int search_for_devices();

    int start(int idx = 0);
    int shut_down();

    int set_frame_callback(void *user);

    int start_grabbing();
    int stop_grabbing();

    int get_img_buffer(MV_FRAME_OUT* frame, int msec);
    int free_img_buffer(MV_FRAME_OUT* frame);

    void get_frame_size(int &w, int &h);
    void get_max_frame_size(int *w, int *h);
    void frame_size(bool read, int *w, int *h, int *inc_w = NULL, int *inc_h = NULL);
    void frame_offset(bool read, int *x, int *y, int *inc_x = NULL, int *inc_y = NULL);
    void time_exposure(bool read, float *val);
    void frame_rate(bool read, float *val);
    void gain_analog(bool read, float *val);
    void trigger_mode(bool read, bool *val);
    void trigger_source(bool read, bool *val);
    void binning(bool read, int *val);
    int ip_address(bool read, int *ip, int *gateway, int *nic_address = NULL);
    int pixel_type(bool read, int *val);
    void trigger_once();

    static void __stdcall frame_cb(unsigned char* data, MV_FRAME_OUT_INFO_EX *frame_info, void* user_data);
};

#endif // MVCAM_H
