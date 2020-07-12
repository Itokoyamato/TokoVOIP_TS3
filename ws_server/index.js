const bootTime = new Date();

const express = require('express');
const app = express();
const http = require('http').createServer(app);
const io = require('socket.io')(http);
const axios = require('axios');
const lodash = require('lodash');
const chalk = require('chalk');
const config = require('./config.js');
const publicIp = require('public-ip');

let hostIP;
let runningOnFivem = false;

try {
  eval('GetResourcePath(GetCurrentResourceName())');
  runningOnFivem = true;
} catch(e) {}

require('console-stamp')(console, { pattern: 'dd/mm/yyyy HH:MM:ss.l' });

let masterHeartbeatInterval;
const clients = {};

const handshakes = [];

console.log(chalk`Like {cyan TokoVOIP} ? Consider supporting the development: {hex('#f96854') https://patreon.com/Itokoyamato}`);

app.use(express.json());

(async _ => {
  config.TSServer = process.env.TSServer || config.TSServer;
  config.WSServerPort = parseInt(process.env.WSServerPort, 10) || parseInt(config.WSServerPort, 10);
  config.WSServerIP = process.env.WSServerIP || config.WSServerIP;
  config.FivemServerIP = process.env.FivemServerIP || config.FivemServerIP;
  config.FivemServerPort = parseInt(process.env.FivemServerPort, 10) || parseInt(config.FivemServerPort, 10);
  hostIP = await publicIp.v4();
  if (config.WSServerIP === undefined) {
    config.WSServerIP = hostIP;
    console.log(chalk`{yellow AUTOCONFIG:} Setting {cyan WSServerIP} to {cyan ${hostIP}} (you can manually edit in config.js)`);
    await sleep(0);
  }
  if (config.FivemServerIP === undefined) {
    config.FivemServerIP = hostIP;
    console.log(chalk`{yellow AUTOCONFIG:} Setting {cyan FivemServerIP} to {cyan ${hostIP}} (you can manually edit in config.js)`);
    await sleep(0);
  }
  if (config.FivemServerPort === undefined && runningOnFivem) {
    config.FivemServerPort = GetConvar('netPort');
    console.log(chalk`{yellow AUTOCONFIG:} Setting {cyan FivemServerPort} to {cyan ${config.FivemServerPort}} (you can manually edit in config.js)`);
    await sleep(0);
  }

  if (!config.TSServer || !config.WSServerIP || !config.WSServerPort || !config.FivemServerIP || !config.FivemServerPort) {
    console.error(chalk`{red Config error:
Missing one of TSServer, WSServerIP, WSServerPort, FivemServerIP or FivemServerPort}`
    );
    return;
  }

  // Boot
  http.listen(config.WSServerPort, async _ => {
    console.log('Checking configuration ...');
    let configError = false;
    const IPv4Regex = new RegExp('^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$');
    if (!IPv4Regex.test(config.TSServer)) {
      configError = true;
      console.error(chalk`{red Config error:
TSServer is invalid.
It must be an IPv4 address.
Domain names are not supported.}`
      );
    }

    const FiveMURI = `http://${config.FivemServerIP}:${config.FivemServerPort}/info.json`;
    await axios.get(FiveMURI)
    .catch(e => {
      configError = true
      console.error(chalk`{red Config error:
FiveM server does not seem online.
Is it accessible from the internet ?
Make sure your configuration is correct and your ports are open.}
{cyan (${FiveMURI})}`
      );
    });
    const wsURI = `http://${config.WSServerIP}:${config.WSServerPort}`
    await axios.get(wsURI)
    .catch(e => {
      configError = true;
      console.error(chalk`{red Config error:
Could not access WS server.
Is it accessible from the internet ?
Make sure your configuration is correct and your ports are open.}
{cyan (${wsURI})}`
      );
    });

    if (config.FivemServerIP.includes('127.0.0.1') || config.FivemServerIP.includes('localhost')) {
      configError = true;
      console.error(chalk`{red Config error:
FiveMServerIP cannot be 127.0.0.1 or localhost.
It will be blocked by FiveM.
It has to be a valid public IPv4.
You might need to open your ports to run it locally.}`);
    }
    if (configError) return;

    console.log(chalk`Everything looks {green good} ! Have fun`);
    await sleep(0);

    console.log(chalk`Listening on "{cyan ${config.WSServerIP}:${config.WSServerPort}}" (copy and paste this address as "wsServer" in tokovoip_script/c_config.lua)`);

    masterHeartbeat();
    masterHeartbeatInterval = setInterval(masterHeartbeat, 300000);
  });
})()

app.get('/', (_, res) => {
  res.send({
    started: bootTime.toISOString(),
    uptime: process.uptime(),
  });
});

app.get('/playerbyip', (req, res) => {
  const player = handshakes.find(item => item.clientIp === req.query.ip);
  if (!player) return res.status(404).send();
  return res.status(204).send();
});

http.on('upgrade', (req, socket) => {
  if (!req._query || !req._query.from) return socket.destroy();
  if (req._query.from === 'ts3' && !req._query.uuid) return socket.destroy();
});

