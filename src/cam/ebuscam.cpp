#include "ebuscam.h"

cv::Mat ebus_img;

EbusCam::EbusCam() {
    device_type = 2;
    dev_handle = NULL;
    stream_handle = NULL;
    dev_adapter = NULL;
    dev_serial_port = NULL;
    curr_idx = 0;
    ptr_user = NULL;

    is_streaming = false;
}
EbusCam::~EbusCam()
{
    shut_down();
    clear_device_list();
}

void EbusCam::clear_device_list()
{
    for (PvDeviceInfoGEV *info : gige_dev_list) delete info;
    for (PvDeviceInfoU3V *info : usb3_dev_list) delete info;
    gige_dev_list.clear();
    usb3_dev_list.clear();
}

int EbusCam::search_for_devices()
{
    clear_device_list();
    PvSystem dev_system;
    if (!dev_system.Find().IsOK()) return 0;

    uint32_t interface_count = dev_system.GetInterfaceCount();
    for (uint32_t x = 0; x < interface_count; x++) {
        const PvInterface* interface = dev_system.GetInterface(x);

        uint32_t device_count = interface->GetDeviceCount();
        for (uint32_t y = 0; y < device_count ; y++) {
            const PvDeviceInfo *dev_info = interface->GetDeviceInfo( y );

            const PvDeviceInfoGEV* gige_dev_info = dynamic_cast<const PvDeviceInfoGEV*>(dev_info);
            const PvDeviceInfoU3V *usb3_dev_info = dynamic_cast<const PvDeviceInfoU3V *>(dev_info);

            if (gige_dev_info) gige_dev_list.push_back(dynamic_cast<PvDeviceInfoGEV*>(gige_dev_info->Copy()));
            else if (usb3_dev_info) usb3_dev_list.push_back(dynamic_cast<PvDeviceInfoU3V*>(usb3_dev_info->Copy()));
        }
    }
    return gige_dev_list.size() + usb3_dev_list.size() > 0;
}

int EbusCam::start(int idx) {
    curr_idx = idx;
    const int device_count = static_cast<int>(gige_dev_list.size() + usb3_dev_list.size());
    if (idx < 0 || idx >= device_count) return -1;
    if (dev_handle || stream_handle) shut_down();

    const PvDeviceInfo *device_info = idx < static_cast<int>(gige_dev_list.size())
        ? static_cast<PvDeviceInfo*>(gige_dev_list[idx])
        : static_cast<PvDeviceInfo*>(usb3_dev_list[idx - static_cast<int>(gige_dev_list.size())]);
    PvResult ret;

    dev_handle = PvDevice::CreateAndConnect(device_info, &ret);
    if (!dev_handle || !ret.IsOK()) {
        dev_handle = NULL;
        return -1;
    }

    stream_handle = PvStream::CreateAndOpen(device_info, &ret);
    if (!stream_handle || !ret.IsOK()) {
        if (stream_handle) PvStream::Free(stream_handle);
        stream_handle = NULL;
        dev_handle->Disconnect();
        PvDevice::Free(dev_handle);
        dev_handle = NULL;
        return -1;
    }

    PvDeviceGEV* gige_dev = dynamic_cast<PvDeviceGEV *>(dev_handle);
    if (gige_dev) {
        PvStreamGEV *gige_stream = static_cast<PvStreamGEV *>(stream_handle);
        gige_dev->NegotiatePacketSize();
        gige_dev->SetStreamDestination(gige_stream->GetLocalIPAddress(), gige_stream->GetLocalPort());
    }

    PvGenParameterArray *dev_params = dev_handle->GetParameters();
    dev_params->SetEnumValue("AcquisitionMode", "Continuous");
    dev_params->SetEnumValue("SensorDigitizationTaps", "Two");
    dev_params->SetEnumValue("Uart0BaudRate", "Baud115200");
    dev_params->SetEnumValue("Uart0NumOfStopBits", "One");
    dev_params->SetEnumValue("Uart0Parity", "None");

    dev_adapter = new PvDeviceAdapter(dev_handle);
    dev_serial_port = new PvDeviceSerialPort;
    dev_serial_port->Open(dev_adapter, PvDeviceSerial0);

    return !ret.IsOK();
}

