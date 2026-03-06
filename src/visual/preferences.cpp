#include "preferences.h"
#include "ui_preferences.h"

Preferences::Preferences(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Preferences),
    pressed(false),
    split(false),
    cameralink(false),
    port_idx(0),
    use_tcp(false),
    dist_ns(3e8 / 2e9),
    laser_grp(NULL)
{
    ui->setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint);

//[0] set up ui for info tabs
    ui->DEVICES_TAB  ->setup(0, ui->SEP_0->pos().y() + 10);
    ui->SERIAL_TAB   ->setup(0, ui->SEP_1->pos().y() + 10);
    ui->TCU_TAB      ->setup(0, ui->SEP_2->pos().y() + 10);
    ui->SAVE_LOAD_TAB->setup(0, ui->SEP_3->pos().y() + 10);
    ui->IMG_PROC_TAB ->setup(0, ui->SEP_4->pos().y() + 10);

    connect(ui->DEVICES_TAB,   SIGNAL(index_label_clicked(int)), SLOT(jump_to_content(int)));
    connect(ui->SERIAL_TAB,    SIGNAL(index_label_clicked(int)), SLOT(jump_to_content(int)));
    connect(ui->TCU_TAB,       SIGNAL(index_label_clicked(int)), SLOT(jump_to_content(int)));
    connect(ui->SAVE_LOAD_TAB, SIGNAL(index_label_clicked(int)), SLOT(jump_to_content(int)));
    connect(ui->IMG_PROC_TAB,  SIGNAL(index_label_clicked(int)), SLOT(jump_to_content(int)));
//![0]

//[1] set up ui for device config
    ui->IP_EDIT->setEnabled(false);
    ui->DEVICE_LIST->installEventFilter(this);
    connect(ui->DEVICE_LIST, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), this,
            [this](int index){ if (m_config) m_config->get_data().tcu.type = index; emit query_dev_ip(); });
    ui->ROTATE_OPTION_LIST->addItem("  0°");
    ui->ROTATE_OPTION_LIST->addItem(" 90°");
    ui->ROTATE_OPTION_LIST->addItem("180°");
    ui->ROTATE_OPTION_LIST->addItem("270°");
    ui->ROTATE_OPTION_LIST->installEventFilter(this);
    connect(ui->ROTATE_OPTION_LIST, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), this,
            [this](int index){ if (m_config) m_config->get_data().device.rotation = index; emit rotate_image(index); });
    ui->FLIP_OPTION_LIST->addItem("None");
    ui->FLIP_OPTION_LIST->addItem("Both");
    ui->FLIP_OPTION_LIST->addItem("X");
    ui->FLIP_OPTION_LIST->addItem("Y");
    ui->FLIP_OPTION_LIST->installEventFilter(this);
    connect(ui->FLIP_OPTION_LIST, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), this,
            [this](int index){ if (m_config) m_config->get_data().device.flip = index; });
    ui->PIXEL_FORMAT_LIST->addItem("Mono8");
    ui->PIXEL_FORMAT_LIST->addItem("Mono10");
    ui->PIXEL_FORMAT_LIST->addItem("Mono12");
    ui->PIXEL_FORMAT_LIST->addItem("RGB8");
    ui->PIXEL_FORMAT_LIST->addItem("BayerRG8");
    ui->PIXEL_FORMAT_LIST->installEventFilter(this);
    connect(ui->PIXEL_FORMAT_LIST, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), this,
            [this](int index){
                static int pixel_format[5] = {PixelType_Gvsp_Mono8, PixelType_Gvsp_Mono10, PixelType_Gvsp_Mono12, PixelType_Gvsp_RGB8_Packed, PixelType_Gvsp_BayerRG8};
                emit change_pixel_format(pixel_format[index]);
            });
#ifndef WIN32
    ui->EBUS_CHK->setCheckable(false);
#endif
    connect(ui->SPLIT_CHK, &QCheckBox::stateChanged, this, [this](int arg1){ split = arg1; });
    connect(ui->UNDERWATER_CHK, &QCheckBox::stateChanged, this, [this](int arg1){ emit device_underwater(arg1); });
    connect(ui->EBUS_CHK, &QCheckBox::stateChanged, this, [this](int arg1){ if (m_config) m_config->get_data().device.ebus = arg1; emit search_for_devices(); });
    ui->PTZ_TYPE_LIST->addItem("pelco-p");
    ui->PTZ_TYPE_LIST->addItem("usbcan");
    ui->PTZ_TYPE_LIST->addItem("udp-scw");
    ui->PTZ_TYPE_LIST->installEventFilter(this);
    connect(ui->PTZ_TYPE_LIST, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), this,
            [this](int index){ if (m_config) m_config->get_data().device.ptz_type = index; });
    connect(ui->CAMERALINK_CHK, &QCheckBox::stateChanged, this, [this](int arg1){ cameralink = arg1; emit search_for_devices(); });
//![1]

//[2] set up ui for serial comm.
    ui->COM_LIST->addItem("TCU");
    ui->COM_LIST->addItem("RANGE");
#ifdef ICMOS
    ui->COM_LIST->addItem("R1");
    ui->COM_LIST->addItem("R2");
