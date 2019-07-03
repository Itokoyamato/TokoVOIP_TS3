#include "settings_radio.h"

#include <QtCore/QSettings>
#include "core/ts_helpers_qt.h"
#include "core/ts_logging_qt.h"

#include "plugin.h" //pluginID
#include "ts3_functions.h"
#include "core/ts_serversinfo.h"
#include <teamspeak/public_errors.h>

#include "core/plugin_base.h"
#include "tokovoip.h"

SettingsRadio::SettingsRadio(TSServersInfo& servers_info, QObject* parent)
	: QObject(parent)
	, m_servers_info(servers_info)
{
    setObjectName("SettingsRadio");
}

void SettingsRadio::Init(Radio *radio)
{
    if (m_ContextMenuUi == -1)
    {
		auto plugin = qobject_cast<Plugin_Base*>(parent());
		auto& context_menu = plugin->context_menu();
		m_ContextMenuUi = context_menu.Register(this, PLUGIN_MENU_TYPE_GLOBAL, "Radio FX", "walkie_talkie_16.png");
		m_ContextMenuUnmute = context_menu.Register(this, PLUGIN_MENU_TYPE_GLOBAL, "Unmute All", "");
		// m_ContextMenuChannelUi = context_menu.Register(this,PLUGIN_MENU_TYPE_CHANNEL,"Radio FX (Channel)","walkie_talkie_16.png");
        m_ContextMenuToggleClientBlacklisted = context_menu.Register(this,PLUGIN_MENU_TYPE_CLIENT, "Radio FX: Toggle Client Blacklisted [temp]", "walkie_talkie_16.png");
        connect(&context_menu, &TSContextMenu::MenusInitialized, this, [=]()
        {
            if (m_ContextMenuUi == -1)
                TSLogging::Error(QString("%1: Menu wasn't registered.").arg(this->objectName()));

			if (m_ContextMenuUnmute == -1)
				TSLogging::Error(QString("%1: Menu unmute wasn't registered.").arg(this->objectName()));

            // if (m_ContextMenuChannelUi == -1)
            //     TSLogging::Error(QString("%1: Channel Menu wasn't registered.").arg(this->objectName()));

            if (m_ContextMenuToggleClientBlacklisted == -1)
                TSLogging::Error(QString("%1: Toggle Client Blacklisted menu item wasn't registered.").arg(this->objectName()));
        });
        connect(&context_menu, &TSContextMenu::FireContextMenuEvent, this, &SettingsRadio::onContextMenuEvent);

		auto& info_data = plugin->info_data();
        info_data.Register(this, true, 1);
    }

    connect(this, &SettingsRadio::EnabledSet, radio, &Radio::setChannelStripEnabled);
    connect(this, &SettingsRadio::InLoFreqSet, radio, &Radio::setInLoFreq);
    connect(this, &SettingsRadio::InHiFreqSet, radio, &Radio::setInHiFreq);
    connect(this, &SettingsRadio::DestructionSet, radio, &Radio::setFudge);
    connect(this, &SettingsRadio::RingModFrequencySet, radio, &Radio::setRingModFrequency);
    connect(this, &SettingsRadio::RingModMixSet, radio, &Radio::setRingModMix);
    connect(this, &SettingsRadio::OutLoFreqSet, radio, &Radio::setOutLoFreq);
    connect(this, &SettingsRadio::OutHiFreqSet, radio, &Radio::setOutHiFreq);

    connect(this, &SettingsRadio::ToggleClientBlacklisted, radio, &Radio::ToggleClientBlacklisted);

    QSettings cfg(TSHelpers::GetFullConfigPath(), QSettings::IniFormat);
    cfg.beginGroup(radio->objectName());

    const auto kStringList = cfg.childGroups().isEmpty() ? QStringList{"Home", "Whisper", "Other"} : cfg.childGroups();
    for (int i = 0; i < kStringList.size(); ++i)
    {
        auto name = kStringList.at(i);
        cfg.beginGroup(name);
        if (name == "HomeTab")
            name = "Home";

        radio->setChannelStripEnabled(name, cfg.value("enabled", true).toBool());
        radio->setInLoFreq(name, cfg.value("low_freq", 300.0f).toDouble());
        radio->setInHiFreq(name, cfg.value("high_freq", 3000.0f).toDouble());
        radio->setFudge(name, cfg.value("fudge", 2.0f).toDouble());
        cfg.beginGroup("RingMod");
        radio->setRingModFrequency(name, cfg.value("rm_mod_freq", 0.0f).toDouble());
        radio->setRingModMix(name, cfg.value("rm_mix", 0.0f).toDouble());
        cfg.endGroup();
        radio->setOutLoFreq(name, cfg.value("o_freq_lo", 300.0f).toDouble());
        radio->setOutHiFreq(name, cfg.value("o_freq_hi", 3000.0f).toDouble());
        cfg.endGroup();
    }

    radio->setEnabled(true);
    mP_radio = radio;
}

