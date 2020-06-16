#pragma once

#include "core/plugin_base.h"
#include "json.hpp"

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

json getServerInfoFromMaster();
json getServerFromClientIP(json servers);

void initWebSocket();
void resetState();
void resetChannel();
string getWebSocketEndpoint();
void sendWSMessage(string eventName, json value);
void initDataThread();
json TS3DataToJSON();

class Tokovoip {
private:
	map<string, bool> radioData;
	map<string, bool> safeRadioData;
	bool processingState = false;
	char *plugin_id;
	char *plugin_path;

public:
	int initialize(char* id);
	void shutdown();

	char *getPluginID() {
		return plugin_id;
	}

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

	void setProcessingState(bool state) {
		 processingState = state;
		 if (state == false) {
		 	map<string, bool> updatedRadioData;
		 	for (const auto &data : radioData) {
		 		updatedRadioData[data.first] = radioData[data.first];
		 	}
		 	safeRadioData = updatedRadioData;
		 }
	}

	bool getProcessingState() {
		return processingState;
	}
};
