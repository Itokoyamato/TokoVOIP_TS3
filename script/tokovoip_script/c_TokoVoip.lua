------------------------------------------------------------
------------------------------------------------------------
---- Author: Dylan 'Itokoyamato' Thuillier              ----
----                                                    ----
---- Email: itokoyamato@hotmail.fr                      ----
----                                                    ----
---- Resource: tokovoip_script                          ----
----                                                    ----
---- File: c_TokoVoip.lua                               ----
----                                                    ----
---- Notice: please report any changes, bugs, fixes     ----
----         or improvements to the author              ----
----         This script and the methods used is not    ----
----         to be shared without the author's          ----
----         permission.                                ----
------------------------------------------------------------
------------------------------------------------------------

--------------------------------------------------------------------------------
--	Client: TokoVoip functions
--------------------------------------------------------------------------------

TokoVoip = {};
TokoVoip.__index = TokoVoip;
local lastTalkState = false

function TokoVoip.init()
	local self = setmetatable({}, TokoVoip);
	self.lastNetworkUpdate = 0;
	self.lastPlayerListUpdate = 0;
	self.playerList = {};
	return (self);
end

function TokoVoip.loop(self)
	Citizen.CreateThread(function()
		while (true) do
			Citizen.Wait(self.refreshRate);
			self:processFunction();
			self:sendDataToTS3(self);

			self.lastNetworkUpdate = self.lastNetworkUpdate + self.refreshRate;
			self.lastPlayerListUpdate = self.lastPlayerListUpdate + self.refreshRate;
			if (self.lastNetworkUpdate >= self.networkRefreshRate) then -- Update the ped's network data, currently unused since switched out decors
				-- setPlayerData(GetPlayerName(-1), "voip:mode", self.mode, true);
				-- setPlayerData(GetPlayerName(-1), "radio:channel", self.plugin_data.radioChannel, true);
				-- setPlayerData(GetPlayerName(-1), "radio:talking", self.plugin_data.radioTalking, true);
				-- setPlayerData(GetPlayerName(-1), "voip:pluginStatus", self.pluginStatus, true);
				-- setPlayerData(GetPlayerName(-1), "voip:pluginVersion", self.pluginVersion, true);
				self.lastNetworkUpdate = 0;
				self:updateTokoVoipInfo(self);
			end
			if (self.lastPlayerListUpdate >= self.playerListRefreshRate) then
				self.playerList = getPlayers();
				self.lastPlayerListUpdate = 0;
			end
		end
	end);
end

function TokoVoip.sendDataToTS3(self) -- Send usersdata to the Javascript Websocket
	SendNUIMessage(
		{
			type = "updateTokoVoip",
			data = "plugin_data = "..table.tostring(self.plugin_data)
		}
	);
end

function TokoVoip.updateTokoVoipInfo(self) -- Update the top-left info
	local info = "";
	if (self.mode == 1) then
		info = "Normal";
	elseif (self.mode == 2) then
		info = "Whispering";
	elseif (self.mode == 3) then
		info = "Shouting";
	end

	if (self.plugin_data.radioTalking) then
		info = info .. " on radio ";
	end
	if (self.talking == 1 or self.plugin_data.radioTalking) then
		info = "<font class='talking'>" .. info .. "</font>";
	end
	if (self.plugin_data.radioChannel ~= 0) then
		if (string.match(self.channels[self.plugin_data.radioChannel].name, "Call")) then
			info = info  .. "<br> [Phone] " .. self.channels[self.plugin_data.radioChannel].name;
		else
			info = info  .. "<br> [Radio] " .. self.channels[self.plugin_data.radioChannel].name;
		end
	end
	SendNUIMessage(
		{
			type = "updateTokovoipInfo",
			data = "" .. info
		}
	);
end

function TokoVoip.initialize(self)
	SendNUIMessage(
		{	
			type = "initializeSocket",
			latestVersion = self.latestVersion,
			TSServer = self.plugin_data.TSServer,
			TSChannel = self.plugin_data.TSChannelWait,
			TSChannelSupport = self.plugin_data.TSChannelSupport
		}
	);
	Citizen.CreateThread(function()
		while (true) do
			Citizen.Wait(5);

			if (IsControlPressed(0, Keys["LEFTSHIFT"])) then -- Switch radio channels (main / shared)
				if (IsControlJustPressed(0, Keys["Z"]) and tablelength(self.myChannels) > 0) then
					local myChannels = {};
					local currentChannel = 0;
					local currentChannelID = 0;

					for channel, _ in pairs(self.myChannels) do
						if (channel == self.plugin_data.radioChannel) then
							currentChannel = #myChannels + 1;
							currentChannelID = channel;
						end
						myChannels[#myChannels + 1] = {channelID = channel};
					end
					if (currentChannel == #myChannels) then
						currentChannelID = myChannels[1].channelID;
					else
						currentChannelID = myChannels[currentChannel + 1].channelID;
					end
					self.plugin_data.radioChannel = currentChannelID;
					setPlayerData(GetPlayerName(PlayerId()), "radio:channel", currentChannelID, true);
					self.updateTokoVoipInfo(self);
				end
			elseif (IsControlJustPressed(0, Keys["Z"])) then -- Switch proximity modes (normal / whisper / shout)
				if (not self.mode) then
					self.mode = 1;
				end
				self.mode = self.mode + 1;
				if (self.mode > 3) then
					self.mode = 1;
				end
				setPlayerData(GetPlayerName(PlayerId()), "voip:mode", self.mode, true);
				self.updateTokoVoipInfo(self);
			end


			if (IsControlPressed(0, self.radioKey) and self.plugin_data.radioChannel ~= 0) then -- Talk on radio
				self.plugin_data.radioTalking = true;
				self.plugin_data.localRadioClicks = true;
				if (self.plugin_data.radioChannel > 100) then
					self.plugin_data.localRadioClicks = false;
				end
				if (not getPlayerData(GetPlayerName(PlayerId()), "radio:talking")) then
					setPlayerData(GetPlayerName(PlayerId()), "radio:talking", true, true);
				end
				self.updateTokoVoipInfo(self);
				if lastTalkState == false then
					if (not string.match(self.channels[self.plugin_data.radioChannel].name, "Call") and not IsPedSittingInAnyVehicle(PlayerPedId())) then
						RequestAnimDict("random@arrests");
						while not HasAnimDictLoaded("random@arrests") do
							Wait(5);
						end
						TaskPlayAnim(PlayerPedId(),"random@arrests","generic_radio_chatter", 8.0, 0.0, -1, 49, 0, 0, 0, 0);
					end
					--TriggerEvent("meCommand", " started talking on their radio/phone")
					lastTalkState = true
				end
			else
				self.plugin_data.radioTalking = false;
				if (getPlayerData(GetPlayerName(PlayerId()), "radio:talking")) then
					setPlayerData(GetPlayerName(PlayerId()), "radio:talking", false, true);
				end
				self.updateTokoVoipInfo(self);
				
				if lastTalkState == true then
					--TriggerEvent("meCommand", " stopped talking on their radio/phone")
					lastTalkState = false
					StopAnimTask(PlayerPedId(), "random@arrests","generic_radio_chatter", -4.0);
				end
			end
		end
	end);
end

function TokoVoip.disconnect()
	SendNUIMessage(
		{	
			type = "disconnect"
		}
	);
end
