#include "mvcam.h"

// Static arrays for each camera instance (up to 4 cameras)
static cv::Mat mv_img[4];
static cv::Mat bayer_temp[4];

// Static device lists - shared across all MvCam instances
MV_CC_DEVICE_INFO_LIST MvCam::gige_dev_list = {0};
MV_CC_DEVICE_INFO_LIST MvCam::usb3_dev_list = {0};

MvCam::MvCam()
{
    device_type = 1;
    dev_handle = NULL;
    curr_idx = 0;
    cam_idx = 0;  // Default to 0, will be set properly via set_user_pointer
}
MvCam::~MvCam() {if (dev_handle) MV_CC_DestroyHandle(dev_handle), dev_handle = NULL;}

int MvCam::search_for_devices()
{
    // FIXME: error 2 -2146885623
    MV_CC_EnumDevices(MV_GIGE_DEVICE, &gige_dev_list);
    MV_CC_EnumDevices(MV_USB_DEVICE,  &usb3_dev_list);
    int ret = 0;
    if (gige_dev_list.nDeviceNum) ret = MV_CC_CreateHandle(&dev_handle, gige_dev_list.pDeviceInfo[0]);
    else if (usb3_dev_list.nDeviceNum) ret = MV_CC_CreateHandle(&dev_handle, usb3_dev_list.pDeviceInfo[0]);

    return gige_dev_list.nDeviceNum + usb3_dev_list.nDeviceNum;
}

int MvCam::start(int idx) {
    curr_idx = idx;
    if (dev_handle) MV_CC_DestroyHandle(dev_handle), dev_handle = NULL;
    int ret = MV_CC_CreateHandle(&dev_handle, idx < gige_dev_list.nDeviceNum ? gige_dev_list.pDeviceInfo[idx] : usb3_dev_list.pDeviceInfo[idx - gige_dev_list.nDeviceNum]);

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

    // qDebug() << "start cam:" << hex << (uint)ret;
    return ret;
}

int MvCam::shut_down()
{
    if (dev_handle == NULL) return MV_E_HANDLE;

    int ret = MV_CC_CloseDevice(dev_handle);

    MV_CC_DestroyHandle(dev_handle);
    dev_handle = NULL;
    return ret;
}

int MvCam::set_user_pointer(void *user)
{
    // Extract camera index from the user data structure
    struct main_ui_info *info = (struct main_ui_info*)user;
    if (info) {
        cam_idx = info->cam_index;
    }
    return MV_CC_RegisterImageCallBackEx(dev_handle, frame_cb, user);
}

int MvCam::start_grabbing() {
    return MV_CC_StartGrabbing(dev_handle);
}

int MvCam::stop_grabbing() {
    return MV_CC_StopGrabbing(dev_handle);
}

void MvCam::get_max_frame_size(int *w, int *h)
{
    MVCC_INTVALUE int_value;
    MV_CC_GetIntValue(dev_handle, "WidthMax", &int_value);
    *w = int_value.nCurValue;
    MV_CC_GetIntValue(dev_handle, "HeightMax", &int_value);
    *h = int_value.nCurValue;
}

void MvCam::frame_size(bool read, int *w, int *h, int *inc_w, int *inc_h)
{
    if (read) {
        MVCC_INTVALUE int_value;
        MV_CC_GetIntValue(dev_handle, "Width", &int_value);
        *w = int_value.nCurValue;
        if (inc_w) *inc_w = int_value.nInc;
        MV_CC_GetIntValue(dev_handle, "Height", &int_value);
        *h = int_value.nCurValue;
        if (inc_h) *inc_h = int_value.nInc;
    }
    else {
        MV_CC_SetIntValue(dev_handle, "Width", *w);
        MV_CC_SetIntValue(dev_handle, "Height", *h);
    }
    MVCC_ENUMVALUE temp = {0};
    MV_CC_GetPixelFormat(dev_handle, &temp);
    switch (temp.nCurValue) {
    case PixelType_Gvsp_RGB8_Packed:
        mv_img[cam_idx] = cv::Mat(*h, *w, CV_8UC3);
        break;
    case PixelType_Gvsp_Mono8:
        mv_img[cam_idx] = cv::Mat(*h, *w, CV_8UC1);
        break;
    case PixelType_Gvsp_Mono10:
    case PixelType_Gvsp_Mono12:
    case PixelType_Gvsp_Mono10_Packed:
    case PixelType_Gvsp_Mono12_Packed:
        mv_img[cam_idx] = cv::Mat(*h, *w, CV_16UC1);
        break;
    default:
        mv_img[cam_idx] = cv::Mat();
        break;
    }
}

