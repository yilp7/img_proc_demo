#include "preferences.h"
#include "ui_preferences.h"

Preferences::Preferences(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Preferences),
    pressed(false),
    device_idx(0),
    symmetry(0),
    cameralink(false),
    split(false),
    port_idx(0),
    share_port(false),
    use_tcp(false),
#ifdef LVTONG
    dist_ns(3e8 * 0.75 / 2e9),
#else
    dist_ns(3e8 / 2e9),
#endif
    auto_rep_freq(true),
    auto_mcp(false),
    hz_unit(0),
    base_unit(0),
#ifdef LVTONG
    max_dist(1125),
#else
    max_dist(15000),
#endif
    delay_offset(0),
    max_dov(750),
    gate_width_offset(0),
    max_laser_width(5000),
    laser_width_offset(0),
    laser_grp(NULL),
    laser_on(0),
    accu_base(1),
    gamma(1.2),
    low_in(0),
    high_in(0.05),
    low_out(0),
    high_out(1),
    dehaze_pct(0.95),
    sky_tolerance(40),
    fast_gf(1),
    colormap(cv::COLORMAP_PARULA),
    lower_3d_thresh(0),
    upper_3d_thresh(0.981),
    truncate_3d(false),
    custom_3d_param(false),
    custom_3d_delay(0),
    custom_3d_gate_width(0),
    fishnet_recog(false),
    fishnet_thresh(0.99)
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
            [this](int index){ device_idx = index; emit query_dev_ip(); });
    ui->FLIP_OPTION_LIST->addItem("None");
    ui->FLIP_OPTION_LIST->addItem("Both");
    ui->FLIP_OPTION_LIST->addItem("X");
    ui->FLIP_OPTION_LIST->addItem("Y");
    ui->FLIP_OPTION_LIST->installEventFilter(this);
    connect(ui->FLIP_OPTION_LIST, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), this,
            [this](int index){ symmetry = index; });
    ui->PIXEL_FORMAT_LIST->addItem("Mono8");
    ui->PIXEL_FORMAT_LIST->addItem("Mono10");
    ui->PIXEL_FORMAT_LIST->addItem("Mono12");
    ui->PIXEL_FORMAT_LIST->addItem("RGB8");
    ui->PIXEL_FORMAT_LIST->installEventFilter(this);
    connect(ui->PIXEL_FORMAT_LIST, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), this,
            [this](int index){
                static int pixel_format[4] = {PixelType_Gvsp_Mono8, PixelType_Gvsp_Mono10, PixelType_Gvsp_Mono12, PixelType_Gvsp_RGB8_Packed};
                emit change_pixel_format(pixel_format[index]);
            });
    connect(ui->CAMERALINK_CHK, &QCheckBox::stateChanged, this, [this](int arg1){ cameralink = arg1; });
    connect(ui->SPLIT_CHK, &QCheckBox::stateChanged, this, [this](int arg1){ split = arg1; });
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
    connect(ui->SHARE_CHK, &QCheckBox::stateChanged, this, [this](int arg1){ share_port = arg1; emit share_tcu_port(arg1); });

//    QFont temp = QFont(consolas);
//    temp.setPixelSize(11);
//    ui->COM_DATA_EDT->setFont(temp);
    ui->COM_DATA_EDT->setFont(consolas);
//![2]

//[3] set up ui for tcu config
    ui->TCU_LIST->addItem("default");
    ui->TCU_LIST->addItem("#0 50ps step");
    ui->TCU_LIST->addItem("#1 50ps step");
    ui->TCU_LIST->addItem("#2 50ps step");
    ui->TCU_LIST->addItem("#3 50ps step");
    ui->TCU_LIST->installEventFilter(this);

    ui->HZ_LIST->addItem("kHz");
    ui->HZ_LIST->addItem("Hz");
    ui->HZ_LIST->installEventFilter(this);

    ui->UNIT_LIST->addItem("ns");
