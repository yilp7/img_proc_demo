#include "visionwidget.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

VisionWidget::VisionWidget(QWidget *parent) :
    QFrame(parent),
    m_main_row(nullptr),
    m_alt1_row(nullptr),
    m_alt2_row(nullptr),
    m_main_chk(nullptr),
    m_alt1_chk(nullptr),
    m_alt2_chk(nullptr)
{
    setObjectName("VISION_WIDGET");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(6, 6, 6, 6);
    root->setSpacing(4);

    m_main_row = make_row("MAIN", tr("MAIN"), &m_main_chk, this);
    m_alt1_row = make_row("ALT1", tr("ALT1"), &m_alt1_chk, this);
    m_alt2_row = make_row("ALT2", tr("ALT2"), &m_alt2_chk, this);
    root->addWidget(m_main_row);
    root->addWidget(m_alt1_row);
    root->addWidget(m_alt2_row);
    root->addStretch(1);

    // the ALT rows are hidden until their channel is active
    m_alt1_row->hide();
    m_alt2_row->hide();

    connect(m_main_chk, &QCheckBox::toggled, this, &VisionWidget::main_enabled_changed);
    connect(m_alt1_chk, &QCheckBox::toggled, this, &VisionWidget::alt1_enabled_changed);
    connect(m_alt2_chk, &QCheckBox::toggled, this, &VisionWidget::alt2_enabled_changed);
}

QFrame *VisionWidget::make_row(const QString &key, const QString &label, QCheckBox **out_chk, QFrame *parent)
{
    auto *row = new QFrame(parent);
    row->setObjectName(QString("VISION_ROW_%1").arg(key));
    auto *l = new QHBoxLayout(row);
    l->setContentsMargins(2, 2, 2, 2);
    l->setSpacing(6);

    auto *chk = new QCheckBox(label, row);
    chk->setObjectName(QString("VISION_%1_CHK").arg(key));
    *out_chk = chk;

    l->addWidget(chk);
    l->addStretch(1);
    return row;
}

void VisionWidget::set_alt1_visible(bool visible) { if (m_alt1_row) m_alt1_row->setVisible(visible); }
void VisionWidget::set_alt2_visible(bool visible) { if (m_alt2_row) m_alt2_row->setVisible(visible); }
