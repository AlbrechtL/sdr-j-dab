/********************************************************************************
** Form generated from reading UI file 'rtl_tcp-widget.ui'
**
** Created by: Qt User Interface Compiler version 5.6.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_RTL_TCP_2D_WIDGET_H
#define UI_RTL_TCP_2D_WIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_rtl_tcp_widget
{
public:
    QFrame *frame;
    QLabel *label;
    QLabel *state;
    QPushButton *tcp_connect;
    QPushButton *tcp_disconnect;
    QLabel *connectedLabel;
    QSpinBox *tcp_gain;
    QSpinBox *tcp_ppm;
    QLabel *label_2;
    QLabel *label_3;
    QSpinBox *khzOffset;
    QLabel *label_4;

    void setupUi(QWidget *rtl_tcp_widget)
    {
        if (rtl_tcp_widget->objectName().isEmpty())
            rtl_tcp_widget->setObjectName(QStringLiteral("rtl_tcp_widget"));
        rtl_tcp_widget->resize(154, 223);
        frame = new QFrame(rtl_tcp_widget);
        frame->setObjectName(QStringLiteral("frame"));
        frame->setGeometry(QRect(0, 0, 151, 221));
        frame->setFrameShape(QFrame::StyledPanel);
        frame->setFrameShadow(QFrame::Raised);
        label = new QLabel(frame);
        label->setObjectName(QStringLiteral("label"));
        label->setGeometry(QRect(10, 190, 111, 21));
        state = new QLabel(frame);
        state->setObjectName(QStringLiteral("state"));
        state->setGeometry(QRect(10, 170, 131, 21));
        tcp_connect = new QPushButton(frame);
        tcp_connect->setObjectName(QStringLiteral("tcp_connect"));
        tcp_connect->setGeometry(QRect(0, 20, 111, 21));
        tcp_disconnect = new QPushButton(frame);
        tcp_disconnect->setObjectName(QStringLiteral("tcp_disconnect"));
        tcp_disconnect->setGeometry(QRect(0, 40, 111, 21));
        connectedLabel = new QLabel(frame);
        connectedLabel->setObjectName(QStringLiteral("connectedLabel"));
        connectedLabel->setGeometry(QRect(10, 170, 121, 20));
        tcp_gain = new QSpinBox(frame);
        tcp_gain->setObjectName(QStringLiteral("tcp_gain"));
        tcp_gain->setGeometry(QRect(0, 70, 71, 21));
        tcp_gain->setMaximum(999);
        tcp_ppm = new QSpinBox(frame);
        tcp_ppm->setObjectName(QStringLiteral("tcp_ppm"));
        tcp_ppm->setGeometry(QRect(0, 100, 71, 21));
        tcp_ppm->setMinimum(-100);
        label_2 = new QLabel(frame);
        label_2->setObjectName(QStringLiteral("label_2"));
        label_2->setGeometry(QRect(86, 70, 51, 21));
        label_3 = new QLabel(frame);
        label_3->setObjectName(QStringLiteral("label_3"));
        label_3->setGeometry(QRect(86, 100, 61, 21));
        khzOffset = new QSpinBox(frame);
        khzOffset->setObjectName(QStringLiteral("khzOffset"));
        khzOffset->setGeometry(QRect(0, 130, 71, 21));
        khzOffset->setMinimum(-100);
        label_4 = new QLabel(frame);
        label_4->setObjectName(QStringLiteral("label_4"));
        label_4->setGeometry(QRect(80, 130, 51, 21));

        retranslateUi(rtl_tcp_widget);

        QMetaObject::connectSlotsByName(rtl_tcp_widget);
    } // setupUi

    void retranslateUi(QWidget *rtl_tcp_widget)
    {
        rtl_tcp_widget->setWindowTitle(QApplication::translate("rtl_tcp_widget", "rtl-tcp", 0));
        label->setText(QApplication::translate("rtl_tcp_widget", "rtl_tcp_client", 0));
        state->setText(QString());
        tcp_connect->setText(QApplication::translate("rtl_tcp_widget", "connect", 0));
        tcp_disconnect->setText(QApplication::translate("rtl_tcp_widget", "disconnect", 0));
        connectedLabel->setText(QString());
        label_2->setText(QApplication::translate("rtl_tcp_widget", " gain", 0));
        label_3->setText(QApplication::translate("rtl_tcp_widget", "ppm", 0));
        label_4->setText(QApplication::translate("rtl_tcp_widget", "Offset", 0));
    } // retranslateUi

};

namespace Ui {
    class rtl_tcp_widget: public Ui_rtl_tcp_widget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_RTL_TCP_2D_WIDGET_H
