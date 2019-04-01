/*
 * TeamSpeak RadioFX plugin
 *
 * Author: Thorsten Weinz (philosound@gmail.com)
 */

#pragma once

#ifdef WIN32
#define PLUGINS_EXPORTDLL __declspec(dllexport)
#else
#define PLUGINS_EXPORTDLL __attribute__ ((visibility("default")))
#endif

#include "teamspeak/public_definitions.h"
#include "plugin_definitions.h"

extern struct TS3Functions ts3Functions;

#ifdef __cplusplus
extern "C" {
#endif

/* Required functions */
PLUGINS_EXPORTDLL const char* ts3plugin_name();
PLUGINS_EXPORTDLL const char* ts3plugin_version();
PLUGINS_EXPORTDLL int ts3plugin_apiVersion();
PLUGINS_EXPORTDLL const char* ts3plugin_author();
PLUGINS_EXPORTDLL const char* ts3plugin_description();
PLUGINS_EXPORTDLL void ts3plugin_setFunctionPointers(const struct TS3Functions funcs);
PLUGINS_EXPORTDLL int ts3plugin_init();
PLUGINS_EXPORTDLL void ts3plugin_shutdown();

/* Optional functions */
PLUGINS_EXPORTDLL int ts3plugin_offersConfigure();
PLUGINS_EXPORTDLL void ts3plugin_registerPluginID(const char* id);
PLUGINS_EXPORTDLL const char* ts3plugin_commandKeyword();
PLUGINS_EXPORTDLL int ts3plugin_processCommand(uint64 serverConnectionHandlerID, const char* command);
PLUGINS_EXPORTDLL void ts3plugin_currentServerConnectionChanged(uint64 serverConnectionHandlerID);
PLUGINS_EXPORTDLL const char* ts3plugin_infoTitle();
PLUGINS_EXPORTDLL void ts3plugin_infoData(uint64 serverConnectionHandlerID, uint64 id, enum PluginItemType type, char** data);
PLUGINS_EXPORTDLL void ts3plugin_freeMemory(void* data);
PLUGINS_EXPORTDLL void ts3plugin_initMenus(struct PluginMenuItem*** menuItems, char** menuIcon);

/* Clientlib */
PLUGINS_EXPORTDLL void ts3plugin_onConnectStatusChangeEvent(uint64 serverConnectionHandlerID, int newStatus, unsigned int errorNumber);
PLUGINS_EXPORTDLL void ts3plugin_onClientMoveEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, const char* moveMessage);
PLUGINS_EXPORTDLL void ts3plugin_onClientMoveTimeoutEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, const char* timeoutMessage);
PLUGINS_EXPORTDLL void ts3plugin_onClientMoveMovedEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, anyID moverID, const char* moverName, const char* moverUniqueIdentifier, const char* moveMessage);
PLUGINS_EXPORTDLL int  ts3plugin_onServerErrorEvent(uint64 serverConnectionHandlerID, const char* errorMessage, unsigned int error, const char* returnCode, const char* extraMessage);
PLUGINS_EXPORTDLL void ts3plugin_onTalkStatusChangeEvent(uint64 serverConnectionHandlerID, int status, int isReceivedWhisper, anyID clientID);
PLUGINS_EXPORTDLL void ts3plugin_onEditPlaybackVoiceDataEvent(uint64 serverConnectionHandlerID, anyID clientID, short* samples, int sampleCount, int channels);

/* Clientlib rare */
PLUGINS_EXPORTDLL void ts3plugin_onClientSelfVariableUpdateEvent(uint64 serverConnectionHandlerID, int flag, const char* oldValue, const char* newValue);
PLUGINS_EXPORTDLL void ts3plugin_onServerGroupListEvent(uint64 serverConnectionHandlerID, uint64 serverGroupID, const char* name, int type, int iconID, int saveDB);
PLUGINS_EXPORTDLL void ts3plugin_onServerGroupListFinishedEvent(uint64 serverConnectionHandlerID);
PLUGINS_EXPORTDLL void ts3plugin_onChannelGroupListEvent(uint64 serverConnectionHandlerID, uint64 channelGroupID, const char* name, int type, int iconID, int saveDB);
PLUGINS_EXPORTDLL void ts3plugin_onChannelGroupListFinishedEvent(uint64 serverConnectionHandlerID);

/* Client UI callbacks */
PLUGINS_EXPORTDLL void ts3plugin_onMenuItemEvent(uint64 serverConnectionHandlerID, enum PluginMenuType type, int menuItemID, uint64 selectedItemID);

#ifdef __cplusplus
}
#endif
