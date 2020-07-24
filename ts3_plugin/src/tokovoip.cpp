#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "core/ts_helpers_qt.h"
#include "core/ts_serversinfo.h"
#include "core/ts_helpers_qt.h"
#include "core/ts_logging_qt.h"

#include <QDesktopServices>
#include <QUrl>

#include "plugin.h" //pluginID
#include "ts3_functions.h"
#include "core/ts_serversinfo.h"
#include <teamspeak/public_errors.h>

#include "core/plugin_base.h"
#include "mod_radio.h"

#include "tokovoip.h"
#include "client_ws.hpp"

#include "httplib.h"
using namespace httplib;

Tokovoip *tokovoip;
using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;

int isRunning = 0;

HANDLE threadWebSocket = INVALID_HANDLE_VALUE;
bool exitWebSocketThread = FALSE;
shared_ptr<WsClient::Connection> wsConnection;

bool isTalking = false;
char* originalName = "";
time_t lastNameSetTick = 0;
string mainChannel = "";
string waitChannel = "";
time_t lastChannelJoin = 0;
string clientIP = "";
time_t lastWSConnection = 0;

time_t noiseWait = 0;

int connectButtonId;
int disconnectButtonId;
int unmuteButtonId;
int supportButtonId;
int projectButtonId;
bool isPTT = true;

float defaultMicClicksVolume = -15;
int oldClickVolume;