void SettingsRadio::shutdown()
{
    if (m_config)
    {
        m_config->done(QDialog::Rejected);  // will save anyways
        delete m_config;
    }
    for (auto config : m_channel_configs)
    {
        config->done(QDialog::Rejected);
        delete config;
    }
    m_channel_configs.clear();
}

bool SettingsRadio::onInfoDataChanged(uint64 serverConnectionHandlerID, uint64 id, PluginItemType type, uint64 mine, QTextStream &data)
{
    auto isDirty = false;
    if (type == PLUGIN_CLIENT)
    {
        if (m_ContextMenuToggleClientBlacklisted != -1)
		{
			auto plugin = qobject_cast<Plugin_Base*>(parent());
            ts3Functions.setPluginMenuEnabled(plugin->id().c_str(), m_ContextMenuToggleClientBlacklisted, (id != mine) ? 1 : 0);
		}

        if ((id != mine) && mP_radio && mP_radio->isClientBlacklisted(serverConnectionHandlerID,(anyID)id))
        {
            data << mP_radio->objectName() << ":";
            isDirty = true;
            data << "blacklisted [temp]";
        }
    }
    return isDirty;
}

void SettingsRadio::onContextMenuEvent(uint64 serverConnectionHandlerID, PluginMenuType type, int menuItemID, uint64 selectedItemID)
{
    if (type == PLUGIN_MENU_TYPE_GLOBAL)
    {
        if (menuItemID == m_ContextMenuUi)
        {
            if (m_config)
                m_config->activateWindow();
            else if (mP_radio)
            {
                auto p_config = new ConfigRadio(TSHelpers::GetMainWindow());  //has delete on close attribute

                QSettings cfg(TSHelpers::GetFullConfigPath(), QSettings::IniFormat);
                cfg.beginGroup(mP_radio->objectName());

                QStringList stringList {"HomeTab", "Whisper", "Other"};
                for (int i = 0; i < stringList.size(); ++i)
                {
                    auto name = stringList.at(i);
                    cfg.beginGroup(name);
                    if (name == "HomeTab")
                        name = "Home";

                    p_config->UpdateEnabled(name, cfg.value("enabled", true).toBool());
                    p_config->UpdateBandpassInLowFrequency(name, cfg.value("low_freq", 300.0).toDouble());
                    p_config->UpdateBandpassInHighFrequency(name, cfg.value("high_freq", 3000.0).toDouble());
                    p_config->UpdateDestruction(name, cfg.value("fudge", 2.0).toDouble());
                    cfg.beginGroup("RingMod");
                    p_config->UpdateRingModFrequency(name, cfg.value("rm_mod_freq", 0.0f).toDouble());
                    p_config->UpdateRingModMix(name, cfg.value("rm_mix", 0.0f).toDouble());
                    cfg.endGroup();
                    p_config->UpdateBandpassOutLowFrequency(name, cfg.value("o_freq_lo", 300.0).toDouble());
                    p_config->UpdateBandpassOutHighFrequency(name, cfg.value("o_freq_hi", 3000.0).toDouble());
                    cfg.endGroup();
                }

                connect(p_config, &ConfigRadio::EnabledSet, this, &SettingsRadio::EnabledSet);
                connect(p_config, &ConfigRadio::InLoFreqSet, this, &SettingsRadio::InLoFreqSet);
                connect(p_config, &ConfigRadio::InHiFreqSet, this,  &SettingsRadio::InHiFreqSet);
                connect(p_config, &ConfigRadio::DestructionSet, this, &SettingsRadio::DestructionSet);
                connect(p_config, &ConfigRadio::RingModFrequencySet, this, &SettingsRadio::RingModFrequencySet);
                connect(p_config, &ConfigRadio::RingModMixSet, this, &SettingsRadio::RingModMixSet);
                connect(p_config, &ConfigRadio::OutLoFreqSet, this, &SettingsRadio::OutLoFreqSet);
                connect(p_config, &ConfigRadio::OutHiFreqSet, this, &SettingsRadio::OutHiFreqSet);

                connect(p_config, &ConfigRadio::finished, this, &SettingsRadio::saveSettings);
                p_config->show();
                m_config = p_config;
            }
        }
		if (menuItemID == m_ContextMenuUnmute)
		{
			unmuteAll(ts3Functions.getCurrentServerConnectionHandlerID());
			resetVolumeAll(ts3Functions.getCurrentServerConnectionHandlerID());
		}
    }
    else if (type == PLUGIN_MENU_TYPE_CHANNEL)
    {
        if (menuItemID == m_ContextMenuChannelUi)
        {
            auto server_channel = qMakePair<uint64, uint64> (serverConnectionHandlerID, selectedItemID);
            if (m_channel_configs.contains(server_channel) && m_channel_configs.value(server_channel))
            {
                m_channel_configs.value(server_channel)->activateWindow();
                return;
            }

            if (!mP_radio)
                return;

            unsigned int error;

            char* channel_name_c;
            if ((error = ts3Functions.getChannelVariableAsString(serverConnectionHandlerID, selectedItemID, CHANNEL_NAME, &channel_name_c)) != ERROR_ok)
            {
                TSLogging::Error(QString("%1: Could not get channel name").arg(this->objectName()));
                return;
            }
            const auto kChannelName = QString::fromUtf8(channel_name_c);
            ts3Functions.freeMemory(channel_name_c);

            auto p_config = new ConfigRadio(TSHelpers::GetMainWindow(), m_servers_info.get_server_info(serverConnectionHandlerID, true)->getName(), kChannelName);  //has delete on close attribute
            QSettings cfg(TSHelpers::GetFullConfigPath(), QSettings::IniFormat);
            cfg.beginGroup(mP_radio->objectName());

            const auto kCustomChannelId = m_servers_info.get_server_info(serverConnectionHandlerID, true)->getUniqueId() + TSHelpers::GetChannelPath(serverConnectionHandlerID, selectedItemID);
            cfg.beginGroup(kCustomChannelId);

            // also push temp default setting to radio
            mP_radio->setChannelStripEnabled(kCustomChannelId, cfg.value("enabled",false).toBool());
            mP_radio->setInLoFreq(kCustomChannelId, cfg.value("low_freq",300.0).toDouble());
            mP_radio->setInHiFreq(kCustomChannelId, cfg.value("high_freq",3000.0).toDouble());
            mP_radio->setFudge(kCustomChannelId, cfg.value("fudge",2.0).toDouble());
            p_config->UpdateEnabled(kCustomChannelId, cfg.value("enabled",false).toBool());
            p_config->UpdateBandpassInLowFrequency(kCustomChannelId, cfg.value("low_freq",300.0).toDouble());
            p_config->UpdateBandpassInHighFrequency(kCustomChannelId, cfg.value("high_freq",3000.0).toDouble());
            p_config->UpdateDestruction(kCustomChannelId, cfg.value("fudge",2.0).toDouble());
            cfg.beginGroup("RingMod");
            mP_radio->setRingModFrequency(kCustomChannelId, cfg.value("rm_mod_freq",0.0f).toDouble());
            mP_radio->setRingModMix(kCustomChannelId, cfg.value("rm_mix",0.0f).toDouble());
            p_config->UpdateRingModFrequency(kCustomChannelId, cfg.value("rm_mod_freq",0.0f).toDouble());
            p_config->UpdateRingModMix(kCustomChannelId, cfg.value("rm_mix",0.0f).toDouble());
            cfg.endGroup();
            mP_radio->setOutLoFreq(kCustomChannelId, cfg.value("o_freq_lo",300.0).toDouble());
            mP_radio->setOutHiFreq(kCustomChannelId, cfg.value("o_freq_hi",3000.0).toDouble());
            p_config->UpdateBandpassOutLowFrequency(kCustomChannelId, cfg.value("o_freq_lo",300.0).toDouble());
            p_config->UpdateBandpassOutHighFrequency(kCustomChannelId, cfg.value("o_freq_hi",3000.0).toDouble());

            cfg.endGroup(); // Channel
            cfg.endGroup(); // radio module

            connect(p_config, &ConfigRadio::EnabledSet, this, &SettingsRadio::EnabledSet);
            connect(p_config, &ConfigRadio::InLoFreqSet, this, &SettingsRadio::InLoFreqSet);
            connect(p_config, &ConfigRadio::InHiFreqSet, this, &SettingsRadio::InHiFreqSet);
            connect(p_config, &ConfigRadio::DestructionSet, this, &SettingsRadio::DestructionSet);
            connect(p_config, &ConfigRadio::RingModFrequencySet, this, &SettingsRadio::RingModFrequencySet);
            connect(p_config, &ConfigRadio::RingModMixSet, this, &SettingsRadio::RingModMixSet);
            connect(p_config, &ConfigRadio::OutLoFreqSet, this, &SettingsRadio::OutLoFreqSet);
            connect(p_config, &ConfigRadio::OutHiFreqSet, this, &SettingsRadio::OutHiFreqSet);

            connect(p_config, &ConfigRadio::channel_closed, this, &SettingsRadio::on_channel_settings_finished);
            p_config->show();
            m_channel_configs.insert(server_channel,p_config);
        }
    }
    else if (type == PLUGIN_MENU_TYPE_CLIENT)
    {
        if (menuItemID == m_ContextMenuToggleClientBlacklisted)
            emit ToggleClientBlacklisted(serverConnectionHandlerID, (anyID)selectedItemID);
    }
}

