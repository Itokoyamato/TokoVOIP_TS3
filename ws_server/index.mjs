import express from "express";
import httpModule from "http";
import { Server as SocketIO } from "socket.io";
import axios from "axios";
import lodash from "lodash";
import chalk from "chalk";
import configModule from "./config.js";
import { networkInterfaces } from "os";

const config = configModule;
const bootTime = new Date();
const app = express();
const http = httpModule.createServer(app);
const io = new SocketIO(http, { maxHttpBufferSize: 1e8 });

let hostIP;
let masterHeartbeatInterval;
const clients = {};
const handshakes = {};

const IPv4Regex = /^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$/;
const IPv6Regex = /^(([0-9a-fA-F]{1,4}):){1,7}([0-9a-fA-F]{1,4})$/;

app.use(express.json());

const sleep = ms => new Promise(resolve => setTimeout(resolve, ms));

function log(type, msg) {
  if (!config.enableLogs) return;
  console[type](msg);
}

function socketHeartbeat(socket) {
  if (!socket) return;
  const start = Date.now();
  socket.once("pong", () => {
    setTimeout(() => socketHeartbeat(socket), 1000);
    socket.latency = Date.now() - start;
    if (!socket.uuid || !clients[socket.uuid]) return;

    clients[socket.uuid].latency =
      lodash.get(clients[socket.uuid], "fivem.socket.latency", 0) +
      lodash.get(clients[socket.uuid], "ts3.socket.latency", 0);

    if (socket.from === "fivem") {
      socket.emit("onLatency", {
        total: clients[socket.uuid].latency,
        fivem: lodash.get(clients[socket.uuid], "fivem.socket.latency", 0),
        ts3: lodash.get(clients[socket.uuid], "ts3.socket.latency", 0),
      });
    }
  });
  socket.emit("ping");
}

async function masterHeartbeat() {
  try {
    await axios.post(config.masterServer.heartbeatUrl, {
      tsServer: config.TSServer,
      WSServerIP: config.WSServerIP,
      WSServerPort: config.WSServerPort,
    });
    console.log(chalk.green("Heartbeat sent"));
  } catch (e) {
    console.error(chalk.red("Sending heartbeat failed:"), e);
  }
}

function setTS3Data(socket, data) {
  const client = clients[socket.uuid];
  if (!client) return;
  lodash.set(client.ts3, `data.${data.key}`, data.value);
  if (client.fivem.socket) {
    client.fivem.socket.emit("setTS3Data", client.ts3.data);
  }
}

function onIncomingData(socket, data) {
  const client = clients[socket.uuid];
  if (!socket.uuid || !client || !client.ts3.socket || typeof data !== "object") return;
  socket.tokoData = data;
  client.fivem.data = data;
  client.fivem.updatedAt = new Date().toISOString();
  client.ts3.socket.emit("processTokovoip", client.fivem.data);
}

async function onSocketDisconnect(socket) {
  log("log", `${socket.from === "ts3" ? chalk.cyan(socket.from) : chalk.yellow(socket.from)} | Connection ${chalk.red("lost")} - ${socket.safeIp}`);
  if (socket.from === "fivem" && handshakes[socket.clientIp]) delete handshakes[socket.clientIp];

  if (socket.uuid && clients[socket.uuid]) {
    const client = clients[socket.uuid];
    delete clients[socket.uuid];
    const secondary = socket.from === "fivem" ? "ts3" : "fivem";
    if (client[secondary].socket) {
      client[secondary].socket.emit("disconnectMessage", `${socket.from}Disconnected`);
      if (secondary === "ts3") {
        await sleep(100);
        client[secondary].socket.disconnect(true);
      } else {
        await registerHandshake(client[secondary].socket);
      }
    }
  }
}

async function registerHandshake(socket) {
  if (socket.from === "fivem" && handshakes[socket.clientIp]) return;
  handshakes[socket.clientIp] = socket;
  let client;
  let tries = 0;

  while (!client) {
    ++tries;
    if (tries > 1) await sleep(5000);
    if (tries > 12) {
      handshakes[socket.clientIp] = null;
      socket.emit("disconnectMessage", "ts3HandshakeFailed");
      socket.disconnect(true);
      return;
    }

    client = Object.values(clients).find(c => !c.fivem.socket && c.ip === socket.clientIp);

    try {
      await axios.post(config.masterServer.registerUrl, {
        ip: socket.clientIp,
        server: { tsServer: config.TSServer, ip: config.WSServerIP, port: config.WSServerPort },
      });
    } catch (e) {
      console.error(e);
      throw e;
    }
  }

  socket.uuid = client.uuid;
  client.fivem.socket = socket;
  client.fivem.linkedAt = new Date().toISOString();
  if (lodash.get(client, "ts3.data.uuid")) {
    socket.emit("setTS3Data", client.ts3.data);
  }
}

function getHostIP() {
  const nets = networkInterfaces();

  let ipv4Address = null;
  let ipv6Address = null;

  for (const name of Object.keys(nets)) {
    for (const net of nets[name] || []) {
      if (!net.internal) {
        if (net.family === "IPv4" && !ipv4Address) {
          ipv4Address = net.address;
        } else if (net.family === "IPv6" && !ipv6Address) {
          ipv6Address = `[${net.address}]`;
        }
      }
    }
  }

  return ipv4Address || ipv6Address || "127.0.0.1";
}

