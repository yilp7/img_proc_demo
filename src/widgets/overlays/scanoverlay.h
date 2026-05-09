#ifndef SCANOVERLAY_H
#define SCANOVERLAY_H

#include <QFrame>

class QHBoxLayout;
class QScrollArea;
class QVBoxLayout;
class QWidget;

// Full-window SCAN-mode overlay. A single horizontal row of config group cards
// inside a QScrollArea (left) plus a fixed preview sidebar (right, ~220 px).
//
// Cards are inserted at run time via cardsLayout().  UserPanel builds 4 titled
// group cards (Delay / Gate Width / PTZ / Capture+Record) plus a vertical
// action column from scan_config->ui->* widgets — see sprint-19-progress.md.
//
// Hidden by default; UserPanel toggles on TopBar::modeChanged(Scan).
class ScanOverlay : public QFrame
{
    Q_OBJECT
public:
    explicit ScanOverlay(QWidget *parent = nullptr);

    // The QWidget whose layout holds the cards row (pass as parent_w to addWidget).
    QWidget     *contentWidget()  const { return m_cards_container; }

    // QHBoxLayout of the cards row — callers addWidget() cards here, then
    // addStretch(1) once all cards are inserted.
    QHBoxLayout *cardsLayout()    const { return m_cards_layout; }

    // QVBoxLayout of the right preview sidebar (task 19.5).
    QVBoxLayout *previewLayout()  const { return m_preview_layout; }

private:
    QScrollArea *m_scroll;
    QWidget     *m_cards_container;
    QHBoxLayout *m_cards_layout;
    QWidget     *m_preview_sidebar;
    QVBoxLayout *m_preview_layout;
};

#endif // SCANOVERLAY_H