#else
    ui->COM_LIST->addItem("LENS");
    ui->COM_LIST->addItem("LASER");
#endif
    ui->COM_LIST->addItem("PTZ");
    ui->COM_LIST->installEventFilter(this);

    QStringList available_ports;
    for (const QSerialPortInfo &info: QSerialPortInfo::availablePorts()) available_ports << info.portName();
    ui->AVAILABLE_COM_LIST->addItems(available_ports);
    ui->AVAILABLE_COM_LIST->installEventFilter(this);
    connect(ui->REFRESH_AVAILABLE_PORTS_BTN, &QPushButton::clicked, this,
            [this](){
                QStringList available_ports;
                for (const QSerialPortInfo &info: QSerialPortInfo::availablePorts()) available_ports << info.portName();
                ui->AVAILABLE_COM_LIST->clear();
                ui->AVAILABLE_COM_LIST->addItems(available_ports);
            });

    ui->BAUDRATE_LIST->addItem("------");
    ui->BAUDRATE_LIST->addItem("  1200");
    ui->BAUDRATE_LIST->addItem("  2400");
    ui->BAUDRATE_LIST->addItem("  4800");
    ui->BAUDRATE_LIST->addItem("  9600");
    ui->BAUDRATE_LIST->addItem(" 19200");
    ui->BAUDRATE_LIST->addItem(" 38400");
    ui->BAUDRATE_LIST->addItem(" 57600");
    ui->BAUDRATE_LIST->addItem("115200");
    ui->BAUDRATE_LIST->installEventFilter(this);

    connect(ui->COM_LIST, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), this,
            [this](int index){ emit get_port_info(index); });
    connect(ui->BAUDRATE_LIST, static_cast<void (QComboBox::*)(const QString &arg1)>(&QComboBox::activated), this,
            [this](const QString &arg1){ emit change_baudrate(ui->COM_LIST->currentIndex(), arg1.toInt()); });
    connect(ui->TCP_SERVER_CHK, &QCheckBox::stateChanged, this,
            [this](int arg1){ emit set_tcp_status(ui->COM_LIST->currentIndex(), arg1); });

    connect(ui->SHARE_CHK, &QCheckBox::stateChanged, this, [this](int arg1){ if (m_config) m_config->get_data().device.share_tcu_port = arg1; emit share_tcu_port(arg1); });

//    QFont temp = QFont(consolas);
//    temp.setPixelSize(11);
//    ui->COM_DATA_EDT->setFont(temp);
    ui->COM_DATA_EDT->setFont(consolas);
//![2]

//[3] set up ui for tcu config
    ui->TCU_LIST->addItem("default");
//    ui->TCU_LIST->addItem("#0 50ps step");
//    ui->TCU_LIST->addItem("#1 50ps step");
//    ui->TCU_LIST->addItem("#2 50ps step");
//    ui->TCU_LIST->addItem("#3 50ps step");
    ui->TCU_LIST->addItem("40ps step");
    ui->TCU_LIST->addItem("10ps step");
    ui->TCU_LIST->installEventFilter(this);
    connect(ui->TCU_LIST, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), this,
            [this](int index){
                emit tcu_type_changed(index);
                switch (index) {
                case 1:  ui->PS_CONFIG_GRP->show(); break;
                default: ui->PS_CONFIG_GRP->hide(); break;
                }
            });

    connect(ui->AUTO_REP_FREQ_CHK, &QCheckBox::stateChanged, this, [this](int arg1){ if (m_config) m_config->get_data().tcu.auto_rep_freq = arg1; emit set_auto_rep_freq(arg1); });
    connect(ui->AUTO_MCP_CHK, &QCheckBox::stateChanged, this, [this](int arg1){ if (m_config) m_config->get_data().tcu.auto_mcp = arg1; emit set_auto_mcp(arg1); });
    connect(ui->AB_LOCK_CHK, &QCheckBox::stateChanged, this, [this](int arg1){ if (m_config) m_config->get_data().tcu.ab_lock = arg1; emit set_ab_lock(arg1); });

    ui->HZ_LIST->addItem("kHz");
    ui->HZ_LIST->addItem("Hz");
    ui->HZ_LIST->setCurrentIndex(0);
    ui->HZ_LIST->installEventFilter(this);

    ui->UNIT_LIST->addItem("ns");
