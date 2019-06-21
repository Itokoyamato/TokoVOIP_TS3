------------------------------------------------------------
------------------------------------------------------------
---- Author: Dylan 'Itokoyamato' Thuillier              ----
----                                                    ----
---- Email: itokoyamato@hotmail.fr                      ----
----                                                    ----
---- Resource: tokovoip_script                          ----
----                                                    ----
---- File: c_main.lua                                   ----
------------------------------------------------------------
------------------------------------------------------------

--------------------------------------------------------------------------------
--	Client: Voip data processed before sending it to TS3Plugin
--------------------------------------------------------------------------------

local isLoggedIn = true;
local targetPed;
local useLocalPed = true;
local isRunning = false;

function initializeVoip()
	voip = TokoVoip:init(TokoVoipConfig); -- Initialize TokoVoip and set default settings

	-- Variables used script-side
	voip.plugin_data.Users = {};
	voip.plugin_data.radioTalking = false;
	voip.plugin_data.radioChannel = 0;
	voip.plugin_data.localRadioClicks = false;
	voip.mode = 1;
	voip.talking = false;
	voip.pluginStatus = 0;
	voip.pluginVersion = "0";
	voip.serverId = GetPlayerServerId(PlayerId());

	-- Radio channels
	voip.channels = {};
	voip.myChannels = {};

	-- Player data shared on the network
	setPlayerData(voip.serverId, "voip:mode", voip.mode, true);
	setPlayerData(voip.serverId, "voip:talking", voip.talking, true);
	setPlayerData(voip.serverId, "radio:channel", voip.plugin_data.radioChannel, true);
	setPlayerData(voip.serverId, "radio:talking", voip.plugin_data.radioTalking, true);
	setPlayerData(voip.serverId, "voip:pluginStatus", voip.pluginStatus, true);
	setPlayerData(voip.serverId, "voip:pluginVersion", voip.pluginVersion, true);

	-- Set targetped (used for spectator mod for admins)
	targetPed = GetPlayerPed(-1);

	voip.processFunction = clientProcessing; -- Link the processing function that will be looped
	voip.initialize(voip); -- Initialize the websocket and controls
	voip:loop(voip); -- Start TokoVoip's loop
	requestUpdateChannels(); -- Retrieve the channels list

	-- View block screen handling
	Citizen.CreateThread(function()
		local lastTSConnected = 0;
		while not isLoggedIn do
			Wait(50);
		end
		while true do
			Wait(50);
			lastTSConnected = lastTSConnected + 50;
			if (voip.pluginStatus == 3 and voip.pluginVersion == voip.latestVersion) then
				lastTSConnected = 0;
				displayPluginScreen(false);
			else
				if (lastTSConnected > 5000) then
					displayPluginScreen(true);
				end
			end
		end
	end);

	Citizen.Trace("TokoVoip: Initialized script (1.2.11)\n");

	-- Debug data stuff
	local debugData = false;
	Citizen.CreateThread(function()
		while true do
			Wait(5)

			if (IsControlPressed(0, Keys["LEFTSHIFT"])) then
				if (IsControlJustPressed(1, Keys["9"]) or IsDisabledControlJustPressed(1, Keys["9"])) then
					debugData = not debugData;
				end
			end

			if (debugData) then
				local pos_y;
				local pos_x;
				local players = getPlayers();

				for i = 1, #players do
					local player = players[i];
					local playerServerId = GetPlayerServerId(players[i]);

					pos_y = 1.1 + (math.ceil(i/12) * 0.1);
					pos_x = 0.60 + ((i - (12 * math.floor(i/12)))/15);

					drawTxt(pos_x, pos_y, 1.0, 1.0, 0.2, "[" .. playerServerId .. "] " .. GetPlayerName(player) .. "\nMode: " .. tostring(getPlayerData(playerServerId, "voip:mode")) .. "\nChannel: " .. tostring(getPlayerData(playerServerId, "radio:channel")) .. "\nRadioTalking: " .. tostring(getPlayerData(playerServerId, "radio:talking")) .. "\npluginStatus: " .. tostring(getPlayerData(playerServerId, "voip:pluginStatus")) .. "\npluginVersion: " .. tostring(getPlayerData(playerServerId, "voip:pluginVersion")) .. "\nTalking: " .. tostring(getPlayerData(playerServerId, "voip:talking")), 255, 255, 255, 255);
				end
				local i = 0;
				for channelIndex, channel in pairs(voip.channels) do
					i = i + 1;
					drawTxt(0.8 + i/12, 0.5, 1.0, 1.0, 0.2, channel.name .. "(" .. channelIndex .. ")", 255, 255, 255, 255);
					local j = 0;
					for _, player in pairs(channel.subscribers) do
						j = j + 1;
						drawTxt(0.8 + i/12, 0.5 + j/60, 1.0, 1.0, 0.2, player, 255, 255, 255, 255);
					end
				end
				i = 0;
				for channelIndex, channel in pairs(voip.myChannels) do
					i = i + 1;
					drawTxt(0.5, 0.75 + i/60, 1.0, 1.0, 0.2, channelIndex .. ": true", 255, 255, 255, 255);
				end
			end
		end
	end);