int handleMessage(shared_ptr<WsClient::Connection> connection, string message_str) {
	int currentPluginStatus = 1;

	if (!isConnected(ts3Functions.getCurrentServerConnectionHandlerID()))
	{
		tokovoip->setProcessingState(false, currentPluginStatus);
		return (0);
	}

	DWORD error;
	anyID clientId;
	uint64 serverId = ts3Functions.getCurrentServerConnectionHandlerID();
	std::vector<anyID> clients = getChannelClients(serverId, getCurrentChannel(serverId));
	string thisChannelName = getChannelName(serverId, getMyId(serverId));

	//--------------------------------------------------------

	// Check if connected to any channel
	if (thisChannelName == "")
	{
		tokovoip->setProcessingState(false, currentPluginStatus);
		return (0);
	}

	//--------------------------------------------------------

	// Load the json //
	json json_data = json::parse(message_str.c_str(), nullptr, false);
	if (json_data.is_discarded()) {
		ts3Functions.logMessage("Invalid JSON data", LogLevel_INFO, "TokoVOIP", 0);
		tokovoip->setProcessingState(false, currentPluginStatus);
		return (0);
	}

	json data = json_data["Users"];
	string channelName = json_data["TSChannel"];
	string channelPass = json_data["TSPassword"];
	string waitingChannelName = json_data["TSChannelWait"];
	mainChannel = channelName;
	waitChannel = waitingChannelName;

	bool radioTalking = json_data["radioTalking"];
	bool radioClicks = json_data["localRadioClicks"];
	bool local_click_on = json_data["local_click_on"];
	bool local_click_off = json_data["local_click_off"];
	bool remote_click_on = json_data["remote_click_on"];
	bool remote_click_off = json_data["remote_click_off"];

	int clickVolume;
	if (json_data["ClickVolume"].is_number()) clickVolume = json_data["ClickVolume"];
	if (json_data["ClickVolume"].is_string()) clickVolume = stoi((string)json_data["ClickVolume"]);

	if (clickVolume && clickVolume != oldClickVolume) {
		oldClickVolume = clickVolume;
		ts3Functions.setPlaybackConfigValue(ts3Functions.getCurrentServerConnectionHandlerID(), "volume_factor_wave", to_string(clickVolume).c_str());
	}

	//--------------------------------------------------------

	if (isChannelWhitelisted(json_data, thisChannelName)) {
		resetClientsAll();
		currentPluginStatus = 3;
		tokovoip->setProcessingState(false, currentPluginStatus);
		return (0);
	}

	// Check if right channel
	if (channelName != thisChannelName) {
		if (originalName != "")
			setClientName(originalName);

		// Handle auto channel switch
		if (thisChannelName == waitingChannelName)
		{
			if (noiseWait == 0 || (time(nullptr) - noiseWait) > 1)
				noiseWait = time(nullptr);
			uint64* result;
			if ((error = ts3Functions.getChannelList(serverId, &result)) != ERROR_ok)
			{
				outputLog("Can't get channel list", error);
			}
			else
			{
				bool joined = false;
				uint64* iter = result;
				while (*iter && !joined && (time(nullptr) - lastChannelJoin) > 1)
				{
					uint64 channelId = *iter;
					iter++;
					char* cName;
					if ((error = ts3Functions.getChannelVariableAsString(serverId, channelId, CHANNEL_NAME, &cName)) != ERROR_ok) {
						outputLog("Can't get channel name", error);
					}
					else
					{
						if (!strcmp(channelName.c_str(), cName))
						{
							if (time(nullptr) - noiseWait < 1)
							{
								std::vector<anyID> channelClients = getChannelClients(serverId, channelId);
								for (auto clientIdIterator = channelClients.begin(); clientIdIterator != channelClients.end(); clientIdIterator++)
								{
									setClientMuteStatus(serverId, *clientIdIterator, true);
								}
								tokovoip->setProcessingState(false, currentPluginStatus);
								return (0);
							}
							lastChannelJoin = time(nullptr);
							if ((error = ts3Functions.requestClientMove(serverId, getMyId(serverId), channelId, channelPass.c_str(), NULL)) != ERROR_ok) {
								outputLog("Can't join channel", error);
								currentPluginStatus = 2;
								tokovoip->setProcessingState(false, currentPluginStatus);
								return (0);
							}
							else
							{
								joined = true;
							}
						}
						ts3Functions.freeMemory(cName);
					}
				}
				ts3Functions.freeMemory(result);
			}
		}
		else
		{
			resetClientsAll();
			currentPluginStatus = 2;
			tokovoip->setProcessingState(false, currentPluginStatus);
			return (0);
		}
	}

	serverId = ts3Functions.getCurrentServerConnectionHandlerID();

	// Save client's original name
	if (originalName == "")
		if ((error = ts3Functions.getClientVariableAsString(serverId, getMyId(serverId), CLIENT_NICKNAME, &originalName)) != ERROR_ok) {
			outputLog("Error getting client nickname", error);
			tokovoip->setProcessingState(false, currentPluginStatus);
			return (0);
		}

	// Set client's name to ingame name
	string newName = originalName;

	if (json_data.find("localName") != json_data.end()) {
		string localName = json_data["localName"];
		if (localName != "") newName = localName;
	}

	if (json_data.find("localNamePrefix") != json_data.end()) {
		string localNamePrefix = json_data["localNamePrefix"];
		if (localNamePrefix != "") newName = localNamePrefix + newName;
	}

	if (newName != "") {
		setClientName(newName);
	}

	// Activate input if talking on radio
	if (radioTalking)
	{
		setClientTalking(true);
		if (!isTalking && radioClicks == true && local_click_on == true)
			playWavFile("mic_click_on");
		isTalking = true;
	}
	else
	{
		if (isTalking)
		{
			setClientTalking(false);
			if (radioClicks == true && local_click_off == true)
				playWavFile("mic_click_off");
			isTalking = false;
		}
	}

	// Handle positional audio
	if (json_data.find("posX") != json_data.end() && json_data.find("posY") != json_data.end() && json_data.find("posZ") != json_data.end()) {
		TS3_VECTOR myPosition;
		myPosition.x = (float)json_data["posX"];
		myPosition.y = (float)json_data["posY"];
		myPosition.z = (float)json_data["posZ"];
		ts3Functions.systemset3DListenerAttributes(serverId, &myPosition, NULL, NULL);
	}

	// Process other clients
	for (auto clientIdIterator = clients.begin(); clientIdIterator != clients.end(); clientIdIterator++) {
		clientId = *clientIdIterator;
		char *UUID;
		if ((error = ts3Functions.getClientVariableAsString(serverId, clientId, CLIENT_UNIQUE_IDENTIFIER, &UUID)) != ERROR_ok) {
			outputLog("Error getting client UUID", error);
		} else {
			bool foundPlayer = false;
			if (clientId == getMyId(serverId)) {
				foundPlayer = true;
				continue;
			}
			for (auto user : data) {
				if (!user.is_object()) continue;
				if (!user["uuid"].is_string()) continue;

				string gameUUID = user["uuid"];
				int muted = user["muted"];
				float volume = user["volume"];
				bool isRadioEffect = user["radioEffect"];

				if (channelName == thisChannelName && UUID == gameUUID) {
					foundPlayer = true;
					if (isRadioEffect == true && tokovoip->getRadioData(UUID) == false && remote_click_on == true)
						playWavFile("mic_click_on");
					if (remote_click_off == true && isRadioEffect == false && tokovoip->getRadioData(UUID) == true && clientId != getMyId(serverId))
						playWavFile("mic_click_off");
					tokovoip->setRadioData(UUID, isRadioEffect);
					if (muted)
						setClientMuteStatus(serverId, clientId, true);
					else {
						setClientMuteStatus(serverId, clientId, false);
						ts3Functions.setClientVolumeModifier(serverId, clientId, volume);
						if (json_data.find("posX") != json_data.end() && json_data.find("posY") != json_data.end() && json_data.find("posZ") != json_data.end()) {
							TS3_VECTOR Vector;
							Vector.x = (float)user["posX"];
							Vector.y = (float)user["posY"];
							Vector.z = (float)user["posZ"];
							ts3Functions.channelset3DAttributes(serverId, clientId, &Vector);
						}
					}
				}
			};
			// Checks if ts3 user is on fivem server. Fixes onesync infinity issues as big mode is using
			// player culling (Removes a player from your game if he is far away), this fix should mute him when he is removed from your game (too far away)

			// Also keep in mind teamspeak3 defautly mutes everyone in channel if theres more than 100 people,
			// this can be changed in the server settings -> Misc -> Min clients in channel before silence

			if (!foundPlayer) setClientMuteStatus(serverId, clientId, true);
			ts3Functions.freeMemory(UUID);
		}
	}
	currentPluginStatus = 3;
	tokovoip->setProcessingState(false, currentPluginStatus);
}

