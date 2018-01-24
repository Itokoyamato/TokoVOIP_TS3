/*
 * Author: Thorsten Weinz (philosound@gmail.com)
 */

#ifdef _WIN32
#pragma warning (disable : 4100)  /* Disable Unreferenced parameter warning */
#endif

#include <memory>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <QtCore/qglobal.h>

#include "teamspeak/public_errors.h"
#include "teamspeak/public_errors_rare.h"
#include "teamspeak/public_definitions.h"
#include "teamspeak/public_rare_definitions.h"
#include "ts3_functions.h"
#include "plugin.h"
#include "plugin_qt.h"

#include "core/ts_logging_qt.h"
#include "core/ts_settings_qt.h"
#include "core/ts_helpers_qt.h"

struct TS3Functions ts3Functions;

std::unique_ptr<Plugin> plugin;

#ifdef _WIN32
#define _strcpy(dest, destSize, src) strcpy_s(dest, destSize, src)
#define snprintf sprintf_s
#else
#define _strcpy(dest, destSize, src) { strncpy(dest, src, destSize-1); dest[destSize-1] = '\0'; }
#endif

#define PATH_BUFSIZE 512
#define COMMAND_BUFSIZE 128
#define SERVERINFO_BUFSIZE 256
#define CHANNELINFO_BUFSIZE 512
#define RETURNCODE_BUFSIZE 128

QMutex command_mutex;

/*********************************** Required functions ************************************/
/*
 * If any of these required functions is not implemented, TS3 will refuse to load the plugin
 */

/* Unique name identifying this plugin */
const char* ts3plugin_name() { return plugin->kPluginName; }

/* Plugin version */
const char* ts3plugin_version() { return plugin->kPluginVersion; }

/* Plugin API version. Must be the same as the clients API major version, else the plugin fails to load. */
int ts3plugin_apiVersion() { return plugin->kPluginApiVersion; }

/* Plugin author */
const char* ts3plugin_author() { return plugin->kPluginAuthor; }

/* Plugin description */
const char* ts3plugin_description() { return plugin->kPluginDescription; }

/* Set TeamSpeak 3 callback functions */
void ts3plugin_setFunctionPointers(const struct TS3Functions funcs) {
    ts3Functions = funcs;
}

/*
 * Custom code called right after loading the plugin. Returns 0 on success, 1 on failure.
 * If the function returns 1 on failure, the plugin will be unloaded again.
 */
int ts3plugin_init() { return plugin->init();  /* 0 = success, 1 = failure */ }

/* Custom code called right before the plugin is unloaded */
void ts3plugin_shutdown()
{
	plugin->shutdown();
	plugin.release();
}

/****************************** Optional functions ********************************/
/*
 * Following functions are optional, if not needed you don't need to implement them.
 */

/* Tell client if plugin offers a configuration window. If this function is not implemented, it's an assumed "does not offer" (PLUGIN_OFFERS_NO_CONFIGURE). */
int ts3plugin_offersConfigure() { return plugin->kPluginOffersConfigure; }

/*
 * If the plugin wants to use error return codes or plugin commands, it needs to register a command ID. This function will be automatically
 * called after the plugin was initialized. This function is optional. If you don't use error return codes or plugin commands, the function
 * can be omitted.
 * Note the passed pluginID parameter is no longer valid after calling this function, so you must copy it and store it in the plugin.
 */
void ts3plugin_registerPluginID(const char* id) { plugin = std::make_unique<Plugin>(id); }

/* Plugin command keyword. Return NULL or "" if not used. */
const char* ts3plugin_commandKeyword() {
    return "radio";
}

/* Plugin processes console command. Return 0 if plugin handled the command, 1 if not handled. */
int ts3plugin_processCommand(uint64 serverConnectionHandlerID, const char* command) { return 0; }

/*
 * Initialize plugin menus.
 * This function is called after ts3plugin_init and ts3plugin_registerPluginID. A pluginID is required for plugin menus to work.
 * Both ts3plugin_registerPluginID and ts3plugin_freeMemory must be implemented to use menus.
 * If plugin menus are not used by a plugin, do not implement this function or return NULL.
 */
