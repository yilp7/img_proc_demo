#include "cam.h"

Cam::Cam() {dev_handle = NULL;}
Cam::~Cam() {if (dev_handle) CloseHandle(dev_handle), dev_handle = NULL;}

int Cam::search_for_devices()
{
    device_type = 0;
    BYTE dev_num = 0;
    HQV_DeviceScan(&dev_num, (HQV_DEVICETYPE)3);
    if (dev_num) device_type = 2;
    MV_CC_DEVICE_INFO_LIST st_dev_list;
    MV_CC_EnumDevices(MV_GIGE_DEVICE, &st_dev_list);
    if (st_dev_list.nDeviceNum) device_type = 1;

    return device_type;
}

int Cam::start() {
    if (device_type == 1) {
        // get and store devices list to m_stDevList
        MV_CC_DEVICE_INFO_LIST st_dev_list;
        MV_CC_EnumDevices(MV_GIGE_DEVICE, &st_dev_list);
        int ret = MV_CC_CreateHandle(&dev_handle, st_dev_list.pDeviceInfo[0]);

        ret = MV_CC_OpenDevice(dev_handle);

        int size = MV_CC_GetOptimalPacketSize(dev_handle);
        if (size >= MV_OK) MV_CC_SetIntValueEx(dev_handle, "GevSCPSPacketSize", size);

        MV_CC_SetEnumValue(dev_handle, "ExposureMode", MV_EXPOSURE_MODE_TIMED);
        MV_CC_SetEnumValue(dev_handle, "ExposureAuto", MV_EXPOSURE_AUTO_MODE_OFF);
        MV_CC_SetEnumValue(dev_handle, "GainAuto", 0);
        MV_CC_SetBoolValue(dev_handle, "AcquisitionFrameRateEnable", true);

        return ret;
    }
    else if (device_type == 2) {
        DWORD idx = 0;
        return HQV_DeviceOpen(&idx, &dev_handle, DEVICE_INDEX, DEVICE_USB);
    }
    return -1;
}

int Cam::shut_down()
{
    if (device_type == 1) {
        if (dev_handle == NULL) return MV_E_HANDLE;

        int ret = MV_CC_CloseDevice(dev_handle);

        MV_CC_DestroyHandle(dev_handle);
        dev_handle = NULL;
        return ret;
    }
    else if (device_type == 2) return HQV_DeviceClose(&dev_handle);
    return -1;
}

int Cam::set_frame_callback(void *user)
{
    if (device_type == 1) return MV_CC_RegisterImageCallBackEx(dev_handle, frame_cb, user);
    else if (device_type == 2) return HQV_SetFrameCallback(dev_handle, frame_cb, user, DATA_GRAY);
    return -1;
}

int Cam::start_grabbing() {
    if (device_type == 1) return MV_CC_StartGrabbing(dev_handle);
    else if (device_type == 2) return HQV_CaptureStart(dev_handle);
    return -1;
}

int Cam::stop_grabbing() {
    if (device_type == 1) return MV_CC_StopGrabbing(dev_handle);
    else if (device_type == 2) return HQV_CaptureStop(dev_handle);
    return -1;
}

void Cam::get_frame_size(int &w, int &h)
{
    if (device_type == 1) {
        MVCC_INTVALUE int_value;
        MV_CC_GetIntValue(dev_handle, "Width", &int_value);
        w = int_value.nCurValue;
        MV_CC_GetIntValue(dev_handle, "Height", &int_value);
        h = int_value.nCurValue;
    }
    else if (device_type == 2) {
        HQV_ParamGetValue(dev_handle, PARAM_ID_SFNC_WIDTH, &w, VALUE_INT);
        HQV_ParamGetValue(dev_handle, PARAM_ID_SFNC_HEIGHT, &h, VALUE_INT);
    }
}

void Cam::time_exposure(bool read, float *val)
{
    if (device_type == 1) {
        if (read) {
            MVCC_FLOATVALUE f_val;
            MV_CC_GetFloatValue(dev_handle, "ExposureTime", &f_val);
            *val = f_val.fCurValue;
        }
        else MV_CC_SetFloatValue(dev_handle, "ExposureTime", *val);
    }
    else if (device_type == 2) {
        int time_expo = *val;
        if (read) HQV_ParamGetValue(dev_handle, PARAM_ID_SFNC_EXPOSURETIME, &time_expo, VALUE_INT), *val = time_expo;
        else HQV_ParamSetValue(dev_handle, PARAM_ID_SFNC_EXPOSURETIME, &time_expo, VALUE_INT);
    }
}