//    ui->UNIT_LIST->addItem(QString::fromLocal8Bit("μs"));
    ui->UNIT_LIST->addItem("μs");
    ui->UNIT_LIST->addItem("m");
    ui->UNIT_LIST->setCurrentIndex(0);
    ui->UNIT_LIST->installEventFilter(this);

    connect(ui->HZ_LIST, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), this,
            [this](int index){ if (m_config) m_config->get_data().tcu.hz_unit = index; emit rep_freq_unit_changed(index); });
    connect(ui->UNIT_LIST, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), this,
            [this](int index) {
                if (m_config) m_config->get_data().tcu.base_unit = index;
                emit base_unit_changed(index);
                update_distance_display();
            });

    connect(ui->AB_LOCK_CHK, &QCheckBox::stateChanged, this, [this](){});

    connect(ui->MAX_DIST_EDT, &QLineEdit::editingFinished, this, [this](){ if (m_config) emit max_dist_changed(m_config->get_data().tcu.max_dist); });
    connect(ui->DELAY_OFFSET_EDT, &QLineEdit::editingFinished, this,
            [this](){
                if (m_config) emit delay_offset_changed(m_config->get_data().tcu.delay_offset * dist_ns);
    });
    connect(ui->MAX_DOV_EDT, &QLineEdit::editingFinished, this, [this](){ if (m_config) emit max_dov_changed(m_config->get_data().tcu.max_dov); });
    connect(ui->GATE_WIDTH_OFFSET_EDT, &QLineEdit::editingFinished, this,
            [this](){
                if (m_config) emit gate_width_offset_changed(m_config->get_data().tcu.gate_width_offset * dist_ns);
    });
    connect(ui->MAX_LASER_EDT, &QLineEdit::editingFinished, this, [this](){ if (m_config) emit max_laser_changed(m_config->get_data().tcu.max_laser_width); });
    connect(ui->LASER_OFFSET_EDT, &QLineEdit::editingFinished, this,
            [this](){
                if (m_config) emit laser_offset_changed(m_config->get_data().tcu.laser_width_offset);
    });

    ui->PS_CONFIG_GRP->hide();
    ui->TCU_PS_CONFIG_LIST->addItem("GW_A");
    ui->TCU_PS_CONFIG_LIST->addItem("DELAY_A");
    ui->TCU_PS_CONFIG_LIST->addItem("GW_B");
    ui->TCU_PS_CONFIG_LIST->addItem("DELAY_B");
    ui->TCU_PS_CONFIG_LIST->installEventFilter(this);
    ui->TCU_PS_CONFIG_LIST->setCurrentIndex(0);
    connect(ui->TCU_PS_CONFIG_LIST, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), this,
            [this](int idx){ emit ps_config_updated(true, idx, 0); });
    connect(ui->PS_STEPPING_EDT, &QLineEdit::editingFinished, this,
            [this](){
                if (!m_config) return;
                int idx = ui->TCU_PS_CONFIG_LIST->currentIndex();
                auto& ps = m_config->get_data().tcu.ps_step[idx];
                ps = std::min(std::max(ui->PS_STEPPING_EDT->text().toUInt(), (uint)1), (uint)4000);
                ui->MAX_PS_STEP_EDT->setText(QString::number(int(std::round(4000. / ps))));
                ui->PS_STEPPING_EDT->setText(QString::number(ps));
                emit ps_config_updated(false, idx, ui->PS_STEPPING_EDT->text().toInt());
            });
    connect(ui->MAX_PS_STEP_EDT, &QLineEdit::editingFinished, this,
            [this](){
                if (!m_config) return;
                int idx = ui->TCU_PS_CONFIG_LIST->currentIndex();
                int max_step = std::min(std::max(ui->MAX_PS_STEP_EDT->text().toInt(), 1), 100);
                ui->MAX_PS_STEP_EDT->setText(QString::number(max_step));
                auto& ps = m_config->get_data().tcu.ps_step[idx];
                ps = int(std::round(4000. / max_step));
                ui->PS_STEPPING_EDT->setText(QString::number(ps));
                emit ps_config_updated(false, idx, ui->PS_STEPPING_EDT->text().toInt());
            });

    connect(ui->LASER_ENABLE_CHK, &QCheckBox::stateChanged, this,
            [this](int arg1){
                uchar cmd[7] = {0x88, 0x1F, 0x00, 0x00, 0x00, uchar(arg1 ? 0x01 : 0x00), 0x99};
                emit com_write(0, QByteArray((char*)cmd, 7));
//                send_cmd(arg1 ? "88 1F 00 00 00 01 99" : "88 1F 00 00 00 00 99");
            });
    laser_grp = new QButtonGroup(this);
    laser_grp->addButton(ui->LASER_CHK_1, 0);
    laser_grp->addButton(ui->LASER_CHK_2, 1);
    laser_grp->addButton(ui->LASER_CHK_3, 2);
    laser_grp->addButton(ui->LASER_CHK_4, 3);
    laser_grp->setExclusive(false);
    connect(laser_grp, SIGNAL(buttonToggled(int, bool)), SLOT(toggle_laser(int, bool)));
//![3]

//[4]
    connect(ui->SAVE_INFO_CHK, &QCheckBox::stateChanged, this, [this](int arg1){ if (m_config) m_config->get_data().save.save_info = arg1; });
    QFont temp_f(consolas);
    temp_f.setPixelSize(11);
    ui->CUSTOM_INFO_EDT->setFont(temp_f);
    connect(ui->CUSTOM_INFO_CHK, &QCheckBox::stateChanged, this,
            [this](int arg1){ if (m_config) m_config->get_data().save.custom_topleft_info = arg1; ui->CUSTOM_INFO_EDT->setEnabled(arg1); });
    connect(ui->GRAYSCALE_CHK, &QCheckBox::stateChanged, this,
            [this](int arg1){ if (m_config) m_config->get_data().save.save_in_grayscale = arg1; });
    connect(ui->INTEGRATE_INFO_CHK, &QCheckBox::stateChanged, this,
            [this](int arg1){ if (m_config) m_config->get_data().save.integrate_info = arg1; });
    ui->IMG_FORMAT_LST->addItem("bmp/tiff");
    ui->IMG_FORMAT_LST->addItem("jpg");
    ui->IMG_FORMAT_LST->setCurrentIndex(0);
    connect(ui->IMG_FORMAT_LST, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), this,
            [this](int idx) { if (m_config) m_config->get_data().save.img_format = idx; });
    connect(ui->CONSECUTIVE_CAPTURE_CHK, &QCheckBox::stateChanged, this,
            [this](int arg1){ if (m_config) m_config->get_data().save.consecutive_capture = arg1; });
