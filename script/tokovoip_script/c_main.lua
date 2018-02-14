--------------------------------------------------------------------------------
--	Client: Voip data processed before sending it to TS3Plugin
--------------------------------------------------------------------------------

local isLoggedIn = true;
local targetPed;
local useLocalPed = true;

function initializeVoip()
	voip = TokoVoip:init(); -- Initialize TokoVoip and set default settings
	voip.refreshRate = 100; -- Rate at which the data is sent to the TSPlugin
	voip.networkRefreshRate = 2000; -- Rate at which the network data is updated/reset on the local ped
	voip.playerListRefreshRate = 5000; -- Rate at which the playerList is updated
	voip.latestVersion = 115;
	voip.distance = {};
	voip.distance[1] = 15; -- Normal speech
	voip.distance[2] = 5; -- Whisper
	voip.distance[3] = 40; -- Shout

	-- Data that will be sent to the TS plugin
	voip.data = {	
					TSChannelWait = "TokoVOIP Server Waiting Room IPS DESC", -- TeamSpeak waiting channel name
					TSChannel = "SERVER_1", -- TeamSpeak channel name
					TSPassword = "Revolution_pass", -- TeamSpeak channel password
					localName = GetPlayerName(PlayerId()), -- The local username
					radioTalking = false, -- Is the player talking on radio (used to force active mic on TS)
					radioChannel = 0, -- The radio channel
					localRadioClicks = false, -- Should the local radio clicks be active (mostly to handle phone calls with no clicks)
					-- The following is purely client settings, to match tastes
					local_click_on = true, -- Is local click on active
					local_click_off = true, -- Is local click off active
					remote_click_on = false, -- Is remote click on active
					remote_click_off = true, -- Is remote click off active
					Users = {} -- All users data
				};

	-- Variables used script-side
	voip.mode = 1;
	voip.talking = false;
	voip.pluginStatus = 0;
	voip.pluginVersion = 0;

	-- Radio channels
	voip.channels = {};
	voip.myChannels = {};

	-- Player data shared on the network
	setPlayerData(GetPlayerName(PlayerId()), "voip:mode", voip.mode, true);
	setPlayerData(GetPlayerName(PlayerId()), "voip:talking", voip.talking, true);
	setPlayerData(GetPlayerName(PlayerId()), "radio:channel", voip.data.radioChannel, true);
	setPlayerData(GetPlayerName(PlayerId()), "radio:talking", voip.data.radioTalking, true);
	setPlayerData(GetPlayerName(PlayerId()), "voip:pluginStatus", voip.pluginStatus, true);
	setPlayerData(GetPlayerName(PlayerId()), "voip:pluginVersion", voip.pluginVersion, true);

	targetPed = GetPlayerPed(-1);

	--	Remove for deploy	----------------------------------------------------
	local debugData = false;
	Citizen.CreateThread(function()
		while true do
			Wait(5)

			if (IsControlPressed(0, Keys["LEFTSHIFT"])) then
				if (IsControlJustPressed(1, Keys["0"]) or IsDisabledControlJustPressed(1, Keys["0"])) then
					debugData = not debugData;
				end
				
				if (IsDisabledControlJustPressed(1, Keys["1"])) then
					if (exports.GTALife:isPlayerCop()) then
						if (not voip.myChannels[1]) then
							addPlayerToRadio(1);
						else
							removePlayerFromRadio(1);
						end
					elseif (exports.GTALife:isPlayerFire()) then
						if (not voip.myChannels[2]) then
							addPlayerToRadio(2);
						else
							removePlayerFromRadio(2);
						end
					end
				elseif ((IsDisabledControlJustPressed(1, Keys["2"]))) then
					if (exports.GTALife:isPlayerCop() or exports.GTALife:isPlayerFire()) then
						if (not voip.myChannels[3]) then
							addPlayerToRadio(3);
						else
							removePlayerFromRadio(3);
						end
					end
				elseif ((IsDisabledControlJustPressed(1, Keys["3"]))) then
					if (exports.GTALife:isPlayerCop()) then
						if (not voip.myChannels[4]) then
							addPlayerToRadio(4);
						else
							removePlayerFromRadio(4);
						end
					end
				elseif ((IsDisabledControlJustPressed(1, Keys["4"]))) then
					if (exports.GTALife:isPlayerCop()) then
						if (not voip.myChannels[5]) then
							addPlayerToRadio(5);
						else
							removePlayerFromRadio(5);
						end
					end
				end
			end

			if (debugData) then
				for i, player in ipairs(getPlayers()) do
					if (i <= 12) then
						drawTxt(0.60 + i/15, 1.3, 1.0, 1.0, 0.2, GetPlayerName(player) .. "\nMode: " .. tostring(getPlayerData(GetPlayerName(player), "voip:mode")) .. "\nChannel: " .. tostring(getPlayerData(GetPlayerName(player), "radio:channel")) .. "\nRadioTalking: " .. tostring(getPlayerData(GetPlayerName(player), "radio:talking")) .. "\npluginStatus: " .. tostring(getPlayerData(GetPlayerName(player), "voip:pluginStatus")) .. "\npluginVersion: " .. tostring(getPlayerData(GetPlayerName(player), "voip:pluginVersion")), 255, 255, 255, 255);
					elseif (i < 24) then
						drawTxt(0.60 + (i-12)/15, 1.4, 1.0, 1.0, 0.2, GetPlayerName(player) .. "\nMode: " .. tostring(getPlayerData(GetPlayerName(player), "voip:mode")) .. "\nChannel: " .. tostring(getPlayerData(GetPlayerName(player), "radio:channel")) .. "\nRadioTalking: " .. tostring(getPlayerData(GetPlayerName(player), "radio:talking")) .. "\npluginStatus: " .. tostring(getPlayerData(GetPlayerName(player), "voip:pluginStatus")) .. "\npluginVersion: " .. tostring(getPlayerData(GetPlayerName(player), "voip:pluginVersion")), 255, 255, 255, 255);
					end
				end
			end
		end
	end);
	----------------------------------------------------------------------------

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

	Citizen.Trace("TokoVoip: Initialized script (1.2.3)\n");
