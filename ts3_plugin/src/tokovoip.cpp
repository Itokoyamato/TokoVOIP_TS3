#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "core/ts_helpers_qt.h"
#include "core/ts_serversinfo.h"
#include "core/ts_helpers_qt.h"
#include "core/ts_logging_qt.h"

#include "plugin.h" //pluginID
#include "ts3_functions.h"
#include "core/ts_serversinfo.h"
#include <teamspeak/public_errors.h>

#include "core/plugin_base.h"
#include "mod_radio.h"

#include "tokovoip.h"
#include "server_ws.hpp"

#define CURL_STATICLIB
#include "curl.h"

Tokovoip *tokovoip;
typedef SimpleWeb::SocketServer<SimpleWeb::WS> WsServer;

int isRunning = 0;

HANDLE threadService = INVALID_HANDLE_VALUE;
HANDLE threadTimeout = INVALID_HANDLE_VALUE;
HANDLE threadSendData = INVALID_HANDLE_VALUE;
HANDLE threadCheckUpdate = INVALID_HANDLE_VALUE;
volatile bool exitTimeoutThread = FALSE;
volatile bool exitSendDataThread = FALSE;

bool isTalking = false;
char* originalName = "";
time_t lastPingTick = 0;
int pluginStatus = 0;
bool processingMessage = false;
time_t lastNameSetTick = 0;
string mainChannel = "";
string waitChannel = "";
time_t lastChannelJoin = 0;

WsServer server;
shared_ptr<WsServer::Connection> ServerConnection;
time_t noiseWait = 0;