io.on('connection', async socket => {
  socket.from = socket.request._query.from;
  socket.clientIp = socket.handshake.headers['x-forwared-for'] || socket.request.connection.remoteAddress.replace('::ffff:', '');
  socket.safeIp = Buffer.from(socket.clientIp).toString('base64');
  if (socket.clientIp.includes('::1') || socket.clientIp.includes('127.0.0.1')) socket.clientIp = process.env.LOCAL_IP;

  socket.on('disconnect', _ => onSocketDisconnect(socket));

  log('log', chalk`{${socket.from === 'ts3' ? 'cyan' : 'yellow'} ${socket.from}} | Connection {cyan opened} - ${socket.safeIp}`);

  // TS3 Handshake
  if (socket.from === 'ts3') {
    let client = clients[socket.request._query.uuid];
    socket.uuid = socket.request._query.uuid;

    const handshake = handshakes.findIndex(item => item.clientIp === socket.clientIp);
    if (handshake === -1) {
      socket.emit('disconnectMessage', 'handshakeNotFound');
      socket.disconnect(true);
      return;
    }

    // Initialize client data
    client = clients[socket.request._query.uuid] = {
      ip: socket.clientIp,
      uuid: socket.request._query.uuid,
      fivem: {},
      ts3: {
        uuid: socket.request._query.uuid,
      },
    };
    client.ts3.socket = socket;
    client.ts3.linkedAt = (new Date()).toISOString();
    handshakes.splice(handshake, 1);

    log('log', chalk`{${socket.from === 'ts3' ? 'cyan' : 'yellow'} ${socket.from}} | Handshake {green successful} - ${socket.safeIp}`);

    socket.on('setTS3Data', data => setTS3Data(socket, data));
    socket.on('onTalkStatusChanged', data => setTS3Data(socket, { key: 'talking', value: data }));
    socketHeartbeat(socket);

  // FiveM Handshake
  } else if (socket.from === 'fivem') {
    socketHeartbeat(socket);
    await registerHandshake(socket);
    socket.on('data', (data) => onIncomingData(socket, data));
  }
});

async function registerHandshake(socket) {
  handshakes.push(socket);
  let client;
  let tries = 0;
  while (!client) {
    ++tries;
    if (tries > 1) await sleep(5000);
    if (tries > 12) {
      socket.emit('disconnectMessage', 'ts3HandshakeFailed');
      socket.disconnect(true);
      return;
    }
    client = Object.values(clients).find(item => !item.fivem.socket && item.ip === socket.clientIp);
    await axios.post('https://master.tokovoip.itokoyamato.net/register', {
      ip: socket.clientIp,
      server: {
        tsServer: config.TSServer,
        ip: config.WSServerIP,
        port: config.WSServerPort,
      },
    });
  }
  socket.uuid = client.uuid;
  client.fivem.socket = socket;
  client.fivem.linkedAt = (new Date()).toISOString();
  if (lodash.get(client, 'ts3.data.uuid')) socket.emit('setTS3Data', client.ts3.data);
}

function setTS3Data(socket, data) {
  const client = clients[socket.uuid];
  if (!client) return;
  lodash.set(client.ts3, `data.${data.key}`, data.value);
  if (client.fivem.socket) {
    client.fivem.socket.emit('setTS3Data', client.ts3.data);
  }
}

function onIncomingData(socket, data) {
  const client = clients[socket.uuid];
  if (!socket.uuid || !client || !client.ts3.socket) return;
  socket.tokoData = data;
  client.fivem.data = socket.tokoData;
  client.fivem.updatedAt = (new Date()).toISOString();
  client.ts3.socket.emit('processTokovoip', client.fivem.data);
}

async function onSocketDisconnect(socket) {
  log('log', chalk`{${socket.from === 'ts3' ? 'cyan' : 'yellow'} ${socket.from}} | Connection {red lost} - ${socket.safeIp}`);
  if (socket.from === 'fivem') {
    const handshake = handshakes.findIndex(item => item == socket);
    if (handshake !== -1) handshakes.splice(handshake, 1);
  }
  if (socket.uuid && clients[socket.uuid]) {
    const client = clients[socket.uuid];
    delete clients[socket.uuid];
    const secondary = (socket.from === 'fivem') ? 'ts3' : 'fivem';
    if (client[secondary].socket) {
      client[secondary].socket.emit('disconnectMessage', `${socket.from}Disconnected`);
      if (secondary === 'ts3') sleep(100) && client[secondary].socket.disconnect(true);
      else registerHandshake(client[secondary].socket);
    }
  }
}

function socketHeartbeat(socket) {
  if (!socket) return;
  const start = new Date();
  socket.emit('ping');
  socket.once('pong', _ => {
    setTimeout(_ => socketHeartbeat(socket), 1000);
    socket.latency = (new Date).getTime() - start.getTime();
    if (!socket.uuid || !clients[socket.uuid]) return;
    clients[socket.uuid].latency = lodash.get(clients[socket.uuid], 'fivem.latency', 0) + lodash.get(clients[socket.uuid], 'ts3.latency', 0);
  });
}

async function masterHeartbeat() {
  axios.post('https://master.tokovoip.itokoyamato.net/heartbeat', {
    tsServer: config.TSServer,
    WSServerIP: config.WSServerIP,
    WSServerPort: config.WSServerPort,
    FivemServerIP: config.FivemServerIP,
    FivemServerPort: config.FivemServerPort,
  })
  .then(_ => console.log('Heartbeat sent'))
  .catch(e => console.error('Sending heartbeat failed with error:', e.code));
}

const sleep = ms => new Promise(resolve => setTimeout(resolve, ms));

function log(type, msg) {
  if (!config.enableLogs) return;
  console[type](msg);
}
