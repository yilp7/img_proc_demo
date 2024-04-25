#ifndef PRESETPANEL_H
#define PRESETPANEL_H

#include "util/util.h"

#if DEPRECATED_PRESET_STRUCT
struct PresetData{
    // name
    char name[64];
    // TCU data
    double rep_freq, laser_width, delay_a, gw_a, delay_b, gw_b, ccd_freq, duty;
    uint mcp;
    double delay_n, gatewidth_n; // unit: ns
    // lens data
    uint zoom, focus, laser_radius, lens_speed;
    // PTZ data
    double angle_h, angle_v;
    uint ptz_speed;

    friend QDebug& operator << (QDebug& qd, const PresetData& pd) {
        qd << "\"PRESET DATA for " << QString::fromUtf8(pd.name) << ": '";
        qd << "PRF: " << pd.rep_freq << ", laser width: " << pd.laser_width;
        qd << ", delay A: " << pd.delay_a << ", gatewidth A: " << pd.gw_a;
        qd << ", delay B: " << pd.delay_b << ", gatewidth B: " << pd.gw_b;
        qd << ", fps: " << pd.ccd_freq << ", te: " << pd.duty;
        qd << ", mcp: " << pd.mcp << ", delay N: " << pd.delay_n << ", gatewidth N:" << pd.gatewidth_n;
        qd << ", zoom: " << pd.zoom << ", focus: " << pd.focus << ", laser radius: " << pd.laser_radius << ", lens speed:" << pd.lens_speed;
        qd << ", angle H: " << pd.angle_h << ", angle V: " << pd.angle_v << ", ptz speed: " << pd.ptz_speed;
        qd << "'\"";
        return qd;
    }
};
#endif

class PresetNameItem : public QStandardItem
{
public :
    PresetNameItem(const QString text): QStandardItem(text) { }

    void setData(const QVariant &value, int role = Qt::UserRole + 1) override
    {
        if (role == Qt::EditRole) {
            QString text = value.toString();
            // if (text.length() > 16) text.truncate(16);
            if (text.toUtf8().length() > 64) text = QString::fromUtf8(text.toUtf8().left(64));
            QStandardItem::setData(text, role);
        }
        else QStandardItem::setData(value, role);
    }
};

class PresetTableView : public QTableView
{
    Q_OBJECT
public:
    explicit PresetTableView(QWidget *parent = nullptr) : QTableView(parent) { this->setMinimumWidth(400); }

protected:
    void resizeEvent(QResizeEvent *event)
    {
        this->setColumnWidth(0, this->width() - 120);
        this->setColumnWidth(1, 60);
        this->setColumnWidth(2, 60);

        QTableView::resizeEvent(event);
    }
};

class PresetPanel : public QDialog
{
    Q_OBJECT

public:
    explicit PresetPanel(QWidget *parent = nullptr);
    ~PresetPanel();

    void show_ui();

signals:
    void preset_updated(int idx);
    void preset_selected(nlohmann::json preset_data);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void save_preset(int idx, nlohmann::json preset_data);
    void preset_name_updated(QStandardItem *item);

public:
    int total;
    int current_selection;

private:
    PresetTableView    *preset_table;
    QStandardItemModel *preset_model;
    QButtonGroup       *apply_btn_grp;
    QButtonGroup       *save_btn_grp;

    nlohmann::json presets;
};

#endif // PRESETPANEL_H