int tries = 0;
void tokovoipProcess() {
	tries = 0;
	while (true) {
		++tries;
		uint64 serverId = ts3Functions.getCurrentServerConnectionHandlerID();
		if (isConnected(serverId)) break;
		if (tries > 5) {
			outputLog("WebsocketServer: Not connected to a TS server.");
			return;
		}
		Sleep(1000);
	}

	string endpoint = getWebSocketEndpoint();
	if (endpoint == "") {
		outputLog("WebsocketServer: Failed to retrieve the websocket endpoint.");
		return;
	}

	char *UUID;
	uint64 serverId = ts3Functions.getCurrentServerConnectionHandlerID();
	if (ts3Functions.getClientSelfVariableAsString(serverId, CLIENT_UNIQUE_IDENTIFIER, &UUID) != ERROR_ok) {
		outputLog("WebsocketServer: Failed to get UUID");
		return;
	}

	WsClient client(endpoint + "&uuid=" + (string)UUID);
	lastWSConnection = time(nullptr);

	client.on_message = [](shared_ptr<WsClient::Connection> connection, shared_ptr<WsClient::InMessage> in_message) {
		string message = in_message->string();
		if (message.find("processTokovoip") != string::npos) {
			int pos = message.find("42[\"");
			message.erase(pos, 2);
			json json_data = json::parse(message, nullptr, false);
			if (json_data.is_discarded()) return;
			handleMessage(connection, json_data[1].dump());
		} else if (message.find("disconnect") != string::npos) outputLog(message);
		else if (message.find("ping") != string::npos) sendWSMessage("pong", "{}");
		else outputLog(message);
	};

	client.on_open = [&](shared_ptr<WsClient::Connection> connection) {
		outputLog("WebsocketServer: connection opened");
		wsConnection = connection;

		json data = {
			{ "key", "pluginVersion" },
			{ "value", (string)ts3plugin_version() },
		};
		sendWSMessage("setTS3Data", data);

		data = {
			{ "key", "uuid" },
			{ "value", (string)UUID },
		};
		sendWSMessage("setTS3Data", data);

		data = {
				{ "key", "pluginStatus" },
				{ "value", 0 },
		};
		sendWSMessage("setTS3Data", data);
	};

	client.on_close = [&](shared_ptr<WsClient::Connection>, int status, const string &) {
		outputLog("WebsocketServer: connection closed: " + to_string(status));
		client.stop();
		resetState();
		if (exitWebSocketThread) return;
		int sleepTime = (5 - (time(nullptr) - lastWSConnection)) * 1000;
		if (sleepTime > 0) Sleep(sleepTime);
		initWebSocket(true);
	};

	client.on_error = [&](shared_ptr<WsClient::Connection>, const SimpleWeb::error_code &ec) {
		outputLog("WebsocketServer: error: " + ec.message());
		client.stop();
		resetState();
		if (exitWebSocketThread) return;
		int sleepTime = (5 - (time(nullptr) - lastWSConnection)) * 1000;
		if (sleepTime > 0) Sleep(sleepTime);
		initWebSocket(true);
	};

	client.start();
}
DWORD WINAPI WebSocketService(LPVOID lpParam) {
	updateWebsocketState();
	tokovoipProcess();
	updateWebsocketState(true, false);
	return NULL;
}

void initWebSocket(bool ignoreRunning) {
	if (!ignoreRunning && isWebsocketThreadRunning()) {
		outputLog("Tokovoip is already running or handshaking. You can force disconnect it via the menu Plugins->TokoVoip->Disconnect");
		return;
	}
	outputLog("Initializing WebSocket Thread", 0);
	exitWebSocketThread = false;
	threadWebSocket = CreateThread(NULL, 0, WebSocketService, NULL, 0, NULL);
}

