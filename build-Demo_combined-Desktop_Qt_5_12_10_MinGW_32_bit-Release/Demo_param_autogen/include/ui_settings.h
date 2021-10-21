/********************************************************************************
** Form generated from reading UI file 'settings.ui'
**
** Created by: Qt User Interface Compiler version 5.12.10
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SETTINGS_H
#define UI_SETTINGS_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>

QT_BEGIN_NAMESPACE

class Ui_ProgSettings
{
public:
    QDialogButtonBox *USER_BTN_GRP;
    QGroupBox *SCAN_GRP;
    QLineEdit *START_POS_EDIT;
    QLabel *START_LBL;
    QLabel *END_LBL;
    QLineEdit *END_POS_EDIT;
    QLabel *FRAME_LBL;
    QLineEdit *FRAME_COUNT_EDIT;
    QLabel *STEP_LBL;
    QLineEdit *STEP_SIZE_EDIT;
    QLabel *UNIT_NS;
    QLabel *UNIT_NS_2;
    QLabel *UNIT_NS_3;
    QGroupBox *ENHANCE_GROUP;
    QLineEdit *KERNEL_EDIT;
    QLabel *KERNEL_SIZE;
    QLineEdit *LOW_IN_EDIT;
    QLineEdit *LOW_OUT_EDIT;
    QLineEdit *HIGH_IN_EDIT;
    QLineEdit *HIGH_OUT_EDIT;
    QLineEdit *GAMMA_EDIT;
    QLabel *LOW_IN;
    QLabel *HIGH_IN;
    QLabel *LOW_OUT;
    QLabel *HIGH_OUT;
    QLabel *GAMMA;
    QLineEdit *LOG_EDIT;
    QLabel *LOG;

    void setupUi(QDialog *ProgSettings)
    {
        if (ProgSettings->objectName().isEmpty())
            ProgSettings->setObjectName(QString::fromUtf8("ProgSettings"));
        ProgSettings->resize(389, 282);
        QFont font;
        font.setFamily(QString::fromUtf8("Consolas"));
        font.setPointSize(9);
        ProgSettings->setFont(font);
        USER_BTN_GRP = new QDialogButtonBox(ProgSettings);
        USER_BTN_GRP->setObjectName(QString::fromUtf8("USER_BTN_GRP"));
        USER_BTN_GRP->setGeometry(QRect(210, 240, 161, 32));
        USER_BTN_GRP->setOrientation(Qt::Horizontal);
        USER_BTN_GRP->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
        SCAN_GRP = new QGroupBox(ProgSettings);
        SCAN_GRP->setObjectName(QString::fromUtf8("SCAN_GRP"));
        SCAN_GRP->setGeometry(QRect(20, 20, 111, 181));
        START_POS_EDIT = new QLineEdit(SCAN_GRP);
        START_POS_EDIT->setObjectName(QString::fromUtf8("START_POS_EDIT"));
        START_POS_EDIT->setGeometry(QRect(10, 35, 61, 21));
        START_LBL = new QLabel(SCAN_GRP);
        START_LBL->setObjectName(QString::fromUtf8("START_LBL"));
        START_LBL->setGeometry(QRect(10, 20, 70, 14));
        END_LBL = new QLabel(SCAN_GRP);
        END_LBL->setObjectName(QString::fromUtf8("END_LBL"));
        END_LBL->setGeometry(QRect(10, 60, 56, 14));
        END_POS_EDIT = new QLineEdit(SCAN_GRP);
        END_POS_EDIT->setObjectName(QString::fromUtf8("END_POS_EDIT"));
        END_POS_EDIT->setGeometry(QRect(10, 75, 61, 21));
        FRAME_LBL = new QLabel(SCAN_GRP);
        FRAME_LBL->setObjectName(QString::fromUtf8("FRAME_LBL"));
        FRAME_LBL->setGeometry(QRect(10, 100, 77, 14));
        FRAME_COUNT_EDIT = new QLineEdit(SCAN_GRP);
        FRAME_COUNT_EDIT->setObjectName(QString::fromUtf8("FRAME_COUNT_EDIT"));
        FRAME_COUNT_EDIT->setGeometry(QRect(10, 115, 61, 21));
        STEP_LBL = new QLabel(SCAN_GRP);
        STEP_LBL->setObjectName(QString::fromUtf8("STEP_LBL"));
        STEP_LBL->setGeometry(QRect(10, 140, 63, 14));
        STEP_SIZE_EDIT = new QLineEdit(SCAN_GRP);
        STEP_SIZE_EDIT->setObjectName(QString::fromUtf8("STEP_SIZE_EDIT"));
        STEP_SIZE_EDIT->setGeometry(QRect(10, 155, 61, 21));
        UNIT_NS = new QLabel(SCAN_GRP);
        UNIT_NS->setObjectName(QString::fromUtf8("UNIT_NS"));
        UNIT_NS->setGeometry(QRect(75, 40, 14, 14));
        UNIT_NS_2 = new QLabel(SCAN_GRP);
        UNIT_NS_2->setObjectName(QString::fromUtf8("UNIT_NS_2"));
        UNIT_NS_2->setGeometry(QRect(75, 80, 14, 14));
        UNIT_NS_3 = new QLabel(SCAN_GRP);
        UNIT_NS_3->setObjectName(QString::fromUtf8("UNIT_NS_3"));
        UNIT_NS_3->setGeometry(QRect(75, 160, 14, 14));
        ENHANCE_GROUP = new QGroupBox(ProgSettings);
        ENHANCE_GROUP->setObjectName(QString::fromUtf8("ENHANCE_GROUP"));
        ENHANCE_GROUP->setGeometry(QRect(150, 20, 161, 181));
        KERNEL_EDIT = new QLineEdit(ENHANCE_GROUP);
        KERNEL_EDIT->setObjectName(QString::fromUtf8("KERNEL_EDIT"));
        KERNEL_EDIT->setGeometry(QRect(10, 35, 41, 21));
        KERNEL_SIZE = new QLabel(ENHANCE_GROUP);
        KERNEL_SIZE->setObjectName(QString::fromUtf8("KERNEL_SIZE"));
        KERNEL_SIZE->setGeometry(QRect(10, 20, 77, 14));
        LOW_IN_EDIT = new QLineEdit(ENHANCE_GROUP);
        LOW_IN_EDIT->setObjectName(QString::fromUtf8("LOW_IN_EDIT"));
        LOW_IN_EDIT->setGeometry(QRect(10, 115, 41, 21));
        LOW_OUT_EDIT = new QLineEdit(ENHANCE_GROUP);
        LOW_OUT_EDIT->setObjectName(QString::fromUtf8("LOW_OUT_EDIT"));
        LOW_OUT_EDIT->setGeometry(QRect(80, 115, 41, 21));
        HIGH_IN_EDIT = new QLineEdit(ENHANCE_GROUP);
        HIGH_IN_EDIT->setObjectName(QString::fromUtf8("HIGH_IN_EDIT"));
        HIGH_IN_EDIT->setGeometry(QRect(10, 155, 41, 21));
        HIGH_OUT_EDIT = new QLineEdit(ENHANCE_GROUP);
        HIGH_OUT_EDIT->setObjectName(QString::fromUtf8("HIGH_OUT_EDIT"));
        HIGH_OUT_EDIT->setGeometry(QRect(80, 155, 41, 21));
        GAMMA_EDIT = new QLineEdit(ENHANCE_GROUP);
        GAMMA_EDIT->setObjectName(QString::fromUtf8("GAMMA_EDIT"));
        GAMMA_EDIT->setGeometry(QRect(10, 75, 41, 21));
        LOW_IN = new QLabel(ENHANCE_GROUP);
        LOW_IN->setObjectName(QString::fromUtf8("LOW_IN"));
        LOW_IN->setGeometry(QRect(10, 100, 47, 13));
        HIGH_IN = new QLabel(ENHANCE_GROUP);
        HIGH_IN->setObjectName(QString::fromUtf8("HIGH_IN"));
        HIGH_IN->setGeometry(QRect(10, 140, 49, 14));
        LOW_OUT = new QLabel(ENHANCE_GROUP);
        LOW_OUT->setObjectName(QString::fromUtf8("LOW_OUT"));
        LOW_OUT->setGeometry(QRect(80, 100, 49, 14));
        HIGH_OUT = new QLabel(ENHANCE_GROUP);
        HIGH_OUT->setObjectName(QString::fromUtf8("HIGH_OUT"));
        HIGH_OUT->setGeometry(QRect(80, 140, 56, 14));
        GAMMA = new QLabel(ENHANCE_GROUP);
        GAMMA->setObjectName(QString::fromUtf8("GAMMA"));
        GAMMA->setGeometry(QRect(10, 60, 35, 14));
        LOG_EDIT = new QLineEdit(ENHANCE_GROUP);
        LOG_EDIT->setObjectName(QString::fromUtf8("LOG_EDIT"));
        LOG_EDIT->setGeometry(QRect(80, 75, 41, 21));
        LOG = new QLabel(ENHANCE_GROUP);
        LOG->setObjectName(QString::fromUtf8("LOG"));
        LOG->setGeometry(QRect(80, 60, 21, 14));

        retranslateUi(ProgSettings);
        QObject::connect(USER_BTN_GRP, SIGNAL(accepted()), ProgSettings, SLOT(accept()));
        QObject::connect(USER_BTN_GRP, SIGNAL(rejected()), ProgSettings, SLOT(reject()));

        QMetaObject::connectSlotsByName(ProgSettings);
    } // setupUi

    void retranslateUi(QDialog *ProgSettings)
    {
        ProgSettings->setWindowTitle(QApplication::translate("ProgSettings", "Dialog", nullptr));
        SCAN_GRP->setTitle(QApplication::translate("ProgSettings", "Scan Options", nullptr));
        START_LBL->setText(QApplication::translate("ProgSettings", "Start pos.", nullptr));
        END_LBL->setText(QApplication::translate("ProgSettings", "End pos.", nullptr));
        FRAME_LBL->setText(QApplication::translate("ProgSettings", "Frame count", nullptr));
        STEP_LBL->setText(QApplication::translate("ProgSettings", "Step size", nullptr));
        UNIT_NS->setText(QApplication::translate("ProgSettings", "ns", nullptr));
        UNIT_NS_2->setText(QApplication::translate("ProgSettings", "ns", nullptr));
        UNIT_NS_3->setText(QApplication::translate("ProgSettings", "ns", nullptr));
        ENHANCE_GROUP->setTitle(QApplication::translate("ProgSettings", "Img Enhance Options", nullptr));
        KERNEL_SIZE->setText(QApplication::translate("ProgSettings", "Kernel size", nullptr));
        LOW_IN->setText(QApplication::translate("ProgSettings", "low in", nullptr));
        HIGH_IN->setText(QApplication::translate("ProgSettings", "high in", nullptr));
        LOW_OUT->setText(QApplication::translate("ProgSettings", "low out", nullptr));
        HIGH_OUT->setText(QApplication::translate("ProgSettings", "high out", nullptr));
        GAMMA->setText(QApplication::translate("ProgSettings", "Gamma", nullptr));
        LOG->setText(QApplication::translate("ProgSettings", "Log", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ProgSettings: public Ui_ProgSettings {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SETTINGS_H
