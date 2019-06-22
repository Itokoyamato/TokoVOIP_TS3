// ------------------------------------------------------------
// ------------------------------------------------------------
// ---- Author: Dylan 'Itokoyamato' Thuillier              ----
// ----                                                    ----
// ---- Email: itokoyamato@hotmail.fr                      ----
// ----                                                    ----
// ---- Resource: tokovoip_script                          ----
// ----                                                    ----
// ---- File: script.js                                    ----
// ------------------------------------------------------------
// ------------------------------------------------------------

// --------------------------------------------------------------------------------
// --	Using websockets to send data to TS3Plugin via local network
// --------------------------------------------------------------------------------

function getTickCount() {
	var date = new Date();
	var tick = date.getTime();
	return (tick);
}

var websocket;
var connected = false;
var lastPing = 0;
var lastReconnect = 0;
var pluginStatus = 0;
var pluginVersion = '0';
var pluginUUID = "";
var TSServer = ""
var TSChannel = "";
var TSChannelSupport= "";
var TSDownload= "";
var pluginLatestVersion = "";

function init() {
	console.log("TokoVOIP: attempt new connection");
	websocket = new WebSocket("ws://127.0.0.1:1337/tokovoip");

	websocket.onopen = function()
	{
		console.log("TokoVOIP: connection opened");
		connected = true;
		lastPing = getTickCount();
		updateTokovoipInfo("Connected to TeamSpeak", 1, 1, 2000);
	};

	websocket.onmessage = function(evt)
	{
		// Handle plugin status
		if (evt.data.includes("TokoVOIP status:"))
		{
			connected = true;
			lastPing = getTickCount();
			forcedInfo = false;
			var pluginStatus = evt.data.split(":")[1].replace(/\./g, '');
			setPluginStatus(parseInt(pluginStatus));
			switch (parseInt(pluginStatus)) {
				case 0:
					document.getElementById("pluginScreenStatus").innerHTML = "Plugin status: <font color='red'>Initializing</font>";
					updateTokovoipInfo("Initializing", 2, 1, 2000);
					break;
				case 1:
					document.getElementById("pluginScreenStatus").innerHTML = "Plugin status: <font color='red'>wrong TeamSpeak server</font><br>Join the server: <font color='yellow'>" + TSServer + "</font>";
					updateTokovoipInfo("Connected to the wrong TeamSpeak server", 2, 1, 2000);
					break;
				case 2:
					document.getElementById("pluginScreenStatus").innerHTML = "Plugin status: <font color='red'>wrong TeamSpeak channel</font><br>Join the channel: <font color='yellow'>" + TSChannel + "</font>";
					updateTokovoipInfo("Connected to the wrong TeamSpeak channel", 2, 1, 2000);
					break;
				case 3:
					document.getElementById("pluginScreenStatus").innerHTML = "Plugin status: <font color='green'>online</font>";
					break;
			}
		}

		// Handle plugin version
		if (evt.data.includes("TokoVOIP version:"))
		{
			setPluginVersion(evt.data.split(":")[1]);
			if (evt.data.split(":")[1] == pluginLatestVersion)
				document.getElementById("pluginScreenVersion").innerHTML = "Plugin version: <font color='green'>" + evt.data.split(":")[1] + "</font> (up-to-date)";
			else
				document.getElementById("pluginScreenVersion").innerHTML = "Plugin version: <font color='red'>" + evt.data.split(":")[1] + "</font> (" + pluginLatestVersion + " is available)";
		}

		// Handle plugin UUID
		if (evt.data.includes("TokoVOIP UUID:"))
		{
			setPluginUUID(evt.data.split(":")[1]);
		}

		// Handle talking states
		if (evt.data == "startedtalking")
		{
			$.post('http://tokovoip_script/setPlayerTalking', JSON.stringify({state: 1}));
		}
		if (evt.data == "stoppedtalking")
		{
			$.post('http://tokovoip_script/setPlayerTalking', JSON.stringify({state: 0}));
		}
	};

	websocket.onerror = function(evt)
	{
		console.log("TokoVOIP: error - " + evt.data);
	};

	websocket.onclose = function () {
		sendData("disconnect");
		var reason;
		if (event.code == 1000)
		    reason = "Normal closure, meaning that the purpose for which the connection was established has been fulfilled.";
		else if(event.code == 1001)
		    reason = "An endpoint is \"going away\", such as a server going down or a browser having navigated away from a page.";
		else if(event.code == 1002)
		    reason = "An endpoint is terminating the connection due to a protocol error";
		else if(event.code == 1003)
		    reason = "An endpoint is terminating the connection because it has received a type of data it cannot accept (e.g., an endpoint that understands only text data MAY send this if it receives a binary message).";
		else if(event.code == 1004)
		    reason = "Reserved. The specific meaning might be defined in the future.";
		else if(event.code == 1005)
		    reason = "No status code was actually present.";
		else if(event.code == 1006)
		   reason = "The connection was closed abnormally, e.g., without sending or receiving a Close control frame";
		else if(event.code == 1007)
		    reason = "An endpoint is terminating the connection because it has received data within a message that was not consistent with the type of the message (e.g., non-UTF-8 [http://tools.ietf.org/html/rfc3629] data within a text message).";
		else if(event.code == 1008)
		    reason = "An endpoint is terminating the connection because it has received a message that \"violates its policy\". This reason is given either if there is no other sutible reason, or if there is a need to hide specific details about the policy.";
		else if(event.code == 1009)
		   reason = "An endpoint is terminating the connection because it has received a message that is too big for it to process.";
		else if(event.code == 1010) // Note that this status code is not used by the server, because it can fail the WebSocket handshake instead.
		    reason = "An endpoint (client) is terminating the connection because it has expected the server to negotiate one or more extension, but the server didn't return them in the response message of the WebSocket handshake. <br /> Specifically, the extensions that are needed are: " + event.reason;
		else if(event.code == 1011)
		    reason = "A server is terminating the connection because it encountered an unexpected condition that prevented it from fulfilling the request.";
		else if(event.code == 1015)
		    reason = "The connection was closed due to a failure to perform a TLS handshake (e.g., the server certificate can't be verified).";
		else
		    reason = "Unknown reason";
		console.log("TokoVOIP: closed connection - " + reason);
		lastReconnect = getTickCount();
		connected = false;
		document.getElementById("pluginScreenStatus").innerHTML = "Plugin status: <font color='red'>not running</font>";
		document.getElementById("pluginScreenVersion").innerHTML = "Plugin version: <font color='red'>" + pluginLatestVersion + "</font> is available, it is required to play";
		updateTokovoipInfo("OFFLINE", 2, 1, 5000);
		setPluginStatus(0);
		init();
	};
}

