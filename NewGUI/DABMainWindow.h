#ifndef DABMAINWINDOW_H
#define DABMAINWINDOW_H

#include <QMainWindow>
#include "ui_DABMainWindow.h"
#include "DialogUtil.h"
#include "serviceselector.h"

class DABMainWindow : public QMainWindow, public Ui::DABMainWindow
{
    Q_OBJECT
public:
    DABMainWindow(QWidget *parent = 0);

    void OnInputSignalLevelChanged(double);
    void SetStationName(QString StationName);
    void SetLanguage(QString Language);
    void SetProgrType(QString ProgrType);
    void SetBitrate(QString Bitrate);
    void SetTextMessage(QString TextMessage);
    void onServiceChanged(int short_id, QString text);

private:
    LevelMeter* inputLevel;
    ServiceSelector *pServiceSelector;

signals:

public slots:
    //void OnInputSignalLevelChanged(double);

};

#endif // DABMAINWINDOW_H
