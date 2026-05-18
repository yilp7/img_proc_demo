#include "setupoverlay.h"
#include "ui_preferences.h"

#include "util/config.h"
#include "util/util.h"
#include "widgets/flowlayout.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QEvent>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSerialPortInfo>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QWidget>
#include <algorithm>
#include <cmath>
#include <cstdio>

SetupOverlay::SetupOverlay(QWidget *parent)
    : QFrame(parent),
      m_scroll(nullptr),
      m_rows_container(nullptr),
      m_preview_sidebar(nullptr),
      m_preview_layout(nullptr),
      m_form_root(nullptr),
      m_ui(nullptr),
      m_config(nullptr),
      m_laser_grp(nullptr),
      m_split(false),
      m_cameralink(false),
      m_dist_ns(3e8f / 2e9f)
{
    setObjectName("SETUP_OVERLAY");
    for (int i = 0; i < 3; ++i) {
        m_row[i]        = nullptr;
        m_row_layout[i] = nullptr;
    }

    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(8);

    m_scroll = new QScrollArea(this);
    m_scroll->setObjectName("SETUP_SCROLL");
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);

    m_rows_container = new QWidget(m_scroll);
    auto *rows_layout = new QVBoxLayout(m_rows_container);
    rows_layout->setContentsMargins(0, 0, 0, 0);
    rows_layout->setSpacing(8);

    static const char *row_names[3] = { "SETUP_ROW_1", "SETUP_ROW_2", "SETUP_ROW_3" };
    for (int i = 0; i < 3; ++i) {
        m_row[i] = new QWidget(m_rows_container);
        m_row[i]->setObjectName(row_names[i]);
        m_row_layout[i] = new QHBoxLayout(m_row[i]);
        m_row_layout[i]->setContentsMargins(0, 0, 0, 0);
        m_row_layout[i]->setSpacing(8);
        rows_layout->addWidget(m_row[i]);
    }
    rows_layout->addStretch(1);
    m_scroll->setWidget(m_rows_container);

    m_preview_sidebar = new QWidget(this);
    m_preview_sidebar->setObjectName("SETUP_PREVIEW_SIDEBAR");
    m_preview_sidebar->setAttribute(Qt::WA_StyledBackground, true);
    m_preview_sidebar->setFixedWidth(220);
    m_preview_layout = new QVBoxLayout(m_preview_sidebar);
    m_preview_layout->setContentsMargins(0, 0, 0, 0);
    m_preview_layout->setSpacing(6);
    auto *placeholder = new QLabel("Preview", m_preview_sidebar);
    placeholder->setObjectName("SETUP_PREVIEW_PLACEHOLDER");
    placeholder->setAlignment(Qt::AlignCenter);
    m_preview_layout->addWidget(placeholder);
    m_preview_layout->addStretch(1);

    root->addWidget(m_scroll, 1);
    root->addWidget(m_preview_sidebar);
}

SetupOverlay::~SetupOverlay()
{
    delete m_ui;
    delete m_form_root;
}

QWidget *SetupOverlay::contentRow(int idx) const
{
    if (idx < 0 || idx >= 3) return nullptr;
    return m_row[idx];
}

QHBoxLayout *SetupOverlay::rowLayout(int idx) const
{
    if (idx < 0 || idx >= 3) return nullptr;
    return m_row_layout[idx];
}

void SetupOverlay::setup_form()
{
    if (m_ui) return;

    m_form_root = new QDialog(nullptr);
    m_form_root->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    m_form_root->hide();

    m_ui = new Ui::Preferences;
    m_ui->setupUi(m_form_root);

    // Move the 5 top-level groups out of m_form_root and into the SetupOverlay rows.
    auto move_card = [](QHBoxLayout *row, QWidget *card, QWidget *parent_w) {
        if (!row || !card) return;
        const QSize orig_size = card->size();
        card->setParent(parent_w);
        card->move(0, 0);
        if (!card->layout()) card->setMinimumSize(orig_size);
        card->show();
        row->addWidget(card);
    };

    move_card(m_row_layout[0], m_ui->SAVE_LOAD_GRP,    m_row[0]);
    move_card(m_row_layout[0], m_ui->SERIAL_PORTS_GRP, m_row[0]);
    move_card(m_row_layout[0], m_ui->TCU_GRP,          m_row[0]);
    move_card(m_row_layout[1], m_ui->DEVICES_GRP,      m_row[1]);

    // Row 3 — Image Processing as a single full-width card whose body is a
    // FlowLayout. The 6 sub-groups (ACCUMULATIVE / ADAPTIVE / DCP / IMAGE_3D /
    // FISHNET / ECC) re-parent out of the legacy IMG_PROC_GRP into the flow,
    // tiling left-to-right and wrapping when total width exceeds the card.
    // IMG_PROC_GRP itself stays under m_form_root as an empty husk.
    auto *img_proc_card = new QGroupBox(tr("Image Processing"), m_row[2]);
    img_proc_card->setObjectName("IMG_PROC_FLOW");
    auto *flow = new FlowLayout(img_proc_card, 8, 8, 8);
    auto move_into_flow = [&img_proc_card, flow](QWidget *w) {
        if (!w) return;
        // Capture original .ui geometry BEFORE re-parent; the sub-groups
        // have no layouts (absolute-positioned children) so without an
        // explicit minimumSize the FlowLayout would collapse them to ~30px.
        const QSize orig_size = w->size();
        w->setParent(img_proc_card);
        w->setGeometry(0, 0, orig_size.width(), orig_size.height());
        if (!w->layout()) w->setMinimumSize(orig_size);
        flow->addWidget(w);
        w->show();
    };
    move_into_flow(m_ui->ACCUMULATIVE_GRP);
    move_into_flow(m_ui->ADAPTIVE_GRP);
    move_into_flow(m_ui->DCP_GRP);
    move_into_flow(m_ui->IMAGE_3D_GRP);
    move_into_flow(m_ui->FISHNET);
    move_into_flow(m_ui->ECC_GRP);
    QSizePolicy sp = img_proc_card->sizePolicy();
    sp.setHorizontalPolicy(QSizePolicy::Expanding);
    sp.setVerticalPolicy(QSizePolicy::MinimumExpanding);
    sp.setHeightForWidth(true);
    img_proc_card->setSizePolicy(sp);
    // Backstop minimum height: tallest sub-group (ECC=205) + groupbox title +
    // margins. Guarantees the card shows full content even when the parent
    // QVBoxLayout doesn't propagate heightForWidth. If sub-groups wrap to
    // multiple rows on narrow displays, heightForWidth bumps this up further.
    img_proc_card->setMinimumHeight(240);
    img_proc_card->show();
    m_row_layout[2]->addWidget(img_proc_card, 1);  // stretch factor: span full width
}