end

function resourceStart(resource)
	if (resource == GetCurrentResourceName()) then	--	Initialize the script when this resource is started
		Citizen.CreateThread(function()
			Wait(3000);
			if (not isRunning) then
				isRunning = true;
				initializeVoip();
			end
		end);
	end
end
AddEventHandler("onClientResourceStart", resourceStart);

function clientProcessing()
		local playerList = voip.playerList;
		local usersdata = {};
		local localHeading = math.rad(GetEntityHeading(GetPlayerPed(-1)));
		local localPos;

		if useLocalPed then
			localPos = GetEntityCoords(GetPlayerPed(-1));
		else
			localPos = GetEntityCoords(targetPed);
		end

		for i = 1, #playerList do
			local player = playerList[i];
			local playerServerId = GetPlayerServerId(player);

				if (GetPlayerPed(-1) and GetPlayerPed(player)) then

					local playerPos = GetEntityCoords(GetPlayerPed(player));
					local dist = GetDistanceBetweenCoords(localPos, playerPos, true);

					if (not getPlayerData(playerServerId, "voip:mode")) then
						setPlayerData(playerServerId, "voip:mode", 1);
					end

					--	Process the volume for proximity voip
					local mode = tonumber(getPlayerData(playerServerId, "voip:mode"));
					if (not mode or (mode ~= 1 and mode ~= 2 and mode ~= 3)) then mode = 1 end;
					local volume = -30 + (30 - dist / voip.distance[mode] * 30);
					if (volume >= 0) then
						volume = 0;
					end
					--

					local angleToTarget = localHeading - math.atan(playerPos.y - localPos.y, playerPos.x - localPos.x);

					-- Set player's default data
					usersdata[i] = {	
								username = (TokoVoipConfig.showPlayerId and "[" .. voip.serverId .. "] " or "") .. escape(GetPlayerName(player)),
								id = playerServerId,
								volume = -30,
								muted = 1,
								radioEffect = false,
								posX = math.cos(angleToTarget) * 1.0,
								posY = math.sin(angleToTarget) * 1.0,
								posZ = 0.0
					};
					--

					-- Process proximity
					if (dist >= voip.distance[mode]) then
						usersdata[i].muted = 1;
					else
						usersdata[i].volume = volume;
						usersdata[i].muted = 0;
					end
					--

					-- Process channels
					local remotePlayerUsingRadio = getPlayerData(playerServerId, "radio:talking");
					local remotePlayerChannel = getPlayerData(playerServerId, "radio:channel");

					for j, data in pairs(voip.channels) do
						local channel = voip.channels[j];
						if (channel.subscribers[voip.serverId] and channel.subscribers[playerServerId] and voip.myChannels[remotePlayerChannel] and remotePlayerUsingRadio) then
							if (remotePlayerChannel <= 100) then
								usersdata[i].radioEffect = true;
							end
							usersdata[i].volume = 0;
							usersdata[i].muted = 0;
							usersdata[i].posX = 0;
							usersdata[i].posY = 0;
							usersdata[i].posZ = 0;
						end
					end
					--
					setPlayerTalkingState(player, playerServerId);
				end
		end
		voip.plugin_data.Users = usersdata;	--	Update TokoVoip's data
		voip.plugin_data.posX = 0.0;
		voip.plugin_data.posY = 0.0;
		voip.plugin_data.posZ = 0.0;
		voip.plugin_data.localName = (TokoVoipConfig.showPlayerId and "[" .. voip.serverId .. "] " or "") .. escape(GetPlayerName(PlayerId()));		-- Update the localName
end

--------------------------------------------------------------------------------
--	Radio functions
--------------------------------------------------------------------------------

function addPlayerToRadio(channel)
	if voip.channels[channel] == nil then
		voip.channels[channel] = {name = "Call with " .. tostring(channel), subscribers = {}}
		voip.myChannels[channel] = true;
	end

	voip.channels[channel].subscribers[voip.serverId] = voip.serverId;
	voip.myChannels[channel] = true;
	voip.plugin_data.radioChannel = channel;
	setPlayerData(voip.serverId, "radio:channel", channel, true);
	notification("Joined channel: " .. voip.channels[channel].name);
	TriggerServerEvent("TokoVoip:addPlayerToRadio", channel, voip.serverId);
end
RegisterNetEvent("TokoVoip:addPlayerToRadio");
AddEventHandler("TokoVoip:addPlayerToRadio", addPlayerToRadio);

function removePlayerFromRadio(channel)
	voip.channels[channel].subscribers[voip.serverId] = nil;
	voip.myChannels[channel] = nil;
	if (voip.plugin_data.radioChannel == channel) then
		if (tablelength(voip.myChannels) > 0) then
			for channelID, _ in pairs(voip.myChannels) do
				voip.plugin_data.radioChannel = channelID;
				break;
			end
		else
			voip.plugin_data.radioChannel = 0;
		end
	end
	setPlayerData(voip.serverId, "radio:channel", voip.plugin_data.radioChannel, true);
	notification("Left channel: " .. voip.channels[channel].name);
	TriggerServerEvent("TokoVoip:removePlayerFromRadio", channel, voip.serverId);