//    ui->UNIT_LIST->addItem(QString::fromLocal8Bit("μs"));
    ui->UNIT_LIST->addItem("μs");
    ui->UNIT_LIST->addItem("m");
    ui->UNIT_LIST->setCurrentIndex(0);
    ui->UNIT_LIST->installEventFilter(this);

    connect(ui->TCU_LIST, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), this,
            [this](int index){ emit tcu_type_changed(index); });
    connect(ui->AUTO_REP_FREQ_CHK, &QCheckBox::stateChanged, this, [this](int arg1){ emit set_auto_rep_freq(auto_rep_freq = arg1); });
    connect(ui->AUTO_MCP_CHK, &QCheckBox::stateChanged, this, [this](int arg1){ auto_mcp = arg1; });
    connect(ui->HZ_LIST, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), this,
            [this](int index){ emit rep_freq_unit_changed(hz_unit = index); });
    connect(ui->UNIT_LIST, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), this,
            [this](int index) {
                emit base_unit_changed(base_unit = index);
                switch (base_unit) {
                case 0: // ns
                    ui->MAX_DIST_EDT->setText(QString::number(std::round(max_dist / dist_ns), 'f', 0));
                    ui->MAX_DIST_UNIT->setText("ns");
                    ui->DELAY_OFFSET_EDT->setText(QString::number(std::round(delay_offset / dist_ns), 'f', 0));
                    ui->DELAY_OFFSET_UNIT->setText("ns");
                    ui->MAX_DOV_EDT->setText(QString::number(std::round(max_dov / dist_ns), 'f', 0));
                    ui->MAX_DOV_UNIT->setText("ns");
                    ui->GATE_WIDTH_OFFSET_EDT->setText(QString::number(std::round(gate_width_offset / dist_ns), 'f', 0));
                    ui->GATE_WIDTH_OFFSET_UNIT->setText("ns");
                    break;
                case 1: // μs
                    ui->MAX_DIST_EDT->setText(QString::number(max_dist / dist_ns / 1000, 'f', 3));
                    ui->MAX_DIST_UNIT->setText("μs");
                    ui->DELAY_OFFSET_EDT->setText(QString::number(delay_offset / dist_ns / 1000, 'f', 3));
                    ui->DELAY_OFFSET_UNIT->setText("μs");
                    ui->MAX_DOV_EDT->setText(QString::number(max_dov / dist_ns / 1000, 'f', 3));
                    ui->MAX_DOV_UNIT->setText("μs");
                    ui->GATE_WIDTH_OFFSET_EDT->setText(QString::number(gate_width_offset / dist_ns / 1000, 'f', 3));
                    ui->GATE_WIDTH_OFFSET_UNIT->setText("μs");
                    break;
                case 2: // m
                    ui->MAX_DIST_EDT->setText(QString::number(max_dist, 'f', 2));
                    ui->MAX_DIST_UNIT->setText("m");
                    ui->DELAY_OFFSET_EDT->setText(QString::number(delay_offset, 'f', 2));
                    ui->DELAY_OFFSET_UNIT->setText("m");
                    ui->MAX_DOV_EDT->setText(QString::number(max_dov, 'f', 2));
                    ui->MAX_DOV_UNIT->setText("m");
                    ui->GATE_WIDTH_OFFSET_EDT->setText(QString::number(gate_width_offset, 'f', 2));
                    ui->GATE_WIDTH_OFFSET_UNIT->setText("m");
                    break;
                default: break;
                }
            });

    connect(ui->MAX_DIST_EDT, &QLineEdit::editingFinished, this, [this](){ emit max_dist_changed(max_dist); });
    connect(ui->DELAY_OFFSET_EDT, &QLineEdit::editingFinished, this,
            [this](){
                emit delay_offset_changed(delay_offset);
                FILE *f = fopen("user_default", "rb+");
                if (!f) return;
                int delay_offset_int = std::round(delay_offset / dist_ns);
                fseek(f, 9, SEEK_SET);
                fwrite(&delay_offset_int, 4, 1, f);
                fclose(f);
    });
    connect(ui->MAX_DOV_EDT, &QLineEdit::editingFinished, this, [this](){ emit max_dov_changed(max_dov); });
    connect(ui->GATE_WIDTH_OFFSET_EDT, &QLineEdit::editingFinished, this,
            [this](){
                emit gate_width_offset_changed(gate_width_offset);
                FILE *f = fopen("user_default", "rb+");
                if (!f) return;
                int gate_width_offset_int = std::round(gate_width_offset / dist_ns);
                fseek(f, 13, SEEK_SET);
                fwrite(&gate_width_offset_int, 4, 1, f);
                fclose(f);
    });
    connect(ui->MAX_LASER_EDT, &QLineEdit::editingFinished, this, [this](){ emit max_laser_changed(max_laser_width); });
    connect(ui->LASER_OFFSET_EDT, &QLineEdit::editingFinished, this,
            [this](){
                emit laser_offset_changed(laser_width_offset);
                FILE *f = fopen("user_default", "rb+");
                if (!f) return;
                int laser_offset_int = std::round(laser_width_offset);
                fseek(f, 17, SEEK_SET);
                fwrite(&laser_offset_int, 4, 1, f);
                fclose(f);
    });
    FILE *f = fopen("user_default", "rb");
    if (f) {
        int delay_offset_int, gate_width_offset_int, laser_offset_int;
        fseek(f, 9, SEEK_SET);
        fread(&delay_offset_int, 4, 1, f);
        emit delay_offset_changed(delay_offset = delay_offset_int * dist_ns);
        fread(&gate_width_offset_int, 4, 1, f);
        emit gate_width_offset_changed(gate_width_offset = gate_width_offset_int * dist_ns);
        fread(&laser_offset_int, 4, 1, f);
        emit laser_offset_changed(laser_width_offset = laser_offset_int);
        fclose(f);
    }

    connect(ui->LASER_ENABLE_CHK, &QCheckBox::stateChanged, this,
            [this](int arg1){ send_cmd(arg1 ? "88 1F 00 00 00 01 99" : "88 1F 00 00 00 00 99"); });
    laser_grp = new QButtonGroup(this);
    laser_grp->addButton(ui->LASER_CHK_1, 0);
    laser_grp->addButton(ui->LASER_CHK_2, 1);
    laser_grp->addButton(ui->LASER_CHK_3, 2);
    laser_grp->addButton(ui->LASER_CHK_4, 3);
    laser_grp->setExclusive(false);
    connect(laser_grp, SIGNAL(buttonToggled(int, bool)), SLOT(toggle_laser(int, bool)));