string getWebSocketEndpoint() {
	json fivemServer = NULL;

	tries = 0;
	string verifyData = "";
	while (verifyData == "") {
		tries += 1;
		outputLog("Verifying TS server (attempt " + to_string(tries) + ")");
		verifyData = verifyTSServer();
		if (verifyData == "") {
			Sleep(5000);
			if(tries >= 2) {
				outputLog("Failed to verify TS server");
				return "";
			}
		}
	}
	clientIP = verifyData;
	if (clientIP == "") {
		outputLog("Failed to retrieve clientIP");
		return NULL;
	}

	outputLog("Successfully verified TS server");

	tries = 0;
	uint64 serverId = ts3Functions.getCurrentServerConnectionHandlerID();
	string channelOnBoot = getChannelName(serverId, getMyId(serverId));
	bool wasAutoBoot = stringIncludes(getChannelName(serverId, getMyId(serverId)), "tokovoip");
	while (fivemServer == NULL) {
		tries += 1;
		outputLog("Handshaking (attempt " + to_string(tries) + ")");
		fivemServer = handshake(clientIP);
		if (fivemServer == NULL) {
			Sleep(5000);
			bool inTokovoipChannel = stringIncludes((string)getChannelName(serverId, getMyId(serverId)), "tokovoip");
			// If auto detect boot, stop handhshaking once we leave the channel
			if (wasAutoBoot && !inTokovoipChannel) {
				outputLog("Not in tokovoip channel anymore, stopping handshake");
				return "";
			}
			// More retries if current channel has tokovoip in name, only 5 reties otherwise
			if (tries >= ((inTokovoipChannel) ? 60 : 5)) {
				outputLog("Failed to handshake");
				return "";
			}
		}
	}
	if (fivemServer == NULL) return "";

	outputLog("Successfully handshaked");

	json server = fivemServer["server"];
	if (!server["ip"].is_string()) {
		outputLog("Invalid type in WS server IP");
		return "";
	}
	string fivemServerIP = server["ip"];
	if (!server["port"].is_number()) {
		outputLog("Invalid type in WS server PORT");
		return "";
	}
	int fivemServerPORT = server["port"];

	return fivemServerIP + ":" + to_string(fivemServerPORT) + "/socket.io/?EIO=3&transport=websocket&from=ts3";
}

void resetChannel() {
	uint64 serverId = ts3Functions.getCurrentServerConnectionHandlerID();
	uint64* result;
	DWORD error;
	if ((error = ts3Functions.getChannelList(serverId, &result)) != ERROR_ok) {
		outputLog("resetChannel: Can't get channel list", error);
		return;
	}

	uint64* iter = result;
	while (*iter) {
		uint64 channelId = *iter;
		iter++;
		char* cName;
		if ((error = ts3Functions.getChannelVariableAsString(serverId, channelId, CHANNEL_NAME, &cName)) != ERROR_ok) {
			outputLog("resetChannel: Can't get channel name", error);
			continue;
		}
		if (!strcmp(waitChannel.c_str(), cName)) {
			if ((error = ts3Functions.requestClientMove(serverId, getMyId(serverId), channelId, "", NULL)) != ERROR_ok) {
				outputLog("resetChannel: Can't join channel", error);
				tokovoip->setProcessingState(false, 0);
				return;
			}
			break;
		}
		ts3Functions.freeMemory(cName);
	}
	ts3Functions.freeMemory(result);
}

void resetState() {
	wsConnection = NULL;
	updateWebsocketState();
	uint64 serverId = ts3Functions.getCurrentServerConnectionHandlerID();
	if (isConnected(serverId)) {
		string currentChannelName = getChannelName(serverId, getMyId(serverId));
		if (mainChannel == currentChannelName) resetChannel();
		resetClientsAll();
	}
	if (originalName != "") setClientName(originalName);
}


void sendWSMessage(string endpoint, json value) {
	if (!wsConnection) return;

	json send = json::array({ endpoint, value });
	wsConnection->send("42" + send.dump());
}