function sendData(message) {
	if (websocket.readyState == websocket.OPEN) {
		websocket.send(message);
	}
}

function receivedClientCall(event) {
	if (event.data.type == "initializeSocket")
	{
		pluginLatestVersion = event.data.latestVersion;
		TSServer = event.data.TSServer;
		TSChannel = event.data.TSChannel;
		TSDownload = event.data.TSDownload;
		TSChannelSupport = event.data.TSChannelSupport;
		document.getElementById("TSServer").innerHTML = TSServer;
		document.getElementById("TSDownload").innerHTML = TSDownload;
		document.getElementById("TSChannelSupport").innerHTML = TSChannelSupport;
		lastPing = getTickCount();
		lastReconnect = getTickCount();
		init();
	}
	else if (event.data.type == "updateTokovoipInfo")
	{
		if (connected)
			updateTokovoipInfo(event.data.data, 1);
	}
	else if (event.data.type == "updateTokoVoip")
	{
		var timeout = getTickCount() - lastPing;
		var lastRetry = getTickCount() - lastReconnect;
		if (timeout >= 10000 && lastRetry >= 5000)
		{
			console.log("TokoVOIP: timed out - " + (timeout) + " - " + (lastRetry));
			lastReconnect = getTickCount();
			connected = false;
			document.getElementById("pluginScreenStatus").innerHTML = "Plugin status: <font color='red'>not running</font>";
			updateTokovoipInfo("OFFLINE", 2, 1, 5000);
			setPluginStatus(0);
			init();
		}
		if (connected) {
			if (parseInt(pluginVersion.replace(/\./g, '')) >= 120) {
				try {
					JSON.parse(event.data.data);
				} catch (e) {
					return console.log("TokoVOIP: error in JSON data", e);
				}
			}
			sendData(event.data.data);
		}
	}
	else if (event.data.type == "disconnect")
	{
		sendData("disconnect");
	}
	else if (event.data.type == "displayPluginScreen")
	{
		var toggle = (event.data.data == true) ? "block" : "none";
		document.getElementById("pluginScreen").style.display = toggle;
	}
}

var forcedInfo = false;
var forcedInfoInterval;
function updateTokovoipInfo(msg, code, force, forcetime) {
	document.getElementById("tokovoipInfo").style.fontSize = "12px";
	if (force && forcetime)
	{
		forcedInfo = true;
		if (forcedInfoInterval)
			clearInterval(forcedInfoInterval);
		forcedInfoInterval = setInterval(function(){forcedInfo = false;}, forcetime);
	}

	if (forcedInfo && !force)
		return (0);
	if (code == 1)
		document.getElementById("tokovoipInfo").style.color = "#01b0f0";
	else if (code == 2)
		document.getElementById("tokovoipInfo").style.color = "red";
	document.getElementById("tokovoipInfo").innerHTML = "[TokoVoip] " + msg;
}

function setPluginStatus(data) {
	if (data != pluginStatus) {
		$.post('http://tokovoip_script/setPluginStatus', JSON.stringify({msg: data}));
		pluginStatus = parseInt(data);
	}
}

function setPluginVersion(data) {
	if (pluginVersion != data)
	{
		$.post('http://tokovoip_script/setPluginVersion', JSON.stringify({msg: data}));
		pluginVersion = data;
	}
}

function setPluginUUID(data) {
	if (pluginUUID != data)
	{
		$.post('http://tokovoip_script/setPluginUUID', JSON.stringify({msg: data}));
		pluginUUID = data;
	}
}

window.addEventListener("message", receivedClientCall, false);
