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

function addPlayerToRadio(channel, playerServerId)
	if not channels[channel] then
		channels[channel] = {name = "Call with " .. channel, subscribers = {}};
	end

	channels[channel].subscribers[playerServerId] = playerServerId;
	print("Added [" .. playerServerId .. "] " .. GetPlayerName(playerServerId) .. " to channel " .. channel);
	TriggerClientEvent("TokoVoip:updateChannels", -1, channels);
end
RegisterServerEvent("TokoVoip:addPlayerToRadio");
AddEventHandler("TokoVoip:addPlayerToRadio", addPlayerToRadio);

function removePlayerFromRadio(channel, playerServerId)
	if (channels[channel] and channels[channel].subscribers[playerServerId]) then
		channels[channel].subscribers[playerServerId] = nil;
		if (channel > 100) then
			if (#channels[channel].subscribers == 0) then
				channels[channel] = nil;
				TriggerClientEvent("TokoVoip:removeChannel", -1, channel);
			end
		end
		TriggerClientEvent("TokoVoip:updateChannels", -1, channels);
		print("Removed [" .. playerServerId .. "] " .. GetPlayerName(playerServerId) .. " from channel " .. channel);
	end
end
RegisterServerEvent("TokoVoip:removePlayerFromRadio");
AddEventHandler("TokoVoip:removePlayerFromRadio", removePlayerFromRadio);

function removePlayerFromAllRadio(playerServerId)
	for channelID, channel in pairs(channels) do
		if (channel.subscribers[playerServerId]) then
			removePlayerFromRadio(channelID, playerServerId);
		end
	end
end
RegisterServerEvent("TokoVoip:removePlayerFromAllRadio");
AddEventHandler("TokoVoip:removePlayerFromAllRadio", removePlayerFromAllRadio);

function clientRequestUpdateChannels()
	TriggerClientEvent("TokoVoip:updateChannels", source, channels);
end
RegisterServerEvent("TokoVoip:clientRequestUpdateChannels");
AddEventHandler("TokoVoip:clientRequestUpdateChannels", clientRequestUpdateChannels);

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