void SetupOverlay::bind()
{
    if (!m_ui) return;

    auto *u = m_ui;

//[1] device config
    u->IP_EDIT->setEnabled(false);
    u->DEVICE_LIST->installEventFilter(this);
    connect(u->DEVICE_LIST, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            [this, u](int index){ if (m_config) m_config->get_data().tcu.type = index; emit query_dev_ip(); });
    u->ROTATE_OPTION_LIST->addItem("  0\xc2\xb0");
    u->ROTATE_OPTION_LIST->addItem(" 90\xc2\xb0");
    u->ROTATE_OPTION_LIST->addItem("180\xc2\xb0");
    u->ROTATE_OPTION_LIST->addItem("270\xc2\xb0");
    u->ROTATE_OPTION_LIST->installEventFilter(this);
    connect(u->ROTATE_OPTION_LIST, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            [this](int index){ if (m_config) m_config->get_data().device.rotation = index; emit rotate_image(index); });
    u->FLIP_OPTION_LIST->addItem("None");
    u->FLIP_OPTION_LIST->addItem("Both");
    u->FLIP_OPTION_LIST->addItem("X");
    u->FLIP_OPTION_LIST->addItem("Y");
    u->FLIP_OPTION_LIST->installEventFilter(this);
    connect(u->FLIP_OPTION_LIST, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            [this](int index){ if (m_config) m_config->get_data().device.flip = index; });
    u->PIXEL_FORMAT_LIST->addItem("Mono8");
    u->PIXEL_FORMAT_LIST->addItem("Mono10");
    u->PIXEL_FORMAT_LIST->addItem("Mono12");
    u->PIXEL_FORMAT_LIST->addItem("RGB8");
    u->PIXEL_FORMAT_LIST->addItem("BayerRG8");
    u->PIXEL_FORMAT_LIST->installEventFilter(this);
    connect(u->PIXEL_FORMAT_LIST, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            [this](int index){
                static int pixel_format[5] = {PixelType_Gvsp_Mono8, PixelType_Gvsp_Mono10, PixelType_Gvsp_Mono12, PixelType_Gvsp_RGB8_Packed, PixelType_Gvsp_BayerRG8};
                emit change_pixel_format(pixel_format[index]);
            });
#ifndef WIN32
    u->EBUS_CHK->setCheckable(false);
#endif
    connect(u->SPLIT_CHK,       &QCheckBox::stateChanged, this, [this](int arg1){ m_split = arg1; });
    connect(u->UNDERWATER_CHK,  &QCheckBox::stateChanged, this, [this](int arg1){ emit device_underwater(arg1); });
    connect(u->EBUS_CHK,        &QCheckBox::stateChanged, this, [this](int arg1){ if (m_config) m_config->get_data().device.ebus = arg1; emit search_for_devices(); });
    u->PTZ_TYPE_LIST->addItem("pelco-p");
    u->PTZ_TYPE_LIST->addItem("usbcan");
    u->PTZ_TYPE_LIST->addItem("udp-scw");
    u->PTZ_TYPE_LIST->installEventFilter(this);
    connect(u->PTZ_TYPE_LIST, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            [this](int index){ if (m_config) m_config->get_data().device.ptz_type = index; });
    connect(u->CAMERALINK_CHK, &QCheckBox::stateChanged, this, [this](int arg1){ m_cameralink = arg1; emit search_for_devices(); });

//[2] serial comm.
    u->COM_LIST->addItem("TCU");
    u->COM_LIST->addItem("RANGE");
#ifdef ICMOS
    u->COM_LIST->addItem("R1");
    u->COM_LIST->addItem("R2");
#else
    u->COM_LIST->addItem("LENS");
    u->COM_LIST->addItem("LASER");
#endif
    u->COM_LIST->addItem("PTZ");
    u->COM_LIST->installEventFilter(this);

    QStringList available_ports;
    for (const QSerialPortInfo &info: QSerialPortInfo::availablePorts()) available_ports << info.portName();
    u->AVAILABLE_COM_LIST->addItems(available_ports);
    u->AVAILABLE_COM_LIST->installEventFilter(this);
    connect(u->REFRESH_AVAILABLE_PORTS_BTN, &QPushButton::clicked, this,
            [u](){
                QStringList ports;
                for (const QSerialPortInfo &info: QSerialPortInfo::availablePorts()) ports << info.portName();
                u->AVAILABLE_COM_LIST->clear();
                u->AVAILABLE_COM_LIST->addItems(ports);
            });

    u->BAUDRATE_LIST->addItem("------");
    u->BAUDRATE_LIST->addItem("  1200");
    u->BAUDRATE_LIST->addItem("  2400");
    u->BAUDRATE_LIST->addItem("  4800");
    u->BAUDRATE_LIST->addItem("  9600");
    u->BAUDRATE_LIST->addItem(" 19200");
    u->BAUDRATE_LIST->addItem(" 38400");
    u->BAUDRATE_LIST->addItem(" 57600");
    u->BAUDRATE_LIST->addItem("115200");
    u->BAUDRATE_LIST->installEventFilter(this);

    connect(u->COM_LIST, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            [this](int index){ emit get_port_info(index); });
    connect(u->BAUDRATE_LIST, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::activated), this,
            [this, u](const QString &arg1){ emit change_baudrate(u->COM_LIST->currentIndex(), arg1.toInt()); });
    connect(u->TCP_SERVER_CHK, &QCheckBox::stateChanged, this,
            [this, u](int arg1){ emit set_tcp_status(u->COM_LIST->currentIndex(), arg1); });

    connect(u->SHARE_CHK, &QCheckBox::stateChanged, this,
            [this](int arg1){ if (m_config) m_config->get_data().device.share_tcu_port = arg1; emit share_tcu_port(arg1); });

    u->COM_DATA_EDT->setFont(consolas);