void SettingsRadio::saveSettings(int r)
{
    Q_UNUSED(r);
    if (mP_radio)
    {
        QSettings cfg(TSHelpers::GetFullConfigPath(), QSettings::IniFormat);
        cfg.beginGroup(mP_radio->objectName());
        {
            auto settings_map = mP_radio->GetSettingsMap();
            for (auto i = settings_map.constBegin(); i != settings_map.constEnd(); ++i)
            {
                QString name = i.key();
                if (name == "Home")
                    name = "HomeTab";

                cfg.beginGroup(name);
                auto settings = i.value();
                cfg.setValue("enabled",settings.enabled);
                cfg.setValue("low_freq",settings.freq_low);
                cfg.setValue("high_freq",settings.freq_hi);
                cfg.setValue("o_freq_lo",settings.o_freq_lo);
                cfg.setValue("o_freq_hi",settings.o_freq_hi);
                cfg.setValue("fudge",settings.fudge);
                cfg.beginGroup("RingMod");
                cfg.setValue("rm_mod_freq",settings.rm_mod_freq);
                cfg.setValue("rm_mix",settings.rm_mix);
                cfg.endGroup();
                cfg.endGroup();
            }
        }
        cfg.endGroup();
    }
}

void SettingsRadio::on_channel_settings_finished(int r, QString setting_id)
{
    // clean up and close widget
    const auto kSchId = m_servers_info.find_server_by_unique_id(setting_id.left(28));    // 28 length always? or always ends with = ?
    if (kSchId)
    {
        auto channel_id = TSHelpers::GetChannelIDFromPath(kSchId, setting_id.right(setting_id.length() - 28));
        if (channel_id)
        {
            //TSLogging::Log(QString("serverid: %1 channelid: %2").arg(serverConnectionHandlerID).arg(channel_id));

            auto server_channel = qMakePair<uint64, uint64>(kSchId, channel_id);
            if (!m_channel_configs.contains(server_channel))
            {
                TSLogging::Error("Could not remove setting dialog from map");
            }
            else
                m_channel_configs.remove(server_channel);
        }
    }

    // Settings
    auto do_save = true;
    QSettings cfg(TSHelpers::GetFullConfigPath(), QSettings::IniFormat);
    cfg.beginGroup(mP_radio->objectName());

    if (r == QDialog::DialogCode::Accepted) // delete button
    {
        TSLogging::Log(QString("Removing setting %1").arg(setting_id));

        // remove from setting
        cfg.remove(setting_id);

        // This makes me wish for a redesign
        auto& settings_map = mP_radio->GetSettingsMapRef();
        if (settings_map.contains(setting_id))
            settings_map.remove(setting_id);

        // TODO Update all current talkers
    }
    else    // save / create
    {
        TSLogging::Log(QString("Save channel settings: %1").arg(setting_id));

        if (!(cfg.childGroups().contains(setting_id)))
        {   // if not enabled remove setting (aka don't create setting) to not pollute for every dialog open
            // This makes me wish for a redesign
            auto& settings_map = mP_radio->GetSettingsMapRef();
            if ((settings_map.contains(setting_id)) && (!settings_map.value(setting_id).enabled))
            {
                settings_map.remove(setting_id);
                do_save = false;
            }
        }
    }

    cfg.endGroup();
    if (do_save)
        this->saveSettings(NULL);
}
