#include "euresyscam.h"

int x;
int y;
QMutex mutex;

EuresysCam::EuresysCam()
{
    device_type = 3;
    dev_handle = NULL;
    curr_idx = 0;
}
EuresysCam::~EuresysCam() {if (dev_handle) shut_down();}

int EuresysCam::search_for_devices()
{
    // Initialize driver and error handling
    MCSTATUS ret = McOpenDriver(NULL);
    if (ret == MC_NO_BOARD_FOUND) return 0;

    // Activate message box error handling and generate an error log file
    // McSetParamInt (MC_CONFIGURATION, MC_ErrorHandling, MC_ErrorHandling_MSGBOX);
    // McSetParamStr (MC_CONFIGURATION, MC_ErrorLog, "error.log");

    return 1;
}

int EuresysCam::start(int idx) {
#if CLALLSERIAL
    clSerialClose(serial_ref);
    int cl_ret = clSerialInit(0, &serial_ref);
//    if (cl_ret != CL_ERR_NO_ERR) return cl_ret;
    cl_ret = clSetBaudRate(serial_ref, CL_BAUDRATE_115200);
#endif

    MCSTATUS ret = 0;
    // Create a channel and associate it with the first connector on the first board
    McCreate(MC_CHANNEL, &dev_handle);
    McSetParamInt(dev_handle, MC_DriverIndex, 0);

    // In order to use single camera on connector A
    // MC_Connector need to be set to A for Grablink DualBase
    // For all the other Grablink boards the parameter has to be set to M  
    
    // For all GrabLink boards but Grablink DualBase
    ret = McSetParamStr(dev_handle, MC_Connector, "M");

    // Configure the height of a slice (1920 lines)
    ret = McSetParamInt(dev_handle, MC_Hactive_Px, 1920);
    ret = McSetParamInt(dev_handle, MC_Vactive_Ln, 1080);

    // Choose the pixel color format
    ret = McSetParamInt(dev_handle, MC_Spectrum, MC_Spectrum_COLOR);
    // ret = McSetParamInt(dev_handle, MC_ColorMethod, MC_ColorMethod_PRISM);
    // ret = McSetParamInt(dev_handle, MC_ColorFormat, MC_ColorFormat_Y8);
    ret = McSetParamInt(dev_handle, MC_ColorFormat, MC_ColorFormat_RGB24);

    // ret = McSetParamInt(dev_handle, MC_TapConfiguration, MC_TapConfiguration_BASE_2T8);
    // ret = McSetParamInt(dev_handle, MC_TapConfiguration, MC_TapConfiguration_BASE_1T8);
    ret = McSetParamInt(dev_handle, MC_TapConfiguration, MC_TapConfiguration_BASE_1T24);

    // Set the acquisition mode
    ret = McSetParamInt(dev_handle, MC_AcquisitionMode, MC_AcquisitionMode_HFR);

    // Choose the number of Frames in a phase
    ret = McSetParamInt(dev_handle, MC_PhaseLength_Fr, 1);

    // Choose the way the first acquisition is triggered
    ret = McSetParamInt(dev_handle, MC_TrigMode, MC_TrigMode_IMMEDIATE);
    // Choose the triggering mode for subsequent acquisitions
    ret = McSetParamInt(dev_handle, MC_NextTrigMode, MC_NextTrigMode_REPEAT);
    // Choose the number of images to acquire
    ret = McSetParamInt(dev_handle, MC_SeqLength_Fr, MC_INDETERMINATE);

    // Enable MultiCam signals
    ret = McSetParamInt(dev_handle, MC_SignalEnable + MC_SIG_SURFACE_PROCESSING, MC_SignalEnable_ON);
    ret = McSetParamInt(dev_handle, MC_SignalEnable + MC_SIG_ACQUISITION_FAILURE, MC_SignalEnable_ON);

    return ret;
}

int EuresysCam::shut_down()
{
    if (dev_handle == NULL) return MC_NO_BOARD_FOUND;

    int ret = McDelete(dev_handle);
    dev_handle = NULL;

#ifdef CLALLSERIAL
    if (serial_ref) clSerialClose(serial_ref);
#endif

    return ret;
}

int EuresysCam::set_user_pointer(void *user)
{
    // Register the callback function
    return McRegisterCallback(dev_handle, frame_cb, user);
}

int EuresysCam::start_grabbing() {
    // Start an acquisition sequence by activating the channel
    return McSetParamInt(dev_handle, MC_ChannelState, MC_ChannelState_ACTIVE);
}

