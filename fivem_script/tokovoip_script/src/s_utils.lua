------------------------------------------------------------
------------------------------------------------------------
---- Author: Dylan 'Itokoyamato' Thuillier              ----
----                                                    ----
---- Email: itokoyamato@hotmail.fr                      ----
----                                                    ----
---- Resource: tokovoip_script                          ----
----                                                    ----
---- File: s_utils.lua                                  ----
------------------------------------------------------------
------------------------------------------------------------

--------------------------------------------------------------------------------
--	Server_utils: Data system functions
--------------------------------------------------------------------------------

local playersData = {};

function setPlayerData(playerName, key, data, shared)
	if (shared) then
		if (not playersData[playerName]) then
			playersData[playerName] = {};
		end
		playersData[playerName][key] = data;
		TriggerClientEvent("Tokovoip:setPlayerData", -1, playerName, key, data);
	else
		for i = -1,256 do
			if (GetPlayerName(i) == playerName) then
				TriggerClientEvent("Tokovoip:setPlayerData", i, playerName, key, data);
				break;
			end
		end
	end
end
RegisterNetEvent("Tokovoip:setPlayerData");
AddEventHandler("Tokovoip:setPlayerData", setPlayerData);

function refreshAllPlayerData(toEveryone)
	if (toEveryone) then
		TriggerClientEvent("Tokovoip:doRefreshAllPlayerData", -1, playersData);
	else
		TriggerClientEvent("Tokovoip:doRefreshAllPlayerData", source, playersData);
	end
end
RegisterNetEvent("Tokovoip:refreshAllPlayerData");
AddEventHandler("Tokovoip:refreshAllPlayerData", refreshAllPlayerData);

AddEventHandler("playerDropped", function()
	local playerName = GetPlayerName(source);
	playersData[playerName] = nil;
	refreshAllPlayerData(true);
end);