void ts3plugin_initMenus(struct PluginMenuItem*** menuItems, char** menuIcon)
{
	plugin->context_menu().onInitMenus(menuItems, menuIcon);
    /*
     * Create the menus
     * There are three types of menu items:
     * - PLUGIN_MENU_TYPE_CLIENT:  Client context menu
     * - PLUGIN_MENU_TYPE_CHANNEL: Channel context menu
     * - PLUGIN_MENU_TYPE_GLOBAL:  "Plugins" menu in menu bar of main window
     *
     * Menu IDs are used to identify the menu item when ts3plugin_onMenuItemEvent is called
     *
     * The menu text is required, max length is 128 characters
     *
     * The icon is optional, max length is 128 characters. When not using icons, just pass an empty string.
     * Icons are loaded from a subdirectory in the TeamSpeak client plugins folder. The subdirectory must be named like the
     * plugin filename, without dll/so/dylib suffix
     * e.g. for "test_plugin.dll", icon "1.png" is loaded from <TeamSpeak 3 Client install dir>\plugins\test_plugin\1.png
     */

    /*
     * Menus can be enabled or disabled with: ts3Functions.setPluginMenuEnabled(pluginID, menuID, 0|1);
     * Test it with plugin command: /test enablemenu <menuID> <0|1>
     * Menus are enabled by default. Please note that shown menus will not automatically enable or disable when calling this function to
     * ensure Qt menus are not modified by any thread other the UI thread. The enabled or disable state will change the next time a
     * menu is displayed.
     */
    /* For example, this would disable MENU_ID_GLOBAL_2: */
    /* ts3Functions.setPluginMenuEnabled(pluginID, MENU_ID_GLOBAL_2, 0); */

    /* All memory allocated in this function will be automatically released by the TeamSpeak client later by calling ts3plugin_freeMemory */
}

/*
* Implement the following three functions when the plugin should display a line in the server,channel,client info.
* If any of ts3plugin_infoTitle, ts3plugin_infoData or ts3plugin_freeMemory is missing, the info text will not be displayed.
*/

/* Static title shown in the left column in the info frame */
const char* ts3plugin_infoTitle() { return ts3plugin_name(); }

/*
 * Dynamic content shown in the right column in the info frame. Memory for the data string needs to be allocated in this
 * function. The client will call ts3plugin_freeMemory once done with the string to release the allocated memory again.
 * Check the parameter "type" if you want to implement this feature only for specific item types. Set the parameter
 * "data" to NULL to have the client ignore the info data.
 */
void ts3plugin_infoData(uint64 serverConnectionHandlerID, uint64 id, enum PluginItemType type, char** data)
{
	plugin->info_data().onInfoData(serverConnectionHandlerID, id, type, data);
	//don't add code here when using infoData. It's all been done.
}

/* Required to release the memory for parameter "data" allocated in ts3plugin_infoData */
void ts3plugin_freeMemory(void* data) {
    free(data);
}

/* Show an error message if the plugin failed to load */
void ts3plugin_onConnectStatusChangeEvent(uint64 serverConnectionHandlerID, int newStatus, unsigned int errorNumber)
{
	plugin->onConnectStatusChangeEvent(serverConnectionHandlerID, newStatus, errorNumber);
}

void ts3plugin_onClientMoveEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, const char* moveMessage)
{
	plugin->onClientMoveEvent(serverConnectionHandlerID, clientID, oldChannelID, newChannelID, visibility, moveMessage);
}

void ts3plugin_onClientMoveTimeoutEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, const char* timeoutMessage)
{
	plugin->onClientMoveTimeoutEvent(serverConnectionHandlerID, clientID, oldChannelID, newChannelID, visibility, timeoutMessage);
}

void ts3plugin_onClientMoveMovedEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, anyID moverID, const char* moverName, const char* moverUniqueIdentifier, const char* moveMessage)
{
	plugin->onClientMoveMovedEvent(serverConnectionHandlerID, clientID, oldChannelID, newChannelID, visibility, moverID, moverName, moverUniqueIdentifier, moveMessage);
}

