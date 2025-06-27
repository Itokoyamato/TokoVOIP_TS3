const bootTime = new Date();

const port = process.env.PORT || 3001;

const { CronJob } = require('cron');
const express = require('express');
const chalk = require('chalk');
require('console-stamp')(console, { pattern: 'dd/mm/yyyy HH:MM:ss.l' });
const pluginVersion = require('./version.json');

let cachedHandshakes = {};
let cachedServers = {};

//---
// HTTP Server
//---

const app = express();
app.use(express.json());

app.get('/health', (_, res) => {
  res.send({
    started: bootTime.toISOString(),
    uptime: process.uptime(),
  });
});

app.listen(port, _ => {
  console.log(`Listening on port ${port}!`);
});

app.use((err, req, res, next) => {
  console.error(err);
});


//---
// Routes
//---

// Register for handshake
app.post('/register', async (req, res) => {
  if (!req.body.ip || !req.body.server?.ip || !req.body.server?.port || !req.body.server?.tsServer) return res.status(400).send();
  const existing = cachedHandshakes[req.body.ip];
  const now = new Date();
  const data = {
    ip: req.body.ip,
    server: {
      ip: req.body.server.ip,
      port: req.body.server.port,
      tsServer: req.body.server.tsServer,
    },
    registeredAt: (existing) ? existing.registeredAt : now.toISOString(),
    updatedAt: now.toISOString(),
  };
  cachedHandshakes[req.body.ip] = data;
  res.send();
});

// Get handshake
app.get('/handshake', async (req, res) => {
  const ip = (req?.headers?.['x-forwarded-for'] || req?.headers?.['x-real-ip'] || req?.connection?.remoteAddress).replace('::ffff:', '');
  let handshake = cachedHandshakes[ip];
  if (!handshake) return res.status(404).send();
  return res.send(handshake);
});

// Receive heartbeat
app.post('/heartbeat', async (req, res) => {
  if (!req.body.tsServer
    || !req.body.WSServerIP
    || !req.body.WSServerPort) return res.status(400).send();

  let tsServer = cachedServers[req.body.tsServer];
  if (!tsServer) {
    tsServer = {
      ts: req.body.tsServer,
      wsServers: {},
    };
  }

  const date = new Date();

  const idWS = `${req.body.WSServerIP.replace(/\./gm, '-')}:${req.body.WSServerPort}`;
  let server = tsServer.wsServers[idWS];
  if (!server) server = tsServer.wsServers[idWS] = {
    registeredAt: date.toISOString(),
  };
  server.ip = req.body.WSServerIP;
  server.port = req.body.WSServerPort;
  server.updatedAt = date.toISOString();

  cachedServers[req.body.tsServer] = tsServer;
  res.send();
});

// Verify TS Server
app.get('/verify', async (req, res) => {
  if (!req.query.address) return res.status(400).send();
  const server = cachedServers[req.query.address];
  cachedServers[req.query.address] = server || { notRegistered: true };
  if (!server || server.notRegistered) return res.status(404).send();
  const ip = (req?.headers?.['x-forwarded-for'] || req?.headers?.['x-real-ip'] || req?.connection?.remoteAddress).replace('::ffff:', '');
  return res.send(ip);
});

// TS3 Plugin version check
app.get('/version', (_, res) => res.send(pluginVersion));


//---
// Utils
//---

const cleaningJob = new CronJob('0 */5 * * * *', async _ => {
  console.log(chalk`{yellow [CLEAN]} {green started}`);
  const now = new Date();
  const date = new Date();
  date.setMinutes(date.getMinutes() - 2);
  Object.entries(cachedHandshakes).forEach(([ip, handshake]) => {
    if (now.getTime() - (new Date(handshake?.updatedAt)).getTime() > 120000) {
      delete cachedHandshakes[ip];
    }
  });
  console.log(chalk`{yellow [CLEAN]} handshakes timed out`);

  Object.values(cachedServers).forEach((tsServer) => {
    Object.entries(tsServer.wsServers).forEach(async ([ip, server]) => {
      const last = new Date(server.updatedAt);
      if (now.getTime() - last.getTime() > 600000) {
        delete cachedServers[tsServer.ts]?.wsServers[ip];
        if (cachedServers[tsServer.ts] && Object.keys(cachedServers[tsServer.ts]?.wsServers).length === 0) delete cachedServers[tsServer.ts];
      }
    });
  });
  console.log(chalk`{yellow [CLEAN]} wsServers timed out`);
  console.log(chalk`{yellow [CLEAN]} servers timed out`);
  console.log(chalk`{yellow [CLEAN]} {green finished}`);
});
cleaningJob.start();
