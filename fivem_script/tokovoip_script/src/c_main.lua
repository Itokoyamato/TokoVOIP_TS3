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

local targetPed;
local useLocalPed = true;
local isRunning = false;
local scriptVersion = "1.5.0";
local animStates = {}
local displayingPluginScreen = false;
local HeadBone = 0x796e;

--------------------------------------------------------------------------------
--	Plugin functions
--------------------------------------------------------------------------------

-- Handles the talking state of other players to apply talking animation to them
local function setPlayerTalkingState(player, playerServerId)
	local talking = tonumber(getPlayerData(playerServerId, "voip:talking"));
	if (animStates[playerServerId] == 0 and talking == 1) then
		PlayFacialAnim(GetPlayerPed(player), "mic_chatter", "mp_facial");
	elseif (animStates[playerServerId] == 1 and talking == 0) then
		PlayFacialAnim(GetPlayerPed(player), "mood_normal_1", "facials@gen_male@base");
	end
	animStates[playerServerId] = talking;
end

RegisterNUICallback("updatePluginData", function(data)
	local payload = data.payload;
	if (voip[payload.key] == payload.data) then return end
	voip[payload.key] = payload.data;
	setPlayerData(voip.serverId, "voip:" .. payload.key, voip[payload.key], true);
	voip:updateConfig();
	voip:updateTokoVoipInfo(true);
end);

-- Receives data from the TS plugin on microphone toggle
RegisterNUICallback("setPlayerTalking", function(data)
	voip.talking = tonumber(data.state);

	if (voip.talking == 1) then
		setPlayerData(voip.serverId, "voip:talking", 1, true);
		PlayFacialAnim(GetPlayerPed(PlayerId()), "mic_chatter", "mp_facial");
	else
		setPlayerData(voip.serverId, "voip:talking", 0, true);
		PlayFacialAnim(PlayerPedId(), "mood_normal_1", "facials@gen_male@base");
	end
end)

