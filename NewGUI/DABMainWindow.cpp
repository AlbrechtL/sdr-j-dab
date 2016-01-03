#include "DABMainWindow.h"


DABMainWindow::DABMainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    setupUi(this);

    /* Init progress bar for input signal level */
    inputLevel = LevelMeter::createLevelMeter();
    levelMeterLayout->addWidget(inputLevel->widget());

    lineEditFrequency->setVisible(false);
    labelAFS->setVisible(false);
    LabelCountryCode->setVisible(false);
    LabelServiceID->setVisible(false);

    QDoubleValidator* fvalid = new QDoubleValidator(this);
    lineEditFrequency->setValidator(fvalid);
    //connect(ui->lineEditFrequency, SIGNAL(returnPressed ()), this, SLOT(tune()));

    pServiceSelector = new ServiceSelector(this);
    controlsLayout->addWidget(pServiceSelector);
    //connect(pServiceSelector, SIGNAL(audioServiceSelected(int)), controller, SLOT(selectAudioService(int)));
    //connect(pServiceSelector, SIGNAL(dataServiceSelected(int)), this, SLOT(OnSelectDataService(int)));
    //connect(controller, SIGNAL(serviceChanged(int,const CService&)), pServiceSelector, SLOT(onServiceChanged(int, const CService&)));
    //connect(controller, SIGNAL(textMessageChanged(int, QString)), this, SLOT(OnTextMessageChanged(int, const QString&)));
}

void DABMainWindow::OnInputSignalLevelChanged(double d)
{
    inputLevel->setLevel(d);
}

void DABMainWindow::SetStationName(QString StationName)
{
    LabelServiceLabel->setText(StationName);
}

void DABMainWindow::SetLanguage(QString Language)
{
    LabelLanguage->setText(Language);
}

void DABMainWindow::SetProgrType(QString ProgrType)
{
    LabelProgrType->setText(ProgrType);
}

void DABMainWindow::SetBitrate(QString Bitrate)
{
    LabelBitrate->setText(Bitrate);
}

void DABMainWindow::SetTextMessage(QString TextMessage)
{
    TextTextMessage->setText(TextMessage);
}

void DABMainWindow::onServiceChanged(int short_id, QString text)
{
    pServiceSelector->onServiceChanged(short_id, text);
}
