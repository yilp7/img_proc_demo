#ifndef EBUSCAM_H
#define EBUSCAM_H

#include "cam.h"

#include <PvInterface.h>
#include <PvDevice.h>
#include <PvTypes.h>
#include <PvDevice.h>
#include <PvSystem.h>
#include <PvDeviceGEV.h>
#include <PvStream.h>
#include <PvStreamGEV.h>
#include <PvBuffer.h>
#include <PvGenParameter.h>
#include <PvDeviceSerialPort.h>
#include <PvDeviceAdapter.h>

#include <QtCore>

class EbusCam : public Cam {
private:
    PvDevice           *dev_handle;
    PvStream           *stream_handle;
    PvDeviceAdapter    *dev_adapter;
    PvDeviceSerialPort *dev_serial_port;

    uchar buffer_out[7];
    uchar buffer_in[7];

    std::list<PvBuffer*> buffer_list;
    void*                ptr_user;
    bool                 is_streaming;
    bool                 streaming_thread_state;

public:
    int device_type;
    int curr_idx;
    std::vector<PvDeviceInfoGEV*> gige_dev_list;
    std::vector<PvDeviceInfoU3V*> usb3_dev_list;

public:
    EbusCam();
    ~EbusCam();

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
    void trigger_source(bool read, bool *val);
    int ip_config(bool read, int *val);
    int ip_address(bool read, int *ip, int *gateway, int *nic_address = NULL);
    int pixel_type(bool read, int *val);
    void trigger_once();

    QStringList get_device_list();
    int gige_device_num();
    int usb3_device_num();

};

#endif // EBUSCAM_H