int EbusCam::shut_down()
{
    stop_grabbing();

    if (dev_serial_port) {
        if (dev_serial_port->IsOpened()) dev_serial_port->Close();
        delete dev_serial_port;
        dev_serial_port = NULL;
    }
    if (dev_adapter) {
        delete dev_adapter;
        dev_adapter = NULL;
    }

    for (std::list<PvBuffer*>::iterator it = buffer_list.begin(); it != buffer_list.end(); it++) delete *it;
    buffer_list.clear();

    if (stream_handle) {
        stream_handle->Close();
        PvStream::Free(stream_handle);
        stream_handle = NULL;
    }
    if (dev_handle) {
        if (dev_handle->IsConnected()) dev_handle->Disconnect();
        PvDevice::Free(dev_handle);
        dev_handle = NULL;
    }
    return 0;
}

bool EbusCam::is_connected() const
{
    return dev_handle && stream_handle && dev_handle->IsConnected();
}

int EbusCam::reconnect()
{
    if (is_connected()) return 0;

    const int selected_idx = curr_idx;
    shut_down();
    if (!search_for_devices()) return -1;
    if (selected_idx < 0
            || selected_idx >= static_cast<int>(gige_dev_list.size() + usb3_dev_list.size())) {
        return -1;
    }
    return start(selected_idx);
}

int EbusCam::set_user_pointer(StreamFrameSink *sink)
{
    ptr_user = sink;
    return 0;
}

int EbusCam::start_grabbing() {
    if (!is_connected()) return -1;
    if (is_streaming.load()) return 0;
    if (streaming_thread.joinable()) streaming_thread.join();

    for (PvBuffer *buffer : buffer_list) delete buffer;
    buffer_list.clear();

    uint32_t buffer_size = dev_handle->GetPayloadSize();
    uint32_t buffer_count = (stream_handle->GetQueuedBufferMaximum() < 16) ? stream_handle->GetQueuedBufferMaximum() : 16;
    if (!buffer_size || !buffer_count) return -1;

    for (uint32_t i = 0; i < buffer_count; i++) {
        PvBuffer *buffer = new PvBuffer;
        buffer->Alloc(static_cast<uint32_t>(buffer_size));
        buffer_list.push_back(buffer);
    }
    for (PvBuffer *buffer : buffer_list) {
        if (!stream_handle->QueueBuffer(buffer).IsOK()) {
            stop_grabbing();
            return -1;
        }
    }

    PvGenParameterArray *dev_params = dev_handle->GetParameters();
    PvGenCommand *cmd_start = dynamic_cast<PvGenCommand *>(dev_params->Get("AcquisitionStart"));
    if (!cmd_start) {
        stop_grabbing();
        return -1;
    }

    if (!dev_handle->StreamEnable().IsOK() || !cmd_start->Execute().IsOK()) {
        stop_grabbing();
        return -1;
    }

    is_streaming = true;
    streaming_thread = std::thread([this]() {
//        PvGenParameterArray *stream_params = stream_handle->GetParameters();

//        PvGenFloat *lFrameRate = dynamic_cast<PvGenFloat *>(lStreamParams->Get("AcquisitionRate"));
//        double lFrameRateVal = 0.0;
//        PvGenFloat *lBandwidth = dynamic_cast<PvGenFloat *>(lStreamParams->Get("Bandwidth"));
//        double lBandwidthVal = 0.0;
        while (is_streaming.load()) {
            PvBuffer *buffer = NULL;
            PvResult op_result;

            PvResult ret = stream_handle->RetrieveBuffer(&buffer, &op_result, 1000);
//            qDebug() << buffer << ret.GetCodeString().GetAscii() << op_result.GetCodeString().GetAscii();
            if (ret.IsOK() && buffer) {
//                lFrameRate->GetValue( lFrameRateVal );
//                lBandwidth->GetValue( lBandwidthVal );

                if (op_result.IsOK() && buffer->GetPayloadType() == PvPayloadTypeImage) {
                    PvImage *img = buffer->GetImage();
                    StreamFrameSink *sink = ptr_user;
                    if (sink) {
                        if (sink->frame_info) sink->frame_info->push(0);
                        if (sink->frames)     sink->frames->push(cv::Mat(img->GetHeight(), img->GetWidth(), CV_8UC1, buffer->GetDataPointer()).clone());
                    }
                }
                if (is_streaming.load()) stream_handle->QueueBuffer(buffer);
            }
        }
    });

    return 0;
}

