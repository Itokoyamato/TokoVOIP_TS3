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
			if (self.lastNetworkUpdate >= self.networkRefreshRate) then		--	Update the ped's network data
				-- setPlayerData(GetPlayerName(-1), "voip:mode", self.mode, true);
				-- setPlayerData(GetPlayerName(-1), "radio:channel", self.data.radioChannel, true);
				-- setPlayerData(GetPlayerName(-1), "radio:talking", self.data.radioTalking, true);
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

function TokoVoip.sendDataToTS3(self)	--	Send usersdata to the Javascript Websocket
	SendNUIMessage(
		{
			type = "updateTokoVoip",
			data = "data = "..table.tostring(self.data)
		}
	);
end

function TokoVoip.updateTokoVoipInfo(self)	--	Update the top-left info
	local info = "";
	if (self.mode == 1) then
		info = "Normal";
	elseif (self.mode == 2) then
		info = "Whispering";
	elseif (self.mode == 3) then
		info = "Shouting";
	end

	if (self.data.radioTalking) then
		info = info .. " on radio ";
	end
	if (self.data.radioChannel ~= 0) then
		if (string.match(self.channels[self.data.radioChannel].name, "Call")) then
			info = info  .. "<br> [Phone] " .. self.channels[self.data.radioChannel].name;
		else
			info = info  .. "<br> [Radio] " .. self.channels[self.data.radioChannel].name;
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
			channel = self.data.TSChannelWait
		}
	);
	Citizen.CreateThread(function()
		while (true) do
			Citizen.Wait(5);

			if (IsControlPressed(0, Keys["LEFTSHIFT"])) then	--	Switch radio channels (main / shared)
				if (IsControlJustPressed(0, Keys["Z"]) and tablelength(self.myChannels) > 0) then
					local myChannels = {};
					local currentChannel = 0;
					local currentChannelID = 0;

					for channel, _ in pairs(self.myChannels) do
						if (channel == self.data.radioChannel) then
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
					self.data.radioChannel = currentChannelID;
					setPlayerData(GetPlayerName(PlayerId()), "radio:channel", currentChannelID, true);
					self.updateTokoVoipInfo(self);
				end
			elseif (IsControlJustPressed(0, Keys["Z"])) then	--	Switch proximity modes (normal / whisper / shout)
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


			if (IsControlPressed(0, Keys["CAPS"]) and self.data.radioChannel ~= 0) then	--	Talk on radio
				self.data.radioTalking = true;
				self.data.localRadioClicks = true;
				if (self.data.radioChannel > 3) then
					self.data.localRadioClicks = false;
				end
				if (not getPlayerData(GetPlayerName(PlayerId()), "radio:talking")) then
					setPlayerData(GetPlayerName(PlayerId()), "radio:talking", true, true);
				end
				self.updateTokoVoipInfo(self);
				if lastTalkState == false then
					--TriggerEvent("meCommand", " started talking on their radio/phone")
					lastTalkState = true
				end
			else
				self.data.radioTalking = false;
				if (getPlayerData(GetPlayerName(PlayerId()), "radio:talking")) then
					setPlayerData(GetPlayerName(PlayerId()), "radio:talking", false, true);
				end
				self.updateTokoVoipInfo(self);
				
				if lastTalkState == true then
					--TriggerEvent("meCommand", " stopped talking on their radio/phone")
					lastTalkState = false
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