int EuresysCam::stop_grabbing() {
    // Stop an acquisition sequence by deactivating the channel
    return McSetParamInt(dev_handle, MC_ChannelState, MC_ChannelState_IDLE);
}

void EuresysCam::get_max_frame_size(int *w, int *h)
{
    *w = x = 1920;
    *h = y = 1080;
    // MVCC_INTVALUE int_value;
    // MV_CC_GetIntValue(dev_handle, "Width", &int_value);
    // w = x = int_value.nCurValue;
    // MV_CC_GetIntValue(dev_handle, "Height", &int_value);
    // h = y = int_value.nCurValue;
}

void EuresysCam::frame_size(bool read, int *w, int *h, int *inc_w, int *inc_h)
{
    *w = x = 1920;
    *h = y = 1080;
}

void EuresysCam::frame_offset(bool read, int *x, int *y, int *inc_x, int *inc_y)
{

}

void EuresysCam::time_exposure(bool read, float *val)
{
#if CLALLSERIAL
    if (read) {
        uchar out[7] = {0xB0, 0x00, 0x00, 0xFF, 0x21, 0x1D, 0x1A};
        uchar in[6] = {0};
        *val = communicate((char*)out, (char*)in, 7, 6, true);
        out[4] = 0x22;
        *val += communicate((char*)out, (char*)in, 7, 6, true) * 65536;
    }
    else {
        int te = *val;
        uchar out[7] = {0xA0, 0x00, 0x00, 0xFF, 0x21, 0x0D, 0x0A};
        uchar in[1] = {0};
        communicate((char*)out, (char*)in, 7, 1);
        out[0] = 0xA1;
        out[1] = (te >> 24) & 0xFF;
        out[2] = (te >> 16) & 0xFF;
        out[3] = (te >> 8) & 0xFF;
        out[4] = te & 0xFF;
        communicate((char*)out, (char*)in, 7, 1);
    }
#endif
}

void EuresysCam::frame_rate(bool read, float *val)
{
#if CLALLSERIAL
    if (read) {
        uchar out[7] = {0xB0, 0x00, 0x00, 0xFF, 0x1F, 0x1D, 0x1A};
        uchar in[6] = {0};
        *val = communicate((char*)out, (char*)in, 7, 6, true);
    }
    else {
        uchar out[7] = {0xA0, 0x00, 0x00, 0xFF, 0x1F, 0x0D, 0x0A};
        uchar in[1] = {0};
        communicate((char*)out, (char*)in, 7, 1);
        out[0] = 0xA1;
        out[3] = ((int)(*val) >> 8) & 0xFF;
        out[4] = (int)(*val) & 0xFF;
        communicate((char*)out, (char*)in, 7, 1);
    }
#endif
}

void EuresysCam::gain_analog(bool read, float *val)
{
#ifdef CLALLSERIAL
    if (read) {
        uchar out[7] = {0xB0, 0x00, 0x00, 0xFF, 0x17, 0x1D, 0x1A};
        uchar in[6] = {0};
        *val = communicate((char*)out, (char*)in, 7, 6, true);
    }
    else {
        uchar out[7] = {0xA0, 0x00, 0x00, 0xFF, 0x17, 0x0D, 0x0A};
        uchar in[1] = {0};
        communicate((char*)out, (char*)in, 7, 1);
        out[0] = 0xA1;
        out[3] = ((int)(*val) >> 8) & 0xFF;
        out[4] = (int)(*val) & 0xFF;
        communicate((char*)out, (char*)in, 7, 1);
    }
#endif
}

void EuresysCam::trigger_mode(bool read, bool *val)
{
#ifdef CLALLSERIAL
    if (read) {
        uchar out[7] = {0xB0, 0x00, 0x00, 0xFF, 0x1D, 0x1D, 0x1A};
        uchar in[6] = {0};
        *val = communicate((char*)out, (char*)in, 7, 6, true);
    }
    else {
        uchar out[7] = {0xA0, 0x00, 0x00, 0xFF, 0x1D, 0x0D, 0x0A};
        uchar in[1] = {0};
        communicate((char*)out, (char*)in, 7, 1);
        out[0] = 0xA1;
        out[3] = 0x00;
        out[4] = *val ? 0x02 : 0x00;
        communicate((char*)out, (char*)in, 7, 1);
    }
#endif
}