//[3] tcu config
    u->TCU_LIST->addItem("default");
    u->TCU_LIST->addItem("40ps step");
    u->TCU_LIST->addItem("10ps step");
    u->TCU_LIST->installEventFilter(this);
    connect(u->TCU_LIST, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            [this, u](int index){
                emit tcu_type_changed(index);
                switch (index) {
                case 1:  u->PS_CONFIG_GRP->show(); break;
                default: u->PS_CONFIG_GRP->hide(); break;
                }
            });

    connect(u->AUTO_REP_FREQ_CHK, &QCheckBox::stateChanged, this, [this](int arg1){ if (m_config) m_config->get_data().tcu.auto_rep_freq = arg1; emit set_auto_rep_freq(arg1); });
    connect(u->AUTO_MCP_CHK,      &QCheckBox::stateChanged, this, [this](int arg1){ if (m_config) m_config->get_data().tcu.auto_mcp = arg1;      emit set_auto_mcp(arg1); });
    connect(u->AB_LOCK_CHK,       &QCheckBox::stateChanged, this, [this](int arg1){ if (m_config) m_config->get_data().tcu.ab_lock = arg1;       emit set_ab_lock(arg1); });

    u->HZ_LIST->addItem("kHz");
    u->HZ_LIST->addItem("Hz");
    u->HZ_LIST->setCurrentIndex(0);
    u->HZ_LIST->installEventFilter(this);

    u->UNIT_LIST->addItem("ns");
    u->UNIT_LIST->addItem("\xce\xbcs");
    u->UNIT_LIST->addItem("m");
    u->UNIT_LIST->setCurrentIndex(0);
    u->UNIT_LIST->installEventFilter(this);

    connect(u->HZ_LIST, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            [this](int index){ if (m_config) m_config->get_data().tcu.hz_unit = index; emit rep_freq_unit_changed(index); });
    connect(u->UNIT_LIST, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            [this](int index) {
                if (m_config) m_config->get_data().tcu.base_unit = index;
                emit base_unit_changed(index);
                update_distance_display();
            });

    connect(u->MAX_DIST_EDT, &QLineEdit::editingFinished, this, [this](){ if (m_config) emit max_dist_changed(m_config->get_data().tcu.max_dist); });
    connect(u->DELAY_OFFSET_EDT, &QLineEdit::editingFinished, this,
            [this](){ if (m_config) emit delay_offset_changed(m_config->get_data().tcu.delay_offset * m_dist_ns); });
    connect(u->MAX_DOV_EDT, &QLineEdit::editingFinished, this, [this](){ if (m_config) emit max_dov_changed(m_config->get_data().tcu.max_dov); });
    connect(u->GATE_WIDTH_OFFSET_EDT, &QLineEdit::editingFinished, this,
            [this](){ if (m_config) emit gate_width_offset_changed(m_config->get_data().tcu.gate_width_offset * m_dist_ns); });
    connect(u->MAX_LASER_EDT, &QLineEdit::editingFinished, this, [this](){ if (m_config) emit max_laser_changed(m_config->get_data().tcu.max_laser_width); });
    connect(u->LASER_OFFSET_EDT, &QLineEdit::editingFinished, this,
            [this](){ if (m_config) emit laser_offset_changed(m_config->get_data().tcu.laser_width_offset); });

    u->PS_CONFIG_GRP->hide();
    u->TCU_PS_CONFIG_LIST->addItem("GW_A");
    u->TCU_PS_CONFIG_LIST->addItem("DELAY_A");
    u->TCU_PS_CONFIG_LIST->addItem("GW_B");
    u->TCU_PS_CONFIG_LIST->addItem("DELAY_B");
    u->TCU_PS_CONFIG_LIST->installEventFilter(this);
    u->TCU_PS_CONFIG_LIST->setCurrentIndex(0);
    connect(u->TCU_PS_CONFIG_LIST, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            [this](int idx){ emit ps_config_updated(true, idx, 0); });
    connect(u->PS_STEPPING_EDT, &QLineEdit::editingFinished, this,
            [this, u](){
                if (!m_config) return;
                int idx = u->TCU_PS_CONFIG_LIST->currentIndex();
                auto& ps = m_config->get_data().tcu.ps_step[idx];
                ps = std::min(std::max(u->PS_STEPPING_EDT->text().toUInt(), (uint)1), (uint)4000);
                u->MAX_PS_STEP_EDT->setText(QString::number(int(std::round(4000. / ps))));
                u->PS_STEPPING_EDT->setText(QString::number(ps));
                emit ps_config_updated(false, idx, u->PS_STEPPING_EDT->text().toInt());
            });
    connect(u->MAX_PS_STEP_EDT, &QLineEdit::editingFinished, this,
            [this, u](){
                if (!m_config) return;
                int idx = u->TCU_PS_CONFIG_LIST->currentIndex();
                int max_step = std::min(std::max(u->MAX_PS_STEP_EDT->text().toInt(), 1), 100);
                u->MAX_PS_STEP_EDT->setText(QString::number(max_step));
                auto& ps = m_config->get_data().tcu.ps_step[idx];
                ps = int(std::round(4000. / max_step));
                u->PS_STEPPING_EDT->setText(QString::number(ps));
                emit ps_config_updated(false, idx, u->PS_STEPPING_EDT->text().toInt());
            });

    connect(u->LASER_ENABLE_CHK, &QCheckBox::stateChanged, this,
            [this](int arg1){
                uchar cmd[7] = {0x88, 0x1F, 0x00, 0x00, 0x00, uchar(arg1 ? 0x01 : 0x00), 0x99};
                emit com_write(0, QByteArray((char*)cmd, 7));
            });
    m_laser_grp = new QButtonGroup(this);
    m_laser_grp->addButton(u->LASER_CHK_1, 0);
    m_laser_grp->addButton(u->LASER_CHK_2, 1);
    m_laser_grp->addButton(u->LASER_CHK_3, 2);
    m_laser_grp->addButton(u->LASER_CHK_4, 3);
    m_laser_grp->setExclusive(false);
    connect(m_laser_grp, SIGNAL(buttonToggled(int, bool)), this, SLOT(toggle_laser(int, bool)));

