#include "mvcam.h"

Cam::Cam() {dev_handle = NULL;}
Cam::~Cam() {if (dev_handle) CloseHandle(dev_handle), dev_handle = NULL;}

int Cam::search_for_devices()
{
    device_type = 0;
    MV_CC_DEVICE_INFO_LIST st_dev_list;

    // get and store devices list to m_stDevList
    MV_CC_EnumDevices(MV_GIGE_DEVICE, &st_dev_list);

    for (unsigned int i = 0; i < st_dev_list.nDeviceNum; i++) {
        MV_CC_DEVICE_INFO* dev = st_dev_list.pDeviceInfo[i];
        if (dev == NULL) continue;

        unsigned int ip;
        int ip_1, ip_2, ip_3, ip_4;
        char name[256] = {0};

        MV_GIGE_DEVICE_INFO gige_info = dev->SpecialInfo.stGigEInfo;
        ip = gige_info.nCurrentIp;
        ip_4 = ip & 0xFF; ip >>= 8;
        ip_3 = ip & 0xFF; ip >>= 8;
        ip_2 = ip & 0xFF; ip >>= 8;
        ip_1 = ip & 0xFF;

        if (strcmp("", (char*)gige_info.chUserDefinedName) == 0) {
            sprintf_s(name, 256, "%s %s (%s)", gige_info.chManufacturerName, gige_info.chModelName, gige_info.chSerialNumber);
        }
        else {
            sprintf_s(name, 256, "%s", gige_info.chUserDefinedName);
        }
        qDebug("[%d]GigE: %s (%d.%d.%d.%d)", i, name, ip_1, ip_2, ip_3, ip_4);
    }
    if (st_dev_list.nDeviceNum) device_type = 1;
    return st_dev_list.nDeviceNum;
}

int Cam::start() {
    MV_CC_DEVICE_INFO_LIST st_dev_list;

    // get and store devices list to m_stDevList
    int ret;
    MV_CC_EnumDevices(MV_GIGE_DEVICE, &st_dev_list);
    for (uint i = 0; i < st_dev_list.nDeviceNum; i++) {
        ret = MV_CC_CreateHandle(&dev_handle, st_dev_list.pDeviceInfo[0]);
        if (!ret) break;
    }

	ret = MV_CC_OpenDevice(dev_handle);
	if (ret != MV_OK) {
		MV_CC_DestroyHandle(dev_handle);
        dev_handle = NULL;
	}

    int size = MV_CC_GetOptimalPacketSize(dev_handle);
    if (size >= MV_OK) MV_CC_SetIntValueEx(dev_handle, "GevSCPSPacketSize", size);

    MV_CC_SetPixelFormat(dev_handle, PixelType_Gvsp_Mono8);
    MV_CC_SetEnumValue(dev_handle, "ExposureMode", MV_EXPOSURE_MODE_TIMED);
    MV_CC_SetEnumValue(dev_handle, "ExposureAuto", MV_EXPOSURE_AUTO_MODE_OFF);
    MV_CC_SetEnumValue(dev_handle, "GainAuto", 0);
    MV_CC_SetBoolValue(dev_handle, "AcquisitionFrameRateEnable", true);

//    MVCC_ENUMVALUE pix = {0};
//    MV_CC_GetPixelFormat(dev_handle, &pix);
//    switch (pix.nCurValue) {
//    case PixelType_Gvsp_Mono8:
//    case PixelType_Gvsp_Mono10:
//    case PixelType_Gvsp_Mono12:
//    case PixelType_Gvsp_Mono10_Packed:
//    case PixelType_Gvsp_Mono12_Packed:
//    break;
//    }

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

int Cam::get_img_buffer(MV_FRAME_OUT* frame, int msec) {
	return MV_CC_GetImageBuffer(dev_handle, frame, msec);
}

int Cam::free_img_buffer(MV_FRAME_OUT* frame) {
	return MV_CC_FreeImageBuffer(dev_handle, frame);
}

void Cam::get_frame_size(int &w, int &h)
{
    MVCC_INTVALUE int_value;
    MV_CC_GetIntValue(dev_handle, "Width", &int_value);
    w = int_value.nCurValue;
    MV_CC_GetIntValue(dev_handle, "Height", &int_value);
    h = int_value.nCurValue;
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

void Cam::trigger_once()
{
    MV_CC_SetCommandValue(dev_handle, "TriggerSoftware");
}

void Cam::frame_cb(unsigned char *data, MV_FRAME_OUT_INFO_EX *frame_info, void *user_data)
{
    static cv::Mat img(frame_info->nHeight, frame_info->nWidth, CV_8U);
    memcpy(img.data, data, frame_info->nFrameLen);
    ((std::queue<cv::Mat>*)user_data)->push(img.clone());
}