void EuresysCam::trigger_source(bool read, bool *val)
{
#ifdef CLALLSERIAL
    if (read) {
        uchar out[7] = {0xB0, 0x00, 0x00, 0xFF, 0x1D, 0x1D, 0x1A};
        uchar in[6] = {0};
        *val = communicate((char*)out, (char*)in, 7, 6, true) == 1;
    }
    else {
        uchar out[7] = {0xA0, 0x00, 0x00, 0xFF, 0x1D, 0x0D, 0x0A};
        uchar in[1] = {0};
        communicate((char*)out, (char*)in, 7, 1);
        out[0] = 0xA1;
        out[3] = 0x00;
        out[4] = *val ? 0x02 : 0x01;
        communicate((char*)out, (char*)in, 7, 1);
    }
#endif
}

int EuresysCam::ip_config(bool read, int *val)
{
    return 0;
}

int EuresysCam::ip_address(bool read, int *ip, int *gateway, int *nic_address)
{
    return 0;
}

int EuresysCam::pixel_type(bool read, int *val)
{
    if (val) *val = 35127316;
    return 0;
}

void EuresysCam::trigger_once()
{
    // MV_CC_SetCommandValue(dev_handle, "TriggerSoftware");
}

QStringList EuresysCam::get_device_list()
{
    return QStringList();
}

int EuresysCam::gige_device_num()
{
    return 0;
}

int EuresysCam::usb3_device_num()
{
    return 0;
}

void EuresysCam::frame_cb(PMCCALLBACKINFO cb_info)
{
    void *data;
    // Update "current" surface address pointer
    McGetParamPtr(cb_info->SignalInfo, MC_SurfaceAddr, &data);
    static cv::Mat img(y, x, CV_8UC3), img_rb_swapped(y, x, CV_8UC3);
    memcpy(img.data, data, x * y * 3);
    cv::cvtColor(img, img_rb_swapped, cv::COLOR_BGR2RGB);
    // pointer named user passed in here is called by cb_info->Context
    ((struct main_ui_info*)(cb_info->Context))->frame_info_q->push(0);
    ((struct main_ui_info*)(cb_info->Context))->img_q->push(img_rb_swapped.clone());
}

#if CLALLSERIAL
float Cam::communicate(char* out, char* in, uint out_size, uint in_size, bool read) {
    mutex.lock();
    QString str_s = "s", str_r = "r", str_d = "d";
    static uint temp_size = 1;
    static char temp = 0;
    //    while(!clSerialRead(serial_ref, &temp, &temp_size, 20)) temp_size = 1;// qDebug() << temp_size;

    clFlushPort(serial_ref);

    clSerialWrite(serial_ref, out, &out_size, 10);// qDebug() << out_size;
    for (int i = 0; i < 7; i++) str_s += QString::asprintf(" %02X", i + (int)out_size - 7 < 0 ? 0 : ((uchar*)out)[i + out_size - 7]);

    //    clFlushPort(serial_ref);

    QByteArray in_buffer;
    temp_size = 1;
    while(!clSerialRead(serial_ref, &temp, &temp_size, 10)) in_buffer.append(temp), temp_size = 1;
    //    if (in_buffer.size() < (int)in_size) {read = false; goto UNLOCK;}
    static QByteArray ending_r((char*)new uchar[2]{0x55, 0xAA}, 2);
    static QByteArray ending_wa((char*)new uchar[1]{0xA0}, 1);
    static QByteArray ending_wd((char*)new uchar[1]{0xA1}, 1);
    int idx = -1;
    switch ((uchar)out[0]) {
    case 0xA0: idx = in_buffer.indexOf(ending_wa); qDebug() << "wa" << idx; break;
    case 0xA1: idx = in_buffer.indexOf(ending_wd); qDebug() << "wd" << idx; break;
    case 0xB0: idx = in_buffer.indexOf(ending_r) - 4; qDebug() << "rr" << idx; break;
    default: break;
    }
    if (idx < 0) {read = false; goto UNLOCK;}
    memcpy(in, in_buffer.data() + idx, in_size);

    for (int i = 0; i < in_buffer.size(); i++) str_d += QString::asprintf(" %02X", (uchar)in_buffer[i]);
    for (int i = 0; i < 6; i++) str_r += QString::asprintf(" %02X", i + (int)in_size - 6 < 0 ? 0 : ((uchar*)in)[i + in_size - 6]);

    qDebug() << str_s << str_d << str_r;

UNLOCK:
    mutex.unlock();
    return read ? (((uchar*)in)[0] << 24) + (((uchar*)in)[1] << 16) + (((uchar*)in)[2] << 8) + ((uchar*)in)[3] : 0;
}
#endif
