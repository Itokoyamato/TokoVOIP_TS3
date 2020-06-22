const bootTime = new Date();

const express = require('express');
const app = express();
const http = require('http').createServer(app);
const io = require('socket.io')(http);
const axios = require('axios');
const lodash = require('lodash');

let masterHeartbeatInterval;
const clients = {};

const handshakes = [];

app.use(express.json());

http.listen(3000, () => {
  console.log('Listening on *:3000');
  masterHeartbeat();
  masterHeartbeatInterval = setInterval(masterHeartbeat, 30000);
});

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
  if (!req._query.from) socket.destroy();
  if (req._query.from === 'ts3' && !req._query.uuid) socket.destroy();
});

io.on('connection', async socket => {

  socket.clientIp = socket.request.connection.remoteAddress.replace('::ffff:', '');
  if (socket.clientIp.includes('::1') || socket.clientIp.includes('127.0.0.1')) socket.clientIp = process.env.LOCAL_IP;

  socket.on('disconnect', _ => onSocketDisconnect(socket));

  // TS3 Handshake
  if (socket.request._query.from === 'ts3') {
    socket.from = 'ts3';

    let client = clients[socket.request._query.uuid];

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
    socket.uuid = socket.request._query.uuid;
    client.ts3.socket = socket;
    client.ts3.linkedAt = (new Date()).toISOString();
    handshakes.splice(handshake, 1);

    socket.on('setTS3Data', data => setTS3Data(socket, data));
    socket.on('onTalkStatusChanged', data => setTS3Data(socket, { key: 'talking', value: data }));
    socketHeartbeat(socket);

  // FiveM Handshake
  } else if (socket.request._query.from === 'fivem') {
    socket.from = 'fivem';
    handshakes.push(socket);
    socketHeartbeat(socket);
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
    }
    socket.uuid = client.uuid;
    client.fivem.socket = socket;
    client.fivem.linkedAt = (new Date()).toISOString();
    if (client.ts3.data && client.ts3.data.uuid) {
      socket.emit('setTS3Data', client.ts3.data);
    }

    socket.on('data', (data) => onIncomingData(socket, data));
  }
});

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
  if (socket.uuid && clients[socket.uuid]) {
    const client = clients[socket.uuid];
    const secondary = (socket.from === 'fivem') ? 'ts3' : 'fivem';
    if (client[secondary].socket) {
      client[secondary].socket.emit('disconnectMessage', `${socket.from}Disconnected`);
      client[secondary].socket.disconnect(true);
    }
    delete clients[socket.uuid];
  }
  if (!socket.from === 'fivem') return;
  const handshake = handshakes.findIndex(item => item == socket);
  if (handshake !== -1) handshakes.splice(handshake, 1);
}

function socketHeartbeat(socket) {
  if (!socket) return;
  const start = new Date();
  socket.emit('ping');
  socket.once('pong', _ => {
    setTimeout(_ => socketHeartbeat(socket), 1000);
    if (!socket.uuid || !clients[socket.uuid]) return;
    socket.latency = (new Date).getTime() - start.getTime();
    clients[socket.uuid][socket.from].latency = socket.latency;
  });
}

function masterHeartbeat() {
  console.log('Heartbeat sent');
  axios.post('http://localhost:3005/heartbeat', {
    tsServer: '127.0.0.1',
    ip: '127.0.0.1',
    port: '3000',
  }).catch(e => console.error('Sending heartbeat failed with error: ', e));
}

const sleep = ms => new Promise(resolve => setTimeout(resolve, ms));
