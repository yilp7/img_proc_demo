#include "presetpanel.h"

PresetPanel::PresetPanel(QWidget *parent):
    QDialog(parent),
    m_total(0)
{
    qRegisterMetaType<nlohmann::json>("nlohmann::json");

    m_preset_table = new PresetTableView();
    m_preset_model = new QStandardItemModel();
    m_preset_model->setHorizontalHeaderLabels({"Name", "SAVE", "APPLY"});

    m_save_btn_grp = new QButtonGroup(this);
    for (int i = 0; i < 256; i++) {
        QPushButton *btn = new QPushButton("SAVE");
        m_save_btn_grp->addButton(btn, i);
    }
    connect(m_save_btn_grp, static_cast<void (QButtonGroup::*)(int id)>(&QButtonGroup::buttonClicked), this,
            [this](int idx){
                QString preset_name = m_preset_model->item(idx, 0)->text();
                if (idx < m_total) {
                    emit preset_updated(idx);
                }
                else {
                    emit preset_updated(m_total++);

                    PresetNameItem *name_item = new PresetNameItem("");
                    name_item->setEditable(true);
                    name_item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
                    m_preset_model->setItem(m_total, 0, name_item);

                    m_preset_table->setIndexWidget(m_preset_model->index(m_total, 1), m_save_btn_grp->button(m_total));
                    m_preset_table->setIndexWidget(m_preset_model->index(m_total, 2), m_apply_btn_grp->button(m_total));
                }
            });

    m_apply_btn_grp = new QButtonGroup(this);
    for (int i = 0; i < 256; i++) {
        QPushButton *btn = new QPushButton("APPLY");
        m_apply_btn_grp->addButton(btn, i);
    }
    connect(m_apply_btn_grp, static_cast<void (QButtonGroup::*)(int id)>(&QButtonGroup::buttonClicked), this,
            [this](int id){
                if (m_presets[id].contains("data")) emit preset_selected(m_presets[m_current_selection = id]["data"]);
//                std::cout << presets[id] << std::endl;
//                std::cout.flush();
            });

    m_preset_table->setWindowTitle("PRESET");
    m_preset_table->resize(400, 300);
    // m_preset_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_preset_table->horizontalHeader()->setHighlightSections(false);
    m_preset_table->verticalHeader()->setHighlightSections(false);
    m_preset_table->setAlternatingRowColors(true);
    m_preset_table->setModel(m_preset_model);

#ifdef DEPRECATED_PRESET_STRUCT
    FILE *f = fopen("user_presets", "rb+");
    if (f) {
        fseek(f, 0, SEEK_END);
        int size = ftell(f);
        if (size < 28) {
            int preset_heading = 'PSET';
            fseek(f, 23, SEEK_SET);
            fwrite(&preset_heading, 4, 1, f);
            uchar zero = 0;
            fwrite(&zero, 1, 1, f);
        }
        else {
            uchar total_in_file_uchar = 0;
            fseek(f, 27, SEEK_SET);
            fread(&total_in_file_uchar, 1, 1, f);
            struct PresetData preset_in_file;
            for (uchar i = 0; i < total_in_file_uchar; i++) {
                memset(&preset_in_file, 0, sizeof(struct PresetData));
                fread(&preset_in_file, sizeof(struct PresetData), 1, f);
                // m_presets.resize(m_presets.size() + 1);
                m_presets.push_back(preset_in_file);
            }
            m_total = total_in_file_uchar;
        }
        fclose(f);
    }
#endif
    std::ifstream f("user_presets.json");
    m_presets = nlohmann::json::array();
    if (f.is_open() && f.peek() != std::ifstream::traits_type::eof()) {
        try {
            nlohmann::json presets_in_file = nlohmann::json::parse(f);
            if (presets_in_file.contains("num")) m_total = presets_in_file["num"];
            if (presets_in_file.contains("presets")) m_presets = presets_in_file["presets"];
        }
        catch (nlohmann::json::parse_error &ex)
        {
            std::cerr << "Failed to parse user_presets.json: " << ex.what() << " at byte " << ex.byte << std::endl;
            std::cerr.flush();
        }
    }

//    m_presets.resize(m_presets.size() + 1);
//    m_presets.push_back(PresetData{0});
    m_presets.push_back(nlohmann::json());

    for (int i = 0 ; i < m_total + 1; i++) {
//        PresetNameItem *name_item = new PresetNameItem(QString::fromUtf8(m_presets[i]["name"].get<std::string>().c_str()));
        QString name;
        if (m_presets[i].contains("name") && m_presets[i]["name"].is_string()) {
            try {
                name = QString::fromUtf8(m_presets[i]["name"].get<std::string>().c_str());
            }
            catch (const nlohmann::json::exception &e) {
                qWarning("JSON error parsing preset name: %s", e.what());
                name = QString("Preset %1").arg(i);
            }
        }
        PresetNameItem *name_item = new PresetNameItem(name);
        name_item->setEditable(true);
        name_item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
        m_preset_model->setItem(i, 0, name_item);

        m_preset_table->setIndexWidget(m_preset_model->index(i, 1), m_save_btn_grp->button(i));
        m_preset_table->setIndexWidget(m_preset_model->index(i, 2), m_apply_btn_grp->button(i));
    }

    connect(m_preset_model, &QStandardItemModel::itemChanged, this, &PresetPanel::preset_name_updated);
}

