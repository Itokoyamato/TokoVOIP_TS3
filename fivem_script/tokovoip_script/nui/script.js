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

let voip = {};

const OK = 0;
const NOT_CONNECTED = 1;
const PLUGIN_INITIALIZING = 2;
const WRONG_SERVER = 3;
const WRONG_CHANNEL = 4;
const INCORRECT_VERSION = 5;

function init() {
	console.log('TokoVOIP: attempt new connection');
	websocket = new WebSocket('ws://127.0.0.1:38204/tokovoip');

	websocket.onopen = () => {
		console.log('TokoVOIP: connection opened');
		connected = true;
		lastPing = getTickCount();
	};

	websocket.onmessage = (evt) => {
		// Handle plugin status
		if (evt.data.includes('TokoVOIP status:')) {
			connected = true;
			lastPing = getTickCount();
			forcedInfo = false;
			const pluginStatus = evt.data.split(':')[1].replace(/\./g, '');
			updatePluginData('pluginStatus', parseInt(pluginStatus));
		}

		// Handle plugin version
		if (evt.data.includes('TokoVOIP version:')) {
			updatePluginData('pluginVersion', evt.data.split(':')[1]);
		}

		// Handle plugin UUID
		if (evt.data.includes('TokoVOIP UUID:')) {
			updatePluginData('pluginUUID', evt.data.split(':')[1]);
		}

		// Handle talking states
		if (evt.data == 'startedtalking') {
			$.post('http://tokovoip_script/setPlayerTalking', JSON.stringify({state: 1}));
		}
		if (evt.data == 'stoppedtalking') {
			$.post('http://tokovoip_script/setPlayerTalking', JSON.stringify({state: 0}));
		}
	};

	websocket.onerror = (evt) => {
		console.log('TokoVOIP: error - ' + evt.data);
	};

	websocket.onclose = () => {
		sendData('disconnect');

		let reason;
		if (event.code == 1000)
		    reason = 'Normal closure, meaning that the purpose for which the connection was established has been fulfilled.';
		else if(event.code == 1001)
		    reason = 'An endpoint is \'going away\', such as a server going down or a browser having navigated away from a page.';
		else if(event.code == 1002)
		    reason = 'An endpoint is terminating the connection due to a protocol error';
		else if(event.code == 1003)
		    reason = 'An endpoint is terminating the connection because it has received a type of data it cannot accept (e.g., an endpoint that understands only text data MAY send this if it receives a binary message).';
		else if(event.code == 1004)
		    reason = 'Reserved. The specific meaning might be defined in the future.';
		else if(event.code == 1005)
		    reason = 'No status code was actually present.';
		else if(event.code == 1006)
		   reason = 'The connection was closed abnormally, e.g., without sending or receiving a Close control frame';
		else if(event.code == 1007)
		    reason = 'An endpoint is terminating the connection because it has received data within a message that was not consistent with the type of the message (e.g., non-UTF-8 [http://tools.ietf.org/html/rfc3629] data within a text message).';
		else if(event.code == 1008)
		    reason = 'An endpoint is terminating the connection because it has received a message that \'violates its policy\'. This reason is given either if there is no other sutible reason, or if there is a need to hide specific details about the policy.';
		else if(event.code == 1009)
		   reason = 'An endpoint is terminating the connection because it has received a message that is too big for it to process.';
		else if(event.code == 1010) // Note that this status code is not used by the server, because it can fail the WebSocket handshake instead.
		    reason = 'An endpoint (client) is terminating the connection because it has expected the server to negotiate one or more extension, but the server didn\'t return them in the response message of the WebSocket handshake. <br /> Specifically, the extensions that are needed are: ' + event.reason;
		else if(event.code == 1011)
		    reason = 'A server is terminating the connection because it encountered an unexpected condition that prevented it from fulfilling the request.';
		else if(event.code == 1015)
		    reason = 'The connection was closed due to a failure to perform a TLS handshake (e.g., the server certificate can\'t be verified).';
		else
			reason = 'Unknown reason';

		console.log('TokoVOIP: closed connection - ' + reason);
		lastReconnect = getTickCount();
		connected = false;
		updatePluginData('pluginStatus', -1);
		init();
	};
}

function sendData(message) {
	if (websocket.readyState == websocket.OPEN) {
		websocket.send(message);
	}
}

