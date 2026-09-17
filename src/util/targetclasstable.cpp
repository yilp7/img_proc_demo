#include "util/targetclasstable.h"

#include <QFile>
#include <QSaveFile>
#include <QTextStream>

const QString TargetClassTable::VisibleLight = QStringLiteral("VL");
const QString TargetClassTable::Thermal = QStringLiteral("THERMAL");
const QString TargetClassTable::Gated = QStringLiteral("GATED");

QStringList TargetClassTable::models()
{
    return {VisibleLight, Thermal, Gated};
}

QString TargetClassTable::modelForIndex(int model_idx)
{
    switch (model_idx) {
    case 1: return VisibleLight;
    case 2: return Thermal;
    case 3: return Gated;
    default: return QString();
    }
}

bool TargetClassTable::load(const QString &filepath, QString *error)
{
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error) *error = QStringLiteral("Cannot open %1: %2").arg(filepath, file.errorString());
        return false;
    }

    // Class names may be non-ASCII, and the whole file is written by us, so
    // UTF-8 is forced rather than left to the system codec.
    QTextStream in(&file);
    in.setCodec("UTF-8");

    QHash<QString, QVector<TargetClassEntry>> parsed;
    QString section;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')) || line.startsWith(QLatin1Char(';')))
            continue;
        if (line.startsWith(QLatin1Char('[')) && line.endsWith(QLatin1Char(']'))) {
            section = line.mid(1, line.size() - 2).trimmed().toUpper();
            parsed.insert(section, parsed.value(section));
            continue;
        }
        if (section.isEmpty()) continue;

        const int split = line.indexOf(QLatin1Char('='));
        if (split < 0) continue;

        TargetClassEntry entry;
        // Class names can contain spaces ("bulk cargo carrier") but not '='.
        entry.name = line.left(split).trimmed();
        if (entry.name.isEmpty()) continue;

        const QStringList values = line.mid(split + 1).split(QLatin1Char(','));
        if (!values.isEmpty()) entry.heightMeters = values.at(0).trimmed().toDouble();
        if (values.size() > 1) entry.targetType = values.at(1).trimmed().toInt();
        if (values.size() > 2) entry.targetCategory = values.at(2).trimmed().toInt();
        parsed[section].push_back(entry);
    }

    m_entries = parsed;
    return true;
}

bool TargetClassTable::save(const QString &filepath, QString *error) const
{
    // QSaveFile so a crash mid-write cannot truncate an existing table.
    QSaveFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error) *error = QStringLiteral("Cannot write %1: %2").arg(filepath, file.errorString());
        return false;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << "# Target class attributes: height (m), 709 target type, 709 target category.\n";
    out << "# Class names come from the model classes files; edit via the target class page.\n";
    out << "# <class name> = <height_m>, <target_type>, <target_category>\n";

    for (const QString &model : models()) {
        out << "\n[" << model << "]\n";
        const QVector<TargetClassEntry> list = m_entries.value(model);
        for (const TargetClassEntry &entry : list) {
            out << entry.name << " = " << QString::number(entry.heightMeters, 'f', 2)
                << ", " << entry.targetType << ", " << entry.targetCategory << "\n";
        }
    }

    if (!file.commit()) {
        if (error) *error = QStringLiteral("Cannot commit %1: %2").arg(filepath, file.errorString());
        return false;
    }
    return true;
}

QVector<TargetClassEntry> TargetClassTable::entries(const QString &model) const
{
    return m_entries.value(model.toUpper());
}

void TargetClassTable::setEntries(const QString &model, const QVector<TargetClassEntry> &entries)
{
    m_entries.insert(model.toUpper(), entries);
}

bool TargetClassTable::lookup(const QString &model, const QString &className,
                              TargetClassEntry *out) const
{
    if (!out) return false;
    const QVector<TargetClassEntry> list = m_entries.value(model.toUpper());
    for (const TargetClassEntry &entry : list) {
        if (entry.name.compare(className, Qt::CaseInsensitive) == 0) {
            *out = entry;
            return true;
        }
    }
    return false;
}

void TargetClassTable::mergeClassNames(const QString &model, const QStringList &names)
{
    const QString key = model.toUpper();
    QVector<TargetClassEntry> merged;
    QVector<TargetClassEntry> existing = m_entries.value(key);

    for (const QString &name : names) {
        TargetClassEntry entry;
        entry.name = name;
        for (const TargetClassEntry &prev : existing) {
            if (prev.name.compare(name, Qt::CaseInsensitive) == 0) {
                entry = prev;
                entry.name = name;
                break;
            }
        }
        merged.push_back(entry);
    }

    // Configured classes the current model file no longer lists are appended
    // rather than dropped, so switching models back keeps their edits.
    for (const TargetClassEntry &prev : existing) {
        bool still_present = false;
        for (const QString &name : names) {
            if (prev.name.compare(name, Qt::CaseInsensitive) == 0) { still_present = true; break; }
        }
        if (!still_present) merged.push_back(prev);
    }

    m_entries.insert(key, merged);
}