(async function init() {
  config.TSServer = process.env.TSServer || config.TSServer;
  config.WSServerPort = parseInt(process.env.WSServerPort, 10) || parseInt(config.WSServerPort, 10);
  config.WSServerIP = process.env.WSServerIP || config.WSServerIP;
  hostIP = getHostIP();

  if (!config.WSServerIP) {
    config.WSServerIP = hostIP;
    console.log(chalk.yellow("AUTOCONFIG:"), "Setting", chalk.cyan("config.WSServerIP"), "to", chalk.cyan(config.WSServerIP));
    await sleep(0);
  }

  if (!config.TSServer || !config.WSServerIP || !config.WSServerPort) {
    console.error(chalk.red("Config error: Missing one of TSServer, WSServerIP or WSServerPort"));
    return;
  }

  http.listen(config.WSServerPort, async () => {
    let configError = false;

    if (!IPv4Regex.test(config.TSServer) && !IPv6Regex.test(config.TSServer)) {
      configError = true;
      console.error(chalk.red("Config error: TSServer must be a valid IPv4 or IPv6 address."));
    }

    if (config.WSServerPort < 30000) {
      console.error(chalk.yellow("Config warning: It's advised to use a WSServerPort above 30000."));
    }

    const wsURI = `http://${config.WSServerIP}:${config.WSServerPort}`;
    try {
      await axios.get(wsURI);
    } catch {
      configError = true;
      console.error(chalk.red("Config error: Could not access WS server at"), chalk.cyan(wsURI));
    }

    if (configError) return;

    await sleep(0);
    console.log("Websocket listening on", chalk.cyan(`${hostIP}:${config.WSServerPort}`));
    masterHeartbeat();
    masterHeartbeatInterval = setInterval(masterHeartbeat, 300000);
  });
})();

app.get("/", (_, res) => res.send({ started: bootTime.toISOString(), uptime: process.uptime() }));

app.get("/playerbyip", (req, res) => {
  const player = handshakes[req.query.ip];
  if (!player) return res.status(404).send();
  res.status(204).send();
});

app.get("/getmyip", (req, res) => {
  const raw = lodash.get(req, 'headers["x-forwarded-for"]') ||
              lodash.get(req, 'headers["x-real-ip"]') ||
              lodash.get(req, "connection.remoteAddress") || "";
  res.send(raw.replace("::ffff:", ""));
});

http.on("upgrade", (req, socket) => {
  if (!req._query?.from) return socket.destroy();
  if (req._query.from === "ts3" && !req._query.uuid) return socket.destroy();
});

io.on("connection", async socket => {
  socket.from = socket.request._query.from;
  socket.clientIp = (socket.handshake.headers["x-forwarded-for"] ||
                     socket.request.connection.remoteAddress || "")
                     .replace("::ffff:", "");
  socket.safeIp = Buffer.from(socket.clientIp).toString("base64");

  if (["::1", "127.0.0.1"].some(v => socket.clientIp.includes(v)) ||
      socket.clientIp.startsWith("192.168.")) {
    socket.clientIp = hostIP;
  }

  socket.fivemServerId = socket.request._query.serverId;
  socket.on("disconnect", () => onSocketDisconnect(socket));

  log("log", `${socket.from === "ts3" ? chalk.cyan(socket.from) : chalk.yellow(socket.from)} | Connection ${chalk.green("opened")} - ${socket.safeIp}`);

  if (socket.from === "ts3") {
    socket.uuid = socket.request._query.uuid;
    if (!handshakes[socket.clientIp]) {
      socket.emit("disconnectMessage", "handshakeNotFound");
      socket.disconnect(true);
      return;
    }

    clients[socket.uuid] = {
      ip: socket.clientIp,
      uuid: socket.uuid,
      fivem: {},
      ts3: { uuid: socket.uuid }
    };
    clients[socket.uuid].ts3.socket = socket;
    clients[socket.uuid].ts3.linkedAt = new Date().toISOString();
    delete handshakes[socket.clientIp];

    log("log", `${chalk.cyan(socket.from)} | Handshake ${chalk.green("successful")} - ${socket.safeIp}`);

    socket.on("setTS3Data", data => setTS3Data(socket, data));
    socket.on("onTalkStatusChanged", data => setTS3Data(socket, { key: "talking", value: data }));
    socketHeartbeat(socket);

  } else if (socket.from === "fivem") {
    socketHeartbeat(socket);
    socket.on("updateClientIP", data => {
      if (!data?.ip || !(IPv4Regex.test(data.ip) || IPv6Regex.test(data.ip)) || !clients[socket.uuid]) return;
      if (clients[socket.uuid].fivem.socket) {
        delete handshakes[clients[socket.uuid].fivem.socket.clientIp];
        clients[socket.uuid].fivem.socket.clientIp = data.ip;
      }
      if (clients[socket.uuid].ts3.socket) {
        clients[socket.uuid].ts3.socket.clientIp = data.ip;
      }
    });

    await registerHandshake(socket);
    socket.on("data", data => onIncomingData(socket, data));
  }
});