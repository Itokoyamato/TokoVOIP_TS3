#pragma once

#include "core/plugin_base.h"

#ifndef _WINDEF_
typedef unsigned long DWORD;
#endif

class Tokovoip
{
public:
	
	int initialize();
	void shutdown();
};

void outputLog(char* message, DWORD errorCode);
void resetVolumeAll(uint64 serverConnectionHandlerID);
void unmuteAll(uint64 serverConnectionHandlerID);
std::vector<anyID> getChannelClients(uint64 serverConnectionHandlerID, uint64 channelId);
uint64 getCurrentChannel(uint64 serverConnectionHandlerID);
anyID getMyId(uint64 serverConnectionHandlerID);
std::string getChannelName(uint64 serverConnectionHandlerID, anyID clientId);
bool isConnected(uint64 serverConnectionHandlerID);
void sendCallback(std::string str);
int	setClientName(char* name);
void setClientTalking(bool status);
void setClientMuteStatus(uint64 serverConnectionHandlerID, anyID clientId, bool status);
char *getPluginVersionAsString();