int ts3plugin_onServerErrorEvent(uint64 serverConnectionHandlerID, const char* errorMessage, unsigned int error, const char* returnCode, const char* extraMessage)
{
    //TSLogging::Print(QString("onServerErrorEvent: %1 %2 %3").arg((returnCode ? returnCode : "")).arg(error).arg(errorMessage),serverConnectionHandlerID,LogLevel_DEBUG);
    int isHandledError = 0;
    if (returnCode)
	{
        //TSLogging::Print("have returnCode");
//        /* A plugin could now check the returnCode with previously (when calling a function) remembered returnCodes and react accordingly */
//        /* In case of using a a plugin return code, the plugin can return:
//         * 0: Client will continue handling this error (print to chat tab)
//         * 1: Client will ignore this error, the plugin announces it has handled it */

        return 0;
    }

    return isHandledError;  /* If no plugin return code was used, the return value of this function is ignored */
}

void ts3plugin_onTalkStatusChangeEvent(uint64 serverConnectionHandlerID, int status, int isReceivedWhisper, anyID clientID)
{
	plugin->onTalkStatusChangeEvent(serverConnectionHandlerID, status, isReceivedWhisper, clientID);
}

void ts3plugin_onEditPlaybackVoiceDataEvent(uint64 serverConnectionHandlerID, anyID clientID, short* samples, int sampleCount, int channels)
{
	plugin->onEditPlaybackVoiceDataEvent(serverConnectionHandlerID, clientID, samples, sampleCount, channels);
}

/* Clientlib rare */
void ts3plugin_onClientSelfVariableUpdateEvent(uint64 serverConnectionHandlerID, int flag, const char* oldValue, const char* newValue)
{
    // seems to be equal to a ts3plugin_currentServerConnectionChanged when initiated by the user
    // except that this version will fire when returning from a cross server ptt event
    // (ts3plugin_currentServerConnectionChanged is not even fired when ptt-switching for whatever reason)
    // Furtunately, we can make use of that difference.
}

/* Client changed current server connection handler */
void ts3plugin_currentServerConnectionChanged(uint64 serverConnectionHandlerID)
{
	plugin->currentServerConnectionChanged(serverConnectionHandlerID);
}

/* Client UI callbacks */

/*
 * Called when a plugin menu item (see ts3plugin_initMenus) is triggered. Optional function, when not using plugin menus, do not implement this.
 *
 * Parameters:
 * - serverConnectionHandlerID: ID of the current server tab
 * - type: Type of the menu (PLUGIN_MENU_TYPE_CHANNEL, PLUGIN_MENU_TYPE_CLIENT or PLUGIN_MENU_TYPE_GLOBAL)
 * - menuItemID: Id used when creating the menu item
 * - selectedItemID: Channel or Client ID in the case of PLUGIN_MENU_TYPE_CHANNEL and PLUGIN_MENU_TYPE_CLIENT. 0 for PLUGIN_MENU_TYPE_GLOBAL.
 */
void ts3plugin_onMenuItemEvent(uint64 serverConnectionHandlerID, enum PluginMenuType type, int menuItemID, uint64 selectedItemID)
{
	plugin->onMenuItemEvent(serverConnectionHandlerID, type, menuItemID, selectedItemID);
}

/*void ts3plugin_onPluginCommandEvent(uint64 serverConnectionHandlerID, const char* pluginName, const char* pluginCommand)
{
    // pluginName is the dll name minus _someOS, not ts3plugin_name();
    // if the user for whatever reason renames the dll, this breaks the code
    if (QString(pluginName) != QString(ts3plugin_name()).toLower())
        return;
}*/

void ts3plugin_onServerGroupListEvent(uint64 serverConnectionHandlerID, uint64 serverGroupID, const char* name, int type, int iconID, int saveDB)
{
	plugin->on_server_group_list(serverConnectionHandlerID, serverGroupID, name, type, iconID, saveDB);
}

void ts3plugin_onServerGroupListFinishedEvent(uint64 serverConnectionHandlerID)
{
	plugin->on_server_group_list_finished(serverConnectionHandlerID);
}

void ts3plugin_onChannelGroupListEvent(uint64 serverConnectionHandlerID, uint64 channelGroupID, const char* name, int type, int iconID, int saveDB)
{
	plugin->on_channel_group_list(serverConnectionHandlerID, channelGroupID, name, type, iconID, saveDB);
}

void ts3plugin_onChannelGroupListFinishedEvent(uint64 serverConnectionHandlerID)
{
	plugin->on_channel_group_list_finished(serverConnectionHandlerID);
}
