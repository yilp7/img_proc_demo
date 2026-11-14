#ifndef TARGETCLASSTABLE_H
#define TARGETCLASSTABLE_H

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>

// Per-class target attributes for range estimation and G6 reporting. The class
// lists belong to the models, so this table only attaches attributes to their
// names. Entries of a class a model no longer lists are kept, so switching back loses nothing.
struct TargetClassEntry {
    QString name;
    // Experience height of the target in metres; drives range estimation.
    // Non-positive means "unknown" and suppresses range for that class.
    double height_meters = 0.0;
    // 709 code tables: 目标类型 0-6, 目标种类 0-14.
    int target_type = 0;
    int target_category = 0;
};

class TargetClassTable
{
public:
    // Section names, matching the model selector rather than the file paths.
    static const QString VISIBLE_LIGHT;
    static const QString THERMAL;
    static const QString GATED;
    static QStringList models();
    // model_idx as used by PipelineConfig: 1 VL, 2 thermal, 3 gated.
    static QString model_for_index(int model_idx);

    bool load(const QString &filepath, QString *error = nullptr);
    bool save(const QString &filepath, QString *error = nullptr) const;

    QVector<TargetClassEntry> get_entries(const QString &model) const;
    void set_entries(const QString &model, const QVector<TargetClassEntry> &entries);

    // Null entry when the class is absent, so callers can tell "unconfigured"
    // from "configured as zero".
    bool lookup(const QString &model, const QString &class_name, TargetClassEntry *out) const;

    // Merges class names from a model's classes file, keeping existing
    // attributes and appending unconfigured names with zeroed defaults.
    void merge_class_names(const QString &model, const QStringList &names);

private:
    QHash<QString, QVector<TargetClassEntry>> m_entries;
};

#endif // TARGETCLASSTABLE_H
