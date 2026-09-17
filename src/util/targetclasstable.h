#ifndef TARGETCLASSTABLE_H
#define TARGETCLASSTABLE_H

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>

// Per-class target attributes used for range estimation and G6 reporting.
//
// Class lists belong to the models (models/VL.txt etc.), so this table only
// attaches attributes to names the model already defines. Entries whose class
// has disappeared from a model file are kept rather than dropped, so that
// swapping a model back does not lose its edits.
struct TargetClassEntry {
    QString name;
    // Experience height of the target in metres; drives range estimation.
    // Non-positive means "unknown" and suppresses range for that class.
    double heightMeters = 0.0;
    // 709 code tables: 目标类型 0-6, 目标种类 0-14.
    int targetType = 0;
    int targetCategory = 0;
};

class TargetClassTable
{
public:
    // Section names, matching the model selector rather than the file paths.
    static const QString VisibleLight;
    static const QString Thermal;
    static const QString Gated;
    static QStringList models();
    // model_idx as used by PipelineConfig: 1 VL, 2 thermal, 3 gated.
    static QString modelForIndex(int model_idx);

    bool load(const QString &filepath, QString *error = nullptr);
    bool save(const QString &filepath, QString *error = nullptr) const;

    QVector<TargetClassEntry> entries(const QString &model) const;
    void setEntries(const QString &model, const QVector<TargetClassEntry> &entries);

    // Null entry when the class is absent, so callers can tell "unconfigured"
    // from "configured as zero".
    bool lookup(const QString &model, const QString &className, TargetClassEntry *out) const;

    // Merges class names from a model's classes file, keeping existing
    // attributes and appending unconfigured names with zeroed defaults.
    void mergeClassNames(const QString &model, const QStringList &names);

private:
    QHash<QString, QVector<TargetClassEntry>> m_entries;
};

#endif // TARGETCLASSTABLE_H
