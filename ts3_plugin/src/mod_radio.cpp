#include "mod_radio.h"

#include "core/ts_helpers_qt.h"
#include "core/ts_serversinfo.h"
#include <ts3_functions.h>
#include "plugin.h"
#include "teamspeak/public_errors.h"
#include "tokovoip.h"

Radio::Radio(TSServersInfo& servers_info, Talkers& talkers, const char* plugin_id, QObject* parent)
	: m_servers_info(servers_info)
	, m_talkers(talkers)
{
	setParent(parent);
    setObjectName("Radio");
    m_isPrintEnabled = false;
	tokovoip.initialize((char *)plugin_id);
}

void Radio::setHomeId(uint64 serverConnectionHandlerID)
{
    if (m_homeId == serverConnectionHandlerID)
        return;

    m_homeId = serverConnectionHandlerID;
    
	if (m_homeId == 0)
        return;

    if (isRunning())
        m_talkers.DumpTalkStatusChanges(this, true);
}

void Radio::setChannelStripEnabled(QString name, bool val)
{
    if (m_SettingsMap.contains(name))
    {
        if (m_SettingsMap.value(name).enabled != val)
            m_SettingsMap[name].enabled = val;
    }
    else
    {
        auto setting = RadioFX_Settings();
        setting.name = name;
        setting.enabled = val;
        m_SettingsMap.insert(name, setting);
    }
    //Log(QString("%1 enabled %2").arg(name).arg(val),LogLevel_DEBUG);
    emit ChannelStripEnabledSet(name, val);
}

void Radio::setFudge(QString name, double val)
{
    if (m_SettingsMap.contains(name))
    {
        if (m_SettingsMap.value(name).fudge != val)
            m_SettingsMap[name].fudge = val;
    }
    else
    {
        auto setting = RadioFX_Settings();
        setting.name = name;
        setting.fudge = val;
        m_SettingsMap.insert(name, setting);
    }
    //Log(QString("%1 fudge %2").arg(name).arg(val),LogLevel_DEBUG);
    emit FudgeChanged(name, val);
}

void Radio::setInLoFreq(QString name, double val)
{
    if (m_SettingsMap.contains(name))
    {
        if (m_SettingsMap.value(name).freq_low != val)
            m_SettingsMap[name].freq_low = val;
    }
    else
    {
        auto setting = RadioFX_Settings();
        setting.name = name;
        setting.freq_low = val;
        m_SettingsMap.insert(name, setting);
    }
    emit InBpCenterFreqSet(name, getCenterFrequencyIn(m_SettingsMap.value(name)));
    emit InBpBandwidthSet(name, getBandWidthIn(m_SettingsMap.value(name)));

    //Log(QString("%1 low_freq %2").arg(name).arg(val),LogLevel_DEBUG);
    emit InLoFreqSet(name,val);
}

void Radio::setInHiFreq(QString name, double val)
{
    if (m_SettingsMap.contains(name))
    {
        if (m_SettingsMap.value(name).freq_hi != val)
            m_SettingsMap[name].freq_hi = val;
    }
    else
    {
        auto setting = RadioFX_Settings();
        setting.name = name;
        setting.freq_hi = val;
        m_SettingsMap.insert(name, setting);
    }
    emit InBpCenterFreqSet(name, getCenterFrequencyIn(m_SettingsMap.value(name)));
    emit InBpBandwidthSet(name, getBandWidthIn(m_SettingsMap.value(name)));

    //Log(QString("%1 hi_freq %2").arg(name).arg(val),LogLevel_DEBUG);
    emit InHiFreqSet(name,val);
}

void Radio::setRingModFrequency(QString name, double val)
{
    if (m_SettingsMap.contains(name))
    {
        if (m_SettingsMap.value(name).rm_mod_freq != val)
            m_SettingsMap[name].rm_mod_freq = val;
    }
    else
    {
        auto setting = RadioFX_Settings();
        setting.name = name;
        setting.rm_mod_freq = val;
        m_SettingsMap.insert(name, setting);
    }
    //Log(QString("%1 rm_mod_freq %2").arg(name).arg(val),LogLevel_DEBUG);
    emit RingModFrequencyChanged(name,val);
}

void Radio::setRingModMix(QString name, double val)
{
    if (m_SettingsMap.contains(name))
    {
        if (m_SettingsMap.value(name).rm_mix != val)
            m_SettingsMap[name].rm_mix = val;
    }
    else
    {
        auto setting = RadioFX_Settings();
        setting.name = name;
        setting.rm_mix = val;
        m_SettingsMap.insert(name,setting);
    }
    //Log(QString("%1 rm_mix %2").arg(name).arg(val),LogLevel_DEBUG);
    emit RingModMixChanged(name,val);
}

