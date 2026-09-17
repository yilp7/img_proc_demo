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

    void toggleVisible();

    const TargetClassTable &table() const { return m_table; }
    // Operator-entered zoom, used to interpolate the lens FOV. The device
    // reports no zoom position, so this is the only source.
    double currentZoom() const;
    // Whether the estimated range is appended to YOLO labels on the video.
    bool showRangeOnLabels() const;

signals:
    void tableChanged();
    void currentZoomChanged(double zoom);
    void showRangeOnLabelsChanged(bool enabled);

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void reloadFromDisk();
    void saveToDisk();
    void modelChanged();

private:
    void setupUi();
    void retranslateUi();
    void populateTable();
    void commitTableToModel();
    QString tableFilePath() const;
    QStringList classNamesForModel(const QString &model) const;
    void setStatus(const QString &message, bool error = false);

    Config *m_config = nullptr;
    TargetClassTable m_table;
    QString m_current_model;

    QLabel *m_modelLabel = nullptr;
    QComboBox *m_modelCombo = nullptr;
    QLabel *m_zoomLabel = nullptr;
    QDoubleSpinBox *m_zoomSpin = nullptr;
    QCheckBox *m_showRangeCheck = nullptr;
    QLabel *m_sourceLabel = nullptr;
    QTableWidget *m_table_widget = nullptr;
    QPushButton *m_reloadButton = nullptr;
    QPushButton *m_saveButton = nullptr;
    QLabel *m_statusLabel = nullptr;
};

#endif // TARGETCLASSWINDOW_H
