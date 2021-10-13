#include "euresyscam.h"

int x;
int y;

Cam::Cam() {dev_handle = NULL;}
Cam::~Cam() {if (dev_handle) shut_down();}

int Cam::search_for_devices()
{
    // Initialize driver and error handling
    MCSTATUS ret = McOpenDriver(NULL);
    if (ret == MC_NO_BOARD_FOUND) return 0;

    // Activate message box error handling and generate an error log file
    // McSetParamInt (MC_CONFIGURATION, MC_ErrorHandling, MC_ErrorHandling_MSGBOX);
    // McSetParamStr (MC_CONFIGURATION, MC_ErrorLog, "error.log");

    // Create a channel and associate it with the first connector on the first board
    McCreate(MC_CHANNEL, &dev_handle);
    McSetParamInt(dev_handle, MC_DriverIndex, 0);

    return 1;
}

int Cam::start() {
    int cl_ret = clSerialInit(0, &serial_ref);
    if (cl_ret != CL_ERR_NO_ERR) return cl_ret;

    MCSTATUS ret = 0;
    // In order to use single camera on connector A
    // MC_Connector need to be set to A for Grablink DualBase
    // For all the other Grablink boards the parameter has to be set to M  
    
    // For all GrabLink boards but Grablink DualBase
    ret = McSetParamStr(dev_handle, MC_Connector, "M");

    // Choose the pixel color format
    ret = McSetParamInt(dev_handle, MC_ColorFormat, MC_ColorFormat_Y8);
    ret = McSetParamInt(dev_handle, MC_TapConfiguration, MC_TapConfiguration_BASE_2T8);

    // Set the acquisition mode
    ret = McSetParamInt(dev_handle, MC_AcquisitionMode, MC_AcquisitionMode_HFR);

    // Configure the height of a slice (1920 lines)
    ret = McSetParamInt(dev_handle, MC_Hactive_Px, 1920);
    ret = McSetParamInt(dev_handle, MC_Vactive_Ln, 1080);

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

    cl_ret = clSetBaudRate(serial_ref, CL_BAUDRATE_115200);

    return ret;
}

int Cam::shut_down()
{
    if (dev_handle == NULL) return MC_NO_BOARD_FOUND;

    int ret = McDelete(dev_handle);
    dev_handle = NULL;

    return ret;
}

int Cam::set_frame_callback(void *user)
{
    // Register the callback function
    return McRegisterCallback(dev_handle, frame_cb, user);
}

int Cam::start_grabbing() {
    // Start an acquisition sequence by activating the channel
    return McSetParamInt(dev_handle, MC_ChannelState, MC_ChannelState_ACTIVE);
}

int Cam::stop_grabbing() {
    // Stop an acquisition sequence by deactivating the channel
    return McSetParamInt(dev_handle, MC_ChannelState, MC_ChannelState_IDLE);
}

void Cam::get_frame_size(int &w, int &h)
{
    w = x = 1920;
    h = y = 1080;
    // MVCC_INTVALUE int_value;
    // MV_CC_GetIntValue(dev_handle, "Width", &int_value);
    // w = x = int_value.nCurValue;
    // MV_CC_GetIntValue(dev_handle, "Height", &int_value);
    // h = y = int_value.nCurValue;
}

float Cam::communicate(char* out, char* in, uint out_size, uint in_size, bool read) {
    clSerialWrite(serial_ref, out, &out_size, 100);
    QThread::msleep(10);

    clSerialRead(serial_ref, (char*)in, &in_size, 100);
    return read ? (in[3] << 8) + in[4] : 0;
}

void Cam::time_exposure(bool read, float *val)
{
    if (read) {
        uchar out[7] = {0xB0, 0x00, 0x00, 0xFF, 0x21, 0x0D, 0x1A};
        uchar in[6] = {0};
        *val = communicate((char*)out, (char*)in, 7, 6, true) / 1000.0;
    }
    else {
        int te = *val / 1000;
        uchar out[7] = {0xA0, 0x00, 0x00, 0xFF, 0x21, 0x0D, 0x0A};
        uchar in[1] = {0};
        communicate((char*)out, (char*)in, 7, 1);
        out[0] = 0xA1;
        out[3] = (te >> 8) & 0xFF;
        out[4] = te & 0xFF;
        communicate((char*)out, (char*)in, 7, 1);
    }
}

void Cam::frame_rate(bool read, float *val)
{
    if (read) {
        uchar out[7] = {0xB0, 0x00, 0x00, 0xFF, 0x1F, 0x0D, 0x1A};
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
}

void Cam::gain_analog(bool read, float *val)
{
    if (read) {
        uchar out[7] = {0xB0, 0x00, 0x00, 0xFF, 0x17, 0x0D, 0x1A};
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
}

void Cam::trigger_mode(bool read, bool *val)
{
    if (read) {
        uchar out[7] = {0xB0, 0x00, 0x00, 0xFF, 0x1D, 0x0D, 0x1A};
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
}

void Cam::trigger_source(bool read, bool *val)
{
    if (read) {
        uchar out[7] = {0xB0, 0x00, 0x00, 0xFF, 0x1D, 0x0D, 0x1A};
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
}

void Cam::trigger_once()
{
    // MV_CC_SetCommandValue(dev_handle, "TriggerSoftware");
}

void Cam::frame_cb(PMCCALLBACKINFO cb_info)
{
    void *data;
    // Update "current" surface address pointer
    McGetParamPtr(cb_info->SignalInfo, MC_SurfaceAddr, &data);
    static cv::Mat img(y, x, CV_8U);
    memcpy(img.data, data, x * y);
    ((std::queue<cv::Mat>*)(cb_info->Context))->push(img.clone());
}

