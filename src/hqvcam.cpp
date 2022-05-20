#include "hqvcam.h"

Cam::Cam() {dev_handle = NULL;}
Cam::~Cam() {if (dev_handle) CloseHandle(dev_handle), dev_handle = NULL;}

int Cam::search_for_devices()
{
    device_type = 0;
    BYTE dev_num = 0;

    // device type = 3 for usb3.0 devices
    HQV_DeviceScan(&dev_num, (HQV_DEVICETYPE)3);
    qDebug("HQV_DeviceScan GetDeviceNumber is %d", dev_num);
    for (int i = 0; i < dev_num; i++)
    {
        HQV_DEVINFO dev_info;
        memset(&dev_info, 0, sizeof(HQV_DEVINFO));
        DWORD dev_idx = i;

        HQV_DeviceGetInfo(&dev_idx, &dev_info, DEVICE_INDEX, (HQV_DEVICETYPE)3);
        HQV_DEVINFO_USB dev_info_USB;
        memset(&dev_info_USB, 0, sizeof(HQV_DEVINFO_USB));
        memcpy(&dev_info_USB, dev_info.pDevInfoUSB, sizeof(HQV_DEVINFO_USB));
        qDebug("[USB] DEV%d: DevName:%s, PID:%x, VID:%x, FW:%x, SN:%s",
               i, dev_info_USB.cDevName, dev_info_USB.iDevPID, dev_info_USB.iDevVID,
               dev_info_USB.iDevVersion, dev_info_USB.cDevSN);
    }
    if (dev_num) device_type = 2;
    return dev_num;
}

int Cam::start()
{
    DWORD idx = 0;
    return HQV_DeviceOpen(&idx, &dev_handle, DEVICE_INDEX, DEVICE_USB);
}

int Cam::shut_down()
{
    return HQV_DeviceClose(&dev_handle);
}

int Cam::set_frame_callback(void *user)
{
    return HQV_SetFrameCallback(dev_handle, frame_cb, user, DATA_GRAY);
}

int Cam::start_grabbing()
{
    return HQV_CaptureStart(dev_handle);
}

int Cam::stop_grabbing()
{
    return HQV_CaptureStop(dev_handle);
}

void Cam::get_frame_size(int &w, int &h)
{
    HQV_ParamGetValue(dev_handle, PARAM_ID_SFNC_WIDTH, &w, VALUE_INT);
    HQV_ParamGetValue(dev_handle, PARAM_ID_SFNC_HEIGHT, &h, VALUE_INT);
}

void Cam::time_exposure(bool read, float *val)
{
    int time_expo = *val;
    if (read) HQV_ParamGetValue(dev_handle, PARAM_ID_SFNC_EXPOSURETIME, &time_expo, VALUE_INT), *val = time_expo;
    else HQV_ParamSetValue(dev_handle, PARAM_ID_SFNC_EXPOSURETIME, &time_expo, VALUE_INT);
}

void Cam::frame_rate(bool read, float *val)
{
    int frame_rate = *val;
    if (read) HQV_ParamGetValue(dev_handle, PARAM_ID_SFNC_ACQUISITIONFRAMERATE, &frame_rate, VALUE_INT), *val = frame_rate;
    else HQV_ParamSetValue(dev_handle, PARAM_ID_SFNC_ACQUISITIONFRAMERATE, &frame_rate, VALUE_INT);
}

void Cam::gain_analog(bool read, float *val)
{
    int gain = *val;
    if (read) HQV_ParamGetValue(dev_handle, PARAM_ID_SFNC_GAIN, &gain, VALUE_INT), *val = gain;
    else HQV_ParamSetValue(dev_handle, PARAM_ID_SFNC_GAIN, &gain, VALUE_INT);
}

void Cam::trigger_mode(bool read, bool *val)
{
    if (read) HQV_ParamGetValue(dev_handle, PARAM_ID_SFNC_TRIGGERMODE, val, VALUE_INT);
    else HQV_ParamSetValue(dev_handle, PARAM_ID_SFNC_TRIGGERMODE, val, VALUE_INT);
}

void Cam::trigger_source(bool read, bool *val)
{
    if (read) {
        HQV_ParamGetValue(dev_handle, PARAM_ID_SFNC_TRIGGERSOURCE, val, VALUE_INT);
    }
    else {
        int source = *val ? 1 : 2;
        HQV_ParamSetValue(dev_handle, PARAM_ID_SFNC_TRIGGERSOURCE, &source, VALUE_INT);
    }
}

void Cam::trigger_once()
{
    int n = 1;
    HQV_ParamSetValue(dev_handle, PARAM_ID_SFNC_TRIGGERSOFTWARE, &n, VALUE_INT);
}

DWORD Cam::frame_cb(HANDLE dev, HQV_FRAMEINFO frame_info, void *user_data)
{
    static cv::Mat raw_data(frame_info.lHeight, frame_info.lWidth, CV_16UC1), temp;
    memcpy(raw_data.data, frame_info.pBufPtr, frame_info.lBufSize);
    cv::cvtColor(raw_data, raw_data, cv::COLOR_BayerGR2GRAY);
    raw_data.convertTo(temp, CV_8UC1, 1.0 / 16);
    ((std::queue<cv::Mat>*)user_data)->push(temp);

    return RESULT_OK;
}