PresetPanel::~PresetPanel()
{
    delete m_preset_model;
    delete m_preset_table;
}

void PresetPanel::show_ui()
{
    m_preset_table->show();
    m_preset_table->raise();
}

void PresetPanel::closeEvent(QCloseEvent *event)
{
    m_preset_table->close();
}

#ifdef DEPRECATED_PRESET_STRUCT
void PresetPanel::save_preset(int idx, struct PresetData data)
{
    if (idx < 0) {
        m_total++;
        idx = m_total - 1;

        PresetNameItem *name_item = new PresetNameItem("");
        name_item->setEditable(true);
        name_item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
        m_preset_model->setItem(m_total, 0, name_item);

        m_preset_table->setIndexWidget(m_preset_model->index(m_total, 1), m_save_btn_grp->button(m_total));
        m_preset_table->setIndexWidget(m_preset_model->index(m_total, 2), m_apply_btn_grp->button(m_total));
    }
    QString name = m_preset_model->item(idx, 0)->text();
    if (name.isEmpty()) name = QDateTime::currentDateTime().toString("MM-dd hh:mm:ss"), m_preset_model->item(idx, 0)->setText(name);
    memcpy(data.name, name.toUtf8().constData(), name.toUtf8().length());
    m_presets[idx] = data;
    // qDebug() << idx << m_presets;
    FILE *f = fopen("user_presets", "rb+");
    if (f) {
        fseek(f, 27, SEEK_SET);
        fwrite(&m_total, 1, 1, f);
        fseek(f, 28 + idx * (sizeof(struct PresetData)), SEEK_SET);
        fwrite(&data, sizeof(struct PresetData), 1, f);
        fclose(f);
    }
    if (idx == m_total - 1) {
        m_presets.resize(m_presets.size() + 1);
        m_presets.push_back(PresetData{0});
    }
}
#endif

void PresetPanel::save_preset(int idx, nlohmann::json preset_data)
{
    if (idx < 0) {
        m_total++;
        idx = m_total - 1;

        PresetNameItem *name_item = new PresetNameItem("");
        name_item->setEditable(true);
        name_item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
        m_preset_model->setItem(m_total, 0, name_item);

        m_preset_table->setIndexWidget(m_preset_model->index(m_total, 1), m_save_btn_grp->button(m_total));
        m_preset_table->setIndexWidget(m_preset_model->index(m_total, 2), m_apply_btn_grp->button(m_total));
    }
    QString name = m_preset_model->item(idx, 0)->text();
    if (name.isEmpty()) name = QDateTime::currentDateTime().toString("MM-dd hh:mm:ss"), m_preset_model->item(idx, 0)->setText(name);
    m_presets[idx] = nlohmann::json{{"name", std::string(name.toUtf8())}, {"data", preset_data}};
//    std::cout << preset_data << std::endl;
//    std::cout.flush();

    std::ofstream o("user_presets.json");
    o << nlohmann::json{{"num", m_total}, {"presets", m_presets}} << std::endl;
}

#ifdef DEPRECATED_PRESET_STRUCT
void PresetPanel::preset_name_updated(QStandardItem *item)
{
    int idx = m_preset_model->indexFromItem(item).row();
    PresetData temp = m_presets[idx];
    memset(temp.name, 0, sizeof(temp.name));
    memcpy(temp.name, item->text().toUtf8(), item->text().toUtf8().length());

    m_presets[idx] = temp;
    if (idx < m_total) {
        FILE *f = fopen("user_presets", "rb+");
        if (f) {
            fseek(f, 28 + idx * (sizeof(struct PresetData)), SEEK_SET);
            fwrite(&temp, sizeof(struct PresetData), 1, f);
            fclose(f);
        }
    }
}
#endif

void PresetPanel::preset_name_updated(QStandardItem *item)
{
    int idx = m_preset_model->indexFromItem(item).row();
    if (idx >= m_total) return;
    nlohmann::json &temp = m_presets[idx];
    temp["name"] = std::string(item->text().toUtf8());

    std::ofstream o("user_presets.json");
    o << nlohmann::json{{"num", m_total}, {"presets", m_presets}} << std::endl;
}