void checkUpdate() {
	json updateJSON;
	httplib::Client cli("master.tokovoip.itokoyamato.net");
	cli.set_follow_location(true);
	auto res = cli.Get("/version");
	if (res && (res->status == 200 || res->status == 301)) {
		updateJSON = json::parse(res->body, nullptr, false);
		if (updateJSON.is_discarded()) {
			outputLog("Downloaded JSON is invalid");
			return;
		}
	} else {
		outputLog("Couldn't retrieve JSON");
		return;
	}

	if (updateJSON != NULL) outputLog("Got update json");

	if (updateJSON == NULL || updateJSON.find("minVersion") == updateJSON.end() || updateJSON.find("currentVersion") == updateJSON.end()) {
		outputLog("Invalid update JSON");
		return;
	}

	string minVersion = updateJSON["minVersion"];
	string minVersionNum = updateJSON["minVersion"];
	minVersionNum.erase(remove(minVersionNum.begin(), minVersionNum.end(), '.'), minVersionNum.end());

	string currentVersion = updateJSON["currentVersion"];
	string currentVersionNum = updateJSON["currentVersion"];
	currentVersionNum.erase(remove(currentVersionNum.begin(), currentVersionNum.end(), '.'), currentVersionNum.end());

	string myVersion = ts3plugin_version();
	myVersion.erase(remove(myVersion.begin(), myVersion.end(), '.'), myVersion.end());

	string updateMessage;
	if (updateJSON.find("versions") != updateJSON.end() &&
		updateJSON["versions"].find(currentVersion) != updateJSON["versions"].end() &&
		updateJSON["versions"][currentVersion].find("updateMessage") != updateJSON["versions"][currentVersion].end()) {

		string str = updateJSON["versions"][currentVersion]["updateMessage"];
		updateMessage = str;
	} else {
		string str = updateJSON["defaultUpdateMessage"];
		updateMessage = str;
	}

	if (myVersion < currentVersionNum) {
		string url = "";
		if (updateJSON.find("versions") != updateJSON.end() &&
			updateJSON["versions"].find(currentVersion) != updateJSON["versions"].end() &&
			updateJSON["versions"][currentVersion].find("updateUrl") != updateJSON["versions"][currentVersion].end()) {

			string tmp = updateJSON["versions"][currentVersion]["updateUrl"];
			url = tmp;
		}

		MessageBox(NULL, updateMessage.c_str(), "TokoVOIP: update", MB_OK);

		if (myVersion < minVersionNum && updateJSON.find("minVersionWarningMessage") != updateJSON.end() && !updateJSON["minVersionWarningMessage"].is_null()) {
			string minVersionWarningMessage = updateJSON["minVersionWarningMessage"];
			MessageBox(NULL, minVersionWarningMessage.c_str(), "TokoVOIP: update", MB_OK);
		}
	}
}

string verifyTSServer() {
	uint64 serverId = ts3Functions.getCurrentServerConnectionHandlerID();
	unsigned int error;
	char* serverIP;

	if ((error = ts3Functions.getConnectionVariableAsString(serverId, getMyId(ts3Functions.getCurrentServerConnectionHandlerID()), CONNECTION_SERVER_IP, &serverIP)) != ERROR_ok) {
		if (error != ERROR_not_connected) ts3Functions.logMessage("Error querying server name", LogLevel_ERROR, "Plugin", serverId);
		return "";
	}

	httplib::Client cli("master.tokovoip.itokoyamato.net");
	string path = "/verify?address=" + string(serverIP);
	cli.set_follow_location(true);
	outputLog("Getting " + path);
	auto res = cli.Get(path.c_str());
	if (res && res->status == 200) return res->body;
	return "";
}

json handshake(string clientIP) {
	if (clientIP == "") return NULL;

	uint64 serverId = ts3Functions.getCurrentServerConnectionHandlerID();
	unsigned int error;

	httplib::Client cli("master.tokovoip.itokoyamato.net");
	string path = "/handshake?ip=" + string(clientIP);
	cli.set_follow_location(true);
	outputLog("Getting " + path);
	auto res = cli.Get(path.c_str());
	if (res && res->status == 200) {
		json parsedJSON = json::parse(res->body, nullptr, false);
		if (parsedJSON.is_discarded()) {
			outputLog("Handshake: Downloaded JSON is invalid");
			return NULL;
		}
		return parsedJSON;
	}
	return NULL;
}

void onButtonClicked(uint64 serverConnectionHandlerID, PluginMenuType type, int menuItemID, uint64 selectedItemID)
{
	if (type == PLUGIN_MENU_TYPE_GLOBAL) {
		if (menuItemID == connectButtonId) {
			initWebSocket();
		} else if (menuItemID == disconnectButtonId) {
			killWebsocketThread();
		} else if (menuItemID == unmuteButtonId) {
			unmuteAll(ts3Functions.getCurrentServerConnectionHandlerID());
		} else if (menuItemID == supportButtonId) {
			QDesktopServices::openUrl(QUrl("https://patreon.com/Itokoyamato"));
		} else if (menuItemID == projectButtonId) {
			QDesktopServices::openUrl(QUrl("https://github.com/Itokoyamato/TokoVOIP_TS3"));
		}
	}
}

