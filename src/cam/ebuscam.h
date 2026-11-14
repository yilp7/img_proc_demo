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
#include <atomic>
#include <thread>

class EbusCam : public Cam
{
private:
    PvDevice           *m_dev_handle;
    PvStream           *m_stream_handle;
    PvDeviceAdapter    *m_dev_adapter;
    PvDeviceSerialPort *m_dev_serial_port;

    uchar m_buffer_out[7];
    uchar m_buffer_in[7];

    std::list<PvBuffer*> m_buffer_list;
    StreamFrameSink     *m_ptr_user;
    std::atomic_bool     m_is_streaming;
    std::thread          m_streaming_thread;

    void clear_device_list();

public:
    std::vector<PvDeviceInfoGEV*> m_gige_dev_list;
    std::vector<PvDeviceInfoU3V*> m_usb3_dev_list;

public:
    EbusCam();
    ~EbusCam();

    int search_for_devices();

    int start(int idx = 0);
    int shut_down();
    bool is_connected() const override;
    int reconnect() override;

    int set_user_pointer(StreamFrameSink *sink);

    int start_grabbing();
    int stop_grabbing();

    void get_max_frame_size(int *w, int *h);
    void frame_size(bool read, int *w, int *h, int *inc_w = nullptr, int *inc_h = nullptr);
    void frame_offset(bool read, int *x, int *y, int *inc_x = nullptr, int *inc_y = nullptr);
    void time_exposure(bool read, float *val);
    void frame_rate(bool read, float *val);
    void gain_analog(bool read, float *val);
    void trigger_mode(bool read, bool *val);
    void trigger_source(bool read, bool *val);
    int ip_config(bool read, int *val);
    int ip_address(bool read, int idx, int *ip, int *gateway, int *nic_address = nullptr);
    int pixel_type(bool read, int *val);
    void trigger_once();

    void set_sensor_digitization_taps(int num);

    QStringList get_device_list();
    int gige_device_num();
    int usb3_device_num();

};

#endif // EBUSCAM_H