//[4] save
    connect(u->SAVE_INFO_CHK, &QCheckBox::stateChanged, this, [this](int arg1){ if (m_config) m_config->get_data().save.save_info = arg1; });
    QFont temp_f(consolas);
    temp_f.setPixelSize(11);
    u->CUSTOM_INFO_EDT->setFont(temp_f);
    connect(u->CUSTOM_INFO_CHK, &QCheckBox::stateChanged, this,
            [this, u](int arg1){ if (m_config) m_config->get_data().save.custom_topleft_info = arg1; u->CUSTOM_INFO_EDT->setEnabled(arg1); });
    connect(u->GRAYSCALE_CHK, &QCheckBox::stateChanged, this,
            [this](int arg1){ if (m_config) m_config->get_data().save.save_in_grayscale = arg1; });
    connect(u->INTEGRATE_INFO_CHK, &QCheckBox::stateChanged, this,
            [this](int arg1){ if (m_config) m_config->get_data().save.integrate_info = arg1; });
    u->IMG_FORMAT_LST->addItem("bmp/tiff");
    u->IMG_FORMAT_LST->addItem("jpg");
    u->IMG_FORMAT_LST->setCurrentIndex(0);
    connect(u->IMG_FORMAT_LST, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            [this](int idx){ if (m_config) m_config->get_data().save.img_format = idx; });
    connect(u->CONSECUTIVE_CAPTURE_CHK, &QCheckBox::stateChanged, this,
            [this](int arg1){ if (m_config) m_config->get_data().save.consecutive_capture = arg1; });

//[5] image proc
    connect(u->LOWER_3D_THRESH_EDT, &QLineEdit::editingFinished, this,
            [this](){ if (m_config && m_config->get_data().image_proc.lower_3d_thresh < 0) m_config->get_data().image_proc.lower_3d_thresh = 0; emit lower_3d_thresh_updated(); });
    connect(u->UPPER_3D_THRESH_EDT, &QLineEdit::editingFinished, this,
            [this](){ if (m_config && m_config->get_data().image_proc.upper_3d_thresh > 1.001) m_config->get_data().image_proc.upper_3d_thresh = 1.001; });
    QStringList colormap_names;
    colormap_names << "AUTUMN" << "BONE" << "JET" << "WINTER" << "RAINBOW" << "OCEAN" << "SUMMER" << "SPRING"
                   << "COOL" << "HSV" << "PINK" << "HOT" << "PARULA" << "MAGMA" << "INFERNO" << "PLASMA"
                   << "VIRIDIS" << "CIVIDIS" << "TWILIGHT" << "TWILIGHT_SHIFTED" << "TURBO" << "DEEPGREEN";
    u->COLORMAP_3D_LIST->addItems(colormap_names);
    u->COLORMAP_3D_LIST->installEventFilter(this);
    connect(u->COLORMAP_3D_LIST, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            [this](int index){ if (m_config) m_config->get_data().image_proc.colormap = index; });
    connect(u->TRUNCATE_3D_CHK, &QCheckBox::stateChanged, this,
            [this](int arg1){ if (m_config) m_config->get_data().image_proc.truncate_3d = arg1; });
    connect(u->CUSTOM_3D_PARAM_CHK, &QCheckBox::stateChanged, this,
            [this, u](int arg1){
                if (m_config) m_config->get_data().image_proc.custom_3d_param = arg1;
                u->CUSTOM_3D_DELAY_EDT->setEnabled(arg1);
                u->CUSTOM_3D_GW_EDT->setEnabled(arg1);
                emit query_tcu_param();
            });
