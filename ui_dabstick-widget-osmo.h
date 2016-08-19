/********************************************************************************
** Form generated from reading UI file 'dabstick-widget-osmo.ui'
**
** Created by: Qt User Interface Compiler version 5.6.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DABSTICK_2D_WIDGET_2D_OSMO_H
#define UI_DABSTICK_2D_WIDGET_2D_OSMO_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_dabstickWidget_osmo
{
public:
    QFrame *contents;
    QLabel *label;
    QSpinBox *f_correction;
    QSpinBox *KhzOffset;
    QLabel *label_3;
    QLabel *label_4;
    QLabel *label_5;
    QComboBox *combo_gain;
    QComboBox *combo_autogain;

    void setupUi(QWidget *dabstickWidget_osmo)
    {
        if (dabstickWidget_osmo->objectName().isEmpty())
            dabstickWidget_osmo->setObjectName(QStringLiteral("dabstickWidget_osmo"));
        dabstickWidget_osmo->resize(260, 226);
        contents = new QFrame(dabstickWidget_osmo);
        contents->setObjectName(QStringLiteral("contents"));
        contents->setGeometry(QRect(0, 0, 221, 191));
        contents->setFrameShape(QFrame::StyledPanel);
        contents->setFrameShadow(QFrame::Raised);
        label = new QLabel(contents);
        label->setObjectName(QStringLiteral("label"));
        label->setGeometry(QRect(10, 150, 101, 21));
        f_correction = new QSpinBox(contents);
        f_correction->setObjectName(QStringLiteral("f_correction"));
        f_correction->setGeometry(QRect(10, 10, 91, 21));
        f_correction->setMinimum(-100);
        f_correction->setMaximum(100);
        KhzOffset = new QSpinBox(contents);
        KhzOffset->setObjectName(QStringLiteral("KhzOffset"));
        KhzOffset->setGeometry(QRect(10, 40, 91, 21));
        KhzOffset->setMinimum(-100);
        KhzOffset->setMaximum(100);
        label_3 = new QLabel(contents);
        label_3->setObjectName(QStringLiteral("label_3"));
        label_3->setGeometry(QRect(110, 40, 41, 21));
        label_4 = new QLabel(contents);
        label_4->setObjectName(QStringLiteral("label_4"));
        label_4->setGeometry(QRect(110, 10, 51, 21));
        label_5 = new QLabel(contents);
        label_5->setObjectName(QStringLiteral("label_5"));
        label_5->setGeometry(QRect(110, 70, 41, 21));
        combo_gain = new QComboBox(contents);
        combo_gain->setObjectName(QStringLiteral("combo_gain"));
        combo_gain->setGeometry(QRect(10, 70, 91, 21));
        combo_autogain = new QComboBox(contents);
        combo_autogain->setObjectName(QStringLiteral("combo_autogain"));
        combo_autogain->setGeometry(QRect(10, 120, 141, 21));

        retranslateUi(dabstickWidget_osmo);

        QMetaObject::connectSlotsByName(dabstickWidget_osmo);
    } // setupUi

    void retranslateUi(QWidget *dabstickWidget_osmo)
    {
        dabstickWidget_osmo->setWindowTitle(QApplication::translate("dabstickWidget_osmo", "RTL2832 dabstick", 0));
        label->setText(QApplication::translate("dabstickWidget_osmo", "dabstick", 0));
        label_3->setText(QApplication::translate("dabstickWidget_osmo", "KHz", 0));
        label_4->setText(QApplication::translate("dabstickWidget_osmo", "ppm", 0));
        label_5->setText(QApplication::translate("dabstickWidget_osmo", "gain", 0));
        combo_autogain->clear();
        combo_autogain->insertItems(0, QStringList()
         << QApplication::translate("dabstickWidget_osmo", "autogain off", 0)
         << QApplication::translate("dabstickWidget_osmo", "autogain on", 0)
        );
    } // retranslateUi

};

namespace Ui {
    class dabstickWidget_osmo: public Ui_dabstickWidget_osmo {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DABSTICK_2D_WIDGET_2D_OSMO_H
