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

function addPlayerToRadio(channel, playerName)
	if not channels[channel] then
		channels[channel] = {name = "Call with " .. channel, subscribers = {}};
	end

	channels[channel].subscribers[playerName] = playerName;
	print("Added " .. playerName .. " to channel " .. channel);
	TriggerClientEvent("TokoVoip:updateChannels", -1, channels);
end
RegisterServerEvent("TokoVoip:addPlayerToRadio");
AddEventHandler("TokoVoip:addPlayerToRadio", addPlayerToRadio);

function removePlayerFromRadio(channel, playerName)
	if (channels[channel] and channels[channel].subscribers[playerName]) then
		channels[channel].subscribers[playerName] = nil;
		if (channel > 100) then
			if (#channels[channel].subscribers == 0) then
				channels[channel] = nil;
				TriggerClientEvent("TokoVoip:removeChannel", -1, channel);
			end
		end
		TriggerClientEvent("TokoVoip:updateChannels", -1, channels);
		print("Removed " .. playerName .. " from channel " .. channel);
	end
end
RegisterServerEvent("TokoVoip:removePlayerFromRadio");
AddEventHandler("TokoVoip:removePlayerFromRadio", removePlayerFromRadio);

function clientRequestUpdateChannels()
	TriggerClientEvent("TokoVoip:updateChannels", source, channels);
end
RegisterServerEvent("TokoVoip:clientRequestUpdateChannels");
AddEventHandler("TokoVoip:clientRequestUpdateChannels", clientRequestUpdateChannels);

function printChannels()
	for i, channel in pairs(channels) do
		RconPrint("Channel: " .. channel.name .. "\n");
		for j, player in pairs(channel.subscribers) do
			RconPrint("- " .. player .. "\n");
		end
	end
end

AddEventHandler('rconCommand', function(commandName, args)
	if commandName == 'voipChannels' then
		printChannels(true);
		CancelEvent();
	end
end)
