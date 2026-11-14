#include "mvcam.h"

cv::Mat mv_img;
cv::Mat bayer_temp;

MvCam::MvCam()
{
    m_device_type = 1;
    m_dev_handle = nullptr;
    m_reconnect_required = false;
    m_curr_idx = 0;
}
MvCam::~MvCam() { release_handle(); }

// Stop, close, destroy, in the order the SDK expects. Stop is sent even when the
// link is down: after a disconnect the handle still has an acquisition armed, and
// reusing it in that state is where the restart crash starts.
int MvCam::release_handle()
{
    if (!m_dev_handle) return MV_E_HANDLE;
    MV_CC_StopGrabbing(m_dev_handle);
    const int ret = MV_CC_CloseDevice(m_dev_handle);
    MV_CC_DestroyHandle(m_dev_handle);
    m_dev_handle = nullptr;
    return ret;
}

void MvCam::configure_device()
{
    if (!m_dev_handle) return;

    MV_CC_RegisterExceptionCallBack(m_dev_handle, exception_cb, this);
    m_reconnect_required = false;

    int size = MV_CC_GetOptimalPacketSize(m_dev_handle);
    if (size >= MV_OK) MV_CC_SetIntValueEx(m_dev_handle, "GevSCPSPacketSize", size);

    // Keep the acquisition engine armed for repeated frames. TriggerMode can
    // still be enabled separately; this only prevents one-shot acquisition.
    MV_CC_SetEnumValueByString(m_dev_handle, "AcquisitionMode", "Continuous");
    MV_CC_SetPixelFormat(m_dev_handle, PixelType_Gvsp_Mono8);
    MV_CC_SetEnumValue(m_dev_handle, "ExposureMode", MV_EXPOSURE_MODE_TIMED);
    MV_CC_SetEnumValue(m_dev_handle, "ExposureAuto", MV_EXPOSURE_AUTO_MODE_OFF);
    MV_CC_SetEnumValue(m_dev_handle, "GainAuto", 0);
    MV_CC_SetBoolValue(m_dev_handle, "AcquisitionFrameRateEnable", true);
    MV_CC_SetEnumValue(m_dev_handle, "BinningHorizontal", 1);
    MV_CC_SetEnumValue(m_dev_handle, "BinningVertical", 1);
}

void __stdcall MvCam::exception_cb(unsigned int message_type, void *user_data)
{
    if (message_type != MV_EXCEPTION_DEV_DISCONNECT || !user_data) return;
    static_cast<MvCam*>(user_data)->m_reconnect_required = true;
}

int MvCam::search_for_devices()
{
    // FIXME: error 2 -2146885623
    MV_CC_EnumDevices(MV_GIGE_DEVICE, &m_gige_dev_list);
    MV_CC_EnumDevices(MV_USB_DEVICE,  &m_usb3_dev_list);
    int ret = 0;
    if (m_gige_dev_list.nDeviceNum) ret = MV_CC_CreateHandle(&m_dev_handle, m_gige_dev_list.pDeviceInfo[0]);
    else if (m_usb3_dev_list.nDeviceNum) ret = MV_CC_CreateHandle(&m_dev_handle, m_usb3_dev_list.pDeviceInfo[0]);

    return m_gige_dev_list.nDeviceNum + m_usb3_dev_list.nDeviceNum;
}

int MvCam::start(int idx)
{
    m_curr_idx = idx;
    MV_CC_DEVICE_INFO *device_info = nullptr;
    if (idx >= 0 && idx < static_cast<int>(m_gige_dev_list.nDeviceNum)) {
        device_info = m_gige_dev_list.pDeviceInfo[idx];
    }
    else {
        const int usb_idx = idx - static_cast<int>(m_gige_dev_list.nDeviceNum);
        if (usb_idx >= 0 && usb_idx < static_cast<int>(m_usb3_dev_list.nDeviceNum))
            device_info = m_usb3_dev_list.pDeviceInfo[usb_idx];
    }
    if (!device_info) return MV_E_PARAMETER;

    if (m_dev_handle) release_handle();

    int ret = MV_CC_CreateHandle(&m_dev_handle, device_info);
    if (ret != MV_OK) {
        m_dev_handle = nullptr;
        return ret;
    }

    ret = MV_CC_OpenDevice(m_dev_handle);
    if (ret != MV_OK) {
        MV_CC_DestroyHandle(m_dev_handle);
        m_dev_handle = nullptr;
        return ret;
    }

    configure_device();

    // qDebug() << "start cam:" << hex << (uint)ret;
    return ret;
}