int Tokovoip::initialize(char *id, QObject* parent) {
	if (isRunning != 0)
		return (0);

	plugin = qobject_cast<Plugin_Base*>(parent);
	auto& context_menu = plugin->context_menu();
	connectButtonId = context_menu.Register(plugin, PLUGIN_MENU_TYPE_GLOBAL, "Connect", "");
	disconnectButtonId = context_menu.Register(plugin, PLUGIN_MENU_TYPE_GLOBAL, "Disconnect", "");
	unmuteButtonId = context_menu.Register(plugin, PLUGIN_MENU_TYPE_GLOBAL, "Unmute All", "");
	supportButtonId = context_menu.Register(plugin, PLUGIN_MENU_TYPE_GLOBAL, "Support on Patreon", "");
	projectButtonId = context_menu.Register(plugin, PLUGIN_MENU_TYPE_GLOBAL, "Project page", "");
	ts3Functions.setPluginMenuEnabled(plugin->id().c_str(), disconnectButtonId, false);
	parent->connect(&context_menu, &TSContextMenu::FireContextMenuEvent, parent, &onButtonClicked);
	ts3Functions.getPlaybackConfigValueAsFloat(ts3Functions.getCurrentServerConnectionHandlerID(), "volume_factor_wave", &defaultMicClicksVolume);

	outputLog("TokoVOIP initialized", 0);

	resetClientsAll();
	checkUpdate();
	isRunning = false;
	tokovoip = this;
	return (1);
}

void Tokovoip::shutdown() {
	if (wsConnection) wsConnection->send_close(0);
	TerminateThread(threadWebSocket, 0);
	resetClientsAll();
}

// Utils

void updateWebsocketState(bool force, bool state) {
	ts3Functions.setPluginMenuEnabled(tokovoip->plugin->id().c_str(), connectButtonId, (force) ? !state : !isWebsocketThreadRunning());
}

bool isWebsocketThreadRunning() {
	DWORD lpExitCode;
	GetExitCodeThread(threadWebSocket, &lpExitCode);
	return lpExitCode == STILL_ACTIVE;
}

bool killWebsocketThread() {
	if (isWebsocketThreadRunning()) {
		outputLog("TokoVoip is still in the process of running or handshaking. Killing ...");
		if (wsConnection) wsConnection->send_close(0);
		TerminateThread(threadWebSocket, 0);
		Sleep(1000);
		if (isWebsocketThreadRunning()) {
			outputLog("Failed to kill running tokovoip.");
			return false;
		}
		outputLog("Successfully killed running instance.");
		resetState();
	}
	updateWebsocketState();
	return true;
}

bool stringIncludes(string target, string toMatch) {
	string tmp = target;
	transform(tmp.begin(), tmp.end(), tmp.begin(), [](unsigned char c) { return tolower(c); });
	return (tmp.find(toMatch) != string::npos) ? true : false;
}

void onTokovoipClientMove(uint64 sch_id, anyID client_id, uint64 old_channel_id, uint64 new_channel_id, int visibility, anyID my_id, const char * move_message) {
	uint64 serverId = ts3Functions.getCurrentServerConnectionHandlerID();
	if (client_id != getMyId(serverId)) return;
	char* channelName = "";
	ts3Functions.getChannelVariableAsString(serverId, new_channel_id, CHANNEL_NAME, &channelName);
	if (channelName == "") return;
	if (stringIncludes((string)channelName, "tokovoip")) {
		outputLog("Detected 'TokoVoip' in channel name, booting ..");
		initWebSocket();
	}
}

void onTokovoipCurrentServerConnectionChanged(uint64 sch_id) {
	uint64 serverId = ts3Functions.getCurrentServerConnectionHandlerID();
	if (!serverId) return;
	char* res;
	ts3Functions.getClientSelfVariableAsString(serverId, CLIENT_INPUT_DEACTIVATED, &res);
	isPTT = (res == "0") ? false : true;
	initWebSocket();
}

bool isChannelWhitelisted(json data, string channel) {
	if (data == NULL) return false;
	if (data.find("TSChannelWhitelist") == data.end()) return false;
	for (json::iterator it = data["TSChannelWhitelist"].begin(); it != data["TSChannelWhitelist"].end(); ++it) {
		string whitelistedChannel = it.value();
		if (whitelistedChannel == channel) return true;
	}
	return false;
}

