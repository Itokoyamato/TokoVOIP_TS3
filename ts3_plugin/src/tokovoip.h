#pragma once

#include "core/plugin_base.h"
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

#ifndef _WINDEF_
typedef unsigned long DWORD;
#endif

class Tokovoip {
	private:
		map<string, bool> radioData;
		char *plugin_id;
		char *plugin_path;

	public:
		int initialize(char* id);
		void shutdown();
		void setRadioData(string uuid, bool state) {
			radioData[uuid] = state;
		}

		char *getPluginID() {
			return plugin_id;
		}

		bool getRadioData(string uuid)
		{
			if (radioData.find(uuid) != radioData.end()) {
				if (radioData[uuid] == true) return (true);
			}
			return false;
		}
};


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

void sendCallback(string str);
void setClientName(string name);
void setClientTalking(bool status);
void setClientMuteStatus(uint64 serverConnectionHandlerID, anyID clientId, bool status);
void playWavFile(const char* fileNameWithoutExtension);
