#include "cam.h"

Cam::Cam() {dev_handle = NULL; cameralink = false;}
Cam::~Cam() {if (dev_handle) CloseHandle(dev_handle), dev_handle = NULL;}

cv::Mat img; // hik
cv::Mat raw_data, temp; // hqv

int Cam::search_for_devices()
{
    device_type = 0;
    MV_CC_DEVICE_INFO_LIST st_dev_list = {0};
    if (!cameralink)
    {
        MV_CC_EnumDevices(MV_GIGE_DEVICE, &st_dev_list);
        if (st_dev_list.nDeviceNum) device_type = 1;
        if (device_type) return device_type;
    }

    BYTE dev_num = 0;
    HQV_DeviceScan(&dev_num, cameralink ? DEVICE_GEV : DEVICE_USB);
    if (dev_num) device_type = 2;
    if(device_type) return device_type;

    return device_type;
}

int Cam::start() {
    switch(device_type) {
    case 1: {
        // get and store devices list to m_stDevList
        MV_CC_DEVICE_INFO_LIST st_dev_list;
        MV_CC_EnumDevices(MV_GIGE_DEVICE, &st_dev_list);
        int ret = MV_CC_CreateHandle(&dev_handle, st_dev_list.pDeviceInfo[0]);

        ret = MV_CC_OpenDevice(dev_handle);

        int size = MV_CC_GetOptimalPacketSize(dev_handle);
        if (size >= MV_OK) MV_CC_SetIntValueEx(dev_handle, "GevSCPSPacketSize", size);

        MV_CC_SetPixelFormat(dev_handle, PixelType_Gvsp_Mono8);
        MV_CC_SetEnumValue(dev_handle, "ExposureMode", MV_EXPOSURE_MODE_TIMED);
        MV_CC_SetEnumValue(dev_handle, "ExposureAuto", MV_EXPOSURE_AUTO_MODE_OFF);
        MV_CC_SetEnumValue(dev_handle, "GainAuto", 0);
        MV_CC_SetBoolValue(dev_handle, "AcquisitionFrameRateEnable", true);
        MV_CC_SetEnumValue(dev_handle, "BinningHorizontal", 1);
        MV_CC_SetEnumValue(dev_handle, "BinningVertical", 1);

        return ret;
    }
    case 2: {
        DWORD idx = 0;
        int ret = HQV_DeviceOpen(&idx, &dev_handle, DEVICE_INDEX, DEVICE_GEV);
        if (cameralink) {
            HQV_RegValueSet(dev_handle, PORT_SERIAL0, 0xFF1E, 1);
            HQV_RegValueSet(dev_handle, PORT_SERIAL0, 0xFF62, 0);
            char baudrate_str[32] = {0};
            sprintf(baudrate_str, "%s", "Uart0BaudRate");
            HQV_DEVPARAM baudrate(baudrate_str);
            int val = 6; // 0: 9600, 1: 14400, 2: 19200, 3: 28800, 4: 38400, 5: 57600, 6: 115200;
            HQV_ParamSetValue(dev_handle, baudrate, &val);
        }
        return ret;
    }
    default:
        break;
    }
    return -1;
}

int Cam::shut_down()
{
    switch (device_type) {
    case 1: {
        if (dev_handle == NULL) return MV_E_HANDLE;

        int ret = MV_CC_CloseDevice(dev_handle);

        MV_CC_DestroyHandle(dev_handle);
        dev_handle = NULL;
        return ret;
    }
    case 2:
        return HQV_DeviceClose(&dev_handle);
    default:
        break;
    }
    return -1;
}

int Cam::set_frame_callback(void *user)
{
    switch (device_type) {
    case 1: return MV_CC_RegisterImageCallBackEx(dev_handle, frame_cb, user);
    case 2:
        return cameralink ? HQV_SetFrameCallback(dev_handle, frame_cb_cl, user, DATA_GRAY) : HQV_SetFrameCallback(dev_handle, frame_cb, user, DATA_GRAY);
    default:
        break;
    }
    return -1;
}

int Cam::start_grabbing() {
    switch (device_type) {
    case 1: return MV_CC_StartGrabbing(dev_handle);
    case 2: return HQV_CaptureStart(dev_handle);
    default: break;
    }
    return -1;
}

int Cam::stop_grabbing() {
    switch (device_type) {
    case 1: return MV_CC_StopGrabbing(dev_handle);
    case 2: return HQV_CaptureStop(dev_handle);
    default: break;
    }
    return -1;
}

