/********************************************************************************
** Form generated from reading UI file 'sdrgui.ui'
**
** Created by: Qt User Interface Compiler version 5.6.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SDRGUI_H
#define UI_SDRGUI_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLCDNumber>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include "qwt_plot.h"
#include "qwt_scale_widget.h"
#include "qwt_text_label.h"

QT_BEGIN_NAMESPACE

class Ui_elektorSDR
{
public:
    QPushButton *startButton;
    QLabel *timeDisplay;
    QPushButton *QuitButton;
    QLCDNumber *coarseCorrectorDisplay;
    QLCDNumber *sampleRateDisplay;
    QLCDNumber *fineCorrectorDisplay;
    QwtPlot *iqDisplay;
    QComboBox *deviceSelector;
    QLCDNumber *avgTokenLengthDisplay;
    QListView *ensembleDisplay;
    QLabel *ensembleName;
    QLCDNumber *ensembleId;
    QLabel *dabMode;
    QLabel *nameofLanguage;
    QLabel *programType;
    QSlider *scopeSlider;
    QPushButton *dumpButton;
    QLCDNumber *uepFlagDisplay;
    QLCDNumber *startAddrDisplay;
    QLCDNumber *LengthDisplay;
    QLCDNumber *protLevelDisplay;
    QLCDNumber *bitRateDisplay;
    QLCDNumber *ASCTyDisplay;
    QLabel *label;
    QLabel *label_2;
    QLabel *label_3;
    QLabel *label_4;
    QLabel *label_5;
    QLabel *label_6;
    QLCDNumber *errorDisplay;
    QLabel *programName;
    QLCDNumber *ficRatioDisplay;
    QLCDNumber *snrDisplay;
    QLabel *label_8;
    QLabel *label_9;
    QLabel *label_10;
    QComboBox *streamOutSelector;
    QPushButton *correctorReset;
    QLabel *versionName;
    QPushButton *audioDump;
    QPushButton *mp2fileButton;
    QPushButton *aacfileButton;
    QComboBox *bandSelector;
    QComboBox *channelSelector;
    QComboBox *modeSelector;
    QLabel *syncedLabel;
    QLabel *dynamicLabel;
    QPushButton *spectrumButton;

    void setupUi(QDialog *elektorSDR)
    {
        if (elektorSDR->objectName().isEmpty())
            elektorSDR->setObjectName(QStringLiteral("elektorSDR"));
        elektorSDR->resize(782, 328);
        QPalette palette;
        QBrush brush(QColor(255, 255, 255, 255));
        brush.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Active, QPalette::Base, brush);
        palette.setBrush(QPalette::Active, QPalette::Window, brush);
        palette.setBrush(QPalette::Inactive, QPalette::Base, brush);
        palette.setBrush(QPalette::Inactive, QPalette::Window, brush);
        palette.setBrush(QPalette::Disabled, QPalette::Base, brush);
        palette.setBrush(QPalette::Disabled, QPalette::Window, brush);
        elektorSDR->setPalette(palette);
        QFont font;
        font.setBold(false);
        font.setWeight(50);
        elektorSDR->setFont(font);
        startButton = new QPushButton(elektorSDR);
        startButton->setObjectName(QStringLiteral("startButton"));
        startButton->setGeometry(QRect(690, 100, 71, 61));
        QPalette palette1;
        palette1.setBrush(QPalette::Active, QPalette::Base, brush);
        QBrush brush1(QColor(255, 0, 0, 255));
        brush1.setStyle(Qt::SolidPattern);
        palette1.setBrush(QPalette::Active, QPalette::Window, brush1);
        palette1.setBrush(QPalette::Inactive, QPalette::Base, brush);
        palette1.setBrush(QPalette::Inactive, QPalette::Window, brush1);
        palette1.setBrush(QPalette::Disabled, QPalette::Base, brush1);
        palette1.setBrush(QPalette::Disabled, QPalette::Window, brush1);
        startButton->setPalette(palette1);
        QFont font1;
        font1.setBold(true);
        font1.setWeight(75);
        startButton->setFont(font1);
        startButton->setAutoFillBackground(true);
        timeDisplay = new QLabel(elektorSDR);
        timeDisplay->setObjectName(QStringLiteral("timeDisplay"));
        timeDisplay->setGeometry(QRect(410, 60, 161, 31));
        timeDisplay->setFrameShape(QFrame::NoFrame);
        timeDisplay->setFrameShadow(QFrame::Raised);
        timeDisplay->setLineWidth(2);
        QuitButton = new QPushButton(elektorSDR);
        QuitButton->setObjectName(QStringLiteral("QuitButton"));
        QuitButton->setGeometry(QRect(690, 160, 71, 61));
        QPalette palette2;
        palette2.setBrush(QPalette::Active, QPalette::Button, brush1);
        palette2.setBrush(QPalette::Inactive, QPalette::Button, brush1);
        palette2.setBrush(QPalette::Disabled, QPalette::Button, brush1);
        QuitButton->setPalette(palette2);
        QuitButton->setFont(font1);
        QuitButton->setAutoFillBackground(true);
        coarseCorrectorDisplay = new QLCDNumber(elektorSDR);
        coarseCorrectorDisplay->setObjectName(QStringLiteral("coarseCorrectorDisplay"));
        coarseCorrectorDisplay->setGeometry(QRect(450, 90, 41, 31));
        QPalette palette3;
        palette3.setBrush(QPalette::Active, QPalette::Base, brush);
        QBrush brush2(QColor(255, 255, 0, 255));
        brush2.setStyle(Qt::SolidPattern);
        palette3.setBrush(QPalette::Active, QPalette::Window, brush2);
        palette3.setBrush(QPalette::Inactive, QPalette::Base, brush);
        palette3.setBrush(QPalette::Inactive, QPalette::Window, brush2);
        palette3.setBrush(QPalette::Disabled, QPalette::Base, brush2);
        palette3.setBrush(QPalette::Disabled, QPalette::Window, brush2);
        coarseCorrectorDisplay->setPalette(palette3);
        coarseCorrectorDisplay->setAutoFillBackground(false);
        coarseCorrectorDisplay->setFrameShape(QFrame::NoFrame);
        coarseCorrectorDisplay->setLineWidth(1);
        coarseCorrectorDisplay->setDigitCount(3);
        coarseCorrectorDisplay->setSegmentStyle(QLCDNumber::Flat);
        sampleRateDisplay = new QLCDNumber(elektorSDR);
        sampleRateDisplay->setObjectName(QStringLiteral("sampleRateDisplay"));
        sampleRateDisplay->setGeometry(QRect(250, 140, 81, 31));
        QPalette palette4;
        palette4.setBrush(QPalette::Active, QPalette::Base, brush);
        palette4.setBrush(QPalette::Active, QPalette::Window, brush2);
        palette4.setBrush(QPalette::Inactive, QPalette::Base, brush);
        palette4.setBrush(QPalette::Inactive, QPalette::Window, brush2);
        palette4.setBrush(QPalette::Disabled, QPalette::Base, brush2);
        palette4.setBrush(QPalette::Disabled, QPalette::Window, brush2);
        sampleRateDisplay->setPalette(palette4);
        sampleRateDisplay->setAutoFillBackground(false);
        sampleRateDisplay->setFrameShape(QFrame::NoFrame);
        sampleRateDisplay->setLineWidth(1);
        sampleRateDisplay->setDigitCount(7);
        sampleRateDisplay->setSegmentStyle(QLCDNumber::Flat);
        fineCorrectorDisplay = new QLCDNumber(elektorSDR);
        fineCorrectorDisplay->setObjectName(QStringLiteral("fineCorrectorDisplay"));
        fineCorrectorDisplay->setGeometry(QRect(350, 90, 71, 31));
        QPalette palette5;
        palette5.setBrush(QPalette::Active, QPalette::Base, brush);
        palette5.setBrush(QPalette::Active, QPalette::Window, brush2);
        palette5.setBrush(QPalette::Inactive, QPalette::Base, brush);
        palette5.setBrush(QPalette::Inactive, QPalette::Window, brush2);
        palette5.setBrush(QPalette::Disabled, QPalette::Base, brush2);
        palette5.setBrush(QPalette::Disabled, QPalette::Window, brush2);
        fineCorrectorDisplay->setPalette(palette5);
        fineCorrectorDisplay->setAutoFillBackground(false);
        fineCorrectorDisplay->setFrameShape(QFrame::NoFrame);
        fineCorrectorDisplay->setLineWidth(1);
        fineCorrectorDisplay->setDigitCount(4);
        fineCorrectorDisplay->setSegmentStyle(QLCDNumber::Flat);
        iqDisplay = new QwtPlot(elektorSDR);
        iqDisplay->setObjectName(QStringLiteral("iqDisplay"));
        iqDisplay->setGeometry(QRect(10, 80, 181, 121));
        deviceSelector = new QComboBox(elektorSDR);
        deviceSelector->setObjectName(QStringLiteral("deviceSelector"));
        deviceSelector->setGeometry(QRect(340, 230, 101, 31));
        avgTokenLengthDisplay = new QLCDNumber(elektorSDR);
        avgTokenLengthDisplay->setObjectName(QStringLiteral("avgTokenLengthDisplay"));
        avgTokenLengthDisplay->setGeometry(QRect(250, 110, 71, 31));
        QPalette palette6;
        palette6.setBrush(QPalette::Active, QPalette::Base, brush);
        palette6.setBrush(QPalette::Active, QPalette::Window, brush2);
        palette6.setBrush(QPalette::Inactive, QPalette::Base, brush);
        palette6.setBrush(QPalette::Inactive, QPalette::Window, brush2);
        palette6.setBrush(QPalette::Disabled, QPalette::Base, brush2);
        palette6.setBrush(QPalette::Disabled, QPalette::Window, brush2);
        avgTokenLengthDisplay->setPalette(palette6);
        avgTokenLengthDisplay->setAutoFillBackground(false);
        avgTokenLengthDisplay->setFrameShape(QFrame::NoFrame);
        avgTokenLengthDisplay->setLineWidth(1);
        avgTokenLengthDisplay->setDigitCount(6);
        avgTokenLengthDisplay->setSegmentStyle(QLCDNumber::Flat);
        ensembleDisplay = new QListView(elektorSDR);
        ensembleDisplay->setObjectName(QStringLiteral("ensembleDisplay"));
        ensembleDisplay->setGeometry(QRect(510, 100, 171, 171));
        ensembleName = new QLabel(elektorSDR);
        ensembleName->setObjectName(QStringLiteral("ensembleName"));
        ensembleName->setGeometry(QRect(200, 70, 131, 21));
        QFont font2;
        font2.setPointSize(11);
        ensembleName->setFont(font2);
        ensembleName->setFrameShape(QFrame::NoFrame);
        ensembleId = new QLCDNumber(elektorSDR);
        ensembleId->setObjectName(QStringLiteral("ensembleId"));
        ensembleId->setGeometry(QRect(330, 60, 61, 21));
        ensembleId->setFrameShape(QFrame::NoFrame);
        ensembleId->setMode(QLCDNumber::Hex);
        ensembleId->setSegmentStyle(QLCDNumber::Flat);
        dabMode = new QLabel(elektorSDR);
        dabMode->setObjectName(QStringLiteral("dabMode"));
        dabMode->setGeometry(QRect(190, 10, 81, 21));
        QFont font3;
        font3.setPointSize(14);
        font3.setBold(true);
        font3.setWeight(75);
        dabMode->setFont(font3);
        dabMode->setFrameShape(QFrame::NoFrame);
        nameofLanguage = new QLabel(elektorSDR);
        nameofLanguage->setObjectName(QStringLiteral("nameofLanguage"));
        nameofLanguage->setGeometry(QRect(80, 30, 111, 21));
        QFont font4;
        font4.setPointSize(12);
        nameofLanguage->setFont(font4);
        nameofLanguage->setFrameShape(QFrame::NoFrame);
        programType = new QLabel(elektorSDR);
        programType->setObjectName(QStringLiteral("programType"));
        programType->setGeometry(QRect(80, 50, 111, 21));
        programType->setFont(font4);
        programType->setFrameShape(QFrame::NoFrame);
        scopeSlider = new QSlider(elektorSDR);
        scopeSlider->setObjectName(QStringLiteral("scopeSlider"));
        scopeSlider->setGeometry(QRect(0, 200, 211, 20));
        scopeSlider->setMaximum(100);
        scopeSlider->setSliderPosition(30);
        scopeSlider->setOrientation(Qt::Horizontal);
        scopeSlider->setTickPosition(QSlider::TicksBelow);
        dumpButton = new QPushButton(elektorSDR);
        dumpButton->setObjectName(QStringLiteral("dumpButton"));
        dumpButton->setGeometry(QRect(420, 150, 81, 31));
        uepFlagDisplay = new QLCDNumber(elektorSDR);
        uepFlagDisplay->setObjectName(QStringLiteral("uepFlagDisplay"));
        uepFlagDisplay->setGeometry(QRect(290, 10, 64, 23));
        uepFlagDisplay->setSegmentStyle(QLCDNumber::Flat);
        startAddrDisplay = new QLCDNumber(elektorSDR);
        startAddrDisplay->setObjectName(QStringLiteral("startAddrDisplay"));
        startAddrDisplay->setGeometry(QRect(363, 10, 51, 23));
        startAddrDisplay->setSegmentStyle(QLCDNumber::Flat);
        LengthDisplay = new QLCDNumber(elektorSDR);
        LengthDisplay->setObjectName(QStringLiteral("LengthDisplay"));
        LengthDisplay->setGeometry(QRect(430, 10, 51, 23));
        LengthDisplay->setSegmentStyle(QLCDNumber::Flat);
        protLevelDisplay = new QLCDNumber(elektorSDR);
        protLevelDisplay->setObjectName(QStringLiteral("protLevelDisplay"));
        protLevelDisplay->setGeometry(QRect(490, 10, 51, 23));
        protLevelDisplay->setSegmentStyle(QLCDNumber::Flat);
        bitRateDisplay = new QLCDNumber(elektorSDR);
        bitRateDisplay->setObjectName(QStringLiteral("bitRateDisplay"));
        bitRateDisplay->setGeometry(QRect(550, 10, 51, 23));
        bitRateDisplay->setSegmentStyle(QLCDNumber::Flat);
        ASCTyDisplay = new QLCDNumber(elektorSDR);
        ASCTyDisplay->setObjectName(QStringLiteral("ASCTyDisplay"));
        ASCTyDisplay->setGeometry(QRect(610, 10, 64, 23));
        ASCTyDisplay->setSegmentStyle(QLCDNumber::Flat);
        label = new QLabel(elektorSDR);
        label->setObjectName(QStringLiteral("label"));
        label->setGeometry(QRect(290, 40, 51, 21));
        label_2 = new QLabel(elektorSDR);
        label_2->setObjectName(QStringLiteral("label_2"));
        label_2->setGeometry(QRect(350, 40, 67, 21));
        label_3 = new QLabel(elektorSDR);
        label_3->setObjectName(QStringLiteral("label_3"));
        label_3->setGeometry(QRect(420, 40, 67, 21));
        label_4 = new QLabel(elektorSDR);
        label_4->setObjectName(QStringLiteral("label_4"));
        label_4->setGeometry(QRect(470, 40, 67, 21));
        label_5 = new QLabel(elektorSDR);
        label_5->setObjectName(QStringLiteral("label_5"));
        label_5->setGeometry(QRect(550, 40, 61, 21));
        label_6 = new QLabel(elektorSDR);
        label_6->setObjectName(QStringLiteral("label_6"));
        label_6->setGeometry(QRect(620, 40, 67, 21));
        errorDisplay = new QLCDNumber(elektorSDR);
        errorDisplay->setObjectName(QStringLiteral("errorDisplay"));
        errorDisplay->setGeometry(QRect(230, 220, 61, 31));
        errorDisplay->setFrameShape(QFrame::NoFrame);
        errorDisplay->setDigitCount(4);
        errorDisplay->setSegmentStyle(QLCDNumber::Flat);
        programName = new QLabel(elektorSDR);
        programName->setObjectName(QStringLiteral("programName"));
        programName->setGeometry(QRect(50, 10, 141, 21));
        programName->setFont(font4);
        programName->setFrameShape(QFrame::NoFrame);
        ficRatioDisplay = new QLCDNumber(elektorSDR);
        ficRatioDisplay->setObjectName(QStringLiteral("ficRatioDisplay"));
        ficRatioDisplay->setGeometry(QRect(130, 220, 64, 31));
        ficRatioDisplay->setFrameShape(QFrame::NoFrame);
        ficRatioDisplay->setDigitCount(4);
        ficRatioDisplay->setSegmentStyle(QLCDNumber::Flat);
        snrDisplay = new QLCDNumber(elektorSDR);
        snrDisplay->setObjectName(QStringLiteral("snrDisplay"));
        snrDisplay->setGeometry(QRect(60, 220, 64, 31));
        snrDisplay->setFrameShape(QFrame::NoFrame);
        snrDisplay->setDigitCount(4);
        snrDisplay->setSegmentStyle(QLCDNumber::Flat);
        label_8 = new QLabel(elektorSDR);
        label_8->setObjectName(QStringLiteral("label_8"));
        label_8->setGeometry(QRect(90, 250, 31, 21));
        label_9 = new QLabel(elektorSDR);
        label_9->setObjectName(QStringLiteral("label_9"));
        label_9->setGeometry(QRect(150, 250, 51, 21));
        label_10 = new QLabel(elektorSDR);
        label_10->setObjectName(QStringLiteral("label_10"));
        label_10->setGeometry(QRect(220, 250, 101, 21));
        streamOutSelector = new QComboBox(elektorSDR);
        streamOutSelector->setObjectName(QStringLiteral("streamOutSelector"));
        streamOutSelector->setGeometry(QRect(340, 180, 161, 21));
        correctorReset = new QPushButton(elektorSDR);
        correctorReset->setObjectName(QStringLiteral("correctorReset"));
        correctorReset->setGeometry(QRect(690, 220, 71, 51));
        QPalette palette7;
        QBrush brush3(QColor(85, 170, 255, 255));
        brush3.setStyle(Qt::SolidPattern);
        palette7.setBrush(QPalette::Active, QPalette::Button, brush3);
        palette7.setBrush(QPalette::Active, QPalette::Base, brush);
        QBrush brush4(QColor(0, 0, 0, 255));
        brush4.setStyle(Qt::SolidPattern);
        palette7.setBrush(QPalette::Active, QPalette::Window, brush4);
        palette7.setBrush(QPalette::Inactive, QPalette::Button, brush3);
        palette7.setBrush(QPalette::Inactive, QPalette::Base, brush);
        palette7.setBrush(QPalette::Inactive, QPalette::Window, brush4);
        palette7.setBrush(QPalette::Disabled, QPalette::Button, brush3);
        palette7.setBrush(QPalette::Disabled, QPalette::Base, brush4);
        palette7.setBrush(QPalette::Disabled, QPalette::Window, brush4);
        correctorReset->setPalette(palette7);
        correctorReset->setAutoFillBackground(true);
        versionName = new QLabel(elektorSDR);
        versionName->setObjectName(QStringLiteral("versionName"));
        versionName->setGeometry(QRect(600, 60, 151, 21));
        QFont font5;
        font5.setPointSize(12);
        font5.setBold(true);
        font5.setWeight(75);
        versionName->setFont(font5);
        versionName->setFrameShape(QFrame::NoFrame);
        audioDump = new QPushButton(elektorSDR);
        audioDump->setObjectName(QStringLiteral("audioDump"));
        audioDump->setGeometry(QRect(340, 150, 81, 31));
        QFont font6;
        font6.setPointSize(9);
        audioDump->setFont(font6);
        mp2fileButton = new QPushButton(elektorSDR);
        mp2fileButton->setObjectName(QStringLiteral("mp2fileButton"));
        mp2fileButton->setGeometry(QRect(340, 130, 81, 21));
        aacfileButton = new QPushButton(elektorSDR);
        aacfileButton->setObjectName(QStringLiteral("aacfileButton"));
        aacfileButton->setGeometry(QRect(420, 130, 81, 21));
        bandSelector = new QComboBox(elektorSDR);
        bandSelector->setObjectName(QStringLiteral("bandSelector"));
        bandSelector->setGeometry(QRect(340, 200, 91, 31));
        channelSelector = new QComboBox(elektorSDR);
        channelSelector->setObjectName(QStringLiteral("channelSelector"));
        channelSelector->setGeometry(QRect(440, 230, 61, 31));
        modeSelector = new QComboBox(elektorSDR);
        modeSelector->setObjectName(QStringLiteral("modeSelector"));
        modeSelector->setGeometry(QRect(430, 200, 71, 31));
        syncedLabel = new QLabel(elektorSDR);
        syncedLabel->setObjectName(QStringLiteral("syncedLabel"));
        syncedLabel->setGeometry(QRect(690, 10, 65, 21));
        QPalette palette8;
        palette8.setBrush(QPalette::Active, QPalette::Base, brush);
        palette8.setBrush(QPalette::Active, QPalette::Window, brush1);
        palette8.setBrush(QPalette::Inactive, QPalette::Base, brush);
        palette8.setBrush(QPalette::Inactive, QPalette::Window, brush1);
        palette8.setBrush(QPalette::Disabled, QPalette::Base, brush1);
        palette8.setBrush(QPalette::Disabled, QPalette::Window, brush1);
        syncedLabel->setPalette(palette8);
        syncedLabel->setAutoFillBackground(true);
        syncedLabel->setFrameShape(QFrame::Box);
        dynamicLabel = new QLabel(elektorSDR);
        dynamicLabel->setObjectName(QStringLiteral("dynamicLabel"));
        dynamicLabel->setGeometry(QRect(20, 280, 601, 31));
        spectrumButton = new QPushButton(elektorSDR);
        spectrumButton->setObjectName(QStringLiteral("spectrumButton"));
        spectrumButton->setGeometry(QRect(690, 270, 71, 41));

        retranslateUi(elektorSDR);

        QMetaObject::connectSlotsByName(elektorSDR);
    } // setupUi

    void retranslateUi(QDialog *elektorSDR)
    {
        elektorSDR->setWindowTitle(QApplication::translate("elektorSDR", "sdr-j DAB/DAB+ receiver ", 0));
        elektorSDR->setWindowIconText(QApplication::translate("elektorSDR", "QUIT", 0));
        startButton->setText(QApplication::translate("elektorSDR", "START", 0));
        timeDisplay->setText(QApplication::translate("elektorSDR", "TextLabel", 0));
        QuitButton->setText(QApplication::translate("elektorSDR", "QUIT", 0));
        deviceSelector->clear();
        deviceSelector->insertItems(0, QStringList()
         << QApplication::translate("elektorSDR", "no device", 0)
         << QApplication::translate("elektorSDR", "file input (.raw)", 0)
         << QApplication::translate("elektorSDR", "file input (.sdr)", 0)
        );
        ensembleName->setText(QString());
        dabMode->setText(QString());
        nameofLanguage->setText(QString());
        programType->setText(QString());
        dumpButton->setText(QApplication::translate("elektorSDR", "dump", 0));
        label->setText(QApplication::translate("elektorSDR", "uepFlag", 0));
        label_2->setText(QApplication::translate("elektorSDR", "start addr", 0));
        label_3->setText(QApplication::translate("elektorSDR", "length", 0));
        label_4->setText(QApplication::translate("elektorSDR", "prot level", 0));
        label_5->setText(QApplication::translate("elektorSDR", "bit rate", 0));
        label_6->setText(QApplication::translate("elektorSDR", "ASCTy", 0));
        programName->setText(QString());
        label_8->setText(QApplication::translate("elektorSDR", "SNR", 0));
        label_9->setText(QApplication::translate("elektorSDR", "fic ratio", 0));
        label_10->setText(QApplication::translate("elektorSDR", "aac/mp2 ratio", 0));
        streamOutSelector->clear();
        streamOutSelector->insertItems(0, QStringList()
         << QApplication::translate("elektorSDR", "select output", 0)
        );
        correctorReset->setText(QApplication::translate("elektorSDR", "reset", 0));
        versionName->setText(QString());
        audioDump->setText(QApplication::translate("elektorSDR", "audioDump", 0));
        mp2fileButton->setText(QApplication::translate("elektorSDR", "MP2", 0));
        aacfileButton->setText(QApplication::translate("elektorSDR", "AAC", 0));
        bandSelector->clear();
        bandSelector->insertItems(0, QStringList()
         << QApplication::translate("elektorSDR", "BAND III", 0)
         << QApplication::translate("elektorSDR", "L BAND", 0)
        );
        modeSelector->clear();
        modeSelector->insertItems(0, QStringList()
         << QApplication::translate("elektorSDR", "1", 0)
         << QApplication::translate("elektorSDR", "2", 0)
         << QApplication::translate("elektorSDR", "4", 0)
        );
        syncedLabel->setText(QString());
        dynamicLabel->setText(QString());
        spectrumButton->setText(QApplication::translate("elektorSDR", "spectrum", 0));
    } // retranslateUi

};

namespace Ui {
    class elektorSDR: public Ui_elektorSDR {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SDRGUI_H
