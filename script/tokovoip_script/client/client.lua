--------------------------------------------------------------------------------
--	Client: Voip data processed before sending it to TS3Plugin
--------------------------------------------------------------------------------

local isLoggedIn = true;
local targetPed;
local useLocalPed = true;

function initializeVoip()
	voip = TokoVoip:init();	--	Initialize TokoVoip and set default settings
	voip.refreshRate = 100;	--	Rate at which the data is sent to the TSPlugin
	voip.networkRefreshRate = 2000;	--	Rate at which the network data is updated/reset on the local ped
	voip.playerListRefreshRate = 5000;	--	Rate at which the playerList is updated
	voip.latestVersion = 109;
	voip.distance = {};
	voip.distance[1] = 15;	--	Normal speech
	voip.distance[2] = 5;	--	Whisper
	voip.distance[3] = 40;	--	Shout
	voip.data = {	
					TSChannelWait = "TokoVOIP Server Waiting Room IPS DESC",	--	TeamSpeak waiting channel name
					TSChannel = "SERVER_3",	--	TeamSpeak channel name
					TSPassword = "pass",	--	TeamSpeak channel password
					localName = GetPlayerName(PlayerId()),	--	The local username
					radioTalking = false,
					radioChannel = 0,
					localRadioClicks = false,
					local_click_on = true,
					local_click_off = true,
					remote_click_on = true,
					remote_click_off = true,
					Users = {}
				};
	voip.mode = 1;
	voip.pluginStatus = 0;
	voip.pluginVersion = 0;
	voip.channels = {
						{name = "Police Radio", subscribers = {}},
						{name = "EMS Radio", subscribers = {}},
						{name = "Police/EMS Shared Radio", subscribers = {}},
						{name = "Call with bleh", subscribers = {}},
	};
	voip.myChannels = {};

	DecorRegister("voip:mode",  3);
	DecorRegister("voip:talking",  3);
	DecorRegister("radio:channel",  3);
	DecorRegister("radio:talking",  3);
	DecorRegister("voip:pluginStatus",  3);
	DecorRegister("voip:pluginVersion",  3);

	DecorSetInt(GetPlayerPed(-1), "voip:pluginStatus", 0);
	DecorSetInt(GetPlayerPed(-1), "voip:pluginVersion", 0);
	DecorSetInt(GetPlayerPed(-1), "voip:mode", 1);

	targetPed = GetPlayerPed(-1);

	--	Remove for deploy	----------------------------------------------------
	local debugData = false;
	Citizen.CreateThread(function()
		while true do
			Wait(5)

			if (IsControlPressed(0, Keys["LEFTSHIFT"])) then
				if (IsControlJustPressed(1, Keys["3"]) and DecorGetInt(GetPlayerPed(-1), "voip:pluginStatus")) then
					debugData = not debugData;
				end
				
				if (IsDisabledControlJustPressed(1, Keys["1"]) and exports.GTALife:isPlayerCop()) then
					if (not voip.myChannels[1]) then
						addPlayerToRadio(1);
						addPlayerToRadio(3);
					else
						removePlayerFromRadio(1);
						removePlayerFromRadio(3);
					end
				end

				if (IsDisabledControlJustPressed(1, Keys["2"]) and exports.GTALife:isPlayerFire()) then
					if (not voip.myChannels[2]) then
						addPlayerToRadio(2);
						addPlayerToRadio(3);
					else
						removePlayerFromRadio(2);
						removePlayerFromRadio(3);
					end
				end
			end

			if (debugData) then
				for i, player in ipairs(getPlayers()) do
					if (i <= 12) then
						drawTxt(0.60 + i/15, 1.3, 1.0, 1.0, 0.2, GetPlayerName(player) .. "\nMode: " .. tostring(DecorGetInt(GetPlayerPed(player), "voip:mode")) .. "\nChannel: " .. tostring(DecorGetInt(GetPlayerPed(player), "radio:channel")) .. "\nRadioTalking: " .. tostring(DecorGetInt(GetPlayerPed(player), "radio:talking")) .. "\npluginStatus: " .. tostring(DecorGetInt(GetPlayerPed(player), "voip:pluginStatus")) .. "\npluginVersion: " .. tostring(DecorGetInt(GetPlayerPed(player), "voip:pluginVersion")), 255, 255, 255, 255);
					elseif (i < 24) then
						drawTxt(0.60 + (i-12)/15, 1.4, 1.0, 1.0, 0.2, GetPlayerName(player) .. "\nMode: " .. tostring(DecorGetInt(GetPlayerPed(player), "voip:mode")) .. "\nChannel: " .. tostring(DecorGetInt(GetPlayerPed(player), "radio:channel")) .. "\nRadioTalking: " .. tostring(DecorGetInt(GetPlayerPed(player), "radio:talking")) .. "\npluginStatus: " .. tostring(DecorGetInt(GetPlayerPed(player), "voip:pluginStatus")) .. "\npluginVersion: " .. tostring(DecorGetInt(GetPlayerPed(player), "voip:pluginVersion")), 255, 255, 255, 255);
					end
				end
			end
		end
	end);
	----------------------------------------------------------------------------

	voip.processFunction = clientProcessing;	--	Link the processing function that will be looped
	voip.initialize(voip);	--	Initialize the websocket and controls
	voip:loop(voip);	--	Start TokoVoip's loop

	Citizen.Trace("TokoVoip: Initialized script (1.1.9)\n");

	-- exports.pNotify:SendNotification(
	-- {
	-- 	text = "TokoVoip Started", 
	-- 	type = "info", 
	-- 	timeout = 3000,
	-- 	layout = "centerRight"
	-- })

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
				--local localPos = GetEntityCoords(GetPlayerPed(-1));
				if useLocalPed then
					localPos = GetEntityCoords(GetPlayerPed(-1));
				else
					localPos = GetEntityCoords(targetPed);
				end

				local playerPos = GetEntityCoords(GetPlayerPed(player));
				local dist = GetDistanceBetweenCoords(localPos, playerPos);

				if (not DecorGetInt(GetPlayerPed(player), "voip:mode")) then
					DecorSetInt(GetPlayerPed(player), "voip:mode", 1);
				end

				--	Process the volume for proximity voip
				local mode = tonumber(DecorGetInt(GetPlayerPed(player), "voip:mode"));
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
				local remotePlayerUsingRadio = DecorGetInt(GetPlayerPed(player), "radio:talking");
				local remotePlayerChannel = DecorGetInt(GetPlayerPed(player), "radio:channel");

				for j, data in pairs(voip.channels) do
					local channel = voip.channels[j];
					if (channel.subscribers[GetPlayerName(PlayerId())] and channel.subscribers[GetPlayerName(player)] and voip.myChannels[remotePlayerChannel] and remotePlayerUsingRadio == 1) then
						if (not string.match(channel.name, "Call")) then
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
	DecorSetInt(GetPlayerPed(-1), "radio:channel", channel);
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
	DecorSetInt(GetPlayerPed(-1), "radio:channel", 0);
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