bool MvCam::is_connected() const
{
    if (!m_dev_handle || m_reconnect_required.load()) return false;
    const bool connected = MV_CC_IsDeviceConnected(m_dev_handle);
    if (!connected) m_reconnect_required = true;
    return connected;
}

int MvCam::reconnect()
{
    if (!m_dev_handle) return MV_E_HANDLE;
    if (is_connected()) return MV_OK;

    // Required by the MVS SDK after an exceptional GigE disconnect. The stop
    // comes first for the same reason as in release_handle().
    MV_CC_StopGrabbing(m_dev_handle);
    MV_CC_CloseDevice(m_dev_handle);
    const int ret = MV_CC_OpenDevice(m_dev_handle);
    if (ret == MV_OK) configure_device();
    return ret;
}

int MvCam::shut_down()
{
    return release_handle();
}

int MvCam::set_user_pointer(StreamFrameSink *sink)
{
    if (!is_connected()) return MV_E_HANDLE;
    void *user = sink;
    return MV_CC_RegisterImageCallBackEx(m_dev_handle, frame_cb, user);
}

int MvCam::start_grabbing()
{
    if (!is_connected()) return MV_E_HANDLE;
    return MV_CC_StartGrabbing(m_dev_handle);
}

// Not gated on the connection check: a stop after a link loss is exactly the
// one the SDK needs before the handle is reused.
int MvCam::stop_grabbing()
{
    if (!m_dev_handle) return MV_E_HANDLE;
    return MV_CC_StopGrabbing(m_dev_handle);
}

// Dimensions read back as zero on any failure, so a caller can tell a dead
// link from a real size instead of building a buffer from stack garbage.
void MvCam::get_max_frame_size(int *w, int *h)
{
    MVCC_INTVALUE width_value = {0};
    MVCC_INTVALUE height_value = {0};
    *w = MV_CC_GetIntValue(m_dev_handle, "WidthMax", &width_value) == MV_OK ? width_value.nCurValue : 0;
    *h = MV_CC_GetIntValue(m_dev_handle, "HeightMax", &height_value) == MV_OK ? height_value.nCurValue : 0;
}

void MvCam::frame_size(bool read, int *w, int *h, int *inc_w, int *inc_h)
{
    if (read) {
        MVCC_INTVALUE width_value = {0};
        MVCC_INTVALUE height_value = {0};
        const bool ok_w = MV_CC_GetIntValue(m_dev_handle, "Width", &width_value) == MV_OK;
        const bool ok_h = MV_CC_GetIntValue(m_dev_handle, "Height", &height_value) == MV_OK;
        *w = ok_w ? width_value.nCurValue : 0;
        *h = ok_h ? height_value.nCurValue : 0;
        if (inc_w) *inc_w = ok_w ? width_value.nInc : 0;
        if (inc_h) *inc_h = ok_h ? height_value.nInc : 0;
    }
    else {
        MV_CC_SetIntValue(m_dev_handle, "Width", *w);
        MV_CC_SetIntValue(m_dev_handle, "Height", *h);
    }
    if (*w <= 0 || *h <= 0) {
        mv_img.release();
        return;
    }
    MVCC_ENUMVALUE temp = {0};
    MV_CC_GetPixelFormat(m_dev_handle, &temp);
    switch (temp.nCurValue) {
    case PixelType_Gvsp_RGB8_Packed:
        mv_img = cv::Mat(*h, *w, CV_8UC3);
        break;
    case PixelType_Gvsp_Mono8:
        mv_img = cv::Mat(*h, *w, CV_8UC1);
        break;
    case PixelType_Gvsp_Mono10:
    case PixelType_Gvsp_Mono12:
    case PixelType_Gvsp_Mono10_Packed:
    case PixelType_Gvsp_Mono12_Packed:
        mv_img = cv::Mat(*h, *w, CV_16UC1);
        break;
    default:
        mv_img = 0;
        break;
    }
}

