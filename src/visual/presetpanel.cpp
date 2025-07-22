#include "presetpanel.h"

PresetPanel::PresetPanel(QWidget *parent):
    QDialog(parent),
    total(0)
{
    qRegisterMetaType<nlohmann::json>("nlohmann::json");

    preset_table = new PresetTableView();
    preset_model = new QStandardItemModel();
    preset_model->setHorizontalHeaderLabels({"Name", "SAVE", "APPLY"});

    save_btn_grp = new QButtonGroup(this);
    for (int i = 0; i < 256; i++) {
        QPushButton *btn = new QPushButton("SAVE");
        save_btn_grp->addButton(btn, i);
    }
    connect(save_btn_grp, static_cast<void (QButtonGroup::*)(int id)>(&QButtonGroup::buttonClicked), this,
            [this](int idx){
                QString preset_name = preset_model->item(idx, 0)->text();
                if (idx < total) {
                    emit preset_updated(idx);
                }
                else {
                    emit preset_updated(total++);

                    PresetNameItem *name_item = new PresetNameItem("");
                    name_item->setEditable(true);
                    name_item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
                    preset_model->setItem(total, 0, name_item);

                    preset_table->setIndexWidget(preset_model->index(total, 1), save_btn_grp->button(total));
                    preset_table->setIndexWidget(preset_model->index(total, 2), apply_btn_grp->button(total));
                }
            });

    apply_btn_grp = new QButtonGroup(this);
    for (int i = 0; i < 256; i++) {
        QPushButton *btn = new QPushButton("APPLY");
        apply_btn_grp->addButton(btn, i);
    }
    connect(apply_btn_grp, static_cast<void (QButtonGroup::*)(int id)>(&QButtonGroup::buttonClicked), this,
            [this](int id){
                if (presets[id].contains("data")) emit preset_selected(presets[current_selection = id]["data"]);
//                std::cout << presets[id] << std::endl;
//                std::cout.flush();
            });

    preset_table->setWindowTitle("PRESET");
    preset_table->resize(400, 300);
    // preset_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    preset_table->horizontalHeader()->setHighlightSections(false);
    preset_table->verticalHeader()->setHighlightSections(false);
    preset_table->setAlternatingRowColors(true);
    preset_table->setModel(preset_model);

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
                // presets.resize(presets.size() + 1);
                presets.push_back(preset_in_file);
            }
            total = total_in_file_uchar;
        }
        fclose(f);
    }
#endif
    std::ifstream f("user_presets.json");
    presets = nlohmann::json::array();
    try
    {
        nlohmann::json presets_in_file = nlohmann::json::parse(f);
        if (presets_in_file.contains("num")) total = presets_in_file["num"];
        if (presets_in_file.contains("presets")) presets = presets_in_file["presets"];
    }
    catch (nlohmann::json::parse_error& ex)
    {
        std::cerr << ex.what() << " at byte " << ex.byte << std::endl;
        std::cerr.flush();
    }

//    presets.resize(presets.size() + 1);
//    presets.push_back(PresetData{0});
    presets.push_back(nlohmann::json());

    for (int i = 0 ; i < total + 1; i++) {
//        PresetNameItem *name_item = new PresetNameItem(QString::fromUtf8(presets[i]["name"].get<std::string>().c_str()));
        QString name;
        if (presets[i].contains("name") && presets[i]["name"].is_string()) {
            try {
                name = QString::fromUtf8(presets[i]["name"].get<std::string>().c_str());
            }
            catch (const nlohmann::json::exception& e) {
                qWarning("JSON error parsing preset name: %s", e.what());
                name = QString("Preset %1").arg(i);
            }
        }
        PresetNameItem *name_item = new PresetNameItem(name);
        name_item->setEditable(true);
        name_item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
        preset_model->setItem(i, 0, name_item);

        preset_table->setIndexWidget(preset_model->index(i, 1), save_btn_grp->button(i));
        preset_table->setIndexWidget(preset_model->index(i, 2), apply_btn_grp->button(i));
    }

    connect(preset_model, &QStandardItemModel::itemChanged, this, &PresetPanel::preset_name_updated);
}

PresetPanel::~PresetPanel()
{
    delete preset_model;
    delete preset_table;
}

void PresetPanel::show_ui()
{
    preset_table->show();
    preset_table->raise();
}

void PresetPanel::closeEvent(QCloseEvent *event)
{
    preset_table->close();
}

#ifdef DEPRECATED_PRESET_STRUCT
void PresetPanel::save_preset(int idx, struct PresetData data)
{
    if (idx < 0) {
        total++;
        idx = total - 1;

        PresetNameItem *name_item = new PresetNameItem("");
        name_item->setEditable(true);
        name_item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
        preset_model->setItem(total, 0, name_item);

        preset_table->setIndexWidget(preset_model->index(total, 1), save_btn_grp->button(total));
        preset_table->setIndexWidget(preset_model->index(total, 2), apply_btn_grp->button(total));
    }
    QString name = preset_model->item(idx, 0)->text();
    if (name.isEmpty()) name = QDateTime::currentDateTime().toString("MM-dd hh:mm:ss"), preset_model->item(idx, 0)->setText(name);
    memcpy(data.name, name.toUtf8().constData(), name.toUtf8().length());
    presets[idx] = data;
    // qDebug() << idx << presets;
    FILE *f = fopen("user_presets", "rb+");
    if (f) {
        fseek(f, 27, SEEK_SET);
        fwrite(&total, 1, 1, f);
        fseek(f, 28 + idx * (sizeof(struct PresetData)), SEEK_SET);
        fwrite(&data, sizeof(struct PresetData), 1, f);
        fclose(f);
    }
    if (idx == total - 1) {
        presets.resize(presets.size() + 1);
        presets.push_back(PresetData{0});
    }
}
#endif

void PresetPanel::save_preset(int idx, nlohmann::json preset_data)
{
    if (idx < 0) {
        total++;
        idx = total - 1;

        PresetNameItem *name_item = new PresetNameItem("");
        name_item->setEditable(true);
        name_item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
        preset_model->setItem(total, 0, name_item);

        preset_table->setIndexWidget(preset_model->index(total, 1), save_btn_grp->button(total));
        preset_table->setIndexWidget(preset_model->index(total, 2), apply_btn_grp->button(total));
    }
    QString name = preset_model->item(idx, 0)->text();
    if (name.isEmpty()) name = QDateTime::currentDateTime().toString("MM-dd hh:mm:ss"), preset_model->item(idx, 0)->setText(name);
    presets[idx] = nlohmann::json{{"name", std::string(name.toUtf8())}, {"data", preset_data}};
//    std::cout << preset_data << std::endl;
//    std::cout.flush();

    std::ofstream o("user_presets.json");
    o << nlohmann::json{{"num", total}, {"presets", presets}} << std::endl;
}

#ifdef DEPRECATED_PRESET_STRUCT
void PresetPanel::preset_name_updated(QStandardItem *item)
{
    int idx = preset_model->indexFromItem(item).row();
    PresetData temp = presets[idx];
    memset(temp.name, 0, sizeof(temp.name));
    memcpy(temp.name, item->text().toUtf8(), item->text().toUtf8().length());

    presets[idx] = temp;
    if (idx < total) {
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
    int idx = preset_model->indexFromItem(item).row();
    if (idx >= total) return;
    nlohmann::json &temp = presets[idx];
    temp["name"] = std::string(item->text().toUtf8());

    std::ofstream o("user_presets.json");
    o << nlohmann::json{{"num", total}, {"presets", presets}} << std::endl;
}
