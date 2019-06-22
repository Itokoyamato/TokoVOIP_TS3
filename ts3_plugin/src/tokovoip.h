#pragma once

#include "core/plugin_base.h"

#ifndef _WINDEF_
typedef unsigned long DWORD;
#endif
void outputLog(char* message, DWORD errorCode);
class Tokovoip
{
private:
	std::map<std::string, bool> radioData;
	char *plugin_id;
	char *plugin_path;

public:
	int initialize(char *id);
	void shutdown();
	void setRadioData(std::string uuid, bool state)
	{
		radioData[uuid] = state;
	}
	char *getPluginID()
	{
		return plugin_id;
	}
	bool getRadioData(std::string uuid)
	{
		if (radioData.find(uuid) != radioData.end())
		{
			if (radioData[uuid] == true)
				return (true);
		}
		return (false);
	}
};


void resetVolumeAll(uint64 serverConnectionHandlerID);
void unmuteAll(uint64 serverConnectionHandlerID);
std::vector<anyID> getChannelClients(uint64 serverConnectionHandlerID, uint64 channelId);
uint64 getCurrentChannel(uint64 serverConnectionHandlerID);
anyID getMyId(uint64 serverConnectionHandlerID);
std::string getChannelName(uint64 serverConnectionHandlerID, anyID clientId);
bool isConnected(uint64 serverConnectionHandlerID);
void sendCallback(std::string str);
void setClientName(std::string name);
void setClientTalking(bool status);
void setClientMuteStatus(uint64 serverConnectionHandlerID, anyID clientId, bool status);
void playWavFile(const char* fileNameWithoutExtension);
void outputLog(char* message, DWORD errorCode);