int EbusCam::stop_grabbing() {
    is_streaming = false;
    if (stream_handle) stream_handle->AbortQueuedBuffers();
    if (streaming_thread.joinable()) streaming_thread.join();

    int result = 0;
    if (dev_handle && dev_handle->IsConnected()) {
        PvGenParameterArray *device_params = dev_handle->GetParameters();
        PvGenCommand *cmd_stop = device_params
            ? dynamic_cast<PvGenCommand *>(device_params->Get("AcquisitionStop")) : NULL;
        if (cmd_stop && !cmd_stop->Execute().IsOK()) result = -1;
        if (!dev_handle->StreamDisable().IsOK()) result = -1;
    }

    if (stream_handle) {
        stream_handle->AbortQueuedBuffers();
        while (stream_handle->GetQueuedBufferCount() > 0) {
            PvBuffer *buffer = NULL;
            PvResult op_result;
            stream_handle->RetrieveBuffer(&buffer, &op_result, 100);
        }
    }

    for (PvBuffer *buffer : buffer_list) delete buffer;
    buffer_list.clear();
    return result;
}

void EbusCam::get_max_frame_size(int *w, int *h)
{
    PvGenParameterArray *device_params = dev_handle->GetParameters();
    int64_t int_value;
    device_params->GetIntegerValue("WidthMax", int_value);
    *w = int_value;
    device_params->GetIntegerValue("HeightMax", int_value);
    *h = int_value;
}

void EbusCam::frame_size(bool read, int *w, int *h, int *inc_w, int *inc_h)
{
    PvGenParameterArray *device_params = dev_handle->GetParameters();
    if (read) {
        int64_t temp_int = 0;
        PvGenInteger *int_value = NULL;

        int_value = device_params->GetInteger("Width");
        int_value->GetValue(temp_int);
        *w = temp_int;
        if (inc_w) int_value->GetIncrement(temp_int), *inc_w = temp_int;

        int_value = device_params->GetInteger("Height");
        int_value->GetValue(temp_int);
        *h = temp_int;
        if (inc_h) int_value->GetIncrement(temp_int), *inc_h = temp_int;
    }
    else {
        device_params->SetIntegerValue("Width", *w);
        device_params->SetIntegerValue("Height", *h);
    }
    // TODO add pixel type query
    ebus_img = cv::Mat(*h, *w, CV_8UC1);
}

void EbusCam::frame_offset(bool read, int *x, int *y, int *inc_x, int *inc_y)
{
    PvGenParameterArray *device_params = dev_handle->GetParameters();
    if (read) {
        int64_t temp_int = 0;
        PvGenInteger *int_value = NULL;

        int_value = device_params->GetInteger("OffsetX");
        int_value->GetValue(temp_int);
        *x = temp_int;
        if (inc_x) int_value->GetIncrement(temp_int), *inc_x = temp_int;

        int_value = device_params->GetInteger("OffsetY");
        int_value->GetValue(temp_int);
        *y = temp_int;
        if (inc_y) int_value->GetIncrement(temp_int), *inc_y = temp_int;
    }
    else {
        device_params->SetIntegerValue("OffsetX", *x);
        device_params->SetIntegerValue("OffsetY", *y);
    }
}