#ifdef LVTONG
    QStringList model_list;
    model_list << "01 legacy" << "02 pix2pix" << "03 maskguided";
    QStandardItemModel *model = new QStandardItemModel();
    for (int i = 0; i < model_list.size(); i++) {
        QStandardItem *item = new QStandardItem(model_list[i]);
        item->setToolTip(model_list[i]);
        model->appendRow(item);
    }
    u->MODEL_LIST->setModel(model);
    u->MODEL_LIST->setCurrentIndex(0);
    connect(u->MODEL_LIST, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            [this, u](int index){
                if (m_config) m_config->get_data().image_proc.model_idx = index;
                u->FISHNET_THRESH_EDIT->setEnabled(!index);
            });
    connect(u->FISHNET_RECOG_CHK, &QCheckBox::stateChanged, this, [this](int arg1){ if (m_config) m_config->get_data().image_proc.fishnet_recog = arg1; });
    connect(u->FISHNET_THRESH_EDIT, &QLineEdit::editingFinished, this,
            [this, u](){
                if (!m_config) return;
                auto& ft = m_config->get_data().image_proc.fishnet_thresh;
                ft = u->FISHNET_THRESH_EDIT->text().toFloat();
                u->FISHNET_THRESH_EDIT->setText(QString::number(ft, 'f', 2));
            });
#else
    u->FISHNET->hide();
    u->MODEL_LIST->hide();
    u->FISHNET_RECOG_CHK->hide();
    u->FISHNET_THRESH->hide();
    u->FISHNET_THRESH_EDIT->hide();
#endif

    u->ECC_WINDOW_MODE_LIST->addItem("backward");
    u->ECC_WINDOW_MODE_LIST->addItem("balanced");
    u->ECC_WINDOW_MODE_LIST->addItem("custom");
    u->ECC_WINDOW_MODE_LIST->installEventFilter(this);

    u->ECC_WARP_MODE_LIST->addItem("translate");
    u->ECC_WARP_MODE_LIST->addItem("euclidean");
    u->ECC_WARP_MODE_LIST->addItem("affine");
    u->ECC_WARP_MODE_LIST->addItem("homography");
    u->ECC_WARP_MODE_LIST->installEventFilter(this);

    u->ECC_FUSION_MODE_LIST->addItem("mean");
    u->ECC_FUSION_MODE_LIST->addItem("median");
    u->ECC_FUSION_MODE_LIST->addItem("med+trim_mean");
    u->ECC_FUSION_MODE_LIST->installEventFilter(this);

    data_exchange(false);
}

