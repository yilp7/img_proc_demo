#include "mvcam.h"

Cam::Cam() {dev_handle = NULL; cameralink = false;}
Cam::~Cam() {if (dev_handle) MV_CC_DestroyHandle(dev_handle), dev_handle = NULL;}

cv::Mat img; // hik

int Cam::search_for_devices()
{
    device_type = 0;
    MV_CC_DEVICE_INFO_LIST st_dev_list = {0};
    // FIXME: error 2 -2146885623
    MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &st_dev_list);
    if (st_dev_list.nDeviceNum) {
        device_type = 1;
        int ret = MV_CC_CreateHandle(&dev_handle, st_dev_list.pDeviceInfo[0]);
    }

    return device_type;
}

int Cam::start() {
    // get and store devices list to m_stDevList
    MV_CC_DEVICE_INFO_LIST st_dev_list;
    MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &st_dev_list);
    if (dev_handle) MV_CC_DestroyHandle(dev_handle), dev_handle = NULL;
    int ret = MV_CC_CreateHandle(&dev_handle, st_dev_list.pDeviceInfo[0]);
    device_type = 1;

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

int Cam::shut_down()
{
    if (dev_handle == NULL) return MV_E_HANDLE;

    int ret = MV_CC_CloseDevice(dev_handle);

    MV_CC_DestroyHandle(dev_handle);
    dev_handle = NULL;
    return ret;
}

int Cam::set_frame_callback(void *user)
{
    return MV_CC_RegisterImageCallBackEx(dev_handle, frame_cb, user);
}

int Cam::start_grabbing() {
    return MV_CC_StartGrabbing(dev_handle);
}

int Cam::stop_grabbing() {
    return MV_CC_StopGrabbing(dev_handle);
}

void Cam::get_frame_size(int &w, int &h)
{
    MVCC_INTVALUE int_value;
    MV_CC_GetIntValue(dev_handle, "Width", &int_value);
    w = int_value.nCurValue;
    MV_CC_GetIntValue(dev_handle, "Height", &int_value);
    h = int_value.nCurValue;
    img = cv::Mat(h, w, CV_8UC1);
}

void Cam::time_exposure(bool read, float *val)
{
    if (read) {
        MVCC_FLOATVALUE f_val;
        MV_CC_GetFloatValue(dev_handle, "ExposureTime", &f_val);
        *val = f_val.fCurValue;
    }
    else MV_CC_SetFloatValue(dev_handle, "ExposureTime", *val);
}

void Cam::frame_rate(bool read, float *val)
{
    if (read) {
        MVCC_FLOATVALUE f_val;
        MV_CC_GetFloatValue(dev_handle, "ResultingFrameRate", &f_val);
        *val = f_val.fCurValue;
    }
    else MV_CC_SetFloatValue(dev_handle, "AcquisitionFrameRate", *val);
}

void Cam::gain_analog(bool read, float *val)
{
    if (read) {
        MVCC_FLOATVALUE f_val;
        MV_CC_GetFloatValue(dev_handle, "Gain", &f_val);
        *val = f_val.fCurValue;
    }
    else MV_CC_SetFloatValue(dev_handle, "Gain", *val);
}

void Cam::trigger_mode(bool read, bool *val)
{
    if (read) {
        MVCC_ENUMVALUE enum_val;
        MV_CC_GetEnumValue(dev_handle, "TriggerMode", &enum_val);
        *val = enum_val.nCurValue;
    }
    else MV_CC_SetEnumValue(dev_handle, "TriggerMode", *val);
}

void Cam::trigger_source(bool read, bool *val)
{
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

void Cam::binning(bool read, int *val)
{
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
}

int Cam::ip_address(bool read, int *ip, int *gateway, int *nic_address)
{
    if (read) {
        MV_CC_DEVICE_INFO_LIST st_dev_list = {0};
        MV_CC_EnumDevices(MV_GIGE_DEVICE, &st_dev_list);
        if (st_dev_list.nDeviceNum) {
            *ip = st_dev_list.pDeviceInfo[0]->SpecialInfo.stGigEInfo.nCurrentIp;
            *gateway = st_dev_list.pDeviceInfo[0]->SpecialInfo.stGigEInfo.nDefultGateWay;
            if (nic_address) *nic_address = st_dev_list.pDeviceInfo[0]->SpecialInfo.stGigEInfo.nNetExport;
        }
        return 0;
    }
    else {
        return MV_GIGE_ForceIpEx(dev_handle, *ip, (255 << 24) + (255 << 16) + (255 << 8), *gateway);
    }
}

int Cam::pixel_type(bool read, int *val)
{
    int ret = 0;
    if (read) {
        MVCC_ENUMVALUE temp = {0};
        ret = MV_CC_GetPixelFormat(dev_handle, &temp);
        *val = temp.nCurValue;
    }
    else {
        switch (*val) {
        case PixelType_Gvsp_RGB8_Packed:
            img = cv::Mat(img.rows, img.cols, CV_8UC3);
            break;
        case PixelType_Gvsp_Mono8:
            img = cv::Mat(img.rows, img.cols, CV_8UC1);
            break;
        case PixelType_Gvsp_Mono10:
        case PixelType_Gvsp_Mono12:
        case PixelType_Gvsp_Mono10_Packed:
        case PixelType_Gvsp_Mono12_Packed:
            img = cv::Mat(img.rows, img.cols, CV_16UC1);
            break;
        default: break;
        }

        ret = MV_CC_SetPixelFormat(dev_handle, *val);
    }
    return ret;
}

void Cam::trigger_once()
{
    MV_CC_SetCommandValue(dev_handle, "TriggerSoftware");
}

void Cam::frame_cb(unsigned char *data, MV_FRAME_OUT_INFO_EX *frame_info, void *user_data)
{
//    static cv::Mat img(frame_info->nHeight, frame_info->nWidth, CV_8UC1);
//    img.data = data;
    switch (frame_info->enPixelType) {
    case PixelType_Gvsp_Mono8:
    case PixelType_Gvsp_Mono10:
    case PixelType_Gvsp_Mono12:
    case PixelType_Gvsp_RGB8_Packed:
        memcpy(img.data, data, frame_info->nFrameLen);
        break;
    case PixelType_Gvsp_Mono10_Packed:
    case PixelType_Gvsp_Mono12_Packed:
        break;
    default:
        img = 0;
        break;
    }

    ((std::queue<cv::Mat>*)user_data)->push(img.clone());
}