void port_io_helper(PvDeviceSerialPort *dev_serial_port, uchar *out, uchar *in, int out_size, int in_size)
{
    PvResult ret;
    uint32_t bytes_written = 0;
    ret = dev_serial_port->Write(out, out_size, bytes_written);

    uint32_t total_bytes_read = 0;
    while (total_bytes_read < in_size)
    {
        uint32_t bytes_read = 0;
        ret = dev_serial_port->Read(in + total_bytes_read, in_size - total_bytes_read, bytes_read, 50);
        if (ret.GetCode() == PvResult::Code::TIMEOUT) break;
        total_bytes_read += bytes_read;
    }
    QString str_s("s"), str_r("r");
    for (int i = 0; i < 7; i++) str_s += QString::asprintf(" %02X", i + (int)out_size - 7 < 0 ? 0 : ((uchar*)out)[i + out_size - 7]);
    for (int i = 0; i < 7; i++) str_r += QString::asprintf(" %02X", i + (int)in_size - 7 < 0 ? 0 : ((uchar*)in)[i + in_size - 7]);
    qDebug() << str_s << str_r;
}

void EbusCam::time_exposure(bool read, float *val)
{
    if (read) {
        buffer_out[0] = 0xB0;
        buffer_out[1] = 0x00;
        buffer_out[2] = 0x00;
        buffer_out[3] = 0xFF;
        buffer_out[4] = 0x21;
        buffer_out[5] = 0x1D;
        buffer_out[6] = 0x1A;
        memset(buffer_in, 0, 7);

        port_io_helper(dev_serial_port, buffer_out, buffer_in, 7, 6);
        *val = ((buffer_in)[0] << 24) + ((buffer_in)[1] << 16) + ((buffer_in)[2] << 8) + (buffer_in)[3];

        buffer_out[4] = 0x22;
        memset(buffer_in, 0, 7);

        port_io_helper(dev_serial_port, buffer_out, buffer_in, 7, 6);
        *val += ((buffer_in)[0] << 24) + ((buffer_in)[1] << 16) + ((buffer_in)[2] << 8) + (buffer_in)[3] * 65536;
    }
    else {
        int te = *val;
        buffer_out[0] = 0xA0;
        buffer_out[1] = 0x00;
        buffer_out[2] = 0x00;
        buffer_out[3] = 0xFF;
        buffer_out[4] = 0x21;
        buffer_out[5] = 0x0D;
        buffer_out[6] = 0x0A;
        memset(buffer_in, 0, 7);
        port_io_helper(dev_serial_port, buffer_out, buffer_in, 7, 1);

        buffer_out[0] = 0xA1;
        buffer_out[1] = (te >> 24) & 0xFF;
        buffer_out[2] = (te >> 16) & 0xFF;
        buffer_out[3] = (te >> 8) & 0xFF;
        buffer_out[4] = te & 0xFF;
        memset(buffer_in, 0, 7);
        port_io_helper(dev_serial_port, buffer_out, buffer_in, 7, 1);
    }
}

void EbusCam::frame_rate(bool read, float *val)
{
    if (read) {
        buffer_out[0] = 0xB0;
        buffer_out[1] = 0x00;
        buffer_out[2] = 0x00;
        buffer_out[3] = 0xFF;
        buffer_out[4] = 0x1F;
        buffer_out[5] = 0x1D;
        buffer_out[6] = 0x1A;
        memset(buffer_in, 0, 7);

        port_io_helper(dev_serial_port, buffer_out, buffer_in, 7, 6);
        *val = ((buffer_in)[0] << 24) + ((buffer_in)[1] << 16) + ((buffer_in)[2] << 8) + (buffer_in)[3];
    }
    else {
        buffer_out[0] = 0xA0;
        buffer_out[1] = 0x00;
        buffer_out[2] = 0x00;
        buffer_out[3] = 0xFF;
        buffer_out[4] = 0x1F;
        buffer_out[5] = 0x0D;
        buffer_out[6] = 0x0A;
        memset(buffer_in, 0, 7);
        port_io_helper(dev_serial_port, buffer_out, buffer_in, 7, 1);

        buffer_out[0] = 0xA1;
        buffer_out[3] = ((int)(*val) >> 8) & 0xFF;
        buffer_out[4] = (int)(*val) & 0xFF;
        memset(buffer_in, 0, 7);
        port_io_helper(dev_serial_port, buffer_out, buffer_in, 7, 1);
    }
}

