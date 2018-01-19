#pragma once

#include <QtCore/QObject>
#include <QtCore/QHash>

#include "core/module.h"
#include "core/talkers.h"
#include "dsp_radio.h"
#include "core/ts_serversinfo.h"
#include "core/talkers.h"

struct RadioFX_Settings
{
    QString name = "";
    bool enabled = false;
    double freq_low = 0.0;
    double freq_hi = 0.0;
    double fudge = 0.0;
    double rm_mod_freq = 0.0;
    double rm_mix = 0.0;
    double o_freq_lo = 0.0;
    double o_freq_hi = 0.0;
};

class Radio : public Module, public TalkInterface
{
    Q_OBJECT
    Q_INTERFACES(TalkInterface)
    Q_PROPERTY(uint64 homeId READ homeId WRITE setHomeId)

public:
    explicit Radio(TSServersInfo& servers_info, Talkers& talkers, QObject* parent = nullptr);
    
    bool onTalkStatusChanged(uint64 serverConnectionHandlerID, int status, bool isReceivedWhisper, anyID clientID, bool isMe);

    void setHomeId(uint64 serverConnectionHandlerID);
    uint64 homeId() {return m_homeId;}

    bool isClientBlacklisted(uint64 serverConnectionHandlerID, anyID clientID);

    // events forwarded from plugin.cpp
    void onEditPlaybackVoiceDataEvent(uint64 serverConnectionHandlerID, anyID clientID, short* samples, int sampleCount, int channels);

    QHash<QString, RadioFX_Settings> GetSettingsMap() const;
    QHash<QString, RadioFX_Settings>& GetSettingsMapRef();

signals:
    void ChannelStripEnabledSet(QString, bool);
    void FudgeChanged(QString, double);
    void InLoFreqSet(QString, double);
    void InHiFreqSet(QString, double);
    void RingModFrequencyChanged(QString, double);
    void RingModMixChanged(QString,double);
    void OutLoFreqSet(QString, double);
    void OutHiFreqSet(QString, double);
    // connected to dsp_radio
    void InBpCenterFreqSet(QString, double);
    void InBpBandwidthSet(QString,double);
    void OutBpCenterFreqSet(QString, double);
    void OutBpBandwidthSet(QString, double);

public slots:
    void setChannelStripEnabled(QString name, bool val);
    void setFudge(QString name, double val);
    void setInLoFreq(QString name, double val);
    void setInHiFreq(QString name, double val);
    void setRingModFrequency(QString name, double val);
    void setRingModMix(QString name, double val);
    void setOutLoFreq(QString name, double val);
    void setOutHiFreq(QString name, double val);

    void ToggleClientBlacklisted(uint64 serverConnectionHandlerID, anyID clientID);

    //void saveSettings(int r);

private:
    uint64 m_homeId = 0;
    
	TSServersInfo& m_servers_info;
	Talkers& m_talkers;
	
    QHash<uint64, QHash<anyID,DspRadio*>* > m_talkers_dspradios;

    QHash<QString, RadioFX_Settings> m_SettingsMap;

    // QMultiMap is reported to be faster than QMultiHash until up to 10 entries in 4.x, oh I dunno
    QMultiMap<uint64,uint64> m_ClientBlacklist;

    //we have low, hi freq as input, convert them here
    static double getCenterFrequencyIn(RadioFX_Settings setting) {return setting.freq_low + (getBandWidthIn(setting) / 2.0);}
    static double getBandWidthIn(RadioFX_Settings setting) {return setting.freq_hi - setting.freq_low;}
    static double getCenterFrequencyOut(RadioFX_Settings setting) {return setting.o_freq_lo + (getBandWidthOut(setting) / 2.0);}
    static double getBandWidthOut(RadioFX_Settings setting) {return setting.o_freq_hi - setting.o_freq_lo;}

protected:
    void onRunningStateChanged(bool value);
};