void Cam::get_frame_size(int &w, int &h)
{
    switch (device_type) {
    case 1: {
        MVCC_INTVALUE int_value;
        MV_CC_GetIntValue(dev_handle, "Width", &int_value);
        w = int_value.nCurValue;
        MV_CC_GetIntValue(dev_handle, "Height", &int_value);
        h = int_value.nCurValue;
        img = cv::Mat(h, w, CV_8UC1);
        break;
    }
    case 2: {
//        char cPixelFormat[25];
//        memset(cPixelFormat,0,25);
//        HQV_ParamGetValue(dev_handle,PARAM_ID_SFNC_PIXELFORMAT,cPixelFormat,VALUE_STRING);
//        qDebug() << cPixelFormat;
        HQV_ParamGetValue(dev_handle, PARAM_ID_SFNC_WIDTH, &w, VALUE_INT);
        HQV_ParamGetValue(dev_handle, PARAM_ID_SFNC_HEIGHT, &h, VALUE_INT);
        raw_data = cv::Mat(h, w, cameralink ? CV_8UC1 : CV_16UC1);
        break;
    }
    default:
        break;
    }
}

void Cam::time_exposure(bool read, float *val)
{
    switch (device_type) {
    case 1: {
        if (read) {
            MVCC_FLOATVALUE f_val;
            MV_CC_GetFloatValue(dev_handle, "ExposureTime", &f_val);
            *val = f_val.fCurValue;
        }
        else MV_CC_SetFloatValue(dev_handle, "ExposureTime", *val);
        break;
    }
    case 2: {
        if (cameralink) {
            unsigned long time_expo = *val;
            if (read) {
                HQV_RegValueGet(dev_handle, PORT_SERIAL0, 0xFF21, &time_expo);
                *val = time_expo;
                HQV_RegValueGet(dev_handle, PORT_SERIAL0, 0xFF22, &time_expo);
                *val = time_expo;
            }
            else HQV_RegValueSet(dev_handle, PORT_SERIAL0, 0xFF21, time_expo);
        }
        else {
            int time_expo = *val;
            if (read) HQV_ParamGetValue(dev_handle, PARAM_ID_SFNC_EXPOSURETIME, &time_expo, VALUE_INT), *val = time_expo;
            else HQV_ParamSetValue(dev_handle, PARAM_ID_SFNC_EXPOSURETIME, &time_expo, VALUE_INT);
        }
        break;
    }
    default:
        break;
    }
}

void Cam::frame_rate(bool read, float *val)
{
    switch (device_type) {
    case 1: {
        if (read) {
            MVCC_FLOATVALUE f_val;
            MV_CC_GetFloatValue(dev_handle, "ResultingFrameRate", &f_val);
            *val = f_val.fCurValue;
        }
        else MV_CC_SetFloatValue(dev_handle, "AcquisitionFrameRate", *val);
        break;
    }
    case 2: {
        if (cameralink) {
            unsigned long frame_rate = *val;
            if (read) HQV_RegValueGet(dev_handle, PORT_SERIAL0, 0xFF1F, &frame_rate), *val = frame_rate;
            else HQV_RegValueSet(dev_handle, PORT_SERIAL0, 0xFF1F, frame_rate);
        }
        else {
            int frame_rate = *val;
            if (read) HQV_ParamGetValue(dev_handle, PARAM_ID_SFNC_ACQUISITIONFRAMERATE, &frame_rate, VALUE_INT), *val = frame_rate;
            else HQV_ParamSetValue(dev_handle, PARAM_ID_SFNC_ACQUISITIONFRAMERATE, &frame_rate, VALUE_INT);
        }
        break;
    }
    default:
        break;
    }
}

void Cam::gain_analog(bool read, float *val)
{
    switch (device_type) {
    case 1: {
        if (read) {
            MVCC_FLOATVALUE f_val;
            MV_CC_GetFloatValue(dev_handle, "Gain", &f_val);
            *val = f_val.fCurValue;
        }
        else MV_CC_SetFloatValue(dev_handle, "Gain", *val);
        break;
    }
    case 2: {
        if (cameralink) {
            unsigned long gain = *val;
            if (read) HQV_RegValueGet(dev_handle, PORT_SERIAL0, 0xFF17, &gain), *val = gain;
            else HQV_RegValueSet(dev_handle, PORT_SERIAL0, 0xFF17, gain);
        }
        else {
            int gain = *val;
            if (read) HQV_ParamGetValue(dev_handle, PARAM_ID_SFNC_GAIN, &gain, VALUE_INT), *val = gain;
            else HQV_ParamSetValue(dev_handle, PARAM_ID_SFNC_GAIN, &gain, VALUE_INT);
        }
        break;
    }
    default:
        break;
    }
}

void Cam::trigger_mode(bool read, bool *val)
{
    switch (device_type) {
    case 1: {
        if (read) {
            MVCC_ENUMVALUE enum_val;
            MV_CC_GetEnumValue(dev_handle, "TriggerMode", &enum_val);
            *val = enum_val.nCurValue;
        }
        else MV_CC_SetEnumValue(dev_handle, "TriggerMode", *val);
        break;
    }
    case 2: {
        if (cameralink) {
            unsigned long trigger_mode = *val;
            if (read) HQV_RegValueGet(dev_handle, PORT_SERIAL0, 0xFF1D, &trigger_mode), *val = trigger_mode;
            else HQV_RegValueSet(dev_handle, PORT_SERIAL0, 0xFF1D, trigger_mode);
        }
        else {
            if (read) HQV_ParamGetValue(dev_handle, PARAM_ID_SFNC_TRIGGERMODE, val, VALUE_INT);
            else HQV_ParamSetValue(dev_handle, PARAM_ID_SFNC_TRIGGERMODE, val, VALUE_INT);
        }
        break;
    }
    default:
        break;
    }
}

