const bootTime = new Date();

const express = require('express');
const app = express();
const http = require('http').createServer(app);
const io = require('socket.io')(http);
const axios = require('axios');
const uuid = require('uuid').v4;

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

io.on('connection', socket => {
  socket.clientIp = socket.request.connection.remoteAddress.replace('::ffff:', '');
  if (socket.clientIp.includes('::1') || socket.clientIp.includes('127.0.0.1')) socket.clientIp = process.env.LOCAL_IP;

  socket.on('data', (data) => onIncomingData(socket, data));
  socket.on('disconnect', _ => onSocketDisconnect(socket));
});

function onIncomingData(socket, data) {
  socket.tokoData = data;

  if (socket.tokoData.from === 'fivem') {
    if (!socket.uuid) {
      const newUUID = uuid();
      socket.uuid = newUUID;
      socket.from = 'fivem';
      clients[newUUID] = {
        ip: socket.clientIp,
        uuid: socket.uuid,
        fivem: {},
        ts3: {},
      };
      clients[socket.uuid].fivem.socket = socket;
      clients[socket.uuid].fivem.linkedAt = (new Date()).toISOString();
    }
    if (!clients[socket.uuid]) return;
    clients[socket.uuid].fivem.data = socket.tokoData;
    clients[socket.uuid].fivem.updatedAt = (new Date()).toISOString();
    if (clients[socket.uuid].ts3.socket) {
      clients[socket.uuid].ts3.socket.send(JSON.stringify(clients[socket.uuid].fivem.data));
    }

  } else if (socket.tokoData.from === 'ts3') {
    if (!socket.uuid) {
      if (!data.tsClientIp) return;
      const client = Object.values(clients).find(item => item.fivem.socket.clientIp === data.tsClientIp);
      if (!client) return;
      socket.uuid = client.uuid;
      socket.from = 'ts3';
      client.ts3.socket = socket;
      client.ts3.linkedAt = (new Date()).toISOString();
    }
    if (!socket.uuid || !clients[socket.uuid]) return;
    clients[socket.uuid].ts3.data = socket.tokoData;
    clients[socket.uuid].ts3.updatedAt = (new Date()).toISOString();
    if (clients[socket.uuid].fivem.socket) {
      clients[socket.uuid].fivem.socket.send(JSON.stringify(clients[socket.uuid].ts3.data));
    }
  }

  return socket;
}

function onSocketDisconnect(socket) {
  if (!socket.uuid) return;
  if (socket.from === 'fivem') {
    if (clients[socket.uuid].ts3.socket) {
      delete clients[socket.uuid].ts3.socket.uuid;
    }
    delete clients[socket.uuid];
  } else if (socket.from === 'ts3') {
    clients[socket.uuid].ts3 = {};
  }
}

function masterHeartbeat() {
  console.log('Heartbeat sent');
  axios.post('http://localhost:3005/heartbeat', {
    tsServer: '127.0.0.1',
    ip: '127.0.0.1',
    port: '3000',
  }).catch(e => console.error('Sending heartbeat failed with error: ', e));
}
