#include "plugin_qt.h"

#include "teamspeak/clientlib_publicdefinitions.h"

#include "core/ts_serversinfo.h"

#include "mod_radio.h"
#include "settings_radio.h"

const char* Plugin::kPluginName = "TokoVoip";
const char* Plugin::kPluginVersion = "1.2.4";
const char* Plugin::kPluginAuthor = "Itokoyamato, Thorsten Weinz";
const char* Plugin::kPluginDescription = "Features:\n- TokoVoip\n- Radio FX\n";

Plugin::Plugin(const char* plugin_id, QObject *parent)
	: Plugin_Base(plugin_id, parent)
	, m_servers_info(new TSServersInfo(this))
	, m_radio(new Radio(*m_servers_info, talkers(), plugin_id, this))
	, m_settings_radio(new SettingsRadio(*m_servers_info, this))
{}

/* required functions */

int Plugin::initialize()
{
	context_menu().setMainIcon("ct_16x16.png");
	translator().update();
	m_settings_radio->Init(m_radio);
	return 0;
}

void Plugin::shutdown()
{
	m_radio->getTokovoip().shutdown();
	m_settings_radio->shutdown();
}

/* optional functions */

void Plugin::on_current_server_connection_changed(uint64 sch_id)
{
	m_radio->setHomeId(sch_id);
}

void Plugin::on_connect_status_changed(uint64 sch_id, int new_status, unsigned int error_number)
{
	m_servers_info->onConnectStatusChangeEvent(sch_id, new_status, error_number);
}

void Plugin::on_client_move(uint64 sch_id, anyID client_id, uint64 old_channel_id, uint64 new_channel_id, int visibility, anyID my_id, const char * move_message)
{
	Q_UNUSED(move_message);
	// TODO
}

void Plugin::on_client_move_timeout(uint64 sch_id, anyID client_id, uint64 old_channel_id, anyID my_id, const char * timeout_message)
{
	Q_UNUSED(timeout_message);
	// TODO
}

void Plugin::on_client_move_moved(uint64 sch_id, anyID client_id, uint64 old_channel_id, uint64 new_channel_id, int visibility, anyID my_id, anyID mover_id, const char * mover_name, const char * mover_unique_id, const char * move_message)
{
	Q_UNUSED(mover_id);
	Q_UNUSED(mover_name);
	Q_UNUSED(mover_unique_id);
	Q_UNUSED(move_message);
	// TODO
}

void Plugin::on_talk_status_changed(uint64 sch_id, int status, int is_received_whisper, anyID client_id, bool is_me)
{
	/*const auto kIsRadioProcessing =*/ m_radio->onTalkStatusChanged(sch_id, status, is_received_whisper, client_id, is_me);
}

void Plugin::on_playback_pre_process(uint64 sch_id, anyID client_id, short* samples, int frame_count, int channels)
{
	m_radio->onEditPlaybackVoiceDataEvent(sch_id, client_id, samples, frame_count, channels);
	//agmu.onEditPlaybackVoiceDataEvent(sch_id, client_id, samples, frame_count, channels);
}

void Plugin::on_server_group_list(uint64 sch_id, uint64 server_group_id, const char * name, int type, int icon_id, int save_db)
{
	m_servers_info->onServerGroupListEvent(sch_id, server_group_id, name, type, icon_id, save_db);
}

void Plugin::on_server_group_list_finished(uint64 sch_id)
{
	m_servers_info->onServerGroupListFinishedEvent(sch_id);
}

void Plugin::on_channel_group_list(uint64 sch_id, uint64 channel_group_id, const char * name, int type, int icon_id, int save_db)
{
	m_servers_info->onChannelGroupListEvent(sch_id, channel_group_id, name, type, icon_id, save_db);
}

void Plugin::on_channel_group_list_finished(uint64 sch_id)
{
	m_servers_info->onChannelGroupListFinishedEvent(sch_id);
}