end

function resourceStart(resource)
	if (resource == GetCurrentResourceName()) then	--	Initialize the script when this resource is started
		Citizen.CreateThread(function()
			Wait(3000);
			initializeVoip();
		end);
	end
end
AddEventHandler("onClientResourceStart", resourceStart);

function clientProcessing()
		local playerList = voip.playerList;
		local usersdata = {};

		for i = 1, #playerList do
			local player = playerList[i];
				if (GetPlayerPed(-1) and GetPlayerPed(player)) then
					if useLocalPed then
						localPos = GetEntityCoords(GetPlayerPed(-1));
					else
						localPos = GetEntityCoords(targetPed);
					end

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

					-- Set player's default data
					usersdata[i] = {	
								username = GetPlayerName(player),
								volume = -30,
								muted = 1,
								radioEffect = false
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
				end
		end
		voip.data.Users = usersdata;	--	Update TokoVoip's data
		voip.data.localName = GetPlayerName(PlayerId());		-- Update the localName
end

function addPlayerToRadio(channel)
	if voip.channels[channel] == nil then
		voip.channels[channel] = {name = "Call with " .. tostring(channel), subscribers = {}}
		voip.myChannels[channel] = true;
	end

	voip.channels[channel].subscribers[GetPlayerName(PlayerId())] = GetPlayerName(PlayerId());
	voip.myChannels[channel] = true;
	voip.data.radioChannel = channel;
	setPlayerData(GetPlayerName(PlayerId()), "radio:channel", channel, true);
	Chat("Joined channel: " .. voip.channels[channel].name);
	TriggerServerEvent("TokoVoip:addPlayerToRadio", channel, GetPlayerName(PlayerId()));
end
RegisterNetEvent("TokoVoip:addPlayerToRadio");
AddEventHandler("TokoVoip:addPlayerToRadio", addPlayerToRadio);

function removePlayerFromRadio(channel)
	voip.channels[channel].subscribers[GetPlayerName(PlayerId())] = nil;
	voip.myChannels[channel] = nil;
	if (voip.data.radioChannel == channel) then
		if (tablelength(voip.myChannels) > 0) then
			for channelID, _ in pairs(voip.myChannels) do
				voip.data.radioChannel = channelID;
				break;
			end
		else
			voip.data.radioChannel = 0;
		end
	end
	setPlayerData(GetPlayerName(PlayerId()), "radio:channel", 0, true);
	Chat("Left channel: " .. voip.channels[channel].name);
	TriggerServerEvent("TokoVoip:removePlayerFromRadio", channel, GetPlayerName(PlayerId()));
end
RegisterNetEvent("TokoVoip:removePlayerFromRadio");
AddEventHandler("TokoVoip:removePlayerFromRadio", removePlayerFromRadio);

function updateChannels(updatedChannels)
	voip.channels = updatedChannels;
end
RegisterNetEvent("TokoVoip:updateChannels");
AddEventHandler("TokoVoip:updateChannels", updateChannels);

function requestUpdateChannels()
	TriggerServerEvent("TokoVoip:clientRequestUpdateChannels");
end

function setPluginStatus(data)
	voip.pluginStatus = tonumber(data.msg);
	setPlayerData(GetPlayerName(PlayerId()), "voip:pluginStatus", voip.pluginStatus, true);
end
RegisterNUICallback("setPluginStatus", setPluginStatus);

function setPluginVersion(data)
	voip.pluginVersion = tonumber(data.msg);
	setPlayerData(GetPlayerName(PlayerId()), "voip:pluginVersion", voip.pluginVersion, true);
end
RegisterNUICallback("setPluginVersion", setPluginVersion);

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
		PlayFacialAnim(PlayerPedId(), "mic_chatter", "mp_facial");
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

function playerLoggedIn(toggle)
	if (toggle) then
		isLoggedIn = true;
	end
end
RegisterNetEvent("onPlayerLoggedIn");
AddEventHandler("onPlayerLoggedIn", playerLoggedIn);

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