void MvCam::frame_offset(bool read, int *x, int *y, int *inc_x, int *inc_y)
{
    if (read) {
        MVCC_INTVALUE int_value = {0};
        MV_CC_GetIntValue(m_dev_handle, "OffsetX", &int_value);
        *x = int_value.nCurValue;
        if (inc_x) *inc_x = int_value.nInc;
        MV_CC_GetIntValue(m_dev_handle, "OffsetY", &int_value);
        *y = int_value.nCurValue;
        if (inc_y) *inc_y = int_value.nInc;
    }
    else {
        MV_CC_SetIntValue(m_dev_handle, "OffsetX", *x);
        MV_CC_SetIntValue(m_dev_handle, "OffsetY", *y);
    }
}

void MvCam::time_exposure(bool read, float *val)
{
    if (read) {
        MVCC_FLOATVALUE f_val = {0};
        MV_CC_GetFloatValue(m_dev_handle, "ExposureTime", &f_val);
        *val = f_val.fCurValue;
    }
    else MV_CC_SetFloatValue(m_dev_handle, "ExposureTime", *val);
}

void MvCam::frame_rate(bool read, float *val)
{
    if (read) {
        MVCC_FLOATVALUE f_val = {0};
        MV_CC_GetFloatValue(m_dev_handle, "ResultingFrameRate", &f_val);
        *val = f_val.fCurValue;
    }
    else MV_CC_SetFloatValue(m_dev_handle, "AcquisitionFrameRate", *val);
}

void MvCam::gain_analog(bool read, float *val)
{
    if (read) {
        MVCC_FLOATVALUE f_val = {0};
        MV_CC_GetFloatValue(m_dev_handle, "Gain", &f_val);
        *val = f_val.fCurValue;
    }
    else MV_CC_SetFloatValue(m_dev_handle, "Gain", *val);
}

void MvCam::trigger_mode(bool read, bool *val)
{
    if (read) {
        MVCC_ENUMVALUE enum_val = {0};
        MV_CC_GetEnumValue(m_dev_handle, "TriggerMode", &enum_val);
        *val = enum_val.nCurValue;
    }
    else MV_CC_SetEnumValue(m_dev_handle, "TriggerMode", *val);
}

void MvCam::trigger_source(bool read, bool *val)
{
    if (read) {
        MVCC_ENUMVALUE enum_val = {0};
        MV_CC_GetEnumValue(m_dev_handle, "TriggerSource", &enum_val);
        *val = enum_val.nCurValue == MV_TRIGGER_SOURCE_SOFTWARE;
    }
    else {
        int source = *val ? MV_TRIGGER_SOURCE_SOFTWARE : MV_TRIGGER_SOURCE_LINE0;
        MV_CC_SetEnumValue(m_dev_handle, "TriggerSource", source);
    }
}

int MvCam::ip_config(bool read, int *val)
{
    int ret = 0;
    if (read) {
        if (m_curr_idx >= m_gige_dev_list.nDeviceNum) return 0;
        *val = m_gige_dev_list.pDeviceInfo[m_curr_idx]->SpecialInfo.stGigEInfo.nIpCfgCurrent;
    }
    else ret = MV_GIGE_SetIpConfig(m_dev_handle, MV_IP_CFG_STATIC);
    return ret;
}

int MvCam::ip_address(bool read, int idx, int *ip, int *gateway, int *nic_address)
{
    if (read) {
        if (idx >= m_gige_dev_list.nDeviceNum) return 0;
        *ip = m_gige_dev_list.pDeviceInfo[idx]->SpecialInfo.stGigEInfo.nCurrentIp;
        *gateway = m_gige_dev_list.pDeviceInfo[idx]->SpecialInfo.stGigEInfo.nDefultGateWay;
        if (nic_address) *nic_address = m_gige_dev_list.pDeviceInfo[idx]->SpecialInfo.stGigEInfo.nNetExport;
        return 0;
    }
    else {
        release_handle();
        int ret = MV_CC_CreateHandle(&m_dev_handle, idx < m_gige_dev_list.nDeviceNum ? m_gige_dev_list.pDeviceInfo[idx] : m_usb3_dev_list.pDeviceInfo[idx - m_gige_dev_list.nDeviceNum]);
        ret = MV_GIGE_ForceIpEx(m_dev_handle, *ip, (255 << 24) + (255 << 16) + (255 << 8), *gateway);
//        qDebug() << "modify ip: " << hex << ret;
        return ret;
    }
}

