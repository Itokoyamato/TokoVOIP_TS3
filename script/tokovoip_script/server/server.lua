local channels = {
					{name = "Police Radio", subscribers = {}},
					{name = "EMS Radio", subscribers = {}},
					{name = "Police/EMS Shared Radio", subscribers = {}},
};

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
	channels[channel].subscribers[playerName] = nil;
	TriggerClientEvent("TokoVoip:updateChannels", -1, channels);
	print("Removed " .. playerName .. " from channel " .. channel);
end
RegisterServerEvent("TokoVoip:removePlayerFromRadio");
AddEventHandler("TokoVoip:removePlayerFromRadio", removePlayerFromRadio);