function receivedClientCall(event) {
	// Start with a status OK by default, and change the status if issues are encountered
	voipStatus = OK;

	if (event.data.voipData) {
		voip = event.data.voipData;
	}

	if (event.data.type == 'initializeSocket') {
		
		document.getElementById('TSServer').innerHTML = `TeamSpeak server: <font color="#01b0f0">${voip.plugin_data.TSServer}</font>`;
		document.getElementById('TSChannel').innerHTML = `TeamSpeak channel: <font color="#01b0f0">${(voip.plugin_data.TSChannelWait) ? voip.plugin_data.TSChannelWait : voip.plugin_data.TSChannel}</font>`;
		document.getElementById('TSDownload').innerHTML = voip.plugin_data.TSDownload;
		document.getElementById('TSChannelSupport').innerHTML = voip.plugin_data.TSChannelSupport;
		lastPing = getTickCount();
		lastReconnect = getTickCount();
		init();
	} else if (event.data.type == "updateTokovoipInfo") {
		if (connected)
			updateTokovoipInfo(event.data.data, 1);
	} else if (event.data.type == 'updateTokoVoip') {
		const timeout = getTickCount() - lastPing;
		const lastRetry = getTickCount() - lastReconnect;
		if (timeout >= 10000 && lastRetry >= 5000) {
			console.log('TokoVOIP: timed out - ' + (timeout) + ' - ' + (lastRetry));
			lastReconnect = getTickCount();
			connected = false;
			updatePluginData('pluginStatus', -1);
			init();
		} else if (connected) {
			try {
				JSON.parse(event.data.data);
			} catch (e) {
				return console.log('TokoVOIP: error in JSON data', e);
			}
			sendData(event.data.data);
		}
	} else if (event.data.type == 'disconnect') {
		sendData('disconnect');
		voipStatus = NOT_CONNECTED;
	}

	checkPluginStatus();
	if (voipStatus != NOT_CONNECTED)
		checkPluginVersion();

	if (voipStatus != OK) {
		displayPluginScreen(true);
	} else {
		displayPluginScreen(false);
	}

	updateTokovoipInfo();
}

function checkPluginStatus() {
	switch (parseInt(voip.pluginStatus)) {
		case -1:
			voipStatus = NOT_CONNECTED;
			break;
		case 0:
			voipStatus = PLUGIN_INITIALIZING;
			break;
		case 1:
			voipStatus = WRONG_SERVER;
			break;
		case 2:
			voipStatus = WRONG_CHANNEL;
			break;
		case 3:
			voipStatus = OK;
			break;
	}
	if (getTickCount() - lastPing > 5000)
		voipStatus = NOT_CONNECTED;
}

function checkPluginVersion() {
	if (isPluginVersionCorrect()) {
		document.getElementById('pluginVersion').innerHTML = `Plugin version: <font color="green">${voip.pluginVersion}</font> (up-to-date)`;
	} else {
		document.getElementById('pluginVersion').innerHTML = `Plugin version: <font color="red">${voip.pluginVersion}</font> (Required: ${voip.minVersion})`;
		voipStatus = INCORRECT_VERSION;
	}
}

function isPluginVersionCorrect() {
	if (parseInt(voip.pluginVersion.replace(/\./g, '')) < parseInt(voip.minVersion.replace(/\./g, ''))) return false;
	return true;
}

function displayPluginScreen(toggle) {
	document.getElementById('pluginScreen').style.display = (toggle) ? 'block' : 'none';
}

function updateTokovoipInfo(msg) {
	document.getElementById('tokovoipInfo').style.fontSize = '12px';
	let screenMessage;

	switch (voipStatus) {
		case NOT_CONNECTED:
			msg = 'OFFLINE';
			color = 'red';
			break;
		case PLUGIN_INITIALIZING:
			msg = 'Initializing';
			color = 'red';
			break;
		case WRONG_SERVER:
			msg = `Connected to the wrong TeamSpeak server, please join the server: <font color="#01b0f0">${voip.plugin_data.TSServer}</font>`;
			screenMessage = 'Wrong TeamSpeak server';
			color = 'red';
			break;
		case WRONG_CHANNEL:
			msg = `Connected to the wrong TeamSpeak channel, please join the channel: <font color="#01b0f0">${voip.plugin_data.TSChannelWait && voip.plugin_data.TSChannelWait !== '' && voip.plugin_data.TSChannelWait || voip.plugin_data.TSChannel}</font>`;
			screenMessage = 'Wrong TeamSpeak channel';
			color = 'red';
			break;
		case INCORRECT_VERSION:
			msg = 'Using incorrect plugin version';
			screenMessage = 'Incorrect plugin version';
			color = 'red';
			break;
		case OK:
			color = '#01b0f0';
			break;
	}
	if (msg) {
		document.getElementById('tokovoipInfo').innerHTML = `<font color="${color}">[TokoVoip] ${msg}</font>`;
	}
	document.getElementById('pluginStatus').innerHTML = `Plugin status: <font color="${color}">${screenMessage || msg}</font>`;
}

function updatePluginData(key, data) {
	if (voip[key] === data) return;
	$.post(`http://tokovoip_script/updatePluginData`, JSON.stringify({
		payload: {
			key,
			data,
		}
	}));
}

window.addEventListener('message', receivedClientCall, false);
