#include "setupoverlay.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

SetupOverlay::SetupOverlay(QWidget *parent)
    : QFrame(parent),
      m_scroll(nullptr),
      m_rows_container(nullptr),
      m_preview_sidebar(nullptr),
      m_preview_layout(nullptr)
{
    setObjectName("SETUP_OVERLAY");
    for (int i = 0; i < 3; ++i) {
        m_row[i]        = nullptr;
        m_row_layout[i] = nullptr;
    }

    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(8);

    // Scrollable rows container (left, stretches to fill)
    m_scroll = new QScrollArea(this);
    m_scroll->setObjectName("SETUP_SCROLL");
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);

    m_rows_container = new QWidget(m_scroll);
    auto *rows_layout = new QVBoxLayout(m_rows_container);
    rows_layout->setContentsMargins(0, 0, 0, 0);
    rows_layout->setSpacing(8);

    static const char *row_names[3] = { "SETUP_ROW_1", "SETUP_ROW_2", "SETUP_ROW_3" };
    for (int i = 0; i < 3; ++i) {
        m_row[i] = new QWidget(m_rows_container);
        m_row[i]->setObjectName(row_names[i]);
        m_row_layout[i] = new QHBoxLayout(m_row[i]);
        m_row_layout[i]->setContentsMargins(0, 0, 0, 0);
        m_row_layout[i]->setSpacing(8);
        rows_layout->addWidget(m_row[i]);
    }
    rows_layout->addStretch(1);
    m_scroll->setWidget(m_rows_container);

    // Fixed preview sidebar (right)
    m_preview_sidebar = new QWidget(this);
    m_preview_sidebar->setObjectName("SETUP_PREVIEW_SIDEBAR");
    m_preview_sidebar->setAttribute(Qt::WA_StyledBackground, true);
    m_preview_sidebar->setFixedWidth(220);
    m_preview_layout = new QVBoxLayout(m_preview_sidebar);
    m_preview_layout->setContentsMargins(0, 0, 0, 0);
    m_preview_layout->setSpacing(6);
    auto *placeholder = new QLabel("Preview — populated in 18.5", m_preview_sidebar);
    placeholder->setObjectName("SETUP_PREVIEW_PLACEHOLDER");
    placeholder->setAlignment(Qt::AlignCenter);
    m_preview_layout->addWidget(placeholder);
    m_preview_layout->addStretch(1);

    root->addWidget(m_scroll, 1);
    root->addWidget(m_preview_sidebar);
}

QWidget *SetupOverlay::contentRow(int idx) const
{
    if (idx < 0 || idx >= 3) return nullptr;
    return m_row[idx];
}

QHBoxLayout *SetupOverlay::rowLayout(int idx) const
{
    if (idx < 0 || idx >= 3) return nullptr;
    return m_row_layout[idx];
}
