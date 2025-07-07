import express from 'express';
import chalk from 'chalk';
import { readFile } from 'fs/promises';
import { fileURLToPath } from 'url';
import { dirname, join } from 'path';
import cron from 'node-cron';

const __dirname = dirname(fileURLToPath(import.meta.url));
const pluginVersion = JSON.parse(await readFile(join(__dirname, 'version.json'), 'utf-8'));

const stamp = (msg) => {
  const date = new Date().toLocaleString('en-GB', { hour12: false });
  console.log(`[${date}] ${msg}`);
};

const bootTime = new Date();
const port = process.env.PORT || 3001;

let cachedHandshakes = {};
let cachedServers = {};

const app = express();
app.use(express.json());

app.get('/health', (_, res) => {
  res.send({
    started: bootTime.toISOString(),
    uptime: process.uptime(),
  });
});

app.post('/register', (req, res) => {
  const { ip, server } = req.body;

  if (!ip || !server?.ip || !server?.port || !server?.tsServer) {
    return res.status(400).send();
  }

  const existing = cachedHandshakes[ip];
  const now = new Date();

  cachedHandshakes[ip] = {
    ip,
    server: {
      ip: server.ip,
      port: server.port,
      tsServer: server.tsServer,
    },
    registeredAt: existing?.registeredAt || now.toISOString(),
    updatedAt: now.toISOString(),
  };

  res.send();
});

app.get('/handshake', (req, res) => {
  const ip = (req.headers['x-forwarded-for'] || req.headers['x-real-ip'] || req.connection?.remoteAddress || '')
    .replace('::ffff:', '');
  const handshake = cachedHandshakes[ip];

  if (!handshake) return res.status(404).send();
  res.send(handshake);
});

app.post('/heartbeat', (req, res) => {
  const { tsServer, WSServerIP, WSServerPort } = req.body;

  if (!tsServer || !WSServerIP || !WSServerPort) {
    return res.status(400).send();
  }

  const now = new Date();
  const idWS = `${WSServerIP.replace(/\./g, '-')}:${WSServerPort}`;

  const tsData = cachedServers[tsServer] ?? { ts: tsServer, wsServers: {} };
  const wsData = tsData.wsServers[idWS] ?? { registeredAt: now.toISOString() };

  tsData.wsServers[idWS] = {
    ...wsData,
    ip: WSServerIP,
    port: WSServerPort,
    updatedAt: now.toISOString(),
  };

  cachedServers[tsServer] = tsData;

  res.send();
});

app.get('/verify', (req, res) => {
  const address = req.query.address;
  if (!address) return res.status(400).send();

  const server = cachedServers[address];
  if (!server) {
    cachedServers[address] = { notRegistered: true };
    return res.status(404).send();
  }

  const ip = (req.headers['x-forwarded-for'] || req.headers['x-real-ip'] || req.connection?.remoteAddress || '')
    .replace('::ffff:', '');
  res.send(ip);
});

app.get('/version', (_, res) => {
  res.send(pluginVersion);
});

app.use((err, req, res, next) => {
  console.error(err);
  res.status(500).send('Internal Server Error');
});

cron.schedule('*/5 * * * *', () => {
  stamp(chalk.yellow('[CLEAN]') + ' ' + chalk.green('started'));
  const now = Date.now();

  for (const [ip, handshake] of Object.entries(cachedHandshakes)) {
    if (now - new Date(handshake.updatedAt).getTime() > 2 * 60 * 1000) {
      delete cachedHandshakes[ip];
    }
  }
  stamp(chalk.yellow('[CLEAN]') + ' handshakes timed out');

  for (const [ts, tsServer] of Object.entries(cachedServers)) {
    for (const [id, server] of Object.entries(tsServer.wsServers)) {
      if (now - new Date(server.updatedAt).getTime() > 10 * 60 * 1000) {
        delete tsServer.wsServers[id];
      }
    }

    if (Object.keys(tsServer.wsServers).length === 0) {
      delete cachedServers[ts];
    }
  }

  stamp(chalk.yellow('[CLEAN]') + ' wsServers and servers timed out');
  stamp(chalk.yellow('[CLEAN]') + ' ' + chalk.green('finished'));
});

app.listen(port, () => {
  stamp(chalk.green('[INFO]') + ` Listening on port ${port}!`);
});