void Radio::setOutLoFreq(QString name, double val)
{
    if (m_SettingsMap.contains(name))
    {
        if (m_SettingsMap.value(name).o_freq_lo != val)
            m_SettingsMap[name].o_freq_lo = val;
    }
    else
    {
        auto setting = RadioFX_Settings();
        setting.name = name;
        setting.o_freq_lo = val;
        m_SettingsMap.insert(name, setting);
    }
    emit OutBpCenterFreqSet(name, getCenterFrequencyOut(m_SettingsMap.value(name)));
    emit OutBpBandwidthSet(name, getBandWidthOut(m_SettingsMap.value(name)));

    emit OutLoFreqSet(name,val);
}

void Radio::setOutHiFreq(QString name, double val)
{
    if (m_SettingsMap.contains(name))
    {
        if (m_SettingsMap.value(name).o_freq_hi != val)
            m_SettingsMap[name].o_freq_hi = val;
    }
    else
    {
        auto setting = RadioFX_Settings();
        setting.name = name;
        setting.o_freq_hi = val;
        m_SettingsMap.insert(name, setting);
    }
    emit OutBpCenterFreqSet(name, getCenterFrequencyOut(m_SettingsMap.value(name)));
    emit OutBpBandwidthSet(name, getBandWidthOut(m_SettingsMap.value(name)));

    //Log(QString("%1 rm_mix %2").arg(name).arg(val),LogLevel_DEBUG);
    emit OutHiFreqSet(name, val);
}

void Radio::ToggleClientBlacklisted(uint64 serverConnectionHandlerID, anyID clientID)
{
    if (m_ClientBlacklist.contains(serverConnectionHandlerID, clientID))
        m_ClientBlacklist.remove(serverConnectionHandlerID, clientID);
    else
        m_ClientBlacklist.insert(serverConnectionHandlerID, clientID);

    if (!(isRunning()))
        return;

    if (!(m_talkers_dspradios.contains(serverConnectionHandlerID)))
        return;

    auto sDspRadios = m_talkers_dspradios.value(serverConnectionHandlerID);
    if (sDspRadios->contains(clientID))
        sDspRadios->value(clientID)->setEnabled(QString::null,!isClientBlacklisted(serverConnectionHandlerID, clientID));
}

bool Radio::isClientBlacklisted(uint64 serverConnectionHandlerID, anyID clientID)
{
    return m_ClientBlacklist.contains(serverConnectionHandlerID,clientID);
}

void Radio::onRunningStateChanged(bool value)
{
    m_talkers.DumpTalkStatusChanges(this, ((value) ? STATUS_TALKING : STATUS_NOT_TALKING));	//FlushTalkStatusChanges((value)?STATUS_TALKING:STATUS_NOT_TALKING);
    Log(QString("enabled: %1").arg((value)?"true":"false"));
}

