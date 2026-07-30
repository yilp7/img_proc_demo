#ifndef CAM_H
#define CAM_H

#include <stddef.h>
#include <opencv2/opencv.hpp>
#include <QtCore>

#include "pipeline/routing.h"  // StreamFrameSink

class Cam {
public:
    int device_type;
    int curr_idx;

public:
    Cam() {};
    virtual ~Cam() {};

    virtual int search_for_devices() = 0;

    virtual int start(int idx = 0) = 0;
    virtual int shut_down() = 0;

    // Connection health is separate from object/handle existence: SDK handles
    // can remain allocated after a physical or network disconnect.
    virtual bool is_connected() const { return true; }
    virtual int reconnect() { return -1; }

    // Cam SDK callbacks push frames through this sink. The sink lives on a
    // StreamSlot owned by UserPanel; cam adapter just stores the pointer and
    // hands it to its SDK as the user_data parameter.
    virtual int set_user_pointer(StreamFrameSink *sink) = 0;

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
#endif // CAM_H