//![4]

//[5] set up ui for image proc
    connect(ui->LOWER_3D_THRESH_EDT, &QLineEdit::editingFinished, this,
            [this](){ if (m_config && m_config->get_data().image_proc.lower_3d_thresh < 0) m_config->get_data().image_proc.lower_3d_thresh = 0; emit lower_3d_thresh_updated(); });
    connect(ui->UPPER_3D_THRESH_EDT, &QLineEdit::editingFinished, this,
            [this](){ if (m_config && m_config->get_data().image_proc.upper_3d_thresh > 1.001) m_config->get_data().image_proc.upper_3d_thresh = 1.001; });
    QStringList colormap_names;
    colormap_names << "AUTUMN" << "BONE" << "JET" << "WINTER" << "RAINBOW" << "OCEAN" << "SUMMER" << "SPRING"
                   << "COOL" << "HSV" << "PINK" << "HOT" << "PARULA" << "MAGMA" << "INFERNO" << "PLASMA"
                   << "VIRIDIS" << "CIVIDIS" << "TWILIGHT" << "TWILIGHT_SHIFTED" << "TURBO" << "DEEPGREEN";
    ui->COLORMAP_3D_LIST->addItems(colormap_names);
    ui->COLORMAP_3D_LIST->installEventFilter(this);
    connect(ui->COLORMAP_3D_LIST, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), this,
            [this](int index){ if (m_config) m_config->get_data().image_proc.colormap = index; });
    connect(ui->TRUNCATE_3D_CHK, &QCheckBox::stateChanged, this,
            [this](int arg1){ if (m_config) m_config->get_data().image_proc.truncate_3d = arg1; });
    connect(ui->CUSTOM_3D_PARAM_CHK, &QCheckBox::stateChanged, this,
            [this](int arg1){
                if (m_config) m_config->get_data().image_proc.custom_3d_param = arg1;
                ui->CUSTOM_3D_DELAY_EDT->setEnabled(arg1);
                ui->CUSTOM_3D_GW_EDT->setEnabled(arg1);
                emit query_tcu_param();
            });
#ifdef LVTONG
    QStringList model_list;
    model_list << "01 legacy" << "02 pix2pix" << "03 maskguided" /*<< "04 semantic"*/;
    QStandardItemModel *model = new QStandardItemModel();//添加提示tootip
    for(int i = 0; i < model_list.size(); i++){
        QStandardItem *item = new QStandardItem(model_list[i]);
        item->setToolTip(model_list[i]);
        model->appendRow(item);
    }
    ui->MODEL_LIST->setModel(model);
    ui->MODEL_LIST->setCurrentIndex(model_idx = 0);
    connect(ui->MODEL_LIST, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), this,
            [this](int index){
                if (m_config) m_config->get_data().image_proc.model_idx = index;
                ui->FISHNET_THRESH_EDIT->setEnabled(!index);
            });
    connect(ui->FISHNET_RECOG_CHK, &QCheckBox::stateChanged, this, [this](int arg1){ if (m_config) m_config->get_data().image_proc.fishnet_recog = arg1; });
    connect(ui->FISHNET_THRESH_EDIT, &QLineEdit::editingFinished, this,
            [this](){
                if (!m_config) return;
                auto& ft = m_config->get_data().image_proc.fishnet_thresh;
                ft = ui->FISHNET_THRESH_EDIT->text().toFloat();
                ui->FISHNET_THRESH_EDIT->setText(QString::number(ft, 'f', 2));
            });
#else
    ui->FISHNET->hide();
    ui->MODEL_LIST->hide();
    ui->FISHNET_RECOG_CHK->hide();
    ui->FISHNET_THRESH->hide();
    ui->FISHNET_THRESH_EDIT->hide();
#endif
    //![5]

    // ECC combo boxes
    ui->ECC_WINDOW_MODE_LIST->addItem("backward");
    ui->ECC_WINDOW_MODE_LIST->addItem("balanced");
    ui->ECC_WINDOW_MODE_LIST->addItem("custom");
    ui->ECC_WINDOW_MODE_LIST->installEventFilter(this);

    ui->ECC_WARP_MODE_LIST->addItem("translate");
    ui->ECC_WARP_MODE_LIST->addItem("euclidean");
    ui->ECC_WARP_MODE_LIST->addItem("affine");
    ui->ECC_WARP_MODE_LIST->addItem("homography");
    ui->ECC_WARP_MODE_LIST->installEventFilter(this);

    ui->ECC_FUSION_MODE_LIST->addItem("mean");
    ui->ECC_FUSION_MODE_LIST->addItem("median");
    ui->ECC_FUSION_MODE_LIST->addItem("med+trim_mean");
    ui->ECC_FUSION_MODE_LIST->installEventFilter(this);

    data_exchange(false);
}