end
RegisterNetEvent("TokoVoip:removePlayerFromRadio");
AddEventHandler("TokoVoip:removePlayerFromRadio", removePlayerFromRadio);

function updateChannels(updatedChannels)
	voip.channels = updatedChannels;
end
RegisterNetEvent("TokoVoip:updateChannels");
AddEventHandler("TokoVoip:updateChannels", updateChannels);

function removeChannel(channel)
	voip.channels[channel] = nil;
	voip.myChannels[channel] = nil;
	if (voip.plugin_data.radioChannel == channel) then
		if (tablelength(voip.myChannels) > 0) then
			for channelID, _ in pairs(voip.myChannels) do
				voip.plugin_data.radioChannel = channelID;
				break;
			end
		else
			voip.plugin_data.radioChannel = 0;
		end
	end
end
RegisterNetEvent("TokoVoip:removeChannel");
AddEventHandler("TokoVoip:removeChannel", removeChannel);

function requestUpdateChannels()
	TriggerServerEvent("TokoVoip:clientRequestUpdateChannels");
end

function isPlayerInChannel(channel)
	if (voip.myChannels[channel]) then
		return true;
	else
		return false;
	end
end


--------------------------------------------------------------------------------
--	Plugin functions
--------------------------------------------------------------------------------

function setPluginStatus(data)
	voip.pluginStatus = tonumber(data.msg);
	setPlayerData(voip.serverId, "voip:pluginStatus", voip.pluginStatus, true);
end
RegisterNUICallback("setPluginStatus", setPluginStatus);

function setPluginVersion(data)
	voip.pluginVersion = data.msg;
	setPlayerData(voip.serverId, "voip:pluginVersion", voip.pluginVersion, true);
end
RegisterNUICallback("setPluginVersion", setPluginVersion);

-- Receives data from the TS plugin on microphone toggle
function setPlayerTalking(data)
	voip.talking = tonumber(data.state);

	if (voip.talking == 1) then
		setPlayerData(voip.serverId, "voip:talking", 1, true);
		RequestAnimDict("mp_facial");
		while not HasAnimDictLoaded("mp_facial") do
			Wait(5);
		end
		local localPos = GetEntityCoords(GetPlayerPed(-1));
		local localHeading = GetEntityHeading(GetPlayerPed(-1));
		PlayFacialAnim(GetPlayerPed(PlayerId()), "mic_chatter", "mp_facial");
	else
		setPlayerData(voip.serverId, "voip:talking", 0, true);
		RequestAnimDict("facials@gen_male@base");
		while not HasAnimDictLoaded("facials@gen_male@base") do
			Wait(5);
		end
		PlayFacialAnim(PlayerPedId(), "mood_normal_1", "facials@gen_male@base");
	end
end
RegisterNUICallback("setPlayerTalking", setPlayerTalking);

-- Handles the talking state of other players to apply talking animation to them
local animStates = {}
function setPlayerTalkingState(player, playerServerId)
	local talking = tonumber(getPlayerData(playerServerId, "voip:talking"));
	if (animStates[playerServerId] == 0 and talking == 1) then
		RequestAnimDict("mp_facial");
		while not HasAnimDictLoaded("mp_facial") do
			Wait(5);
		end
		local localPos = GetEntityCoords(GetPlayerPed(-1));
		local localHeading = GetEntityHeading(GetPlayerPed(-1));
		PlayFacialAnim(GetPlayerPed(player), "mic_chatter", "mp_facial");
	elseif (animStates[playerServerId] == 1 and talking == 0) then
		RequestAnimDict("facials@gen_male@base");
		while not HasAnimDictLoaded("facials@gen_male@base") do
			Wait(5);
		end
		PlayFacialAnim(GetPlayerPed(player), "mood_normal_1", "facials@gen_male@base");
	end
	animStates[GetPlayerName(player)] = talking;
end
RegisterNUICallback("setPlayerTalking", setPlayerTalking);


--------------------------------------------------------------------------------
--	Specific utils
--------------------------------------------------------------------------------

function playerLoggedIn(toggle)
	if (toggle) then
		isLoggedIn = true;
	end
end
RegisterNetEvent("onPlayerLoggedIn");
AddEventHandler("onPlayerLoggedIn", playerLoggedIn);

-- Toggle the blocking screen with usage explanation
local displayingPluginScreen = false;
function displayPluginScreen(toggle)
	if (displayingPluginScreen ~= toggle) then
		SendNUIMessage(
			{
				type = "displayPluginScreen",
				data = toggle
			}
		);
		displayingPluginScreen = toggle;
	end
end

-- Used for admin spectator feature
function updateVoipTargetPed(newTargetPed, useLocal)
	targetPed = newTargetPed
	useLocalPed = useLocal
end
AddEventHandler("updateVoipTargetPed", updateVoipTargetPed)
