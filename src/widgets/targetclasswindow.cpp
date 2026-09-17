#include "widgets/targetclasswindow.h"

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QFile>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QStyle>
#include <QTableWidget>
#include <QTextStream>
#include <QVBoxLayout>

#include "util/config.h"

namespace {

constexpr int ColumnName = 0;
constexpr int ColumnHeight = 1;
constexpr int ColumnType = 2;
constexpr int ColumnCategory = 3;
constexpr int ColumnCount = 4;

// 709 code tables. Values are the wire codes; text is for the operator.
const QVector<QPair<int, QString>> &targetTypes()
{
    static const QVector<QPair<int, QString>> types = {
        {0, QStringLiteral("0 unknown")}, {1, QStringLiteral("1 air")},
        {2, QStringLiteral("2 low air")}, {3, QStringLiteral("3 surface")},
        {4, QStringLiteral("4 underwater")}, {5, QStringLiteral("5 land")},
        {6, QStringLiteral("6 space")}
    };
    return types;
}

const QVector<QPair<int, QString>> &targetCategories()
{
    static const QVector<QPair<int, QString>> categories = {
        {0, QStringLiteral("0 unknown")}, {1, QStringLiteral("1 MJT")},
        {2, QStringLiteral("2 RJT")}, {3, QStringLiteral("3 TJT")},
        {4, QStringLiteral("4 WJT")}, {5, QStringLiteral("5 WJF")},
        {6, QStringLiteral("6 small boat")}, {7, QStringLiteral("7 fishing boat")},
        {8, QStringLiteral("8 cruise ship")}, {9, QStringLiteral("9 cargo ship")},
        {10, QStringLiteral("10 buoy")}, {11, QStringLiteral("11 small UAV")},
        {12, QStringLiteral("12 frogman")}, {13, QStringLiteral("13 X boat")},
        {14, QStringLiteral("14 UUV")}
    };
    return categories;
}

QComboBox *makeCodeCombo(const QVector<QPair<int, QString>> &codes, int value)
{
    auto *combo = new QComboBox();
    for (const auto &code : codes) combo->addItem(code.second, code.first);
    const int index = combo->findData(value);
    combo->setCurrentIndex(index >= 0 ? index : 0);
    return combo;
}

} // namespace

TargetClassWindow::TargetClassWindow(Config *config, QWidget *parent)
    : QDialog(parent),
      m_config(config)
{
    setObjectName(QStringLiteral("TARGET_CLASS_WINDOW"));
    setModal(false);
    setAttribute(Qt::WA_QuitOnClose, false);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    resize(620, 520);
    setMinimumSize(520, 380);

    setupUi();
    retranslateUi();

    m_current_model = TargetClassTable::VisibleLight;
    reloadFromDisk();
}

