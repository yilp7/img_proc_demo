#ifndef TARGETCLASSWINDOW_H
#define TARGETCLASSWINDOW_H

#include <QDialog>

#include "util/targetclasstable.h"

class Config;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QTableWidget;

// Editor for the per-class target attributes used by range estimation and G6
// reporting. Class names are owned by the model files and shown read-only;
// only the attributes are editable here.
class TargetClassWindow : public QDialog
{
    Q_OBJECT

public:
    explicit TargetClassWindow(Config *config, QWidget *parent = nullptr);

    void toggle_visible();

    const TargetClassTable &get_table() const { return m_table; }
    // Operator-entered zoom, used to interpolate the lens FOV. The device
    // reports no zoom position, so this is the only source.
    double get_current_zoom() const;
    // Whether the estimated range is appended to YOLO labels on the video.
    bool get_show_range_on_labels() const;

signals:
    void table_changed();
    void current_zoom_changed(double zoom);
    void show_range_on_labels_changed(bool enabled);

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void reload_from_disk();
    void save_to_disk();
    void model_changed();

private:
    void setup_ui();
    void retranslate_ui();
    void populate_table();
    void commit_table_to_model();
    QString table_file_path() const;
    QStringList class_names_for_model(const QString &model) const;
    void set_status(const QString &message, bool error = false);

    Config *m_config = nullptr;
    TargetClassTable m_table;
    QString m_current_model;

    QLabel *m_model_label = nullptr;
    QComboBox *m_model_combo = nullptr;
    QLabel *m_zoom_label = nullptr;
    QDoubleSpinBox *m_zoom_spin = nullptr;
    QCheckBox *m_show_range_check = nullptr;
    QLabel *m_source_label = nullptr;
    QTableWidget *m_table_widget = nullptr;
    QPushButton *m_reload_button = nullptr;
    QPushButton *m_save_button = nullptr;
    QLabel *m_status_label = nullptr;
};

#endif // TARGETCLASSWINDOW_H
