#ifndef HQVCAM_H
#define HQVCAM_H

#include <HQV_API.h>
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include <queue>
#include <QApplication>

class Cam
{
private:
    void* dev_handle;

public:
    // 0: no device; 1: MvCam; 2: HqvCam
    int device_type;

public:
    Cam();
    ~Cam();

    int search_for_devices();

    int start();
    int shut_down();

    int set_frame_callback(void *user);

    int start_grabbing();
    int stop_grabbing();

    void get_frame_size(int &w, int &h);
    void time_exposure(bool read, float *val);
    void frame_rate(bool read, float *val);
    void gain_analog(bool read, float *val);
    void trigger_mode(bool read, bool *val);
    // val: true->soft trigger; false->external trigger
    void trigger_source(bool read, bool *val);
    void trigger_once();

    static DWORD WINAPI frame_cb(HANDLE dev, HQV_FRAMEINFO frame_info, void* user_data);
};

#endif // HQVCAM_H