void Cam::trigger_source(bool read, bool *val)
{
    switch (device_type) {
    case 1: {
        if (read) {
            MVCC_ENUMVALUE enum_val;
            MV_CC_GetEnumValue(dev_handle, "TriggerSource", &enum_val);
            *val = enum_val.nCurValue == MV_TRIGGER_SOURCE_SOFTWARE;
        }
        else {
            int source = *val ? MV_TRIGGER_SOURCE_SOFTWARE : MV_TRIGGER_SOURCE_LINE0;
            MV_CC_SetEnumValue(dev_handle, "TriggerSource", source);
        }
        break;
    }
    case 2: {
        if (cameralink) {
            unsigned long source = *val ? 1 : 2;
            if (read) HQV_RegValueGet(dev_handle, PORT_SERIAL0, 0xFF1D, &source), *val = source;
            else HQV_RegValueSet(dev_handle, PORT_SERIAL0, 0xFF1D, source);
        }
        else {
            int source = *val ? 1 : 2;
            if (read) HQV_ParamGetValue(dev_handle, PARAM_ID_SFNC_TRIGGERSOURCE, val, VALUE_INT);
            else HQV_ParamSetValue(dev_handle, PARAM_ID_SFNC_TRIGGERSOURCE, &source, VALUE_INT);
        }
        break;
    }
    default:
        break;
    }
}

void Cam::binning(bool read, int *val)
{
    switch (device_type) {
    case 1: {
        if (read) {
            MVCC_ENUMVALUE enum_val;
            MV_CC_GetEnumValue(dev_handle, "BinningHorizontal", &enum_val);
            MV_CC_GetEnumValue(dev_handle, "BinningVertical", &enum_val);
            *val = enum_val.nCurValue;
        }
        else {
            MV_CC_SetEnumValue(dev_handle, "BinningHorizontal", *val);
            MV_CC_SetEnumValue(dev_handle, "BinningVertical", *val);
        }
        break;
    }
    case 2: break;
    default: break;
    }
}

void Cam::ip_address(bool read, int *ip, int *gateway)
{
    switch (device_type) {
    case 1: {
        if (read) {
            MV_CC_DEVICE_INFO_LIST st_dev_list;
            MV_CC_EnumDevices(MV_GIGE_DEVICE, &st_dev_list);
            *ip = st_dev_list.pDeviceInfo[0]->SpecialInfo.stGigEInfo.nCurrentIp;
            *gateway = st_dev_list.pDeviceInfo[0]->SpecialInfo.stGigEInfo.nDefultGateWay;
        }
        else {
            MV_GIGE_ForceIpEx(dev_handle, *ip, (255 << 24) + (255 << 16) + (255 << 8), *gateway);
        }
        break;
    }
    case 2:
        break;
    default:
        break;
    }
}

void Cam::trigger_once()
{
    switch (device_type) {
    case 1: MV_CC_SetCommandValue(dev_handle, "TriggerSoftware"); break;
    case 2: {
        int n = 1;
        if (cameralink) HQV_RegValueSet(dev_handle, PORT_SERIAL0, 0xFF41, n);
        else HQV_ParamSetValue(dev_handle, PARAM_ID_SFNC_TRIGGERSOFTWARE, &n, VALUE_INT);
        break;
    }
    default:
        break;
    }
}

void Cam::frame_cb(unsigned char *data, MV_FRAME_OUT_INFO_EX *frame_info, void *user_data)
{
//    static cv::Mat img(frame_info->nHeight, frame_info->nWidth, CV_8UC1);
    memcpy(img.data, data, frame_info->nFrameLen);
    ((std::queue<cv::Mat>*)user_data)->push(img.clone());
}

DWORD Cam::frame_cb(HANDLE dev, HQV_FRAMEINFO frame_info, void *user_data)
{
//    static cv::Mat raw_data(frame_info.lHeight, frame_info.lWidth, CV_16UC1), temp;
    memcpy(raw_data.data, frame_info.pBufPtr, frame_info.lBufSize);
    cv::cvtColor(raw_data, raw_data, cv::COLOR_BayerGR2GRAY);
    raw_data.convertTo(temp, CV_8UC1, 1.0 / 16);
    ((std::queue<cv::Mat>*)user_data)->push(temp.clone());

    return RESULT_OK;
}

DWORD Cam::frame_cb_cl(HANDLE dev, HQV_FRAMEINFO frame_info, void *user_data)
{
//    static cv::Mat raw_data(frame_info.lHeight, frame_info.lWidth, CV_16UC1), temp;
    memcpy(raw_data.data, frame_info.pBufPtr, frame_info.lBufSize);
    ((std::queue<cv::Mat>*)user_data)->push(raw_data.clone());

    return RESULT_OK;
}