function setPluginStatus(data)
	voip.pluginStatus = tonumber(data.msg);
	DecorSetInt(GetPlayerPed(-1), "voip:pluginStatus", voip.pluginStatus);
end
RegisterNUICallback("setPluginStatus", setPluginStatus);

function setPluginVersion(data)
	voip.pluginVersion = tonumber(data.msg);
end
RegisterNUICallback("setPluginVersion", setPluginVersion);

function setPlayerTalking(data)
	voip.talking = tonumber(data.state);
	if (voip.talking == 1) then
		DecorSetInt(GetPlayerPed(-1), "voip:talking", 1);
		RequestAnimDict("mp_facial");
		while not HasAnimDictLoaded("mp_facial") do
			Wait(5);
		end
		local localPos = GetEntityCoords(GetPlayerPed(-1));
		local localHeading = GetEntityHeading(GetPlayerPed(-1));
		TaskPlayAnim(PlayerPedId(),"mp_facial","mic_chatter", 8.0, 0.0, -1, 32, 0, 0, 0, 0);
	else
		DecorSetInt(GetPlayerPed(-1), "voip:talking", 0);
		StopAnimTask(PlayerPedId(), 'mp_facial', 'mic_chatter', 10.0);
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


function updateVoipTargetPed(newTargetPed, useLocal)
	targetPed = newTargetPed
	useLocalPed = useLocal
end

AddEventHandler("updateVoipTargetPed", updateVoipTargetPed)