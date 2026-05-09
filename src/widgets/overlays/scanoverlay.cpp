#include "scanoverlay.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

ScanOverlay::ScanOverlay(QWidget *parent)
    : QFrame(parent),
      m_scroll(nullptr),
      m_cards_container(nullptr),
      m_cards_layout(nullptr),
      m_preview_sidebar(nullptr),
      m_preview_layout(nullptr)
{
    setObjectName("SCAN_OVERLAY");

    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(8);

    // Left: horizontally scrollable cards row
    m_scroll = new QScrollArea(this);
    m_scroll->setObjectName("SCAN_SCROLL");
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_cards_container = new QWidget(m_scroll);
    m_cards_layout = new QHBoxLayout(m_cards_container);
    m_cards_layout->setContentsMargins(0, 0, 0, 0);
    m_cards_layout->setSpacing(8);
    // Callers insert cards then addStretch(1) — no trailing stretch here.
    m_scroll->setWidget(m_cards_container);

    // Right: fixed preview sidebar
    m_preview_sidebar = new QWidget(this);
    m_preview_sidebar->setObjectName("SCAN_PREVIEW_SIDEBAR");
    m_preview_sidebar->setAttribute(Qt::WA_StyledBackground, true);
    m_preview_sidebar->setFixedWidth(220);
    m_preview_layout = new QVBoxLayout(m_preview_sidebar);
    m_preview_layout->setContentsMargins(0, 0, 0, 0);
    m_preview_layout->setSpacing(6);
    auto *ph = new QLabel("Display Previews\n—\npopulated in 19.5", m_preview_sidebar);
    ph->setObjectName("SCAN_PREVIEW_PH");
    ph->setAlignment(Qt::AlignCenter);
    m_preview_layout->addWidget(ph);
    m_preview_layout->addStretch(1);

    root->addWidget(m_scroll, 1);
    root->addWidget(m_preview_sidebar);
}
