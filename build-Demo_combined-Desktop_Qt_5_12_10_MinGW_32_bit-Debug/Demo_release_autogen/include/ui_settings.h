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
        SCAN_GRP->setGeometry(QRect(20, 20, 101, 181));
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
    } // retranslateUi

};

namespace Ui {
    class ProgSettings: public Ui_ProgSettings {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SETTINGS_H
