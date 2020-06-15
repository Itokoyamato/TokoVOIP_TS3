const express = require('express');
const app = express();
const http = require('http').createServer(app);
const io = require('socket.io')(http);
const axios = require('axios');

let masterHeartbeatInterval;
const clients = {};

app.use(express.json());

http.listen(3000, () => {
  console.log('Listening on *:3000');
  // masterHeartbeat();
  // masterHeartbeatInterval = setInterval(masterHeartbeat, 30000);
});

io.on('connection', socket => {
  socket.clientIp = socket.request.connection.remoteAddress;
  if (socket.clientIp.includes('::1') || socket.clientIp.includes('127.0.0.1')) socket.clientIp = process.env.LOCAL_IP;
});

function masterHeartbeat() {
  console.log('Heartbeat sent');
  axios.post('http://localhost:3005/heartbeat', {
    tsServer: '66.70.204.168',
    ip: '127.0.0.1',
    port: '3007',
  }).catch(e => console.error('Sending heartbeat failed with error: ', e));
}