Preferences::~Preferences()
{
    delete ui;
}

void Preferences::init()
{
}

void Preferences::data_exchange(bool read)
{
    if (!m_config) return;
    auto& ip = m_config->get_data().image_proc;
    auto& tcu = m_config->get_data().tcu;
    auto& sv = m_config->get_data().save;

    auto& dev = m_config->get_data().device;

    if (read) {
        // device
        dev.flip = ui->FLIP_OPTION_LIST->currentIndex();
        dev.rotation = ui->ROTATE_OPTION_LIST->currentIndex();
        dev.ptz_type = ui->PTZ_TYPE_LIST->currentIndex();
        dev.ebus = ui->EBUS_CHK->isChecked();
        dev.share_tcu_port = ui->SHARE_CHK->isChecked();
        dev.underwater = ui->UNDERWATER_CHK->isChecked();

        // save
        sv.save_info = ui->SAVE_INFO_CHK->isChecked();
        sv.custom_topleft_info = ui->CUSTOM_INFO_CHK->isChecked();
        sv.save_in_grayscale = ui->GRAYSCALE_CHK->isChecked();
        sv.consecutive_capture = ui->CONSECUTIVE_CAPTURE_CHK->isChecked();
        sv.integrate_info = ui->INTEGRATE_INFO_CHK->isChecked();
        sv.img_format = ui->IMG_FORMAT_LST->currentIndex();

        // TCU type/hz_unit
        tcu.type = ui->TCU_LIST->currentIndex();
        tcu.hz_unit = ui->HZ_LIST->currentIndex();

        // image proc
        ip.gamma = ui->GAMMA_EDIT->text().toFloat();
        ip.accu_base = ui->ACCU_BASE_EDIT->text().toFloat();
        ip.low_in = ui->LOW_IN_EDIT->text().toFloat();
        ip.high_in = ui->HIGH_IN_EDIT->text().toFloat();
        ip.low_out = ui->LOW_OUT_EDIT->text().toFloat();
        ip.high_out = ui->HIGH_OUT_EDIT->text().toFloat();
        ip.dehaze_pct = ui->DEHAZE_PCT_EDIT->text().toFloat() / 100;
        ip.sky_tolerance = ui->SKY_TOLERANCE_EDIT->text().toFloat();
        ip.fast_gf = ui->FAST_GF_EDIT->text().toInt();
        ip.colormap = ui->COLORMAP_3D_LIST->currentIndex();
        ip.lower_3d_thresh = ui->LOWER_3D_THRESH_EDT->text().toFloat();
        ip.upper_3d_thresh = ui->UPPER_3D_THRESH_EDT->text().toFloat();
        ip.custom_3d_param = ui->CUSTOM_3D_PARAM_CHK->isChecked();
        ip.custom_3d_delay = ui->CUSTOM_3D_DELAY_EDT->text().toFloat();
        ip.custom_3d_gate_width = ui->CUSTOM_3D_GW_EDT->text().toFloat();
#ifdef LVTONG
        ip.fishnet_recog = ui->FISHNET_RECOG_CHK->isChecked();
        ip.fishnet_thresh = ui->FISHNET_THRESH_EDIT->text().toFloat();
#endif

        ip.ecc_window_mode = ui->ECC_WINDOW_MODE_LIST->currentIndex();
        ip.ecc_warp_mode = ui->ECC_WARP_MODE_LIST->currentIndex();
        ip.ecc_fusion_method = ui->ECC_FUSION_MODE_LIST->currentIndex();
        ip.ecc_backward = ui->ECC_BACKWARD_EDIT->text().toInt();
        ip.ecc_forward = ui->ECC_FORWARD_EDIT->text().toInt();
        if (ip.ecc_window_mode == 0) ip.ecc_forward = 0;
        else if (ip.ecc_window_mode == 1) ip.ecc_forward = ip.ecc_backward;
        ip.ecc_levels = ui->ECC_LEVELS_EDIT->text().toInt();
        ip.ecc_max_iter = ui->ECC_MAXITER_EDIT->text().toInt();
        ip.ecc_eps = ui->ECC_EPS_EDIT->text().toDouble();
        ip.ecc_half_res_reg = ui->ECC_HALF_REG_CHK->isChecked();
        ip.ecc_half_res_fuse = ui->ECC_HALF_FUSE_CHK->isChecked();

        tcu.auto_rep_freq = ui->AUTO_REP_FREQ_CHK->isChecked();
        tcu.ab_lock = ui->AB_LOCK_CHK->isChecked();
        tcu.base_unit = ui->UNIT_LIST->currentIndex();
        switch (tcu.base_unit) {
        // ns
        case 0:
            tcu.max_dist = ui->MAX_DIST_EDT->text().toFloat() * dist_ns;
            tcu.delay_offset = ui->DELAY_OFFSET_EDT->text().toFloat();
            tcu.max_dov = ui->MAX_DOV_EDT->text().toFloat() * dist_ns;
            tcu.gate_width_offset = ui->GATE_WIDTH_OFFSET_EDT->text().toFloat();
            tcu.max_laser_width = ui->MAX_LASER_EDT->text().toFloat();
            tcu.laser_width_offset = ui->LASER_OFFSET_EDT->text().toFloat();
            break;
        // μs
        case 1:
            tcu.max_dist = ui->MAX_DIST_EDT->text().toFloat() * dist_ns * 1000;
            tcu.delay_offset = ui->DELAY_OFFSET_EDT->text().toFloat() * 1000;
            tcu.max_dov = ui->MAX_DOV_EDT->text().toFloat() * dist_ns * 1000;
            tcu.gate_width_offset = ui->GATE_WIDTH_OFFSET_EDT->text().toFloat() * 1000;
            tcu.max_laser_width = ui->MAX_LASER_EDT->text().toFloat();
            tcu.laser_width_offset = ui->LASER_OFFSET_EDT->text().toFloat();
            break;
        // m
        case 2:
            tcu.max_dist = ui->MAX_DIST_EDT->text().toFloat();
            tcu.delay_offset = ui->DELAY_OFFSET_EDT->text().toFloat() / dist_ns;
            tcu.max_dov = ui->MAX_DOV_EDT->text().toFloat();
            tcu.gate_width_offset = ui->GATE_WIDTH_OFFSET_EDT->text().toFloat() / dist_ns;
            tcu.max_laser_width = ui->MAX_LASER_EDT->text().toFloat();
            tcu.laser_width_offset = ui->LASER_OFFSET_EDT->text().toFloat();
            break;
        default: break;
        }
        tcu.ps_step[ui->TCU_PS_CONFIG_LIST->currentIndex()] = ui->PS_STEPPING_EDT->text().toInt();
        tcu.laser_on = 0;
        tcu.laser_on |= ui->LASER_CHK_1->isChecked() << 0;
        tcu.laser_on |= ui->LASER_CHK_2->isChecked() << 1;
        tcu.laser_on |= ui->LASER_CHK_3->isChecked() << 2;
        tcu.laser_on |= ui->LASER_CHK_4->isChecked() << 3;
    }
    else {
        // device
        ui->FLIP_OPTION_LIST->setCurrentIndex(dev.flip);
        ui->ROTATE_OPTION_LIST->setCurrentIndex(dev.rotation);
        ui->PTZ_TYPE_LIST->setCurrentIndex(dev.ptz_type);
        ui->EBUS_CHK->setChecked(dev.ebus);
        ui->SHARE_CHK->setChecked(dev.share_tcu_port);
        ui->UNDERWATER_CHK->setChecked(dev.underwater);

        // save
        ui->SAVE_INFO_CHK->setChecked(sv.save_info);
        ui->CUSTOM_INFO_CHK->setChecked(sv.custom_topleft_info);
        ui->GRAYSCALE_CHK->setChecked(sv.save_in_grayscale);
        ui->CONSECUTIVE_CAPTURE_CHK->setChecked(sv.consecutive_capture);
        ui->INTEGRATE_INFO_CHK->setChecked(sv.integrate_info);
        ui->IMG_FORMAT_LST->setCurrentIndex(sv.img_format);

        // TCU type/hz_unit
        ui->TCU_LIST->setCurrentIndex(tcu.type);
        ui->HZ_LIST->setCurrentIndex(tcu.hz_unit);

        // image proc
        ui->GAMMA_EDIT->setText(QString::number(ip.gamma, 'f', 2));
        ui->ACCU_BASE_EDIT->setText(QString::number(ip.accu_base, 'f', 2));
        ui->LOW_IN_EDIT->setText(QString::number(ip.low_in, 'f', 2));
        ui->HIGH_IN_EDIT->setText(QString::number(ip.high_in, 'f', 2));
        ui->LOW_OUT_EDIT->setText(QString::number(ip.low_out, 'f', 2));
        ui->HIGH_OUT_EDIT->setText(QString::number(ip.high_out, 'f', 2));
        ui->DEHAZE_PCT_EDIT->setText(QString::number(ip.dehaze_pct * 100, 'f', 2));
        ui->SKY_TOLERANCE_EDIT->setText(QString::number(ip.sky_tolerance, 'f', 2));
        ui->FAST_GF_EDIT->setText(QString::number(ip.fast_gf));
        ui->COLORMAP_3D_LIST->setCurrentIndex(ip.colormap);
        ui->LOWER_3D_THRESH_EDT->setText(QString::number(ip.lower_3d_thresh, 'f', 3));
        ui->UPPER_3D_THRESH_EDT->setText(QString::number(ip.upper_3d_thresh, 'f', 3));
        ui->CUSTOM_3D_PARAM_CHK->setChecked(ip.custom_3d_param);
        ui->CUSTOM_3D_DELAY_EDT->setText(QString::number((int)std::round(ip.custom_3d_delay)));
        ui->CUSTOM_3D_GW_EDT->setText(QString::number((int)std::round(ip.custom_3d_gate_width)));
#ifdef LVTONG
        ui->FISHNET_RECOG_CHK->setChecked(ip.fishnet_recog);
        ui->FISHNET_THRESH_EDIT->setText(QString::number(ip.fishnet_thresh, 'f', 2));
#endif

        ui->ECC_WINDOW_MODE_LIST->setCurrentIndex(ip.ecc_window_mode);
        ui->ECC_WARP_MODE_LIST->setCurrentIndex(ip.ecc_warp_mode);
        ui->ECC_FUSION_MODE_LIST->setCurrentIndex(ip.ecc_fusion_method);
        ui->ECC_BACKWARD_EDIT->setText(QString::number(ip.ecc_backward));
        ui->ECC_FORWARD_EDIT->setText(QString::number(ip.ecc_forward));
        ui->ECC_LEVELS_EDIT->setText(QString::number(ip.ecc_levels));
        ui->ECC_MAXITER_EDIT->setText(QString::number(ip.ecc_max_iter));
        ui->ECC_EPS_EDIT->setText(QString::number(ip.ecc_eps, 'f', 4));
        ui->ECC_HALF_REG_CHK->setChecked(ip.ecc_half_res_reg);
        ui->ECC_HALF_FUSE_CHK->setChecked(ip.ecc_half_res_fuse);

        ui->AUTO_REP_FREQ_CHK->setChecked(tcu.auto_rep_freq);
        ui->AB_LOCK_CHK->setChecked(tcu.ab_lock);
        ui->UNIT_LIST->setCurrentIndex(tcu.base_unit);
        update_distance_display();
        ui->PS_STEPPING_EDT->setText(QString::number(tcu.ps_step[ui->TCU_PS_CONFIG_LIST->currentIndex()]));
        ui->MAX_PS_STEP_EDT->setText(QString::number(int(std::round(4000. / tcu.ps_step[ui->TCU_PS_CONFIG_LIST->currentIndex()]))));
        ui->LASER_CHK_1->setChecked(tcu.laser_on & 0b0001);
        ui->LASER_CHK_2->setChecked(tcu.laser_on & 0b0010);
        ui->LASER_CHK_3->setChecked(tcu.laser_on & 0b0100);
        ui->LASER_CHK_4->setChecked(tcu.laser_on & 0b1000);
    }
}