void SetupOverlay::data_exchange(bool read)
{
    if (!m_config || !m_ui) return;
    auto *u = m_ui;
    auto& ip = m_config->get_data().image_proc;
    auto& tcu = m_config->get_data().tcu;
    auto& sv = m_config->get_data().save;
    auto& dev = m_config->get_data().device;

    if (read) {
        dev.flip            = u->FLIP_OPTION_LIST->currentIndex();
        dev.rotation        = u->ROTATE_OPTION_LIST->currentIndex();
        dev.ptz_type        = u->PTZ_TYPE_LIST->currentIndex();
        dev.ebus            = u->EBUS_CHK->isChecked();
        dev.share_tcu_port  = u->SHARE_CHK->isChecked();
        dev.underwater      = u->UNDERWATER_CHK->isChecked();

        sv.save_info           = u->SAVE_INFO_CHK->isChecked();
        sv.custom_topleft_info = u->CUSTOM_INFO_CHK->isChecked();
        sv.save_in_grayscale   = u->GRAYSCALE_CHK->isChecked();
        sv.consecutive_capture = u->CONSECUTIVE_CAPTURE_CHK->isChecked();
        sv.integrate_info      = u->INTEGRATE_INFO_CHK->isChecked();
        sv.img_format          = u->IMG_FORMAT_LST->currentIndex();

        tcu.type    = u->TCU_LIST->currentIndex();
        tcu.hz_unit = u->HZ_LIST->currentIndex();

        ip.gamma           = u->GAMMA_EDIT->text().toFloat();
        ip.accu_base       = u->ACCU_BASE_EDIT->text().toFloat();
        ip.low_in          = u->LOW_IN_EDIT->text().toFloat();
        ip.high_in         = u->HIGH_IN_EDIT->text().toFloat();
        ip.low_out         = u->LOW_OUT_EDIT->text().toFloat();
        ip.high_out        = u->HIGH_OUT_EDIT->text().toFloat();
        ip.dehaze_pct      = u->DEHAZE_PCT_EDIT->text().toFloat() / 100;
        ip.sky_tolerance   = u->SKY_TOLERANCE_EDIT->text().toFloat();
        ip.fast_gf         = u->FAST_GF_EDIT->text().toInt();
        ip.colormap        = u->COLORMAP_3D_LIST->currentIndex();
        ip.lower_3d_thresh = u->LOWER_3D_THRESH_EDT->text().toFloat();
        ip.upper_3d_thresh = u->UPPER_3D_THRESH_EDT->text().toFloat();
        ip.custom_3d_param = u->CUSTOM_3D_PARAM_CHK->isChecked();
        ip.custom_3d_delay      = u->CUSTOM_3D_DELAY_EDT->text().toFloat();
        ip.custom_3d_gate_width = u->CUSTOM_3D_GW_EDT->text().toFloat();
#ifdef LVTONG
        ip.fishnet_recog  = u->FISHNET_RECOG_CHK->isChecked();
        ip.fishnet_thresh = u->FISHNET_THRESH_EDIT->text().toFloat();
#endif

        ip.ecc_window_mode  = u->ECC_WINDOW_MODE_LIST->currentIndex();
        ip.ecc_warp_mode    = u->ECC_WARP_MODE_LIST->currentIndex();
        ip.ecc_fusion_method = u->ECC_FUSION_MODE_LIST->currentIndex();
        ip.ecc_backward     = u->ECC_BACKWARD_EDIT->text().toInt();
        ip.ecc_forward      = u->ECC_FORWARD_EDIT->text().toInt();
        if (ip.ecc_window_mode == 0) ip.ecc_forward = 0;
        else if (ip.ecc_window_mode == 1) ip.ecc_forward = ip.ecc_backward;
        ip.ecc_levels       = u->ECC_LEVELS_EDIT->text().toInt();
        ip.ecc_max_iter     = u->ECC_MAXITER_EDIT->text().toInt();
        ip.ecc_eps          = u->ECC_EPS_EDIT->text().toDouble();
        ip.ecc_half_res_reg  = u->ECC_HALF_REG_CHK->isChecked();
        ip.ecc_half_res_fuse = u->ECC_HALF_FUSE_CHK->isChecked();

        tcu.auto_rep_freq = u->AUTO_REP_FREQ_CHK->isChecked();
        tcu.ab_lock       = u->AB_LOCK_CHK->isChecked();
        tcu.base_unit     = u->UNIT_LIST->currentIndex();
        switch (tcu.base_unit) {
        case 0:
            tcu.max_dist           = u->MAX_DIST_EDT->text().toFloat() * m_dist_ns;
            tcu.delay_offset       = u->DELAY_OFFSET_EDT->text().toFloat();
            tcu.max_dov            = u->MAX_DOV_EDT->text().toFloat() * m_dist_ns;
            tcu.gate_width_offset  = u->GATE_WIDTH_OFFSET_EDT->text().toFloat();
            tcu.max_laser_width    = u->MAX_LASER_EDT->text().toFloat();
            tcu.laser_width_offset = u->LASER_OFFSET_EDT->text().toFloat();
            break;
        case 1:
            tcu.max_dist           = u->MAX_DIST_EDT->text().toFloat() * m_dist_ns * 1000;
            tcu.delay_offset       = u->DELAY_OFFSET_EDT->text().toFloat() * 1000;
            tcu.max_dov            = u->MAX_DOV_EDT->text().toFloat() * m_dist_ns * 1000;
            tcu.gate_width_offset  = u->GATE_WIDTH_OFFSET_EDT->text().toFloat() * 1000;
            tcu.max_laser_width    = u->MAX_LASER_EDT->text().toFloat();
            tcu.laser_width_offset = u->LASER_OFFSET_EDT->text().toFloat();
            break;
        case 2:
            tcu.max_dist           = u->MAX_DIST_EDT->text().toFloat();
            tcu.delay_offset       = u->DELAY_OFFSET_EDT->text().toFloat() / m_dist_ns;
            tcu.max_dov            = u->MAX_DOV_EDT->text().toFloat();
            tcu.gate_width_offset  = u->GATE_WIDTH_OFFSET_EDT->text().toFloat() / m_dist_ns;
            tcu.max_laser_width    = u->MAX_LASER_EDT->text().toFloat();
            tcu.laser_width_offset = u->LASER_OFFSET_EDT->text().toFloat();
            break;
        default: break;
        }
        tcu.ps_step[u->TCU_PS_CONFIG_LIST->currentIndex()] = u->PS_STEPPING_EDT->text().toInt();
        tcu.laser_on = 0;
        tcu.laser_on |= u->LASER_CHK_1->isChecked() << 0;
        tcu.laser_on |= u->LASER_CHK_2->isChecked() << 1;
        tcu.laser_on |= u->LASER_CHK_3->isChecked() << 2;
        tcu.laser_on |= u->LASER_CHK_4->isChecked() << 3;
    }
    else {
        u->FLIP_OPTION_LIST->setCurrentIndex(dev.flip);
        u->ROTATE_OPTION_LIST->setCurrentIndex(dev.rotation);
        u->PTZ_TYPE_LIST->setCurrentIndex(dev.ptz_type);
        u->EBUS_CHK->setChecked(dev.ebus);
        u->SHARE_CHK->setChecked(dev.share_tcu_port);
        u->UNDERWATER_CHK->setChecked(dev.underwater);

        u->SAVE_INFO_CHK->setChecked(sv.save_info);
        u->CUSTOM_INFO_CHK->setChecked(sv.custom_topleft_info);
        u->GRAYSCALE_CHK->setChecked(sv.save_in_grayscale);
        u->CONSECUTIVE_CAPTURE_CHK->setChecked(sv.consecutive_capture);
        u->INTEGRATE_INFO_CHK->setChecked(sv.integrate_info);
        u->IMG_FORMAT_LST->setCurrentIndex(sv.img_format);

        u->TCU_LIST->setCurrentIndex(tcu.type);
        u->HZ_LIST->setCurrentIndex(tcu.hz_unit);

        u->GAMMA_EDIT->setText(QString::number(ip.gamma, 'f', 2));
        u->ACCU_BASE_EDIT->setText(QString::number(ip.accu_base, 'f', 2));
        u->LOW_IN_EDIT->setText(QString::number(ip.low_in, 'f', 2));
        u->HIGH_IN_EDIT->setText(QString::number(ip.high_in, 'f', 2));
        u->LOW_OUT_EDIT->setText(QString::number(ip.low_out, 'f', 2));
        u->HIGH_OUT_EDIT->setText(QString::number(ip.high_out, 'f', 2));
        u->DEHAZE_PCT_EDIT->setText(QString::number(ip.dehaze_pct * 100, 'f', 2));
        u->SKY_TOLERANCE_EDIT->setText(QString::number(ip.sky_tolerance, 'f', 2));
        u->FAST_GF_EDIT->setText(QString::number(ip.fast_gf));
        u->COLORMAP_3D_LIST->setCurrentIndex(ip.colormap);
        u->LOWER_3D_THRESH_EDT->setText(QString::number(ip.lower_3d_thresh, 'f', 3));
        u->UPPER_3D_THRESH_EDT->setText(QString::number(ip.upper_3d_thresh, 'f', 3));
        u->CUSTOM_3D_PARAM_CHK->setChecked(ip.custom_3d_param);
        u->CUSTOM_3D_DELAY_EDT->setText(QString::number((int)std::round(ip.custom_3d_delay)));
        u->CUSTOM_3D_GW_EDT->setText(QString::number((int)std::round(ip.custom_3d_gate_width)));
#ifdef LVTONG
        u->FISHNET_RECOG_CHK->setChecked(ip.fishnet_recog);
        u->FISHNET_THRESH_EDIT->setText(QString::number(ip.fishnet_thresh, 'f', 2));
#endif

        u->ECC_WINDOW_MODE_LIST->setCurrentIndex(ip.ecc_window_mode);
        u->ECC_WARP_MODE_LIST->setCurrentIndex(ip.ecc_warp_mode);
        u->ECC_FUSION_MODE_LIST->setCurrentIndex(ip.ecc_fusion_method);
        u->ECC_BACKWARD_EDIT->setText(QString::number(ip.ecc_backward));
        u->ECC_FORWARD_EDIT->setText(QString::number(ip.ecc_forward));
        u->ECC_LEVELS_EDIT->setText(QString::number(ip.ecc_levels));
        u->ECC_MAXITER_EDIT->setText(QString::number(ip.ecc_max_iter));
        u->ECC_EPS_EDIT->setText(QString::number(ip.ecc_eps, 'f', 4));
        u->ECC_HALF_REG_CHK->setChecked(ip.ecc_half_res_reg);
        u->ECC_HALF_FUSE_CHK->setChecked(ip.ecc_half_res_fuse);

        u->AUTO_REP_FREQ_CHK->setChecked(tcu.auto_rep_freq);
        u->AB_LOCK_CHK->setChecked(tcu.ab_lock);
        u->UNIT_LIST->setCurrentIndex(tcu.base_unit);
        update_distance_display();
        u->PS_STEPPING_EDT->setText(QString::number(tcu.ps_step[u->TCU_PS_CONFIG_LIST->currentIndex()]));
        u->MAX_PS_STEP_EDT->setText(QString::number(int(std::round(4000. / tcu.ps_step[u->TCU_PS_CONFIG_LIST->currentIndex()]))));
        u->LASER_CHK_1->setChecked(tcu.laser_on & 0b0001);
        u->LASER_CHK_2->setChecked(tcu.laser_on & 0b0010);
        u->LASER_CHK_3->setChecked(tcu.laser_on & 0b0100);
        u->LASER_CHK_4->setChecked(tcu.laser_on & 0b1000);
    }
}

