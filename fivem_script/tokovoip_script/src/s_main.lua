------------------------------------------------------------
------------------------------------------------------------
---- Author: Dylan 'Itokoyamato' Thuillier              ----
----                                                    ----
---- Email: itokoyamato@hotmail.fr                      ----
----                                                    ----
---- Resource: tokovoip_script                          ----
----                                                    ----
---- File: s_main.lua                                   ----
------------------------------------------------------------
------------------------------------------------------------

--------------------------------------------------------------------------------
--	Server: radio functions
--------------------------------------------------------------------------------

local channels = TokoVoipConfig.channels;
local calls = {};

function addPlayerToRadio(channelId, playerServerId)
	if (not channels[channelId]) then
		channels[channelId] = {id = channelId, name = "Call with " .. channelId, subscribers = {}};
	end
	if (not channels[channelId].id) then
		channels[channelId].id = channelId;
	end

	channels[channelId].subscribers[playerServerId] = playerServerId;
	print("Added [" .. playerServerId .. "] " .. GetPlayerName(playerServerId) .. " to channel " .. channelId);
	updateChannelSubscribers(channelId);
end
RegisterServerEvent("TokoVoip:addPlayerToRadio");
AddEventHandler("TokoVoip:addPlayerToRadio", addPlayerToRadio);

function updateChannelSubscribers(channelId)
	if (not channels[channelId]) then return; end
	for _, playerServerId in pairs(channels[channelId].subscribers) do
		TriggerClientEvent("TokoVoip:updateChannels", playerServerId, channelId, channels[channelId]);
	end
end

function removePlayerFromRadio(channelId, playerServerId)
	if (channels[channelId] and channels[channelId].subscribers[playerServerId]) then
		channels[channelId].subscribers[playerServerId] = nil;
		if (channelId > 100) then
			if (#channels[channelId].subscribers == 0) then
				channels[channelId] = nil;
			end
		end
		print("Removed [" .. playerServerId .. "] " .. GetPlayerName(playerServerId) .. " from channel " .. channelId);

		updateChannelSubscribers(channelId);
		TriggerClientEvent("TokoVoip:updateChannels", playerServerId, channelId, channels[channelId]); -- update channel info for removed player, will remove client references to this channel
	end
end
RegisterServerEvent("TokoVoip:removePlayerFromRadio");
AddEventHandler("TokoVoip:removePlayerFromRadio", removePlayerFromRadio);

function removePlayerFromAllRadio(playerServerId)
	for channelId, channel in pairs(channels) do
		if (channel.subscribers[playerServerId]) then
			removePlayerFromRadio(channelId, playerServerId);
		end
	end
end
RegisterServerEvent("TokoVoip:removePlayerFromAllRadio");
AddEventHandler("TokoVoip:removePlayerFromAllRadio", removePlayerFromAllRadio);

AddEventHandler("playerDropped", function()
	removePlayerFromAllRadio(source);
end);

function printChannels()
	for i, channel in pairs(channels) do
		RconPrint("Channel: " .. channel.name .. "\n");
		for j, player in pairs(channel.subscribers) do
			RconPrint("- [" .. player .. "] " .. GetPlayerName(player) .. "\n");
		end
	end
end

AddEventHandler('rconCommand', function(commandName, args)
	if commandName == 'voipChannels' then
		printChannels();
		CancelEvent();
	end
end)

--------------------------------------------------------------------------------
--	Call functions
--------------------------------------------------------------------------------

function addPlayerToCall(number, playerServerId)
	local number = tostring(number);
	local playerCall = getPlayerData(playerServerId, "voip:call");

	if (playerCall ~= number) then
		removePlayerFromCall(playerServerId, playerCall);

		if (not calls[number]) then
			calls[number] = {};
		end

		calls[number][#calls[number] + 1] = playerServerId;

		TriggerClientEvent("TokoVoip:updateCalls", -1, calls);
	end
end
RegisterServerEvent("TokoVoip:addPlayerToCall");
AddEventHandler("TokoVoip:addPlayerToCall", addPlayerToCall);

function removePlayerFromCall(playerServerId, number)
	local playerCall = number or getPlayerData(playerServerId, "voip:call");

	if (playerCall) then
		if (calls[playerCall]) then
			if (calls[playerCall]) then
				for i = 1, #calls[playerCall] do
					if (playerServerId == calls[playerCall][i]) then
						calls[playerCall][i] = nil;
						break;
					end
				end

				if (#calls[playerCall] == 0) then
					calls[playerCall] = nil;
				end

				TriggerClientEvent("TokoVoip:updateCalls", -1, calls);
			end	
		end
	end
end
RegisterServerEvent("TokoVoip:removePlayerFromCall");
AddEventHandler("TokoVoip:removePlayerFromCall", removePlayerFromCall);