void Preferences::config_ip(bool read, int ip, int gateway, int nic_address)
{
    if (read) {
        ui->IP_EDIT->setText(QString::asprintf("%d.%d.%d.%d", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF));
        ui->LOCAL_IP_EDIT->setText(QString::asprintf("%d.%d.%d.%d", (nic_address >> 24) & 0xFF, (nic_address >> 16) & 0xFF, (nic_address >> 8) & 0xFF, nic_address & 0xFF));
    }
    else {
        int ip1, ip2, ip3, ip4;
        sscanf(ui->IP_EDIT->text().toLatin1().constData(), "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);
        emit set_dev_ip((ip1 << 24) + (ip2 << 16) + (ip3 << 8) + ip4, (ip1 << 24) + (ip2 << 16) + (ip3 << 8) + 1);
    }
}

void Preferences::enable_ip_editing(bool enable)
{
    ui->IP_EDIT->setEnabled(enable);
}

void Preferences::set_pixel_format(int idx)
{
    ui->PIXEL_FORMAT_LIST->setCurrentIndex(idx);
}

void Preferences::update_distance_display()
{
    if (!m_config) return;
    const auto& tcu = m_config->get_data().tcu;

    switch (tcu.base_unit) {
    case 0: // ns
        ui->MAX_DIST_EDT->setText(QString::number(std::round(tcu.max_dist / dist_ns), 'f', 0));
        ui->MAX_DIST_UNIT->setText("ns");
        ui->DELAY_OFFSET_EDT->setText(QString::number(std::round(tcu.delay_offset), 'f', 0));
        ui->DELAY_OFFSET_UNIT->setText("ns");
        ui->MAX_DOV_EDT->setText(QString::number(std::round(tcu.max_dov / dist_ns), 'f', 0));
        ui->MAX_DOV_UNIT->setText("ns");
        ui->GATE_WIDTH_OFFSET_EDT->setText(QString::number(std::round(tcu.gate_width_offset), 'f', 0));
        ui->GATE_WIDTH_OFFSET_UNIT->setText("ns");
        break;
    case 1: // μs
        ui->MAX_DIST_EDT->setText(QString::number(tcu.max_dist / dist_ns / 1000, 'f', 3));
        ui->MAX_DIST_UNIT->setText("μs");
        ui->DELAY_OFFSET_EDT->setText(QString::number(tcu.delay_offset / 1000, 'f', 3));
        ui->DELAY_OFFSET_UNIT->setText("μs");
        ui->MAX_DOV_EDT->setText(QString::number(tcu.max_dov / dist_ns / 1000, 'f', 3));
        ui->MAX_DOV_UNIT->setText("μs");
        ui->GATE_WIDTH_OFFSET_EDT->setText(QString::number(tcu.gate_width_offset / 1000, 'f', 3));
        ui->GATE_WIDTH_OFFSET_UNIT->setText("μs");
        break;
    case 2: // m
        ui->MAX_DIST_EDT->setText(QString::number(tcu.max_dist, 'f', 2));
        ui->MAX_DIST_UNIT->setText("m");
        ui->DELAY_OFFSET_EDT->setText(QString::number(tcu.delay_offset * dist_ns, 'f', 2));
        ui->DELAY_OFFSET_UNIT->setText("m");
        ui->MAX_DOV_EDT->setText(QString::number(tcu.max_dov, 'f', 2));
        ui->MAX_DOV_UNIT->setText("m");
        ui->GATE_WIDTH_OFFSET_EDT->setText(QString::number(tcu.gate_width_offset * dist_ns, 'f', 2));
        ui->GATE_WIDTH_OFFSET_UNIT->setText("m");
        break;
    default: break;
    }
    ui->MAX_LASER_EDT->setText(QString::number(tcu.max_laser_width, 'f', 0));
}

void Preferences::display_baudrate(int id, int baudrate)
{
    if (id != ui->COM_LIST->currentIndex()) return;
    int idx = 0;
    switch (baudrate) {
    case 1200:   idx = 1; break;
    case 2400:   idx = 2; break;
    case 4800:   idx = 3; break;
    case 9600:   idx = 4; break;
    case 19200:  idx = 5; break;
    case 38400:  idx = 6; break;
    case 57600:  idx = 7; break;
    case 115200: idx = 8; break;
    default: break;
    }

    ui->BAUDRATE_LIST->setCurrentIndex(idx);
}

void Preferences::switch_language(bool en, QTranslator *trans)
{
    ui->retranslateUi(this);
}

void Preferences::set_ptz_type_enabled(bool enabled)
{
    ui->PTZ_TYPE_LIST->setEnabled(enabled);
}

void Preferences::keyPressEvent(QKeyEvent *event)
{
    static QLineEdit *edit = NULL;
    if (this->focusWidget()) edit = qobject_cast<QLineEdit*>(this->focusWidget());

    switch(event->key()) {
    case Qt::Key_Escape:
        edit ? data_exchange(false), edit->clearFocus() : this->focusWidget() ? this->focusWidget()->clearFocus() : this->reject();
        break;
    case Qt::Key_Enter:
    case Qt::Key_Return:
        if (edit == NULL)                  this->accept();
        else if (edit == ui->COM_DATA_EDT) send_cmd(edit->text());
        else if (edit == ui->IP_EDIT)      config_ip(false);
        else                               data_exchange(true);
        if (edit) this->focusWidget()->clearFocus();
        data_exchange(false);
    case Qt::Key_S:
        if (event->modifiers() == Qt::AltModifier) this->accept();
        break;
    default: break;
    }
    edit = NULL;
}

void Preferences::mousePressEvent(QMouseEvent *event)
{
    if(event->button() != Qt::LeftButton) return;
    pressed = true;
    prev_pos = event->globalPos();

    QDialog::mousePressEvent(event);
}

void Preferences::mouseMoveEvent(QMouseEvent *event)
{
    if (pressed) {
        // use globalPos instead of pos to prevent window shaking
        window()->move(window()->pos() + event->globalPos() - prev_pos);
        prev_pos = event->globalPos();
    }
    else {
        setCursor(cursor_curr_pointer);
    }
    QDialog::mouseMoveEvent(event);
}

void Preferences::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() != Qt::LeftButton) return;
    pressed = false;

    QDialog::mouseReleaseEvent(event);
}