void Cam::frame_rate(bool read, float *val)
{
    if (device_type == 1) {
        if (read) {
            MVCC_FLOATVALUE f_val;
            MV_CC_GetFloatValue(dev_handle, "ResultingFrameRate", &f_val);
            *val = f_val.fCurValue;
        }
        else MV_CC_SetFloatValue(dev_handle, "AcquisitionFrameRate", *val);
    }
    else if (device_type == 2) {
        int frame_rate = *val;
        if (read) HQV_ParamGetValue(dev_handle, PARAM_ID_SFNC_ACQUISITIONFRAMERATE, &frame_rate, VALUE_INT), *val = frame_rate;
        else HQV_ParamSetValue(dev_handle, PARAM_ID_SFNC_ACQUISITIONFRAMERATE, &frame_rate, VALUE_INT);
    }
}

void Cam::gain_analog(bool read, float *val)
{
    if (device_type == 1) {
        if (read) {
            MVCC_FLOATVALUE f_val;
            MV_CC_GetFloatValue(dev_handle, "Gain", &f_val);
            *val = f_val.fCurValue;
        }
        else MV_CC_SetFloatValue(dev_handle, "Gain", *val);
    }
    else if (device_type == 2) {
        int gain = *val;
        if (read) HQV_ParamGetValue(dev_handle, PARAM_ID_SFNC_GAIN, &gain, VALUE_INT), *val = gain;
        else HQV_ParamSetValue(dev_handle, PARAM_ID_SFNC_GAIN, &gain, VALUE_INT);
    }
}

void Cam::trigger_mode(bool read, bool *val)
{
    if (device_type == 1) {
        if (read) {
            MVCC_ENUMVALUE enum_val;
            MV_CC_GetEnumValue(dev_handle, "TriggerMode", &enum_val);
            *val = enum_val.nCurValue;
        }
        else MV_CC_SetEnumValue(dev_handle, "TriggerMode", *val);
    }
    else if (device_type == 2) {
        if (read) HQV_ParamGetValue(dev_handle, PARAM_ID_SFNC_TRIGGERMODE, val, VALUE_INT);
        else HQV_ParamSetValue(dev_handle, PARAM_ID_SFNC_TRIGGERMODE, val, VALUE_INT);
    }
}

void Cam::trigger_source(bool read, bool *val)
{
    if (device_type == 1) {
        if (read) {
            MVCC_ENUMVALUE enum_val;
            MV_CC_GetEnumValue(dev_handle, "TriggerSource", &enum_val);
            *val = enum_val.nCurValue == MV_TRIGGER_SOURCE_SOFTWARE;
        }
        else {
            int source = *val ? MV_TRIGGER_SOURCE_SOFTWARE : MV_TRIGGER_SOURCE_LINE0;
            MV_CC_SetEnumValue(dev_handle, "TriggerSource", source);
        }
    }
    else if (device_type == 2) {
        if (read) {
            HQV_ParamGetValue(dev_handle, PARAM_ID_SFNC_TRIGGERSOURCE, val, VALUE_INT);
        }
        else {
            int source = *val ? 1 : 2;
            HQV_ParamSetValue(dev_handle, PARAM_ID_SFNC_TRIGGERSOURCE, &source, VALUE_INT);
        }
    }
}

void Cam::trigger_once()
{
    if (device_type == 1) MV_CC_SetCommandValue(dev_handle, "TriggerSoftware");
    else if (device_type == 2) {
        int n = 1;
        HQV_ParamSetValue(dev_handle, PARAM_ID_SFNC_TRIGGERSOFTWARE, &n, VALUE_INT);
    }
}

void Cam::frame_cb(unsigned char *data, MV_FRAME_OUT_INFO_EX *frame_info, void *user_data)
{
    static cv::Mat img(frame_info->nHeight, frame_info->nWidth, CV_8UC1);
    memcpy(img.data, data, frame_info->nFrameLen);
    ((std::queue<cv::Mat>*)user_data)->push(img.clone());
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