int MvCam::pixel_type(bool read, int *val)
{
    int ret = 0;
    if (read) {
        MVCC_ENUMVALUE temp = {0};
        ret = MV_CC_GetPixelFormat(m_dev_handle, &temp);
        *val = temp.nCurValue;
    }
    else {
        switch (*val) {
        case PixelType_Gvsp_BayerRG8:
        case PixelType_Gvsp_RGB8_Packed:
            bayer_temp = cv::Mat(mv_img.rows, mv_img.cols, CV_8UC1);
            mv_img = cv::Mat(mv_img.rows, mv_img.cols, CV_8UC3);
            break;
        case PixelType_Gvsp_Mono8:
            mv_img = cv::Mat(mv_img.rows, mv_img.cols, CV_8UC1);
            break;
        case PixelType_Gvsp_Mono10:
        case PixelType_Gvsp_Mono12:
        case PixelType_Gvsp_Mono10_Packed:
        case PixelType_Gvsp_Mono12_Packed:
            mv_img = cv::Mat(mv_img.rows, mv_img.cols, CV_16UC1);
            break;
        default: break;
        }

        ret = MV_CC_SetPixelFormat(m_dev_handle, *val);
    }
    return ret;
}

void MvCam::trigger_once()
{
    MV_CC_SetCommandValue(m_dev_handle, "TriggerSoftware");
}

QStringList MvCam::get_device_list()
{
    QStringList sl;
    for (int i = 0; i < m_gige_dev_list.nDeviceNum; i++)
        sl << QString::asprintf("%s (%s)", m_gige_dev_list.pDeviceInfo[i]->SpecialInfo.stGigEInfo.chUserDefinedName, m_gige_dev_list.pDeviceInfo[i]->SpecialInfo.stGigEInfo.chModelName);
    for (int i = 0; i < m_usb3_dev_list.nDeviceNum; i++)
        sl << QString::asprintf("%s (%s)", m_usb3_dev_list.pDeviceInfo[i]->SpecialInfo.stUsb3VInfo.chUserDefinedName, m_usb3_dev_list.pDeviceInfo[i]->SpecialInfo.stUsb3VInfo.chModelName);
    return sl;
}

int MvCam::gige_device_num()
{
    return m_gige_dev_list.nDeviceNum;
}

int MvCam::usb3_device_num()
{
    return m_usb3_dev_list.nDeviceNum;
}

void MvCam::binning(bool read, int *val)
{
    if (read) {
        MVCC_ENUMVALUE enum_val = {0};
        MV_CC_GetEnumValue(m_dev_handle, "BinningHorizontal", &enum_val);
        MV_CC_GetEnumValue(m_dev_handle, "BinningVertical", &enum_val);
        *val = enum_val.nCurValue;
    }
    else {
        MV_CC_SetEnumValue(m_dev_handle, "BinningHorizontal", *val);
        MV_CC_SetEnumValue(m_dev_handle, "BinningVertical", *val);
    }
}

void MvCam::frame_cb(unsigned char *data, MV_FRAME_OUT_INFO_EX *frame_info, void *user_data)
{
//    static cv::Mat mv_img(frame_info->nHeight, frame_info->nWidth, CV_8UC1);
//    mv_img.data = data;
    switch (frame_info->enPixelType) {
    case PixelType_Gvsp_Mono8:
    case PixelType_Gvsp_Mono10:
    case PixelType_Gvsp_Mono12:
    case PixelType_Gvsp_RGB8_Packed:
        memcpy(mv_img.data, data, frame_info->nFrameLen);
        break;
    case PixelType_Gvsp_Mono10_Packed:
    case PixelType_Gvsp_Mono12_Packed:
        break;
    case PixelType_Gvsp_BayerRG8:
        memcpy(bayer_temp.data, data, frame_info->nFrameLen);
        cv::cvtColor(bayer_temp, mv_img, cv::COLOR_BayerRG2RGB);
    default:
        mv_img = 0;
        break;
    }

//    ImageIO::save_image_bmp(mv_img, "imgs/" + QDateTime::currentDateTime().toString("hhMMss.zzz") + ".bmp");

    StreamFrameSink *sink = (StreamFrameSink*)user_data;

    // BoundedQueue is internally thread-safe; no external mutex.
    if (sink->frame_info) sink->frame_info->push(frame_info->nLostPacket);
    if (sink->frames)     sink->frames->push(mv_img.clone());
//    cv::imwrite("../mv.bmp", mv_img);
}
