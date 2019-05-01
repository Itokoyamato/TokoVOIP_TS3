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

	-- Radio channels
	voip.channels = {};
	voip.myChannels = {};

	-- Player data shared on the network
	setPlayerData(GetPlayerName(PlayerId()), "voip:mode", voip.mode, true);
	setPlayerData(GetPlayerName(PlayerId()), "voip:talking", voip.talking, true);
	setPlayerData(GetPlayerName(PlayerId()), "radio:channel", voip.plugin_data.radioChannel, true);
	setPlayerData(GetPlayerName(PlayerId()), "radio:talking", voip.plugin_data.radioTalking, true);
	setPlayerData(GetPlayerName(PlayerId()), "voip:pluginStatus", voip.pluginStatus, true);
	setPlayerData(GetPlayerName(PlayerId()), "voip:pluginVersion", voip.pluginVersion, true);

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
				for i, player in ipairs(getPlayers()) do
					if (i <= 12) then
						pos_y = 1.2;
						pos_x = 0.60 + i/15;
					elseif (i <= 24) then
						pos_y = 1.3;
						pos_x = 0.60 + (i-12)/15;
					else
						pos_y = 1.4;
						pos_x = 0.60 + (i-24)/15;
					end
					drawTxt(pos_x, pos_y, 1.0, 1.0, 0.2, GetPlayerName(player) .. "\nMode: " .. tostring(getPlayerData(GetPlayerName(player), "voip:mode")) .. "\nChannel: " .. tostring(getPlayerData(GetPlayerName(player), "radio:channel")) .. "\nRadioTalking: " .. tostring(getPlayerData(GetPlayerName(player), "radio:talking")) .. "\npluginStatus: " .. tostring(getPlayerData(GetPlayerName(player), "voip:pluginStatus")) .. "\npluginVersion: " .. tostring(getPlayerData(GetPlayerName(player), "voip:pluginVersion")) .. "\nTalking: " .. tostring(getPlayerData(GetPlayerName(player), "voip:talking")), 255, 255, 255, 255);
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
				if (GetPlayerPed(-1) and GetPlayerPed(player)) then

					local playerPos = GetEntityCoords(GetPlayerPed(player));
					local dist = GetDistanceBetweenCoords(localPos, playerPos);

					if (not getPlayerData(GetPlayerName(player), "voip:mode")) then
						setPlayerData(GetPlayerName(player), "voip:mode", 1);
					end

					--	Process the volume for proximity voip
					local mode = tonumber(getPlayerData(GetPlayerName(player), "voip:mode"));
					if (not mode or (mode ~= 1 and mode ~= 2 and mode ~= 3)) then mode = 1 end;
					local volume = -30 + (30 - dist / voip.distance[mode] * 30);
					if (volume >= 0) then
						volume = 0;
					end
					--

					local angleToTarget = localHeading - math.atan(playerPos.y - localPos.y, playerPos.x - localPos.x);

					-- Set player's default data
					usersdata[i] = {	
								username = escape(GetPlayerName(player)),
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
					local remotePlayerUsingRadio = getPlayerData(GetPlayerName(player), "radio:talking");
					local remotePlayerChannel = getPlayerData(GetPlayerName(player), "radio:channel");

					for j, data in pairs(voip.channels) do
						local channel = voip.channels[j];
						if (channel.subscribers[GetPlayerName(PlayerId())] and channel.subscribers[GetPlayerName(player)] and voip.myChannels[remotePlayerChannel] and remotePlayerUsingRadio) then
							if (remotePlayerChannel <= 100) then
								usersdata[i].radioEffect = true;
							end
							usersdata[i].volume = 0;
							usersdata[i].muted = 0;
						end
					end
					--
					setPlayerTalkingState(player);
				end
		end
		voip.plugin_data.Users = usersdata;	--	Update TokoVoip's data
		voip.plugin_data.posX = 0.0;
		voip.plugin_data.posY = 0.0;
		voip.plugin_data.posZ = 0.0;
		voip.plugin_data.localName = escape(GetPlayerName(PlayerId()));		-- Update the localName
end

--------------------------------------------------------------------------------
--	Radio functions
--------------------------------------------------------------------------------

function addPlayerToRadio(channel)
	if voip.channels[channel] == nil then
		voip.channels[channel] = {name = "Call with " .. tostring(channel), subscribers = {}}
		voip.myChannels[channel] = true;
	end

	voip.channels[channel].subscribers[GetPlayerName(PlayerId())] = GetPlayerName(PlayerId());
	voip.myChannels[channel] = true;
	voip.plugin_data.radioChannel = channel;
	setPlayerData(GetPlayerName(PlayerId()), "radio:channel", channel, true);
	notification("Joined channel: " .. voip.channels[channel].name);
	TriggerServerEvent("TokoVoip:addPlayerToRadio", channel, GetPlayerName(PlayerId()));
end
RegisterNetEvent("TokoVoip:addPlayerToRadio");
AddEventHandler("TokoVoip:addPlayerToRadio", addPlayerToRadio);

function removePlayerFromRadio(channel)
	voip.channels[channel].subscribers[GetPlayerName(PlayerId())] = nil;
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
	setPlayerData(GetPlayerName(PlayerId()), "radio:channel", voip.plugin_data.radioChannel, true);
	notification("Left channel: " .. voip.channels[channel].name);
	TriggerServerEvent("TokoVoip:removePlayerFromRadio", channel, GetPlayerName(PlayerId()));
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
	setPlayerData(GetPlayerName(PlayerId()), "voip:pluginStatus", voip.pluginStatus, true);
end
RegisterNUICallback("setPluginStatus", setPluginStatus);

function setPluginVersion(data)
	voip.pluginVersion = data.msg;
	setPlayerData(GetPlayerName(PlayerId()), "voip:pluginVersion", voip.pluginVersion, true);
end
RegisterNUICallback("setPluginVersion", setPluginVersion);

-- Receives data from the TS plugin on microphone toggle
function setPlayerTalking(data)
	voip.talking = tonumber(data.state);

	if (voip.talking == 1) then
		setPlayerData(GetPlayerName(PlayerId()), "voip:talking", 1, true);
		RequestAnimDict("mp_facial");
		while not HasAnimDictLoaded("mp_facial") do
			Wait(5);
		end
		local localPos = GetEntityCoords(GetPlayerPed(-1));
		local localHeading = GetEntityHeading(GetPlayerPed(-1));
		PlayFacialAnim(GetPlayerPed(PlayerId()), "mic_chatter", "mp_facial");
	else
		setPlayerData(GetPlayerName(PlayerId()), "voip:talking", 0, true);
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
function setPlayerTalkingState(player)
	local talking = tonumber(getPlayerData(GetPlayerName(player), "voip:talking"));
	if (animStates[GetPlayerName(player)] == 0 and talking == 1) then
		RequestAnimDict("mp_facial");
		while not HasAnimDictLoaded("mp_facial") do
			Wait(5);
		end
		local localPos = GetEntityCoords(GetPlayerPed(-1));
		local localHeading = GetEntityHeading(GetPlayerPed(-1));
		PlayFacialAnim(GetPlayerPed(player), "mic_chatter", "mp_facial");
	elseif (animStates[GetPlayerName(player)] == 1 and talking == 0) then
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
