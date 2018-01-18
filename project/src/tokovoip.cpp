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

#include "tokovoip.h"
#include "client_ws.hpp"
#include "server_ws.hpp"
#include "sol.hpp"

using namespace std;
typedef SimpleWeb::SocketServer<SimpleWeb::WS> WsServer;
typedef SimpleWeb::SocketClient<SimpleWeb::WS> WsClient;

int isRunning = 0;

HANDLE threadService = INVALID_HANDLE_VALUE;

WsServer server;
shared_ptr<WsServer::Connection> ServerConnection;

DWORD WINAPI ServiceThread(LPVOID lpParam)
{
	server.config.port = 1337;
	auto& echo = server.endpoint["^/echo/?$"];

	echo.on_message = [](shared_ptr<WsServer::Connection> connection, shared_ptr<WsServer::Message> message) {

		ServerConnection = connection;

		auto message_str = message->string();
		ts3Functions.logMessage(message_str.c_str(), LogLevel_INFO, "TokoVOIP", 0);
		//--------------------------------------------------------
	};
	echo.on_open = [](shared_ptr<WsServer::Connection> connection) {
		std::ostringstream oss;
		oss << "Server: Opened connection " << (size_t)connection.get();
		outputLog((char*)oss.str().c_str(), 1);
	};
	echo.on_close = [](shared_ptr<WsServer::Connection> connection, int status, const string& /*reason*/) {
		std::ostringstream oss;
		oss << "Server: Closed connection " << (size_t)connection.get() << " with status code " << status;
		//outputLog((char*)oss.str().c_str(), 1);
	};
	echo.on_error = [](shared_ptr<WsServer::Connection> connection, const boost::system::error_code& ec) {
		std::ostringstream oss;
		oss << "Server: Error in connection " << (size_t)connection.get() << ". " <<
			"Error: " << ec << ", error message: " << ec.message();
		//outputLog((char*)oss.str().c_str(), 1);
	};
	server.start();
	return NULL;
}

int Tokovoip::initialize()
{
	if (isRunning != 0)
		return (0);
	outputLog("TokoVOIP initialized", 0);
	unmuteAll(ts3Functions.getCurrentServerConnectionHandlerID());
	resetVolumeAll(ts3Functions.getCurrentServerConnectionHandlerID());
	//exitTimeoutThread = false;
	//exitSendDataThread = false;
	threadService = CreateThread(NULL, 0, ServiceThread, NULL, 0, NULL);
	//threadTimeout = CreateThread(NULL, 0, TimeoutThread, NULL, 0, NULL);
	//threadSendData = CreateThread(NULL, 0, SendDataThread, NULL, 0, NULL);
	//threadCheckUpdate = CreateThread(NULL, 0, checkUpdateThread, NULL, 0, NULL);
	isRunning = false;
	return (1);
}

void Tokovoip::shutdown()
{
	server.stop();
	unmuteAll(ts3Functions.getCurrentServerConnectionHandlerID());
	resetVolumeAll(ts3Functions.getCurrentServerConnectionHandlerID());

	DWORD exitCode;
	BOOL result = GetExitCodeThread(threadService, &exitCode);
	if (!result || exitCode == STILL_ACTIVE)
		outputLog("service thread not terminated", LogLevel_CRITICAL);
}

// Utils

void outputLog(char* message, DWORD errorCode)
{
	char* errorBuffer;
	ts3Functions.getErrorMessage(errorCode, &errorBuffer);
	std::string output = std::string(message) + std::string(" : ") + std::string(errorBuffer);
	ts3Functions.logMessage(output.c_str(), LogLevel_INFO, "TokoVOIP", 141);
	ts3Functions.freeMemory(errorBuffer);
}

void resetVolumeAll(uint64 serverConnectionHandlerID)
{
	std::vector<anyID> clientsIds = getChannelClients(serverConnectionHandlerID, getCurrentChannel(serverConnectionHandlerID));
	anyID myId = getMyId(serverConnectionHandlerID);
	for (auto it = clientsIds.begin(); it != clientsIds.end(); it++)
	{
		if (!(*it == myId))
		{
			ts3Functions.setClientVolumeModifier(serverConnectionHandlerID, (*it), 0.0f);
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

std::vector<anyID> getChannelClients(uint64 serverConnectionHandlerID, uint64 channelId)
{
	std::vector<anyID> result;
	anyID* clients = NULL;
	if (ts3Functions.getChannelClientList(serverConnectionHandlerID, channelId, &clients) == ERROR_ok)
	{
		int i = 0;
		while (clients[i])
		{
			result.push_back(clients[i]);
			i++;
		}
		ts3Functions.freeMemory(clients);
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