void EbusCam::gain_analog(bool read, float *val)
{
    if (read) {
        buffer_out[0] = 0xB0;
        buffer_out[1] = 0x00;
        buffer_out[2] = 0x00;
        buffer_out[3] = 0xFF;
        buffer_out[4] = 0x17;
        buffer_out[5] = 0x1D;
        buffer_out[6] = 0x1A;
        memset(buffer_in, 0, 7);

        port_io_helper(dev_serial_port, buffer_out, buffer_in, 7, 6);
        *val = ((buffer_in)[0] << 24) + ((buffer_in)[1] << 16) + ((buffer_in)[2] << 8) + (buffer_in)[3];
    }
    else {
        buffer_out[0] = 0xA0;
        buffer_out[1] = 0x00;
        buffer_out[2] = 0x00;
        buffer_out[3] = 0xFF;
        buffer_out[4] = 0x17;
        buffer_out[5] = 0x0D;
        buffer_out[6] = 0x0A;
        memset(buffer_in, 0, 7);
        port_io_helper(dev_serial_port, buffer_out, buffer_in, 7, 1);

        buffer_out[0] = 0xA1;
        buffer_out[3] = ((int)(*val) >> 8) & 0xFF;
        buffer_out[4] = (int)(*val) & 0xFF;
        memset(buffer_in, 0, 7);
        port_io_helper(dev_serial_port, buffer_out, buffer_in, 7, 1);
    }
}

void EbusCam::trigger_mode(bool read, bool *val)
{
    if (read) {
        buffer_out[0] = 0xB0;
        buffer_out[1] = 0x00;
        buffer_out[2] = 0x00;
        buffer_out[3] = 0xFF;
        buffer_out[4] = 0x1D;
        buffer_out[5] = 0x1D;
        buffer_out[6] = 0x1A;
        memset(buffer_in, 0, 7);

        port_io_helper(dev_serial_port, buffer_out, buffer_in, 7, 6);
        *val = ((buffer_in)[0] << 24) + ((buffer_in)[1] << 16) + ((buffer_in)[2] << 8) + (buffer_in)[3];
    }
    else {
        buffer_out[0] = 0xA0;
        buffer_out[1] = 0x00;
        buffer_out[2] = 0x00;
        buffer_out[3] = 0xFF;
        buffer_out[4] = 0x1D;
        buffer_out[5] = 0x0D;
        buffer_out[6] = 0x0A;
        memset(buffer_in, 0, 7);
        port_io_helper(dev_serial_port, buffer_out, buffer_in, 7, 1);

        buffer_out[0] = 0xA1;
        buffer_out[3] = 0x00;
        buffer_out[4] = *val ? 0x02 : 0x00;
        memset(buffer_in, 0, 7);
        port_io_helper(dev_serial_port, buffer_out, buffer_in, 7, 1);
    }
}

void EbusCam::trigger_source(bool read, bool *val)
{
    if (read) {
        buffer_out[0] = 0xB0;
        buffer_out[1] = 0x00;
        buffer_out[2] = 0x00;
        buffer_out[3] = 0xFF;
        buffer_out[4] = 0x1D;
        buffer_out[5] = 0x1D;
        buffer_out[6] = 0x1A;
        memset(buffer_in, 0, 7);

        port_io_helper(dev_serial_port, buffer_out, buffer_in, 7, 6);
        *val = (((buffer_in)[0] << 24) + ((buffer_in)[1] << 16) + ((buffer_in)[2] << 8) + (buffer_in)[3]) == 1;
    }
    else {
        buffer_out[0] = 0xA0;
        buffer_out[1] = 0x00;
        buffer_out[2] = 0x00;
        buffer_out[3] = 0xFF;
        buffer_out[4] = 0x1D;
        buffer_out[5] = 0x0D;
        buffer_out[6] = 0x0A;
        memset(buffer_in, 0, 7);
        port_io_helper(dev_serial_port, buffer_out, buffer_in, 7, 1);

        buffer_out[0] = 0xA1;
        buffer_out[3] = 0x00;
        buffer_out[4] = *val ? 0x02 : 0x01;
        memset(buffer_in, 0, 7);
        port_io_helper(dev_serial_port, buffer_out, buffer_in, 7, 1);
    }
}

