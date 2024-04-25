#ifndef EURESYSCAM_H
#define EURESYSCAM_H

#include "cam.h"

#define NOMINMAX
#include "windows.h"
#include "multicam.h"
// #include "clallserial.h"

class EuresysCam : public Cam {
private:
    uint dev_handle;
    void* serial_ref;
    char port_name[128];

public:
    EuresysCam();
    ~EuresysCam();

    int search_for_devices();

    int start(int idx = 0);
    int shut_down();

    int set_user_pointer(void *user);

    int start_grabbing();
    int stop_grabbing();

    void get_max_frame_size(int *w, int *h);
    void frame_size(bool read, int *w, int *h, int *inc_w = NULL, int *inc_h = NULL);
    void frame_offset(bool read, int *x, int *y, int *inc_x = NULL, int *inc_y = NULL);
    void time_exposure(bool read, float *val);
    void frame_rate(bool read, float *val);
    void gain_analog(bool read, float *val);
    void trigger_mode(bool read, bool *val);
    void trigger_source(bool read, bool *val);// val: true->soft trigger; false->external trigger
    int ip_config(bool read, int *val);
    int ip_address(bool read, int *ip, int *gateway, int *nic_address = NULL);
    int pixel_type(bool read, int *val);
    void trigger_once();


    QStringList get_device_list();
    int gige_device_num();
    int usb3_device_num();

    static void WINAPI frame_cb(PMCCALLBACKINFO cb_info);

    float communicate(char* out, char* in, uint out_size, uint in_size, bool read = false);
};

#endif // EURESYSCAM_H