void playWavFile(const char* fileNameWithoutExtension)
{
	char pluginPath[PATH_BUFSIZE];
	ts3Functions.getPluginPath(pluginPath, PATH_BUFSIZE, tokovoip->plugin->id().c_str());
	std::string path = std::string((string)pluginPath);
	DWORD error;
	std::string to_play = path + "tokovoip/" + std::string(fileNameWithoutExtension) + ".wav";
	if ((error = ts3Functions.playWaveFile(ts3Functions.getCurrentServerConnectionHandlerID(), to_play.c_str())) != ERROR_ok)
	{
		outputLog("can't play sound", error);
	}
}

void	setClientName(string name) {
	DWORD error;
	char* currentName;
	uint64 serverId = ts3Functions.getCurrentServerConnectionHandlerID();

	if ((error = ts3Functions.flushClientSelfUpdates(serverId, NULL)) != ERROR_ok && error != ERROR_ok_no_update)
		return outputLog("Can't flush self updates.", error);

	if ((error = ts3Functions.getClientVariableAsString(serverId, getMyId(serverId), CLIENT_NICKNAME, &currentName)) != ERROR_ok)
		return outputLog("Error getting client nickname", error);

	// Cancel name changing if name is already the same
	if (name == (string)currentName) return;

	// Cancel name change is anti-spam timer still active
	if (time(nullptr) - lastNameSetTick < 2) return;

	lastNameSetTick = time(nullptr);

	// Set name
	if ((error = ts3Functions.setClientSelfVariableAsString(serverId, CLIENT_NICKNAME, name.c_str())) != ERROR_ok)
		return outputLog("Error setting client nickname", error);

	if ((error = ts3Functions.flushClientSelfUpdates(serverId, NULL)) != ERROR_ok && error != ERROR_ok_no_update)
		outputLog("Can't flush self updates.", error);
}

void setClientTalking(bool status)
{
	DWORD error;
	uint64 serverId = ts3Functions.getCurrentServerConnectionHandlerID();
	char* vad;
	if ((error = ts3Functions.getPreProcessorConfigValue(serverId, "vad", &vad)) != ERROR_ok) {
		outputLog("Error retrieving vad setting");
		return;
	}
	if (strcmp(vad, "true") == 0 && !isPTT) return;

	if (status) {
		if ((error = ts3Functions.setClientSelfVariableAsInt(serverId, CLIENT_INPUT_DEACTIVATED, 0)) != ERROR_ok)
			outputLog("Can't active input.", error);
		error = ts3Functions.flushClientSelfUpdates(serverId, NULL);
		if (error != ERROR_ok && error != ERROR_ok_no_update)
			outputLog("Can't flush self updates.", error);
	}
	else
	{
		if ((error = ts3Functions.setClientSelfVariableAsInt(serverId, CLIENT_INPUT_DEACTIVATED, 1)) != ERROR_ok)
			outputLog("Can't deactive input.", error);
		error = ts3Functions.flushClientSelfUpdates(serverId, NULL);
		if (error != ERROR_ok && error != ERROR_ok_no_update)
			outputLog("Can't flush self updates.", error);
	}
}

void setClientMuteStatus(uint64 serverConnectionHandlerID, anyID clientId, bool status)
{
	anyID clientIds[2];
	clientIds[0] = clientId;
	clientIds[1] = 0;
	if (clientIds[0] <= 0)
		return;
	DWORD error;
	int isLocalMuted;
	if ((error = ts3Functions.getClientVariableAsInt(ts3Functions.getCurrentServerConnectionHandlerID(), clientId, CLIENT_IS_MUTED, &isLocalMuted)) != ERROR_ok) {
		outputLog("Error getting client nickname", error);
	}
	else
	{
		if (status && isLocalMuted == 0)
		{
			if ((error = ts3Functions.requestMuteClients(serverConnectionHandlerID, clientIds, NULL)) != ERROR_ok)
				outputLog("Can't mute client", error);
		}
		if (!status && isLocalMuted == 1)
		{
			if ((error = ts3Functions.requestUnmuteClients(serverConnectionHandlerID, clientIds, NULL)) != ERROR_ok)
				outputLog("Can't unmute client", error);
		}
	}
}

void outputLog(string message, DWORD errorCode)
{
	string output = message;
	if (errorCode != NULL) {
		char* errorBuffer;
		ts3Functions.getErrorMessage(errorCode, &errorBuffer);
		output = output + " : " + string(errorBuffer);
		ts3Functions.freeMemory(errorBuffer);
	}

	ts3Functions.logMessage(output.c_str(), LogLevel_INFO, "TokoVOIP", 141);
}