void MvCam::frame_offset(bool read, int *x, int *y, int *inc_x, int *inc_y)
{
    if (read) {
        MVCC_INTVALUE int_value;
        MV_CC_GetIntValue(dev_handle, "OffsetX", &int_value);
        *x = int_value.nCurValue;
        if (inc_x) *inc_x = int_value.nInc;
        MV_CC_GetIntValue(dev_handle, "OffsetY", &int_value);
        *y = int_value.nCurValue;
        if (inc_y) *inc_y = int_value.nInc;
    }
    else {
        MV_CC_SetIntValue(dev_handle, "OffsetX", *x);
        MV_CC_SetIntValue(dev_handle, "OffsetY", *y);
    }
}

void MvCam::time_exposure(bool read, float *val)
{
    if (read) {
        MVCC_FLOATVALUE f_val;
        MV_CC_GetFloatValue(dev_handle, "ExposureTime", &f_val);
        *val = f_val.fCurValue;
    }
    else MV_CC_SetFloatValue(dev_handle, "ExposureTime", *val);
}

void MvCam::frame_rate(bool read, float *val)
{
    if (read) {
        MVCC_FLOATVALUE f_val;
        MV_CC_GetFloatValue(dev_handle, "ResultingFrameRate", &f_val);
        *val = f_val.fCurValue;
    }
    else MV_CC_SetFloatValue(dev_handle, "AcquisitionFrameRate", *val);
}

void MvCam::gain_analog(bool read, float *val)
{
    if (read) {
        MVCC_FLOATVALUE f_val;
        MV_CC_GetFloatValue(dev_handle, "Gain", &f_val);
        *val = f_val.fCurValue;
    }
    else MV_CC_SetFloatValue(dev_handle, "Gain", *val);
}

void MvCam::trigger_mode(bool read, bool *val)
{
    if (read) {
        MVCC_ENUMVALUE enum_val;
        MV_CC_GetEnumValue(dev_handle, "TriggerMode", &enum_val);
        *val = enum_val.nCurValue;
    }
    else MV_CC_SetEnumValue(dev_handle, "TriggerMode", *val);
}

void MvCam::trigger_source(bool read, bool *val)
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

int MvCam::ip_config(bool read, int *val)
{
    int ret = 0;
    if (read) {
        if (curr_idx >= gige_dev_list.nDeviceNum) return 0;
        *val = gige_dev_list.pDeviceInfo[curr_idx]->SpecialInfo.stGigEInfo.nIpCfgCurrent;
    }
    else ret = MV_GIGE_SetIpConfig(dev_handle, MV_IP_CFG_STATIC);
    return ret;
}

int MvCam::ip_address(bool read, int idx, int *ip, int *gateway, int *nic_address)
{
    if (read) {
        if (idx >= gige_dev_list.nDeviceNum) return 0;
        *ip = gige_dev_list.pDeviceInfo[idx]->SpecialInfo.stGigEInfo.nCurrentIp;
        *gateway = gige_dev_list.pDeviceInfo[idx]->SpecialInfo.stGigEInfo.nDefultGateWay;
        if (nic_address) *nic_address = gige_dev_list.pDeviceInfo[idx]->SpecialInfo.stGigEInfo.nNetExport;
        return 0;
    }
    else {
        if (dev_handle) MV_CC_DestroyHandle(dev_handle), dev_handle = NULL;
        int ret = MV_CC_CreateHandle(&dev_handle, idx < gige_dev_list.nDeviceNum ? gige_dev_list.pDeviceInfo[idx] : usb3_dev_list.pDeviceInfo[idx - gige_dev_list.nDeviceNum]);
        ret = MV_GIGE_ForceIpEx(dev_handle, *ip, (255 << 24) + (255 << 16) + (255 << 8), *gateway);
//        qDebug() << "modify ip: " << hex << ret;
        return ret;
    }
}