local function clientProcessing()
	local playerList = voip.playerList;
	local usersdata = {};
	local localHeading;
	local ped = PlayerPedId()

	if (voip.headingType == 1) then
		localHeading = math.rad(GetEntityHeading(ped));
	else
		localHeading = math.rad(GetGameplayCamRot().z % 360);
	end
	local localPos;

	if useLocalPed then
		localPos = GetPedBoneCoords(ped, HeadBone);
	else
		localPos = GetPedBoneCoords(targetPed, HeadBone);
	end

	for i=1, #playerList do
		local player = playerList[i];
		local playerServerId = GetPlayerServerId(player);
		if (GetPlayerPed(player) and voip.serverId ~= playerServerId) then
			local playerPos = GetPedBoneCoords(GetPlayerPed(player), HeadBone);
			local dist = #(localPos - playerPos);

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
			local tbl = {
				uuid = getPlayerData(playerServerId, "voip:pluginUUID"),
				volume = -30,
				muted = 1,
				radioEffect = false,
				posX = voip.plugin_data.enableStereoAudio and math.cos(angleToTarget) * dist or 0,
				posY = voip.plugin_data.enableStereoAudio and math.sin(angleToTarget) * dist or 0,
				posZ = voip.plugin_data.enableStereoAudio and playerPos.z or 0
			};
			--

			-- Process proximity
			tbl.forceUnmuted = 0
			if (dist >= voip.distance[mode]) then
				tbl.muted = 1;
			else
				tbl.volume = volume;
				tbl.muted = 0;
				tbl.forceUnmuted = 1
			end

			usersdata[#usersdata + 1] = tbl
			setPlayerTalkingState(player, playerServerId);
		end
	end

	-- Process channels
	for _, channel in pairs(voip.myChannels) do
		for _, subscriber in pairs(channel.subscribers) do
			if (subscriber ~= voip.serverId) then
				local remotePlayerUsingRadio = getPlayerData(subscriber, "radio:talking");
				local remotePlayerChannel = getPlayerData(subscriber, "radio:channel");
					local remotePlayerUuid = getPlayerData(subscriber, "voip:pluginUUID");

					local founduserData = nil
					for k, v in pairs(usersdata) do
						if(v.uuid == remotePlayerUuid) then
							founduserData = v
						end
					end

					if not founduserData then
						founduserData = {
							uuid = getPlayerData(subscriber, "voip:pluginUUID"),
							radioEffect = false,
							resave = true,
							volume = 0,
							muted = 1
						}
					end


					if (remotePlayerChannel <= voip.config.radioClickMaxChannel) then
						founduserData.radioEffect = true;
					end

					if(not remotePlayerUsingRadio or remotePlayerChannel ~= channel.id) then
						founduserData.radioEffect = false;
						if not founduserData.forceUnmuted then
							founduserData.muted = true;
						end
					else
						founduserData.muted = false
						founduserData.volume = 0;
						founduserData.posX = 0;
						founduserData.posY = 0;
						founduserData.posZ = voip.plugin_data.enableStereoAudio and localPos.z or 0;
				 	end
					if(founduserData.resave) then
						usersdata[#usersdata + 1] = founduserData
					end
				end
		end
	end

	voip.plugin_data.Users = usersdata; -- Update TokoVoip's data
	voip.plugin_data.posX = 0;
	voip.plugin_data.posY = 0;
	voip.plugin_data.posZ = voip.plugin_data.enableStereoAudio and localPos.z or 0;
end

RegisterNetEvent("initializeVoip");
AddEventHandler("initializeVoip", function()
	Citizen.Wait(1000);
	if (isRunning) then return Citizen.Trace("TokoVOIP is already running\n"); end
	isRunning = true;

	voip = TokoVoip:init(TokoVoipConfig); -- Initialize TokoVoip and set default settings

	-- Variables used script-side
	voip.plugin_data.Users = {};
	voip.plugin_data.radioTalking = false;
	voip.plugin_data.radioChannel = -1;
	voip.plugin_data.localRadioClicks = false;
	voip.mode = 1;
	voip.talking = false;
	voip.pluginStatus = -1;
	voip.pluginVersion = "0";
	voip.serverId = GetPlayerServerId(PlayerId());

	-- Radio channels
	voip.myChannels = {};

	-- Player data shared on the network
	setPlayerData(voip.serverId, "voip:mode", voip.mode, true);
	setPlayerData(voip.serverId, "voip:talking", voip.talking, true);
	setPlayerData(voip.serverId, "radio:channel", voip.plugin_data.radioChannel, true);
	setPlayerData(voip.serverId, "radio:talking", voip.plugin_data.radioTalking, true);
	setPlayerData(voip.serverId, "voip:pluginStatus", voip.pluginStatus, true);
	setPlayerData(voip.serverId, "voip:pluginVersion", voip.pluginVersion, true);
	refreshAllPlayerData();

	-- Set targetped (used for spectator mod for admins)
	targetPed = GetPlayerPed(-1);

	voip.processFunction = clientProcessing; -- Link the processing function that will be looped
	voip:initialize(); -- Initialize the websocket and controls
	voip:loop(); -- Start TokoVoip's loop

	Citizen.Trace("TokoVoip: Initialized script (" .. scriptVersion .. ")\n");

	-- Request this stuff here only one time
	RequestAnimDict("mp_facial");
	RequestAnimDict("facials@gen_male@base");

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
				local players = GetActivePlayers();

				for i = 1, #players do
					local player = players[i];
					local playerServerId = GetPlayerServerId(players[i]);

					pos_y = 1.1 + (math.ceil(i/12) * 0.1);
					pos_x = 0.60 + ((i - (12 * math.floor(i/12)))/15);

					drawTxt(pos_x, pos_y, 1.0, 1.0, 0.2, "[" .. playerServerId .. "] " .. GetPlayerName(player) .. "\nMode: " .. tostring(getPlayerData(playerServerId, "voip:mode")) .. "\nChannel: " .. tostring(getPlayerData(playerServerId, "radio:channel")) .. "\nRadioTalking: " .. tostring(getPlayerData(playerServerId, "radio:talking")) .. "\npluginStatus: " .. tostring(getPlayerData(playerServerId, "voip:pluginStatus")) .. "\npluginVersion: " .. tostring(getPlayerData(playerServerId, "voip:pluginVersion")) .. "\nTalking: " .. tostring(getPlayerData(playerServerId, "voip:talking")), 255, 255, 255, 255);
				end
				local i = 0;
				for channelIndex, channel in pairs(voip.myChannels) do
					i = i + 1;
					drawTxt(0.8 + i/12, 0.5, 1.0, 1.0, 0.2, channel.name .. "(" .. channelIndex .. ")", 255, 255, 255, 255);
					local j = 0;
					for _, player in pairs(channel.subscribers) do
						j = j + 1;
						drawTxt(0.8 + i/12, 0.5 + j/60, 1.0, 1.0, 0.2, player, 255, 255, 255, 255);
					end
				end
			end
		end
	end);
end)
--------------------------------------------------------------------------------
--	Radio functions
--------------------------------------------------------------------------------

function addPlayerToRadio(channel)
	TriggerServerEvent("TokoVoip:addPlayerToRadio", channel, voip.serverId);
end
RegisterNetEvent("TokoVoip:addPlayerToRadio");
AddEventHandler("TokoVoip:addPlayerToRadio", addPlayerToRadio);

function removePlayerFromRadio(channel)
	TriggerServerEvent("TokoVoip:removePlayerFromRadio", channel, voip.serverId);
end
RegisterNetEvent("TokoVoip:removePlayerFromRadio");
AddEventHandler("TokoVoip:removePlayerFromRadio", removePlayerFromRadio);

RegisterNetEvent("TokoVoip:onPlayerLeaveChannel");
AddEventHandler("TokoVoip:onPlayerLeaveChannel", function(channelId, playerServerId)
	-- Local player left channel
	if (playerServerId == voip.serverId and voip.myChannels[channelId]) then
		local previousChannel = voip.plugin_data.radioChannel;
		voip.myChannels[channelId] = nil;
		if (voip.plugin_data.radioChannel == channelId) then -- If current radio channel is still removed channel, reset to first available channel or none
			if (tablelength(voip.myChannels) > 0) then
				for channelId, _ in pairs(voip.myChannels) do
					voip.plugin_data.radioChannel = channelId;
					break;
				end
			else
				voip.plugin_data.radioChannel = -1; -- No radio channel available
			end
		end

		if (previousChannel ~= voip.plugin_data.radioChannel) then -- Update network data only if we actually changed radio channel
			setPlayerData(voip.serverId, "radio:channel", voip.plugin_data.radioChannel, true);
		end

	-- Remote player left channel we are subscribed to
	elseif (voip.myChannels[channelId]) then
		voip.myChannels[channelId].subscribers[playerServerId] = nil;
	end
end)

RegisterNetEvent("TokoVoip:onPlayerJoinChannel");
AddEventHandler("TokoVoip:onPlayerJoinChannel", function(channelId, playerServerId, channelData)
	-- Local player joined channel
	if (playerServerId == voip.serverId and channelData) then
		local previousChannel = voip.plugin_data.radioChannel;

		voip.plugin_data.radioChannel = channelData.id;
		voip.myChannels[channelData.id] = channelData;

		if (previousChannel ~= voip.plugin_data.radioChannel) then -- Update network data only if we actually changed radio channel
			setPlayerData(voip.serverId, "radio:channel", voip.plugin_data.radioChannel, true);
		end

	-- Remote player joined a channel we are subscribed to
	elseif (voip.myChannels[channelId]) then
		voip.myChannels[channelId].subscribers[playerServerId] = playerServerId;
	end
end)

function isPlayerInChannel(channel)
	if (voip.myChannels[channel]) then
		return true;
	else
		return false;
	end
end

--------------------------------------------------------------------------------
--	Specific utils
--------------------------------------------------------------------------------

-- Toggle the blocking screen with usage explanation
-- Not used
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
AddEventHandler("updateVoipTargetPed", function(newTargetPed, useLocal)
	targetPed = newTargetPed
	useLocalPed = useLocal
end)

-- Make exports available on first tick
exports("addPlayerToRadio", addPlayerToRadio);
exports("removePlayerFromRadio", removePlayerFromRadio);
exports("isPlayerInChannel", isPlayerInChannel);
