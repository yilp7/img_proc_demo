#include "visionwidget.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

VisionWidget::VisionWidget(QWidget *parent)
    : QFrame(parent),
      m_main_row(nullptr), m_alt1_row(nullptr), m_alt2_row(nullptr),
      m_main_chk(nullptr), m_alt1_chk(nullptr), m_alt2_chk(nullptr)
{
    setObjectName("VISION_WIDGET");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(6, 6, 6, 6);
    root->setSpacing(4);

    m_main_row = makeRow("MAIN", &m_main_chk, this);
    m_alt1_row = makeRow("ALT1", &m_alt1_chk, this);
    m_alt2_row = makeRow("ALT2", &m_alt2_chk, this);
    root->addWidget(m_main_row);
    root->addWidget(m_alt1_row);
    root->addWidget(m_alt2_row);
    root->addStretch(1);

    // ALT rows hidden by default — surfaced when their channel is active.
    m_alt1_row->hide();
    m_alt2_row->hide();

    connect(m_main_chk, &QCheckBox::toggled, this, &VisionWidget::mainEnabledChanged);
    connect(m_alt1_chk, &QCheckBox::toggled, this, &VisionWidget::alt1EnabledChanged);
    connect(m_alt2_chk, &QCheckBox::toggled, this, &VisionWidget::alt2EnabledChanged);
}

QFrame *VisionWidget::makeRow(const QString &label, QCheckBox **out_chk, QFrame *parent)
{
    auto *row = new QFrame(parent);
    row->setObjectName(QString("VISION_ROW_%1").arg(label));
    auto *l = new QHBoxLayout(row);
    l->setContentsMargins(2, 2, 2, 2);
    l->setSpacing(6);

    auto *chk = new QCheckBox(label, row);
    chk->setObjectName(QString("VISION_%1_CHK").arg(label));
    *out_chk = chk;

    l->addWidget(chk);
    l->addStretch(1);
    return row;
}

void VisionWidget::setAlt1Visible(bool visible) { if (m_alt1_row) m_alt1_row->setVisible(visible); }
void VisionWidget::setAlt2Visible(bool visible) { if (m_alt2_row) m_alt2_row->setVisible(visible); }