void TargetClassWindow::setupUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(8);

    auto *header = new QHBoxLayout();
    header->setSpacing(6);
    m_modelLabel = new QLabel(this);
    m_modelCombo = new QComboBox(this);
    for (const QString &model : TargetClassTable::models()) m_modelCombo->addItem(model, model);
    m_zoomLabel = new QLabel(this);
    m_zoomSpin = new QDoubleSpinBox(this);
    // Lens range from panorama_optics.zoom_fov: 10 mm at 1x to 860 mm at 86x.
    m_zoomSpin->setRange(1.0, 86.0);
    m_zoomSpin->setDecimals(2);
    m_zoomSpin->setSingleStep(1.0);
    m_zoomSpin->setValue(1.0);
    m_showRangeCheck = new QCheckBox(this);
    m_sourceLabel = new QLabel(this);
    m_sourceLabel->setWordWrap(true);

    header->addWidget(m_modelLabel);
    header->addWidget(m_modelCombo);
    header->addSpacing(12);
    header->addWidget(m_zoomLabel);
    header->addWidget(m_zoomSpin);
    header->addSpacing(12);
    header->addWidget(m_showRangeCheck);
    header->addStretch(1);
    root->addLayout(header);
    root->addWidget(m_sourceLabel);

    m_table_widget = new QTableWidget(this);
    m_table_widget->setColumnCount(ColumnCount);
    m_table_widget->horizontalHeader()->setSectionResizeMode(ColumnName, QHeaderView::Stretch);
    m_table_widget->horizontalHeader()->setSectionResizeMode(ColumnHeight, QHeaderView::ResizeToContents);
    m_table_widget->horizontalHeader()->setSectionResizeMode(ColumnType, QHeaderView::ResizeToContents);
    m_table_widget->horizontalHeader()->setSectionResizeMode(ColumnCategory, QHeaderView::ResizeToContents);
    m_table_widget->verticalHeader()->setVisible(false);
    m_table_widget->setSelectionMode(QAbstractItemView::NoSelection);
    root->addWidget(m_table_widget, 1);

    auto *buttons = new QHBoxLayout();
    m_statusLabel = new QLabel(this);
    m_reloadButton = new QPushButton(this);
    m_saveButton = new QPushButton(this);
    buttons->addWidget(m_statusLabel, 1);
    buttons->addWidget(m_reloadButton);
    buttons->addWidget(m_saveButton);
    root->addLayout(buttons);

    connect(m_modelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TargetClassWindow::modelChanged);
    connect(m_reloadButton, &QPushButton::clicked, this, &TargetClassWindow::reloadFromDisk);
    connect(m_saveButton, &QPushButton::clicked, this, &TargetClassWindow::saveToDisk);
    connect(m_zoomSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &TargetClassWindow::currentZoomChanged);
    connect(m_showRangeCheck, &QCheckBox::toggled,
            this, &TargetClassWindow::showRangeOnLabelsChanged);
}

void TargetClassWindow::retranslateUi()
{
    setWindowTitle(tr("Target Classes"));
    if (m_modelLabel) m_modelLabel->setText(tr("Model"));
    if (m_zoomLabel) m_zoomLabel->setText(tr("Current zoom"));
    if (m_showRangeCheck) m_showRangeCheck->setText(tr("Range on labels"));
    if (m_reloadButton) m_reloadButton->setText(tr("Reload"));
    if (m_saveButton) m_saveButton->setText(tr("Save"));
    if (m_table_widget) m_table_widget->setHorizontalHeaderLabels(
                {tr("Class"), tr("Height (m)"), tr("709 type"), tr("709 category")});
}

void TargetClassWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) retranslateUi();
    QDialog::changeEvent(event);
}

void TargetClassWindow::toggleVisible()
{
    if (isVisible()) {
        hide();
        return;
    }
    show();
    raise();
    activateWindow();
}

double TargetClassWindow::currentZoom() const
{
    return m_zoomSpin ? m_zoomSpin->value() : 1.0;
}

bool TargetClassWindow::showRangeOnLabels() const
{
    return m_showRangeCheck && m_showRangeCheck->isChecked();
}

QString TargetClassWindow::tableFilePath() const
{
    return QCoreApplication::applicationDirPath() + QStringLiteral("/target_classes.conf");
}

QStringList TargetClassWindow::classNamesForModel(const QString &model) const
{
    if (!m_config) return QStringList();
    const Config::YoloSettings &yolo = m_config->get_data().yolo;
    QString relative;
    if (model == TargetClassTable::VisibleLight) relative = yolo.visible_classes_file;
    else if (model == TargetClassTable::Thermal) relative = yolo.thermal_classes_file;
    else if (model == TargetClassTable::Gated) relative = yolo.gated_classes_file;
    if (relative.isEmpty()) return QStringList();

    // QFile rather than std streams: the deployed path may be non-ASCII.
    QFile file(QCoreApplication::applicationDirPath() + QDir::separator() + relative);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return QStringList();

    QStringList names;
    QTextStream in(&file);
    in.setCodec("UTF-8");
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (!line.isEmpty()) names.push_back(line);
    }
    return names;
}