void resetVolumeAll(uint64 serverConnectionHandlerID)
{
	vector<anyID> clientsIds = getChannelClients(serverConnectionHandlerID, getCurrentChannel(serverConnectionHandlerID));
	anyID myId = getMyId(serverConnectionHandlerID);
	DWORD error;
	char *UUID;

	for (auto clientId = clientsIds.begin(); clientId != clientsIds.end(); clientId++) {
		if (*clientId != myId) {
			ts3Functions.setClientVolumeModifier(serverConnectionHandlerID, (*clientId), 0.0f);
			if ((error = ts3Functions.getClientVariableAsString(serverConnectionHandlerID, *clientId, CLIENT_UNIQUE_IDENTIFIER, &UUID)) != ERROR_ok) {
				outputLog("Error getting client UUID", error);
			} else {
				tokovoip->setRadioData(UUID, false);
			}
		}
	}
}

void unmuteAll(uint64 serverConnectionHandlerID)
{
	anyID* ids;
	DWORD error;

	if ((error = ts3Functions.getClientList(serverConnectionHandlerID, &ids)) != ERROR_ok)
	{
		outputLog("Error getting all clients from server", error);
		return;
	}

	if ((error = ts3Functions.requestUnmuteClients(serverConnectionHandlerID, ids, NULL)) != ERROR_ok)
	{
		outputLog("Can't unmute all clients", error);
	}
	ts3Functions.freeMemory(ids);
}

void resetPositionAll(uint64 serverConnectionHandlerID)
{
	vector<anyID> clientsIds = getChannelClients(serverConnectionHandlerID, getCurrentChannel(serverConnectionHandlerID));
	anyID myId = getMyId(serverConnectionHandlerID);

	ts3Functions.systemset3DListenerAttributes(serverConnectionHandlerID, NULL, NULL, NULL);

	for (auto clientId = clientsIds.begin(); clientId != clientsIds.end(); clientId++)
	{
		if (*clientId != myId) ts3Functions.channelset3DAttributes(serverConnectionHandlerID, *clientId, NULL);
	}
}

void resetClientsAll() {
	uint64 serverConnectionHandlerID = ts3Functions.getCurrentServerConnectionHandlerID();
	ts3Functions.setPlaybackConfigValue(serverConnectionHandlerID, "volume_factor_wave", to_string(defaultMicClicksVolume).c_str());
	resetPositionAll(serverConnectionHandlerID);
	resetVolumeAll(serverConnectionHandlerID);
	unmuteAll(serverConnectionHandlerID);
}

vector<anyID> getChannelClients(uint64 serverConnectionHandlerID, uint64 channelId)
{
	DWORD error;
	vector<anyID> result;
	anyID* clients = NULL;
	if ((error = ts3Functions.getChannelClientList(serverConnectionHandlerID, channelId, &clients)) == ERROR_ok) {
		int i = 0;
		while (clients[i]) {
			result.push_back(clients[i]);
			i++;
		}
		ts3Functions.freeMemory(clients);
	} else {
		outputLog("Error getting all clients from the server", error);
	}
	return result;
}

uint64 getCurrentChannel(uint64 serverConnectionHandlerID)
{
	uint64 channelId;
	DWORD error;
	if ((error = ts3Functions.getChannelOfClient(serverConnectionHandlerID, getMyId(serverConnectionHandlerID), &channelId)) != ERROR_ok)
		outputLog("Can't get current channel", error);
	return channelId;
}

anyID getMyId(uint64 serverConnectionHandlerID)
{
	anyID myID = (anyID)-1;
	if (!isConnected(serverConnectionHandlerID)) return myID;
	DWORD error;
	if ((error = ts3Functions.getClientID(serverConnectionHandlerID, &myID)) != ERROR_ok)
		outputLog("Failure getting client ID", error);
	return myID;
}

std::string getChannelName(uint64 serverConnectionHandlerID, anyID clientId)
{
	if (clientId == (anyID)-1) return "";
	uint64 channelId;
	DWORD error;
	if ((error = ts3Functions.getChannelOfClient(serverConnectionHandlerID, clientId, &channelId)) != ERROR_ok)
	{
		outputLog("Can't get channel of client", error);
		return "";
	}
	char* channelName;
	if ((error = ts3Functions.getChannelVariableAsString(serverConnectionHandlerID, channelId, CHANNEL_NAME, &channelName)) != ERROR_ok) {
		outputLog("Can't get channel name", error);
		return "";
	}
	std::string result = std::string(channelName);
	ts3Functions.freeMemory(channelName);
	return result;
}

bool isConnected(uint64 serverConnectionHandlerID)
{
	DWORD error;
	int result;
	if ((error = ts3Functions.getConnectionStatus(serverConnectionHandlerID, &result)) != ERROR_ok)
		return false;
	return result != 0;
}