//![3]

//[4]
    connect(ui->SAVE_INFO_CHK, &QCheckBox::stateChanged, this, [this](int arg1){ save_info = arg1; });
    QFont temp_f(consolas);
    temp_f.setPixelSize(11);
    ui->CUSTOM_INFO_EDT->setFont(temp_f);
    connect(ui->CUSTOM_INFO_CHK, &QCheckBox::stateChanged, this,
            [this](int arg1){ custom_topleft_info = arg1; ui->CUSTOM_INFO_EDT->setEnabled(arg1); });
//![4]

//[5] set up ui for image proc
    connect(ui->LOWER_3D_THRESH_EDT, &QLineEdit::editingFinished, this,
            [this](){ if (lower_3d_thresh < 0) lower_3d_thresh = 0; emit lower_3d_thresh_updated(); });
    connect(ui->UPPER_3D_THRESH_EDT, &QLineEdit::editingFinished, this,
            [this](){ if (upper_3d_thresh > 1.001) upper_3d_thresh = 1.001; });
    QStringList colormap_names;
    colormap_names << "AUTUMN" << "BONE" << "JET" << "WINTER" << "RAINBOW" << "OCEAN" << "SUMMER" << "SPRING"
                   << "COOL" << "HSV" << "PINK" << "HOT" << "PARULA" << "MAGMA" << "INFERNO" << "PLASMA"
                   << "VIRIDIS" << "CIVIDIS" << "TWILIGHT" << "TWILIGHT_SHIFTED" << "TURBO" << "DEEPGREEN";
    ui->COLORMAP_3D_LIST->addItems(colormap_names);
    ui->COLORMAP_3D_LIST->installEventFilter(this);
    connect(ui->COLORMAP_3D_LIST, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), this,
            [this](int index){ colormap = index; });
    connect(ui->TRUNCATE_3D_CHK, &QCheckBox::stateChanged, this,
            [this](int arg1){ truncate_3d = arg1; });
    connect(ui->CUSTOM_3D_PARAM_CHK, &QCheckBox::stateChanged, this,
            [this](int arg1){
                custom_3d_param = arg1;
                ui->CUSTOM_3D_DELAY_EDT->setEnabled(arg1);
                ui->CUSTOM_3D_GW_EDT->setEnabled(arg1);
                emit query_tcu_param();
            });
#ifdef LVTONG
    connect(ui->FISHNET_RECOG_CHK, &QCheckBox::stateChanged, this, [this](int arg1){ fishnet_recog = arg1; });
    connect(ui->FISHNET_THRESH_EDIT, &QLineEdit::editingFinished, this,
            [this](){
                fishnet_thresh = ui->FISHNET_THRESH_EDIT->text().toFloat();
                ui->FISHNET_THRESH_EDIT->setText(QString::number(fishnet_thresh, 'f', 2));
            });