void TargetClassWindow::reloadFromDisk()
{
    QString error;
    // A missing file is the normal first-run state, not a failure: the class
    // names still come from the model and every attribute starts at zero.
    if (!m_table.load(tableFilePath(), &error) && QFile::exists(tableFilePath())) {
        setStatus(error, true);
    } else {
        setStatus(tr("Loaded %1").arg(QDir::toNativeSeparators(tableFilePath())));
    }

    for (const QString &model : TargetClassTable::models())
        m_table.mergeClassNames(model, classNamesForModel(model));

    populateTable();
    emit tableChanged();
}

void TargetClassWindow::saveToDisk()
{
    commitTableToModel();
    QString error;
    if (!m_table.save(tableFilePath(), &error)) {
        setStatus(error, true);
        return;
    }
    setStatus(tr("Saved %1").arg(QDir::toNativeSeparators(tableFilePath())));
    emit tableChanged();
}

void TargetClassWindow::modelChanged()
{
    commitTableToModel();
    m_current_model = m_modelCombo->currentData().toString();
    populateTable();
}

void TargetClassWindow::populateTable()
{
    if (!m_table_widget) return;

    const QStringList model_names = classNamesForModel(m_current_model);
    const QVector<TargetClassEntry> entries = m_table.entries(m_current_model);

    m_table_widget->clearContents();
    m_table_widget->setRowCount(entries.size());
    for (int row = 0; row < entries.size(); ++row) {
        const TargetClassEntry &entry = entries.at(row);

        auto *name_item = new QTableWidgetItem(entry.name);
        name_item->setFlags(name_item->flags() & ~Qt::ItemIsEditable);
        // Rows the current model file no longer lists are kept but marked, so
        // stale edits are visible rather than silently in force.
        if (!model_names.contains(entry.name, Qt::CaseInsensitive)) {
            name_item->setToolTip(tr("Not in the current model classes file"));
            QFont font = name_item->font();
            font.setItalic(true);
            name_item->setFont(font);
        }
        m_table_widget->setItem(row, ColumnName, name_item);

        auto *height = new QDoubleSpinBox();
        height->setRange(0.0, 1000.0);
        height->setDecimals(2);
        height->setSingleStep(0.5);
        height->setValue(entry.heightMeters);
        m_table_widget->setCellWidget(row, ColumnHeight, height);

        m_table_widget->setCellWidget(row, ColumnType, makeCodeCombo(targetTypes(), entry.targetType));
        m_table_widget->setCellWidget(row, ColumnCategory,
                                      makeCodeCombo(targetCategories(), entry.targetCategory));
    }

    if (m_sourceLabel) {
        m_sourceLabel->setText(model_names.isEmpty()
            ? tr("No classes file found for %1 — check the yolo paths in the config.")
              .arg(m_current_model)
            : tr("%1 classes from the model file.").arg(model_names.size()));
    }
}

void TargetClassWindow::commitTableToModel()
{
    if (!m_table_widget) return;

    QVector<TargetClassEntry> entries;
    entries.reserve(m_table_widget->rowCount());
    for (int row = 0; row < m_table_widget->rowCount(); ++row) {
        QTableWidgetItem *name_item = m_table_widget->item(row, ColumnName);
        if (!name_item) continue;

        TargetClassEntry entry;
        entry.name = name_item->text();
        if (auto *height = qobject_cast<QDoubleSpinBox*>(m_table_widget->cellWidget(row, ColumnHeight)))
            entry.heightMeters = height->value();
        if (auto *type = qobject_cast<QComboBox*>(m_table_widget->cellWidget(row, ColumnType)))
            entry.targetType = type->currentData().toInt();
        if (auto *category = qobject_cast<QComboBox*>(m_table_widget->cellWidget(row, ColumnCategory)))
            entry.targetCategory = category->currentData().toInt();
        entries.push_back(entry);
    }
    m_table.setEntries(m_current_model, entries);
}

void TargetClassWindow::setStatus(const QString &message, bool error)
{
    if (!m_statusLabel) return;
    m_statusLabel->setText(message);
    m_statusLabel->setProperty("portState", error ? "error" : "idle");
    m_statusLabel->style()->unpolish(m_statusLabel);
    m_statusLabel->style()->polish(m_statusLabel);
}