DWORD WINAPI ServiceThread(LPVOID lpParam)
{
	server.config.port = 38204;
	auto& echo = server.endpoint["^/tokovoip/?$"];

	echo.on_message = [](shared_ptr<WsServer::Connection> connection, shared_ptr<WsServer::InMessage> message) {

		processingMessage = true;
		lastPingTick = time(nullptr);
		pluginStatus = 1;
		ServerConnection = connection;

		auto message_str = message->string();
		//ts3Functions.logMessage(message_str.c_str(), LogLevel_INFO, "TokoVOIP", 0);


		if (!isConnected(ts3Functions.getCurrentServerConnectionHandlerID()))
		{
			processingMessage = false;
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
			processingMessage = false;
			return (0);
		}

		//--------------------------------------------------------

		// Load the json //
		json json_data = json::parse(message_str.c_str(), nullptr, false);
		if (json_data.is_discarded()) {
			ts3Functions.logMessage("Invalid JSON data", LogLevel_INFO, "TokoVOIP", 0);
			processingMessage = false;
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

		//--------------------------------------------------------

		if (isChannelWhitelisted(json_data, thisChannelName)) {
			resetClientsAll();
			pluginStatus = 3;
			processingMessage = false;
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
									processingMessage = false;
									return (0);
								}
								lastChannelJoin = time(nullptr);
								if ((error = ts3Functions.requestClientMove(serverId, getMyId(serverId), channelId, channelPass.c_str(), NULL)) != ERROR_ok) {
									outputLog("Can't join channel", error);
									pluginStatus = 2;
									processingMessage = false;
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
				pluginStatus = 2;
				processingMessage = false;
				return (0);
			}
		}

		serverId = ts3Functions.getCurrentServerConnectionHandlerID();

		// Save client's original name
		if (originalName == "")
			if ((error = ts3Functions.getClientVariableAsString(serverId, getMyId(serverId), CLIENT_NICKNAME, &originalName)) != ERROR_ok) {
				outputLog("Error getting client nickname", error);
				processingMessage = false;
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
				if (clientId == getMyId(serverId)) continue;
				for (auto user : data) {
					if (!user.is_object()) continue;
					if (!user["uuid"].is_string()) continue;

					string gameUUID = user["uuid"];
					int muted = user["muted"];
					float volume = user["volume"];
					bool isRadioEffect = user["radioEffect"];

					if (channelName == thisChannelName && UUID == gameUUID) {
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
				ts3Functions.freeMemory(UUID);
			}
		}
		pluginStatus = 3;
		processingMessage = false;
		//--------------------------------------------------------


	};
	echo.on_open = [](shared_ptr<WsServer::Connection> connection) {
		std::ostringstream oss;
		oss << "Server: Opened connection " << (size_t)connection.get();
		//outputLog((char*)oss.str().c_str(), 1);
	};
	echo.on_close = [](shared_ptr<WsServer::Connection> connection, int status, const string& /*reason*/) {
		std::ostringstream oss;
		oss << "Server: Closed connection " << (size_t)connection.get() << " with status code " << status;
		//outputLog((char*)oss.str().c_str(), 1);
	};
	echo.on_error = [](shared_ptr<WsServer::Connection> connection, const SimpleWeb::error_code& ec) {
		std::ostringstream oss;
		oss << "Server: Error in connection " << (size_t)connection.get() << ". " <<
			"Error: " << ec << ", error message: " << ec.message();
		//outputLog((char*)oss.str().c_str(), 1);
	};
	server.start();
	return NULL;
}

bool connected = false;
DWORD WINAPI TimeoutThread(LPVOID lpParam)
{
	while (!exitTimeoutThread)
	{
		time_t currentTick = time(nullptr);
		uint64 serverId = ts3Functions.getCurrentServerConnectionHandlerID();

		if (currentTick - lastPingTick < 10)
			connected = true;
		if (lastPingTick > 0 && currentTick - lastPingTick > 10 && connected == true)
		{
			string thisChannelName = getChannelName(serverId, getMyId(serverId));
			if (mainChannel == thisChannelName)
			{
				uint64* result;
				DWORD error;
				if ((error = ts3Functions.getChannelList(serverId, &result)) != ERROR_ok)
				{
					outputLog("Can't get channel list", error);
				}
				else
				{
					bool joined = false;
					uint64* iter = result;
					while (*iter && !joined)
					{
						uint64 channelId = *iter;
						iter++;
						char* cName;
						if ((error = ts3Functions.getChannelVariableAsString(serverId, channelId, CHANNEL_NAME, &cName)) != ERROR_ok) {
							outputLog("Can't get channel name", error);
						}
						else
						{
							if (!strcmp(waitChannel.c_str(), cName))
							{
								if ((error = ts3Functions.requestClientMove(serverId, getMyId(serverId), channelId, "", NULL)) != ERROR_ok) {
									outputLog("Can't join channel", error);
									pluginStatus = 2;
									processingMessage = false;
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
			Sleep(500);
			outputLog("Lost connection: plugin timed out client", 0);
			pluginStatus = 0;
			ServerConnection = NULL;
			connected = false;
			resetClientsAll();
			if (originalName != "")
				setClientName(originalName);
		}
		Sleep(100);
	}
	return NULL;
}

DWORD WINAPI SendDataThread(LPVOID lpParam)
{
	while (!exitSendDataThread)
	{
		if (processingMessage == false) {
			sendCallback("TokoVOIP version:" + (string)ts3plugin_version());
			sendCallback("TokoVOIP status:" + to_string(pluginStatus));

			uint64 serverId = ts3Functions.getCurrentServerConnectionHandlerID();
			if (isConnected(serverId)) {
				char *UUID;
				if (ts3Functions.getClientSelfVariableAsString(serverId, CLIENT_UNIQUE_IDENTIFIER, &UUID) == ERROR_ok)
					sendCallback("TokoVOIP UUID:" + (string)UUID);
				free(UUID);
			}
		}
		if (processingMessage == false)
			Sleep(1000);
		else
			Sleep(50);
	}
	return NULL;
}

// callback function writes data to a std::ostream
static size_t data_write(void* buf, size_t size, size_t nmemb, void* userp)
{
	if (userp)
	{
		std::ostream& os = *static_cast<std::ostream*>(userp);
		std::streamsize len = size * nmemb;
		if (os.write(static_cast<char*>(buf), len))
			return len;
	}

	return 0;
}

size_t curl_callback(const char* in, size_t size, size_t num, string* out) {
	const size_t totalBytes(size * num);
	out->append(in, totalBytes);
	return totalBytes;
}

json downloadJSON(string url) {
    CURL* curl = curl_easy_init();

    // Set remote URL.
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    // Don't bother trying IPv6, which would increase DNS resolution time.
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

    // Don't wait forever, time out after 10 seconds.
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);

    // Follow HTTP redirects if necessary.
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    // Response information.
    int httpCode(0);
    unique_ptr<string> httpData(new string());

    // Hook up data handling function.
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback);

    // Hook up data container (will be passed as the last parameter to the
    // callback handling function).  Can be any pointer type, since it will
    // internally be passed as a void pointer.
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, httpData.get());

    // Run our HTTP GET command, capture the HTTP response code, and clean up.
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_easy_cleanup(curl);

    if (httpCode == 200) {
        // Response looks good - done using Curl now.  Try to parse the results
        // and print them out.
		json parsedJSON = json::parse(*httpData.get(), nullptr, false);
		if (parsedJSON.is_discarded()) {
			outputLog("Downloaded JSON is invalid");
			return NULL;
		}
		return parsedJSON;
    } else {
		outputLog("Couldn't retrieve JSON (Code: " + to_string(httpCode) + ")");
        return NULL;
    }

    return NULL;
}

void checkUpdate() {
	json updateJSON = downloadJSON("http://itokoyamato.net/files/tokovoip/tokovoip_info.json");
	if (updateJSON != NULL) {
		outputLog("Got update json");
	}

	if (updateJSON == NULL || updateJSON.find("minVersion") == updateJSON.end() || updateJSON.find("currentVersion") == updateJSON.end()) {
		outputLog("Invalid update JSON");
		Sleep(600000); // Don't check for another 10mins
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

int Tokovoip::initialize(char *id) {
	plugin_id = id;
	const int sz = strlen(id) + 1;
	plugin_id = (char*)malloc(sz * sizeof(char));
	strcpy(plugin_id, id);
	if (isRunning != 0)
		return (0);
	outputLog("TokoVOIP initialized", 0);
	resetClientsAll();
	exitTimeoutThread = false;
	exitSendDataThread = false;
	threadService = CreateThread(NULL, 0, ServiceThread, NULL, 0, NULL);
	threadTimeout = CreateThread(NULL, 0, TimeoutThread, NULL, 0, NULL);
	threadSendData = CreateThread(NULL, 0, SendDataThread, NULL, 0, NULL);
	checkUpdate();
	isRunning = false;
	tokovoip = this;
	return (1);
}

void Tokovoip::shutdown()
{
	exitTimeoutThread = true;
	exitSendDataThread = true;
	server.stop();
	resetClientsAll();

	DWORD exitCode;
	BOOL result = GetExitCodeThread(threadService, &exitCode);
	if (!result || exitCode == STILL_ACTIVE)
		outputLog("service thread not terminated", LogLevel_CRITICAL);
}

vector<string> explode(const string& str, const char& ch) {
	string next;
	vector<string> result;

	// For each character in the string
	for (string::const_iterator it = str.begin(); it != str.end(); it++) {
		// If we've hit the terminal character
		if (*it == ch) {
			// If we have some characters accumulated
			if (!next.empty()) {
				// Add them to the result vector
				result.push_back(next);
				next.clear();
			}
		}
		else {
			// Accumulate the next character into the sequence
			next += *it;
		}
	}
	if (!next.empty())
		result.push_back(next);
	return result;
}

// Utils

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
	ts3Functions.getPluginPath(pluginPath, PATH_BUFSIZE, tokovoip->getPluginID());
	std::string path = std::string((string)pluginPath);
	DWORD error;
	std::string to_play = path + "tokovoip/" + std::string(fileNameWithoutExtension) + ".wav";
	if ((error = ts3Functions.playWaveFile(ts3Functions.getCurrentServerConnectionHandlerID(), to_play.c_str())) != ERROR_ok)
	{
		outputLog("can't play sound", error);
	}
}

void	setClientName(string name)
{
	DWORD error;
	char* currentName;
	uint64 serverId = ts3Functions.getCurrentServerConnectionHandlerID();

	// Cancel name change is anti-spam timer still active
	if (time(nullptr) - lastNameSetTick < 2) return;

	lastNameSetTick = time(nullptr);

	if ((error = ts3Functions.flushClientSelfUpdates(serverId, NULL)) != ERROR_ok && error != ERROR_ok_no_update)
		return outputLog("Can't flush self updates.", error);

	if ((error = ts3Functions.getClientVariableAsString(serverId, getMyId(serverId), CLIENT_NICKNAME, &currentName)) != ERROR_ok)
		return outputLog("Error getting client nickname", error);

	// Cancel name changing if name is already the same
	if (name == (string)currentName) return;

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
	if (status)
	{
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

void	sendCallback(string str)
{
	for (auto &a_connection : server.get_connections()) {
		auto send_stream = make_shared<WsServer::OutMessage>();
		*send_stream << str;

		a_connection->send(send_stream, [](const SimpleWeb::error_code& ec) {
			if (ec) {
				string errorMsg = "Server: Error sending message:" + ec.message();
				outputLog((char*)errorMsg.c_str(), 1);
			}
		});
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
