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

constexpr int COLUMN_NAME = 0;
constexpr int COLUMN_HEIGHT = 1;
constexpr int COLUMN_TYPE = 2;
constexpr int COLUMN_CATEGORY = 3;
constexpr int COLUMN_COUNT = 4;

// 709 code tables. Values are the wire codes; text is for the operator.
const QVector<QPair<int, QString>> &target_types()
{
    static const QVector<QPair<int, QString>> types = {
        {0, QStringLiteral("0 unknown")}, {1, QStringLiteral("1 air")},
        {2, QStringLiteral("2 low air")}, {3, QStringLiteral("3 surface")},
        {4, QStringLiteral("4 underwater")}, {5, QStringLiteral("5 land")},
        {6, QStringLiteral("6 space")}
    };
    return types;
}

const QVector<QPair<int, QString>> &target_categories()
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

QComboBox *make_code_combo(const QVector<QPair<int, QString>> &codes, int value)
{
    auto *combo = new QComboBox();
    for (const auto &code : codes) combo->addItem(code.second, code.first);
    const int index = combo->findData(value);
    combo->setCurrentIndex(index >= 0 ? index : 0);
    return combo;
}

} // namespace

TargetClassWindow::TargetClassWindow(Config *config, QWidget *parent) :
    QDialog(parent),
    m_config(config)
{
    setObjectName(QStringLiteral("TARGET_CLASS_WINDOW"));
    setModal(false);
    setAttribute(Qt::WA_QuitOnClose, false);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    resize(620, 520);
    setMinimumSize(520, 380);

    setup_ui();
    retranslate_ui();

    m_current_model = TargetClassTable::VISIBLE_LIGHT;
    reload_from_disk();
}

void TargetClassWindow::setup_ui()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(8);

    auto *header = new QHBoxLayout();
    header->setSpacing(6);
    m_model_label = new QLabel(this);
    m_model_combo = new QComboBox(this);
    for (const QString &model : TargetClassTable::models()) m_model_combo->addItem(model, model);
    m_zoom_label = new QLabel(this);
    m_zoom_spin = new QDoubleSpinBox(this);
    // Lens range from panorama_optics.zoom_fov: 10 mm at 1x to 860 mm at 86x.
    m_zoom_spin->setRange(1.0, 86.0);
    m_zoom_spin->setDecimals(2);
    m_zoom_spin->setSingleStep(1.0);
    m_zoom_spin->setValue(1.0);
    m_show_range_check = new QCheckBox(this);
    m_source_label = new QLabel(this);
    m_source_label->setWordWrap(true);

    header->addWidget(m_model_label);
    header->addWidget(m_model_combo);
    header->addSpacing(12);
    header->addWidget(m_zoom_label);
    header->addWidget(m_zoom_spin);
    header->addSpacing(12);
    header->addWidget(m_show_range_check);
    header->addStretch(1);
    root->addLayout(header);
    root->addWidget(m_source_label);

    m_table_widget = new QTableWidget(this);
    m_table_widget->setColumnCount(COLUMN_COUNT);
    m_table_widget->horizontalHeader()->setSectionResizeMode(COLUMN_NAME, QHeaderView::Stretch);
    m_table_widget->horizontalHeader()->setSectionResizeMode(COLUMN_HEIGHT, QHeaderView::ResizeToContents);
    m_table_widget->horizontalHeader()->setSectionResizeMode(COLUMN_TYPE, QHeaderView::ResizeToContents);
    m_table_widget->horizontalHeader()->setSectionResizeMode(COLUMN_CATEGORY, QHeaderView::ResizeToContents);
    m_table_widget->verticalHeader()->setVisible(false);
    m_table_widget->setSelectionMode(QAbstractItemView::NoSelection);
    root->addWidget(m_table_widget, 1);

    auto *buttons = new QHBoxLayout();
    m_status_label = new QLabel(this);
    m_reload_button = new QPushButton(this);
    m_save_button = new QPushButton(this);
    buttons->addWidget(m_status_label, 1);
    buttons->addWidget(m_reload_button);
    buttons->addWidget(m_save_button);
    root->addLayout(buttons);

    connect(m_model_combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TargetClassWindow::model_changed);
    connect(m_reload_button, &QPushButton::clicked, this, &TargetClassWindow::reload_from_disk);
    connect(m_save_button, &QPushButton::clicked, this, &TargetClassWindow::save_to_disk);
    connect(m_zoom_spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &TargetClassWindow::current_zoom_changed);
    connect(m_show_range_check, &QCheckBox::toggled,
            this, &TargetClassWindow::show_range_on_labels_changed);
}

void TargetClassWindow::retranslate_ui()
{
    setWindowTitle(tr("Target Classes"));
    if (m_model_label) m_model_label->setText(tr("Model"));
    if (m_zoom_label) m_zoom_label->setText(tr("Current zoom"));
    if (m_show_range_check) m_show_range_check->setText(tr("Range on labels"));
    if (m_reload_button) m_reload_button->setText(tr("Reload"));
    if (m_save_button) m_save_button->setText(tr("Save"));
    if (m_table_widget) m_table_widget->setHorizontalHeaderLabels(
                {tr("Class"), tr("Height (m)"), tr("709 type"), tr("709 category")});
}

void TargetClassWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) retranslate_ui();
    QDialog::changeEvent(event);
}

void TargetClassWindow::toggle_visible()
{
    if (isVisible()) {
        hide();
        return;
    }
    show();
    raise();
    activateWindow();
}

double TargetClassWindow::get_current_zoom() const
{
    return m_zoom_spin ? m_zoom_spin->value() : 1.0;
}

bool TargetClassWindow::get_show_range_on_labels() const
{
    return m_show_range_check && m_show_range_check->isChecked();
}

QString TargetClassWindow::table_file_path() const
{
    return QCoreApplication::applicationDirPath() + QStringLiteral("/target_classes.conf");
}

QStringList TargetClassWindow::class_names_for_model(const QString &model) const
{
    if (!m_config) return QStringList();
    const Config::YoloSettings &yolo = m_config->get_data().yolo;
    QString relative;
    if (model == TargetClassTable::VISIBLE_LIGHT) relative = yolo.visible_classes_file;
    else if (model == TargetClassTable::THERMAL) relative = yolo.thermal_classes_file;
    else if (model == TargetClassTable::GATED) relative = yolo.gated_classes_file;
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

void TargetClassWindow::reload_from_disk()
{
    QString error;
    // A missing file is the normal first-run state, not a failure: the class
    // names still come from the model and every attribute starts at zero.
    if (!m_table.load(table_file_path(), &error) && QFile::exists(table_file_path())) {
        set_status(error, true);
    }
    else {
        set_status(tr("Loaded %1").arg(QDir::toNativeSeparators(table_file_path())));
    }

    for (const QString &model : TargetClassTable::models())
        m_table.merge_class_names(model, class_names_for_model(model));

    populate_table();
    emit table_changed();
}

void TargetClassWindow::save_to_disk()
{
    commit_table_to_model();
    QString error;
    if (!m_table.save(table_file_path(), &error)) {
        set_status(error, true);
        return;
    }
    set_status(tr("Saved %1").arg(QDir::toNativeSeparators(table_file_path())));
    emit table_changed();
}

void TargetClassWindow::model_changed()
{
    commit_table_to_model();
    m_current_model = m_model_combo->currentData().toString();
    populate_table();
}

void TargetClassWindow::populate_table()
{
    if (!m_table_widget) return;

    const QStringList model_names = class_names_for_model(m_current_model);
    const QVector<TargetClassEntry> entries = m_table.get_entries(m_current_model);

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
        m_table_widget->setItem(row, COLUMN_NAME, name_item);

        auto *height = new QDoubleSpinBox();
        height->setRange(0.0, 1000.0);
        height->setDecimals(2);
        height->setSingleStep(0.5);
        height->setValue(entry.height_meters);
        m_table_widget->setCellWidget(row, COLUMN_HEIGHT, height);

        m_table_widget->setCellWidget(row, COLUMN_TYPE, make_code_combo(target_types(), entry.target_type));
        m_table_widget->setCellWidget(row, COLUMN_CATEGORY,
                                      make_code_combo(target_categories(), entry.target_category));
    }

    if (m_source_label) {
        m_source_label->setText(model_names.isEmpty()
            ? tr("No classes file found for %1 — check the yolo paths in the config.")
              .arg(m_current_model)
            : tr("%1 classes from the model file.").arg(model_names.size()));
    }
}

void TargetClassWindow::commit_table_to_model()
{
    if (!m_table_widget) return;

    QVector<TargetClassEntry> entries;
    entries.reserve(m_table_widget->rowCount());
    for (int row = 0; row < m_table_widget->rowCount(); ++row) {
        QTableWidgetItem *name_item = m_table_widget->item(row, COLUMN_NAME);
        if (!name_item) continue;

        TargetClassEntry entry;
        entry.name = name_item->text();
        if (auto *height = qobject_cast<QDoubleSpinBox*>(m_table_widget->cellWidget(row, COLUMN_HEIGHT)))
            entry.height_meters = height->value();
        if (auto *type = qobject_cast<QComboBox*>(m_table_widget->cellWidget(row, COLUMN_TYPE)))
            entry.target_type = type->currentData().toInt();
        if (auto *category = qobject_cast<QComboBox*>(m_table_widget->cellWidget(row, COLUMN_CATEGORY)))
            entry.target_category = category->currentData().toInt();
        entries.push_back(entry);
    }
    m_table.set_entries(m_current_model, entries);
}

void TargetClassWindow::set_status(const QString &message, bool error)
{
    if (!m_status_label) return;
    m_status_label->setText(message);
    m_status_label->setProperty("portState", error ? "error" : "idle");
    m_status_label->style()->unpolish(m_status_label);
    m_status_label->style()->polish(m_status_label);
}
