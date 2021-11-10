#ifndef EURESYSCAM_H
#define EURESYSCAM_H

#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include <queue>
#include <QApplication>
#include <QDebug>
#include <QThread>
#include <QMutex>
#include "windows.h"
#include "multicam.h"
#include "clallserial.h"

class Cam
{
private:
    uint dev_handle;
    void* serial_ref;
    char port_name[128];

public:
    // 0: no device; 1: MvCam; 2: HqvCam; 3: EuresysCam
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

    static void WINAPI frame_cb(PMCCALLBACKINFO cb_info);

    float communicate(char* out, char* in, uint out_size, uint in_size, bool read = false);
};

#endif // EURESYSCAM_H