int MvCam::pixel_type(bool read, int *val)
{
    int ret = 0;
    if (read) {
        MVCC_ENUMVALUE temp = {0};
        ret = MV_CC_GetPixelFormat(dev_handle, &temp);
        *val = temp.nCurValue;
    }
    else {
        switch (*val) {
        case PixelType_Gvsp_BayerRG8:
        case PixelType_Gvsp_RGB8_Packed:
            bayer_temp[cam_idx] = cv::Mat(mv_img[cam_idx].rows, mv_img[cam_idx].cols, CV_8UC1);
            mv_img[cam_idx] = cv::Mat(mv_img[cam_idx].rows, mv_img[cam_idx].cols, CV_8UC3);
            break;
        case PixelType_Gvsp_Mono8:
            mv_img[cam_idx] = cv::Mat(mv_img[cam_idx].rows, mv_img[cam_idx].cols, CV_8UC1);
            break;
        case PixelType_Gvsp_Mono10:
        case PixelType_Gvsp_Mono12:
        case PixelType_Gvsp_Mono10_Packed:
        case PixelType_Gvsp_Mono12_Packed:
            mv_img[cam_idx] = cv::Mat(mv_img[cam_idx].rows, mv_img[cam_idx].cols, CV_16UC1);
            break;
        default: break;
        }

        ret = MV_CC_SetPixelFormat(dev_handle, *val);
    }
    return ret;
}

void MvCam::trigger_once()
{
    MV_CC_SetCommandValue(dev_handle, "TriggerSoftware");
}

QStringList MvCam::get_device_list()
{
    QStringList sl;
    for (int i = 0; i < gige_dev_list.nDeviceNum; i++)
        sl << QString::asprintf("%s (%s)", gige_dev_list.pDeviceInfo[i]->SpecialInfo.stGigEInfo.chUserDefinedName, gige_dev_list.pDeviceInfo[i]->SpecialInfo.stGigEInfo.chModelName);
    for (int i = 0; i < usb3_dev_list.nDeviceNum; i++)
        sl << QString::asprintf("%s (%s)", usb3_dev_list.pDeviceInfo[i]->SpecialInfo.stUsb3VInfo.chUserDefinedName, usb3_dev_list.pDeviceInfo[i]->SpecialInfo.stUsb3VInfo.chModelName);
    return sl;
}

int MvCam::gige_device_num()
{
    return gige_dev_list.nDeviceNum;
}

int MvCam::usb3_device_num()
{
    return usb3_dev_list.nDeviceNum;
}

void MvCam::binning(bool read, int *val)
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

void MvCam::frame_cb(unsigned char *data, MV_FRAME_OUT_INFO_EX *frame_info, void *user_data)
{
    struct main_ui_info *ptr = (struct main_ui_info*)user_data;
    if (!ptr) return;

    // Get the camera index to access the correct array element
    int idx = ptr->cam_index;
    if (idx < 0 || idx >= 4) return;  // Safety check

    // Use the pre-allocated static arrays indexed by camera
    switch (frame_info->enPixelType) {
    case PixelType_Gvsp_Mono8:
    case PixelType_Gvsp_Mono10:
    case PixelType_Gvsp_Mono12:
    case PixelType_Gvsp_RGB8_Packed:
        memcpy(mv_img[idx].data, data, frame_info->nFrameLen);
        break;
    case PixelType_Gvsp_Mono10_Packed:
    case PixelType_Gvsp_Mono12_Packed:
        break;
    case PixelType_Gvsp_BayerRG8:
        memcpy(bayer_temp[idx].data, data, frame_info->nFrameLen);
        cv::cvtColor(bayer_temp[idx], mv_img[idx], cv::COLOR_BayerRG2RGB);
        break;
    default:
        mv_img[idx] = cv::Mat();
        break;
    }

//    ImageIO::save_image_bmp(mv_img[idx], "imgs/" + QDateTime::currentDateTime().toString("hhMMss.zzz") + ".bmp");

    // Protect both queues with their respective mutexes
    if (ptr->frame_info_mutex) {
        QMutexLocker frame_locker(ptr->frame_info_mutex);
        ptr->frame_info_q->push(frame_info->nLostPacket);
    }

    if (ptr->img_mutex) {
        QMutexLocker img_locker(ptr->img_mutex);
        ptr->img_q->push(mv_img[idx].clone());
    }
//    cv::imwrite("../mv.bmp", mv_img[idx]);
}
