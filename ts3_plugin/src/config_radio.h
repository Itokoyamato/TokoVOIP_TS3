#pragma once

#include <QtWidgets/QDialog>
#include <QtCore/QHash>

#include "config_radio_groupbox.h"

class ConfigRadio : public QDialog
{
    Q_OBJECT
    
public:
    explicit ConfigRadio(
        QWidget *parent = 0,
        QString server_name = QString::null,
        QString channel_name = QString::null
    );
    ~ConfigRadio();

    // For Settings initialization and updating from other sources of interaction
    void UpdateEnabled(QString name, bool val);
    void UpdateBandpassInLowFrequency(QString name, double val);
    void UpdateBandpassInHighFrequency(QString name, double val);
    void UpdateDestruction(QString name, double val);
    void UpdateRingModFrequency(QString name, double val);
    void UpdateRingModMix(QString name, double val);
    void UpdateBandpassOutLowFrequency(QString name, double val);
    void UpdateBandpassOutHighFrequency(QString name, double val);

signals:
    void EnabledSet(QString,bool);
    void InLoFreqSet(QString,double);
    void InHiFreqSet(QString,double);
    void DestructionSet(QString,double);
    void RingModFrequencySet(QString,double);
    void RingModMixSet(QString,double);
    void OutLoFreqSet(QString,double);
    void OutHiFreqSet(QString,double);

    // Accepted: isDeleteButton Rejected: is x button
    void channel_closed(int, QString);   // custom channel setting

private:
    QHash<QString,ConfigRadioGroupBox*> m_channelstrips;
    ConfigRadioGroupBox* GetChannelStrip(QString name);
    QString m_title;    // for channel ident
};
