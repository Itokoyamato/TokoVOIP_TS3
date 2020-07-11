#pragma once

#include "core/plugin_base.h"
#include "json.hpp"
#include <ws2tcpip.h> 
#define CPPHTTPLIB_OPENSSL_SUPPORT

using namespace std;
using json = nlohmann::json;

#ifndef _WINDEF_
typedef unsigned long DWORD;
#endif

void resetVolumeAll(uint64 serverConnectionHandlerID);
void unmuteAll(uint64 serverConnectionHandlerID);
void resetPositionAll(uint64 serverConnectionHandlerID);
void resetClientsAll();

void outputLog(string message, DWORD errorCode = NULL);
bool isConnected(uint64 serverConnectionHandlerID);
bool isChannelWhitelisted(json data, string channel);
void checkUpdate();

vector<anyID> getChannelClients(uint64 serverConnectionHandlerID, uint64 channelId);
uint64 getCurrentChannel(uint64 serverConnectionHandlerID);
anyID getMyId(uint64 serverConnectionHandlerID);
string getChannelName(uint64 serverConnectionHandlerID, anyID clientId);

void setClientName(string name);
void setClientTalking(bool status);
void setClientMuteStatus(uint64 serverConnectionHandlerID, anyID clientId, bool status);
void playWavFile(const char* fileNameWithoutExtension);

string verifyTSServer();
json handshake(string clientIP);

void initWebSocket(bool ignoreRunning = false);
void resetState();
void resetChannel();
string getWebSocketEndpoint();
void sendWSMessage(string eventName, json value);
void onTokovoipClientMove(uint64 sch_id, anyID client_id, uint64 old_channel_id, uint64 new_channel_id, int visibility, anyID my_id, const char * move_message);
void onTokovoipCurrentServerConnectionChanged(uint64 sch_id);
bool isWebsocketThreadRunning();
bool killWebsocketThread();
void updateWebsocketState(bool force = false, bool state = false);
bool stringIncludes(string target, string toMatch);

class Tokovoip {
private:
	map<string, bool> radioData;
	map<string, bool> safeRadioData;
	bool processingState = false;
	char *plugin_id;
	char *plugin_path;
	int pluginStatus = -1;

public:
	Plugin_Base* plugin;

	int initialize(char* id, QObject* parent);
	void shutdown();

	void setRadioData(string uuid, bool state) {
		radioData[uuid] = state;
	}
	bool getRadioData(string uuid) {
		if (radioData.find(uuid) != radioData.end()) {
			if (radioData[uuid] == true) return (true);
		}
		return false;
	}

	bool getSafeRadioData(string uuid) {
		if (safeRadioData.find(uuid) != safeRadioData.end()) {
			if (safeRadioData[uuid] == true) return (true);
		}
		return false;
	}

	void setProcessingState(bool state, int currentPluginStatus) {
		 processingState = state;
		 if (state == false) {
		 	map<string, bool> updatedRadioData;
		 	for (const auto &data : radioData) {
		 		updatedRadioData[data.first] = radioData[data.first];
		 	}
		 	safeRadioData = updatedRadioData;

			if (pluginStatus == currentPluginStatus) return;
			pluginStatus = currentPluginStatus;
			json data = {
				{ "key", "pluginStatus" },
				{ "value", pluginStatus },
			};
			sendWSMessage("setTS3Data", data);
		 }
	}

	bool getProcessingState() {
		return processingState;
	}
};