#else
    ui->FISHNET->hide();
    ui->FISHNET_RECOG_CHK->hide();
    ui->FISHNET_THRESH->hide();
    ui->FISHNET_THRESH_EDIT->hide();
#endif
    //![5]
    data_exchange(false);
}

Preferences::~Preferences()
{
    delete ui;
}

void Preferences::data_exchange(bool read)
{
    if (read) {
        gamma = ui->GAMMA_EDIT->text().toFloat();
        accu_base = ui->ACCU_BASE_EDIT->text().toFloat();
        low_in = ui->LOW_IN_EDIT->text().toFloat();
        high_in = ui->HIGH_IN_EDIT->text().toFloat();
        low_out = ui->LOW_OUT_EDIT->text().toFloat();
        high_out = ui->HIGH_OUT_EDIT->text().toFloat();
        dehaze_pct = ui->DEHAZE_PCT_EDIT->text().toFloat() / 100;
        sky_tolerance = ui->SKY_TOLERANCE_EDIT->text().toFloat();
        fast_gf = ui->FAST_GF_EDIT->text().toInt();
        colormap = ui->COLORMAP_3D_LIST->currentIndex();
        lower_3d_thresh = ui->LOWER_3D_THRESH_EDT->text().toFloat();
        upper_3d_thresh = ui->UPPER_3D_THRESH_EDT->text().toFloat();
        custom_3d_param = ui->CUSTOM_3D_PARAM_CHK->isChecked();
        custom_3d_delay = ui->CUSTOM_3D_DELAY_EDT->text().toFloat();
        custom_3d_gate_width = ui->CUSTOM_3D_GW_EDT->text().toFloat();
#ifdef LVTONG
        fishnet_recog = ui->FISHNET_RECOG_CHK->isChecked();
        fishnet_thresh = ui->FISHNET_THRESH_EDIT->text().toFloat();
#endif

        auto_rep_freq = ui->AUTO_REP_FREQ_CHK->isChecked();
        base_unit = ui->UNIT_LIST->currentIndex();
        switch (base_unit) {
        // ns
        case 0:
            max_dist = ui->MAX_DIST_EDT->text().toFloat() * dist_ns;
            delay_offset = ui->DELAY_OFFSET_EDT->text().toFloat() * dist_ns;
            max_dov = ui->MAX_DOV_EDT->text().toFloat() * dist_ns;
            gate_width_offset = ui->GATE_WIDTH_OFFSET_EDT->text().toFloat() * dist_ns;
            max_laser_width = ui->MAX_LASER_EDT->text().toFloat();
            laser_width_offset = ui->LASER_OFFSET_EDT->text().toFloat();
            break;
        // μs
        case 1:
            max_dist = ui->MAX_DIST_EDT->text().toFloat() * dist_ns * 1000;
            delay_offset = ui->DELAY_OFFSET_EDT->text().toFloat() * dist_ns * 1000;
            max_dov = ui->MAX_DOV_EDT->text().toFloat() * dist_ns * 1000;
            gate_width_offset = ui->GATE_WIDTH_OFFSET_EDT->text().toFloat() * dist_ns * 1000;
            max_laser_width = ui->MAX_LASER_EDT->text().toFloat();
            laser_width_offset = ui->LASER_OFFSET_EDT->text().toFloat();
            break;
        // m
        case 2:
            max_dist = ui->MAX_DIST_EDT->text().toFloat();
            delay_offset = ui->DELAY_OFFSET_EDT->text().toFloat();
            max_dov = ui->MAX_DOV_EDT->text().toFloat();
            gate_width_offset = ui->GATE_WIDTH_OFFSET_EDT->text().toFloat();
            max_laser_width = ui->MAX_LASER_EDT->text().toFloat();
            laser_width_offset = ui->LASER_OFFSET_EDT->text().toFloat();
            break;
        default: break;
        }
        laser_on = 0;
        laser_on |= ui->LASER_CHK_1->isChecked() << 0;
        laser_on |= ui->LASER_CHK_2->isChecked() << 1;
        laser_on |= ui->LASER_CHK_3->isChecked() << 2;
        laser_on |= ui->LASER_CHK_4->isChecked() << 3;
    }
    else {
        ui->GAMMA_EDIT->setText(QString::number(gamma, 'f', 2));
        ui->ACCU_BASE_EDIT->setText(QString::number(accu_base, 'f', 2));
        ui->LOW_IN_EDIT->setText(QString::number(low_in, 'f', 2));
        ui->HIGH_IN_EDIT->setText(QString::number(high_in, 'f', 2));
        ui->LOW_OUT_EDIT->setText(QString::number(low_out, 'f', 2));
        ui->HIGH_OUT_EDIT->setText(QString::number(high_out, 'f', 2));
        ui->DEHAZE_PCT_EDIT->setText(QString::number(dehaze_pct * 100, 'f', 2));
        ui->SKY_TOLERANCE_EDIT->setText(QString::number(sky_tolerance, 'f', 2));
        ui->FAST_GF_EDIT->setText(QString::number(fast_gf));
        ui->COLORMAP_3D_LIST->setCurrentIndex(colormap);
        ui->LOWER_3D_THRESH_EDT->setText(QString::number(lower_3d_thresh, 'f', 3));
        ui->UPPER_3D_THRESH_EDT->setText(QString::number(upper_3d_thresh, 'f', 3));
        ui->CUSTOM_3D_PARAM_CHK->setChecked(custom_3d_param);
        ui->CUSTOM_3D_DELAY_EDT->setText(QString::number((int)std::round(custom_3d_delay)));
        ui->CUSTOM_3D_GW_EDT->setText(QString::number((int)std::round(custom_3d_gate_width)));
#ifdef LVTONG
        ui->FISHNET_RECOG_CHK->setChecked(fishnet_recog);
        ui->FISHNET_THRESH_EDIT->setText(QString::number(fishnet_thresh, 'f', 2));
#endif

        ui->AUTO_REP_FREQ_CHK->setChecked(auto_rep_freq);
        ui->UNIT_LIST->setCurrentIndex(base_unit);
        switch (base_unit) {
        // ns
        case 0:
            ui->MAX_DIST_EDT->setText(QString::number(std::round(max_dist / dist_ns), 'f', 0));
            ui->DELAY_OFFSET_EDT->setText(QString::number(std::round(delay_offset / dist_ns), 'f', 0));
            ui->MAX_DOV_EDT->setText(QString::number(std::round(max_dov / dist_ns), 'f', 0));
            ui->GATE_WIDTH_OFFSET_EDT->setText(QString::number(std::round(gate_width_offset / dist_ns), 'f', 0));
            ui->MAX_LASER_EDT->setText(QString::number(std::round(max_laser_width), 'f', 0));
            ui->LASER_OFFSET_EDT->setText(QString::number(std::round(laser_width_offset), 'f', 0));
            break;
        // μs
        case 1:
            ui->MAX_DIST_EDT->setText(QString::number(max_dist / dist_ns / 1000, 'f', 3));
            ui->DELAY_OFFSET_EDT->setText(QString::number(delay_offset / dist_ns / 1000, 'f', 3));
            ui->MAX_DOV_EDT->setText(QString::number(max_dov / dist_ns / 1000, 'f', 3));
            ui->GATE_WIDTH_OFFSET_EDT->setText(QString::number(gate_width_offset / dist_ns / 1000, 'f', 3));
            ui->MAX_LASER_EDT->setText(QString::number(std::round(max_laser_width), 'f', 0));
            ui->LASER_OFFSET_EDT->setText(QString::number(std::round(laser_width_offset), 'f', 0));
            break;
        // m
        case 2:
            ui->MAX_DIST_EDT->setText(QString::number(max_dist, 'f', 2));
            ui->DELAY_OFFSET_EDT->setText(QString::number(delay_offset, 'f', 2));
            ui->MAX_DOV_EDT->setText(QString::number(max_dov, 'f', 2));
            ui->GATE_WIDTH_OFFSET_EDT->setText(QString::number(gate_width_offset, 'f', 2));
            ui->MAX_LASER_EDT->setText(QString::number(std::round(max_laser_width), 'f', 0));
            ui->LASER_OFFSET_EDT->setText(QString::number(std::round(laser_width_offset), 'f', 0));
            break;
        default: break;
        }
        ui->LASER_CHK_1->setChecked(laser_on & 0b0001);
        ui->LASER_CHK_2->setChecked(laser_on & 0b0010);
        ui->LASER_CHK_3->setChecked(laser_on & 0b0100);
        ui->LASER_CHK_4->setChecked(laser_on & 0b1000);
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
    laser_on ^= 1 << id;
//    if (on) laser_on |= 1 << id;
//    else    laser_on &= 0 << id;
    emit laser_toggled(laser_on);
}
