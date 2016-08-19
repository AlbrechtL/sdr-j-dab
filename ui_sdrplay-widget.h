/********************************************************************************
** Form generated from reading UI file 'sdrplay-widget.ui'
**
** Created by: Qt User Interface Compiler version 5.6.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SDRPLAY_2D_WIDGET_H
#define UI_SDRPLAY_2D_WIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLCDNumber>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_sdrplayWidget
{
public:
    QFrame *frame;
    QLabel *label;
    QSpinBox *f_correction;
    QSpinBox *KhzOffset;
    QLabel *label_3;
    QLabel *label_4;
    QLabel *statusLabel;
    QLCDNumber *api_version;
    QSlider *gainSlider;
    QLCDNumber *gainDisplay;

    void setupUi(QWidget *sdrplayWidget)
    {
        if (sdrplayWidget->objectName().isEmpty())
            sdrplayWidget->setObjectName(QStringLiteral("sdrplayWidget"));
        sdrplayWidget->resize(256, 188);
        frame = new QFrame(sdrplayWidget);
        frame->setObjectName(QStringLiteral("frame"));
        frame->setGeometry(QRect(0, 0, 211, 221));
        frame->setFrameShape(QFrame::StyledPanel);
        frame->setFrameShadow(QFrame::Raised);
        label = new QLabel(frame);
        label->setObjectName(QStringLiteral("label"));
        label->setGeometry(QRect(10, 140, 101, 21));
        f_correction = new QSpinBox(frame);
        f_correction->setObjectName(QStringLiteral("f_correction"));
        f_correction->setGeometry(QRect(0, 20, 91, 21));
        f_correction->setMinimum(-100);
        f_correction->setMaximum(100);
        KhzOffset = new QSpinBox(frame);
        KhzOffset->setObjectName(QStringLiteral("KhzOffset"));
        KhzOffset->setGeometry(QRect(0, 40, 91, 21));
        KhzOffset->setMaximum(1000000);
        label_3 = new QLabel(frame);
        label_3->setObjectName(QStringLiteral("label_3"));
        label_3->setGeometry(QRect(100, 40, 41, 21));
        label_4 = new QLabel(frame);
        label_4->setObjectName(QStringLiteral("label_4"));
        label_4->setGeometry(QRect(100, 20, 51, 21));
        statusLabel = new QLabel(frame);
        statusLabel->setObjectName(QStringLiteral("statusLabel"));
        statusLabel->setGeometry(QRect(16, 180, 121, 21));
        api_version = new QLCDNumber(frame);
        api_version->setObjectName(QStringLiteral("api_version"));
        api_version->setGeometry(QRect(0, 60, 91, 21));
        api_version->setLineWidth(0);
        api_version->setSegmentStyle(QLCDNumber::Flat);
        gainSlider = new QSlider(frame);
        gainSlider->setObjectName(QStringLiteral("gainSlider"));
        gainSlider->setGeometry(QRect(170, 39, 20, 131));
        gainSlider->setOrientation(Qt::Vertical);
        gainDisplay = new QLCDNumber(frame);
        gainDisplay->setObjectName(QStringLiteral("gainDisplay"));
        gainDisplay->setGeometry(QRect(163, 0, 41, 23));
        gainDisplay->setDigitCount(3);
        gainDisplay->setSegmentStyle(QLCDNumber::Flat);

        retranslateUi(sdrplayWidget);

        QMetaObject::connectSlotsByName(sdrplayWidget);
    } // setupUi

    void retranslateUi(QWidget *sdrplayWidget)
    {
        sdrplayWidget->setWindowTitle(QApplication::translate("sdrplayWidget", "SDRplay control", 0));
        label->setText(QApplication::translate("sdrplayWidget", "mirics-SDRplay", 0));
        label_3->setText(QApplication::translate("sdrplayWidget", "KHz", 0));
        label_4->setText(QApplication::translate("sdrplayWidget", "ppm", 0));
        statusLabel->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class sdrplayWidget: public Ui_sdrplayWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SDRPLAY_2D_WIDGET_H