void SetupOverlay::config_ip(bool read, int ip, int /*gateway*/, int nic_address)
{
    if (!m_ui) return;
    if (read) {
        m_ui->IP_EDIT->setText(QString::asprintf("%d.%d.%d.%d", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF));
        m_ui->LOCAL_IP_EDIT->setText(QString::asprintf("%d.%d.%d.%d", (nic_address >> 24) & 0xFF, (nic_address >> 16) & 0xFF, (nic_address >> 8) & 0xFF, nic_address & 0xFF));
    }
    else {
        int ip1, ip2, ip3, ip4;
        sscanf(m_ui->IP_EDIT->text().toLatin1().constData(), "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);
        emit set_dev_ip((ip1 << 24) + (ip2 << 16) + (ip3 << 8) + ip4, (ip1 << 24) + (ip2 << 16) + (ip3 << 8) + 1);
    }
}

void SetupOverlay::enable_ip_editing(bool enable)
{
    if (m_ui) m_ui->IP_EDIT->setEnabled(enable);
}

void SetupOverlay::set_pixel_format(int idx)
{
    if (m_ui) m_ui->PIXEL_FORMAT_LIST->setCurrentIndex(idx);
}

void SetupOverlay::update_distance_display()
{
    if (!m_config || !m_ui) return;
    auto *u = m_ui;
    const auto& tcu = m_config->get_data().tcu;

    switch (tcu.base_unit) {
    case 0:
        u->MAX_DIST_EDT->setText(QString::number(std::round(tcu.max_dist / m_dist_ns), 'f', 0));
        u->MAX_DIST_UNIT->setText("ns");
        u->DELAY_OFFSET_EDT->setText(QString::number(std::round(tcu.delay_offset), 'f', 0));
        u->DELAY_OFFSET_UNIT->setText("ns");
        u->MAX_DOV_EDT->setText(QString::number(std::round(tcu.max_dov / m_dist_ns), 'f', 0));
        u->MAX_DOV_UNIT->setText("ns");
        u->GATE_WIDTH_OFFSET_EDT->setText(QString::number(std::round(tcu.gate_width_offset), 'f', 0));
        u->GATE_WIDTH_OFFSET_UNIT->setText("ns");
        break;
    case 1:
        u->MAX_DIST_EDT->setText(QString::number(tcu.max_dist / m_dist_ns / 1000, 'f', 3));
        u->MAX_DIST_UNIT->setText("\xce\xbcs");
        u->DELAY_OFFSET_EDT->setText(QString::number(tcu.delay_offset / 1000, 'f', 3));
        u->DELAY_OFFSET_UNIT->setText("\xce\xbcs");
        u->MAX_DOV_EDT->setText(QString::number(tcu.max_dov / m_dist_ns / 1000, 'f', 3));
        u->MAX_DOV_UNIT->setText("\xce\xbcs");
        u->GATE_WIDTH_OFFSET_EDT->setText(QString::number(tcu.gate_width_offset / 1000, 'f', 3));
        u->GATE_WIDTH_OFFSET_UNIT->setText("\xce\xbcs");
        break;
    case 2:
        u->MAX_DIST_EDT->setText(QString::number(tcu.max_dist, 'f', 2));
        u->MAX_DIST_UNIT->setText("m");
        u->DELAY_OFFSET_EDT->setText(QString::number(tcu.delay_offset * m_dist_ns, 'f', 2));
        u->DELAY_OFFSET_UNIT->setText("m");
        u->MAX_DOV_EDT->setText(QString::number(tcu.max_dov, 'f', 2));
        u->MAX_DOV_UNIT->setText("m");
        u->GATE_WIDTH_OFFSET_EDT->setText(QString::number(tcu.gate_width_offset * m_dist_ns, 'f', 2));
        u->GATE_WIDTH_OFFSET_UNIT->setText("m");
        break;
    default: break;
    }
    u->MAX_LASER_EDT->setText(QString::number(tcu.max_laser_width, 'f', 0));
}

void SetupOverlay::display_baudrate(int id, int baudrate)
{
    if (!m_ui) return;
    if (id != m_ui->COM_LIST->currentIndex()) return;
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
    m_ui->BAUDRATE_LIST->setCurrentIndex(idx);
}

void SetupOverlay::switch_language(bool /*en*/, QTranslator * /*trans*/)
{
    if (m_ui && m_form_root) m_ui->retranslateUi(m_form_root);
}

void SetupOverlay::set_ptz_type_enabled(bool enabled)
{
    if (m_ui) m_ui->PTZ_TYPE_LIST->setEnabled(enabled);
}

bool SetupOverlay::eventFilter(QObject *obj, QEvent *event)
{
    if (qobject_cast<QComboBox*>(obj) && event->type() == QEvent::MouseMove) return true;
    return QFrame::eventFilter(obj, event);
}

void SetupOverlay::update_device_list(int cmd, QStringList dev_list)
{
    if (!m_ui) return;
    switch (cmd) {
    case 0: m_ui->DEVICE_LIST->setEnabled(!m_ui->DEVICE_LIST->isEnabled()); break;
    case 1: m_ui->DEVICE_LIST->clear(); m_ui->DEVICE_LIST->addItems(dev_list); break;
    default: break;
    }
}

void SetupOverlay::send_cmd(QString str)
{
    if (!m_ui) return;
    QString send_str = str.simplified();
    send_str.replace(" ", "");
    bool ok;
    QByteArray cmd(send_str.length() / 2, 0x00);
    for (int i = 0; i < send_str.length() / 2; i++) cmd[i] = send_str.midRef(i * 2, 2).toInt(&ok, 16);
    emit com_write(m_ui->COM_LIST->currentIndex(), cmd);
}

void SetupOverlay::toggle_laser(int id, bool /*on*/)
{
    if (!m_config) return;
    auto& laser_on = m_config->get_data().tcu.laser_on;
    laser_on ^= 1 << id;
    emit laser_toggled(laser_on);
}