int EbusCam::ip_config(bool read, int *val)
{
    if (curr_idx >= gige_dev_list.size()) return -1;
    PvResult ret;
    dev_handle = PvDevice::CreateAndConnect(gige_dev_list[curr_idx], &ret);
    if (!ret.IsOK()) return -1;
    PvGenParameterArray *device_params = dev_handle->GetParameters();
    bool is_persistent = 0;
    if (read) {
        ret = device_params->GetBooleanValue("GevCurrentIPConfigurationPersistentIP", is_persistent);
        *val = is_persistent;
//        qDebug() << "ip config is persistent" << is_persistent;
    }
    else {
        is_persistent = 1;
        ret = device_params->SetBooleanValue("GevCurrentIPConfigurationPersistentIP", is_persistent);
    }

    dev_handle->Disconnect();
    PvDevice::Free(dev_handle);
    dev_handle = NULL;

    return !ret.IsOK();
}

int ip_calc_helper(const char* ip)
{
    int result = 0;
    char ip_copy[16] = {0};
    strcpy(ip_copy, ip);
    char *res = strtok(ip_copy, ".");
    int shift = 32;
    do result += atoi(res) << (shift -= 8); while (res = strtok(NULL, "."));
    return result;
}

char* ip_calc_helper(int ip)
{
    char *result = (char*)malloc(16 * sizeof(char));
    sprintf(result, "%d.%d.%d.%d", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);
    return result;
}

int EbusCam::ip_address(bool read, int idx, int *ip, int *gateway, int *nic_address)
{
    if (idx >= gige_dev_list.size()) return -1;
    if (read) {
        *ip = ip_calc_helper(gige_dev_list[idx]->GetIPAddress().GetAscii());
        *gateway = ip_calc_helper(gige_dev_list[idx]->GetDefaultGateway().GetAscii());
//        if (nic_address) *nic_address = ip_calc_helper(dynamic_cast<const PvNetworkAdapter*>(gige_dev_list[idx]->GetInterface())->GetIPAddress(0).GetAscii());
        return 0;
    }
    else {
        char *ip_str = ip_calc_helper(*ip);
        char *subnet_mask_str = ip_calc_helper((255 << 24) + (255 << 16) + (255 << 8));
        char *gateway_str = ip_calc_helper(*gateway);
        PvResult ret = PvDeviceGEV::SetIPConfiguration(gige_dev_list[idx]->GetMACAddress(), ip_str, subnet_mask_str, gateway_str);
        free(ip_str);
        free(subnet_mask_str);
        free(gateway_str);
        return ret.IsOK();
    }
}

int EbusCam::pixel_type(bool read, int *val)
{
    return 0;
}

void EbusCam::trigger_once()
{
    return;
}

void EbusCam::set_sensor_digitization_taps(int num)
{
    qDebug() << num;
    PvGenParameterArray *device_params = dev_handle->GetParameters();
    if (device_params) {
        device_params->SetEnumValue("SensorDigitizationTaps", num == 2 ? "Two" : "One");
        PvString temp;
        device_params->GetEnumValue("SensorDigitizationTaps", temp);
        qDebug() << temp.GetAscii();
    }
}

QStringList EbusCam::get_device_list()
{
    QStringList sl;
    for (PvDeviceInfoGEV *info: gige_dev_list)
        sl << QString::asprintf("%s %s", info->GetModelName().GetAscii(), info->GetVendorName().GetAscii());
    for (PvDeviceInfoU3V *info: usb3_dev_list)
        sl << QString::asprintf("%s %s", info->GetModelName().GetAscii(), info->GetVendorName().GetAscii());
    return sl;
}

int EbusCam::gige_device_num()
{
    return gige_dev_list.size();
}

int EbusCam::usb3_device_num()
{
    return usb3_dev_list.size();
}
