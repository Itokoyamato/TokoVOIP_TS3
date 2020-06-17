const bootTime = new Date();

const express = require('express');
const app = express();
const http = require('http').createServer(app);
const io = require('socket.io')(http);
const axios = require('axios');
const lodash = require('lodash');

let masterHeartbeatInterval;
const clients = {};

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
  const player = Object.values(clients).find(item => {
    return !item.ts3.socket && item.ip === req.query.ip;
  });
  if (!player) return res.status(404).send();
  return res.status(204).send();
});

io.on('connection', async socket => {
  if (!socket.request._query.from) return;

  socket.clientIp = socket.request.connection.remoteAddress.replace('::ffff:', '');
  if (socket.clientIp.includes('::1') || socket.clientIp.includes('127.0.0.1')) socket.clientIp = process.env.LOCAL_IP;

  socket.on('disconnect', _ => onSocketDisconnect(socket));

  // TS3 Handshake:
  // main entrypoint
  // fivem will wait for it to be ready and handshake after
  if (socket.request._query.from === 'ts3') {
    if (!socket.request._query.uuid) {
      socket.disconnect('UUID Required');
      return;
    }

    // If client already exist, reset it
    let client = clients[socket.request._query.uuid];
    if (client) {
      if (client.fivem.socket) client.fivem.socket.disconnect();
      if (client.ts3.socket) client.ts3.socket.disconnect();
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
    socket.from = 'ts3';
    client.ts3.socket = socket;
    client.ts3.linkedAt = (new Date()).toISOString();

    socket.on('setTS3Data', (data) => setTS3Data(socket, data));

  // FiveM Handshake:
  // will wait for TS3 handshake to be complete and retry until then
  } else if (socket.request._query.from === 'fivem') {
    let client;
    let tries = 0;
    while (!client) {
      ++tries;
      if (tries > 1) sleep(5000);
      if (tries > 12) {
        socket.disconnect('Failed to handshake with TS3 (Timeout after 60 seconds)');
        return;
      }
      client = Object.values(clients).find(item => !item.fivem.socket && item.ip === socket.clientIp);
    }
    socket.uuid = client.uuid;
    socket.from = 'fivem';
    client.fivem.socket = socket;
    client.fivem.linkedAt = (new Date()).toISOString();

    socket.on('data', (data) => onIncomingData(socket, data));
  }
});

function setTS3Data(socket, data) {
  const client = clients[socket.uuid];
  if (!client) return;
  lodash.set(client.ts3.data, data.key, data.value);
}

function onIncomingData(socket, data) {
  const client = clients[socket.uuid];
  if (!socket.uuid || !client || !client.ts3.socket) return;
  socket.tokoData = data;
  client.fivem.data = socket.tokoData;
  client.fivem.updatedAt = (new Date()).toISOString();
  client.ts3.socket.send(JSON.stringify(client.fivem.data));
}

function onSocketDisconnect(socket) {
  lodash.set(clients[socket.uuid], `${socket.from}`, {});
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
