#ifndef MVCAM_H
#define MVCAM_H

#include "cam.h"

#include <MvCameraControl.h>
#include <atomic>

class MvCam : public Cam {
private:
    void* dev_handle;
    mutable std::atomic_bool reconnect_required;
    void configure_device();
    int release_handle();
    static void __stdcall exception_cb(unsigned int message_type, void *user_data);

public:
    MV_CC_DEVICE_INFO_LIST gige_dev_list;
    MV_CC_DEVICE_INFO_LIST usb3_dev_list;

public:
    MvCam();
    ~MvCam();

    int search_for_devices();

    int start(int idx = 0);
    int shut_down();
    bool is_connected() const override;
    int reconnect() override;

    int set_user_pointer(StreamFrameSink *sink);

    int start_grabbing();
    int stop_grabbing();

    void get_max_frame_size(int *w, int *h);
    void frame_size(bool read, int *w, int *h, int *inc_w = NULL, int *inc_h = NULL);
    void frame_offset(bool read, int *x, int *y, int *inc_x = NULL, int *inc_y = NULL);
    void time_exposure(bool read, float *val);
    void frame_rate(bool read, float *val);
    void gain_analog(bool read, float *val);
    void trigger_mode(bool read, bool *val);
    void trigger_source(bool read, bool *val);
    int ip_config(bool read, int *val);
    int ip_address(bool read, int idx, int *ip, int *gateway, int *nic_address = NULL);
    int pixel_type(bool read, int *val);
    void trigger_once();

    QStringList get_device_list();
    int gige_device_num();
    int usb3_device_num();

//    int get_img_buffer(MV_FRAME_OUT* frame, int msec);
//    int free_img_buffer(MV_FRAME_OUT* frame);
    void binning(bool read, int *val);

    static void __stdcall frame_cb(unsigned char* data, MV_FRAME_OUT_INFO_EX *frame_info, void* user_data);
};

#endif // MVCAM_H