bool Preferences::eventFilter(QObject *obj, QEvent *event)
{
    if (qobject_cast<QComboBox*>(obj) && event->type() == QEvent::MouseMove) return true;
    return QDialog::eventFilter(obj, event);
}

void Preferences::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    if (this->focusWidget()) this->focusWidget()->clearFocus();
    this->setFocus();
}

void Preferences::jump_to_content(int pos)
{
    ui->PREFERENCES->verticalScrollBar()->setSliderPosition(pos);
}

void Preferences::update_device_list(int cmd, QStringList dev_list)
{
    switch (cmd) {
    case 0: ui->DEVICE_LIST->setEnabled(!ui->DEVICE_LIST->isEnabled()); break;
    case 1: ui->DEVICE_LIST->clear(); ui->DEVICE_LIST->addItems(dev_list); break;
    default: break;
    }
}

void Preferences::send_cmd(QString str)
{
    QString send_str = str.simplified();
//    qDebug("%s\n", send_str.toLatin1().data());
    send_str.replace(" ", "");
    bool ok;
    QByteArray cmd(send_str.length() / 2, 0x00);
    for (int i = 0; i < send_str.length() / 2; i++) cmd[i] = send_str.midRef(i * 2, 2).toInt(&ok, 16);

    emit com_write(ui->COM_LIST->currentIndex(), cmd);
}

void Preferences::toggle_laser(int id, bool on)
{
    if (!m_config) return;
    auto& laser_on = m_config->get_data().tcu.laser_on;
    laser_on ^= 1 << id;
    emit laser_toggled(laser_on);
}
