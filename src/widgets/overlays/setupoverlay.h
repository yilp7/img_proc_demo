#ifndef SETUPOVERLAY_H
#define SETUPOVERLAY_H

#include <QFrame>

class QHBoxLayout;
class QScrollArea;
class QVBoxLayout;
class QWidget;

// Full-window SETUP-mode overlay. 3 horizontal rows of config cards inside a
// QScrollArea, plus a fixed preview sidebar on the right. Cards are filled by
// re-parenting existing widgets from the Preferences dialog and ui->LEFT —
// see sprint-18-progress.md for the row breakdown.
//
// The overlay is hidden by default. UserPanel toggles it on TopBar mode
// changes (Setup → show, Monitor/Scan → hide).
class SetupOverlay : public QFrame
{
    Q_OBJECT
public:
    explicit SetupOverlay(QWidget *parent = nullptr);

    // Access the row containers. Tasks 18.2 / 18.3 / 18.4 add cards into
    // these via QHBoxLayout::addWidget on the row's layout.
    QWidget *contentRow(int idx) const;
    QHBoxLayout *rowLayout(int idx) const;

    // Sidebar layout where preview thumbnails go (task 18.5).
    QVBoxLayout *previewSidebarLayout() const { return m_preview_layout; }

private:
    QScrollArea *m_scroll;
    QWidget     *m_rows_container;
    QWidget     *m_row[3];
    QHBoxLayout *m_row_layout[3];
    QWidget     *m_preview_sidebar;
    QVBoxLayout *m_preview_layout;
};

#endif // SETUPOVERLAY_H