//! Returns true iff it will or has been an active processing
bool Radio::onTalkStatusChanged(uint64 serverConnectionHandlerID, int status, bool isReceivedWhisper, anyID clientID, bool isMe)
{
	if (isMe && status == STATUS_TALKING)
		sendCallback("startedtalking");
	if (isMe && status == STATUS_NOT_TALKING)
		sendCallback("stoppedtalking");
    if (isMe || !isRunning())
        return false;

    if (status == STATUS_TALKING)
    {   // Robust against multiple STATUS_TALKING in a row to be able to use it when the user changes settings

        unsigned int error = ERROR_ok;
        uint64 channel_id;
        if (!isReceivedWhisper)
        {   // Filter talk events outside of our channel
            anyID my_id;
            if ((error = ts3Functions.getClientID(serverConnectionHandlerID, &my_id)) != ERROR_ok)
                this->Error("Error getting my id", serverConnectionHandlerID, error);

            uint64 my_channel_id;
            if ((error = ts3Functions.getChannelOfClient(serverConnectionHandlerID, my_id, &my_channel_id)) != ERROR_ok)
                this->Error("Error getting my channel id", serverConnectionHandlerID, error);

            if ((error = ts3Functions.getChannelOfClient(serverConnectionHandlerID, clientID, &channel_id)) != ERROR_ok)
                this->Error("Error getting my channel id", serverConnectionHandlerID, error);

            if (channel_id != my_channel_id)
                return false;
        }

        DspRadio* dsp_obj;
        auto isNewDspObj = true;
        if (!(m_talkers_dspradios.contains(serverConnectionHandlerID)))
        {
            dsp_obj = new DspRadio(this);
            auto sDspRadios = new QHash<anyID, DspRadio*>;
            sDspRadios->insert(clientID, dsp_obj);
            m_talkers_dspradios.insert(serverConnectionHandlerID, sDspRadios);
        }
        else
        {
            auto sDspRadios = m_talkers_dspradios.value(serverConnectionHandlerID);
            if (sDspRadios->contains(clientID))
            {
                dsp_obj = sDspRadios->value(clientID);
                isNewDspObj = false;
            }
            else
            {
                dsp_obj = new DspRadio(this);
                sDspRadios->insert(clientID, dsp_obj);
            }
        }

        if (!isNewDspObj)
            this->disconnect(dsp_obj);

        RadioFX_Settings settings;
        if (isReceivedWhisper)
            settings = m_SettingsMap.value("Whisper");
        else
        {
            // get channel
            auto server_id = m_servers_info.get_server_info(serverConnectionHandlerID, true)->getUniqueId();
            auto channel_path = TSHelpers::GetChannelPath(serverConnectionHandlerID, channel_id);

            QString settings_map_key(server_id + channel_path);
            //this->Log(settings_map_key);
            // if (error == ERROR_ok && (!channel_path.isEmpty()) && m_SettingsMap.contains(settings_map_key))
            // {
            //     //this->Log("Applying custom setting");
            //     settings = m_SettingsMap.value(settings_map_key);
            // }
            if (serverConnectionHandlerID == m_homeId)
                settings = m_SettingsMap.value("Home");
            else
                settings = m_SettingsMap.value("Other");
        }

        dsp_obj->setChannelType(settings.name);
        dsp_obj->setEnabled(settings.name, settings.enabled);
        dsp_obj->setBandpassEqInCenterFrequency(settings.name, getCenterFrequencyIn(settings));
        dsp_obj->setBandpassEqInBandWidth(settings.name, getBandWidthIn(settings));
        dsp_obj->setFudge(settings.name, settings.fudge);
        dsp_obj->setRmModFreq(settings.name, settings.rm_mod_freq);
        dsp_obj->setRmMix(settings.name, settings.rm_mix);
        dsp_obj->setBandpassEqOutCenterFrequency(settings.name, getCenterFrequencyOut(settings));
        dsp_obj->setBandpassEqOutBandWidth(settings.name, getBandWidthOut(settings));
        connect(this, &Radio::ChannelStripEnabledSet, dsp_obj, &DspRadio::setEnabled, Qt::UniqueConnection);
        connect(this, &Radio::FudgeChanged, dsp_obj, &DspRadio::setFudge, Qt::UniqueConnection);
        connect(this, &Radio::InBpCenterFreqSet, dsp_obj, &DspRadio::setBandpassEqInCenterFrequency, Qt::UniqueConnection);
        connect(this, &Radio::InBpBandwidthSet, dsp_obj, &DspRadio::setBandpassEqInBandWidth, Qt::UniqueConnection);
        connect(this, &Radio::RingModFrequencyChanged, dsp_obj, &DspRadio::setRmModFreq, Qt::UniqueConnection);
        connect(this, &Radio::RingModMixChanged, dsp_obj, &DspRadio::setRmMix, Qt::UniqueConnection);
        connect(this, &Radio::OutBpCenterFreqSet, dsp_obj, &DspRadio::setBandpassEqOutCenterFrequency, Qt::UniqueConnection);
        connect(this, &Radio::OutBpBandwidthSet, dsp_obj, &DspRadio::setBandpassEqOutBandWidth, Qt::UniqueConnection);

        return settings.enabled;
    }
    else if (status == STATUS_NOT_TALKING)
    {
        // Removing does not need to be robust against multiple STATUS_NOT_TALKING in a row, since that doesn't happen on user setting change
        if (!m_talkers_dspradios.contains(serverConnectionHandlerID))
            return false;   // return silent bec. of ChannelMuter implementation

        auto server_dsp_radios = m_talkers_dspradios.value(serverConnectionHandlerID);
        if (!(server_dsp_radios->contains(clientID)))
            return false;

        auto dsp_obj = server_dsp_radios->value(clientID);
        dsp_obj->blockSignals(true);
        const auto kIsEnabled = dsp_obj->getEnabled();
        dsp_obj->deleteLater();
        server_dsp_radios->remove(clientID);
        return kIsEnabled;
    }
    return false;
}

//! Routes the arguments of the event to the corresponding volume object
/*!
 * \brief Radio::onEditPlaybackVoiceDataEvent pre-processing voice event
 * \param serverConnectionHandlerID the connection id of the server
 * \param clientID the client-side runtime-id of the sender
 * \param samples the sample array to manipulate
 * \param sampleCount amount of samples in the array
 * \param channels amount of channels
 */
void Radio::onEditPlaybackVoiceDataEvent(uint64 serverConnectionHandlerID, anyID clientID, short *samples, int sampleCount, int channels)
{
	DWORD error;
    if (!(isRunning()))
        return;

    if (!(m_talkers_dspradios.contains(serverConnectionHandlerID)))
        return;

    auto server_dsp_radios = m_talkers_dspradios.value(serverConnectionHandlerID);
    if (!(server_dsp_radios->contains(clientID)))
        return;

	char *UUID;
	if ((error = ts3Functions.getClientVariableAsString(ts3Functions.getCurrentServerConnectionHandlerID(), clientID, CLIENT_UNIQUE_IDENTIFIER, &UUID)) != ERROR_ok) {
		outputLog("Error getting client UUID", error);
	}
	else
	{
		if (tokovoip.getRadioData(UUID))
			server_dsp_radios->value(clientID)->process(samples, sampleCount, channels);
		ts3Functions.freeMemory(UUID);
	}
}

QHash<QString, RadioFX_Settings> Radio::GetSettingsMap() const
{
    return m_SettingsMap;
}

QHash<QString, RadioFX_Settings>& Radio::GetSettingsMapRef()
{
    return m_SettingsMap;
}
