import express from "express";
import { config } from "./config";

import bodyParser from 'body-parser';
const app = express();

app.use(bodyParser.json());

interface VoipClient {
    clientIP: string;
    tsServer: string;
    wsServer: string;
    wsPort: number;
}

const voipClients: VoipClient[] = [];

app.post('/register', (req, res) => {
    const { ip: clientIP, server } = req.body;

    if (!clientIP || !server || !server.port || (server.tsServer && server.tsServer !== config.tsServer) || (server.ip && server.ip !== config.wsServer)) {
        res.status(400).json({ error: "Invalid parameters" });
        return;
    }

    let foundClient = false;
    for (const voipClient of voipClients) {
        if (voipClient.clientIP === clientIP) {
            voipClient.tsServer = server.tsServer || config.tsServer;
            voipClient.wsServer = server.ip || config.wsServer;
            voipClient.wsPort = server.port;
            foundClient = true;
            break;
        }
    }

    if (!foundClient) {
        voipClients.push({
            clientIP: clientIP,
            tsServer: server.tsServer || config.tsServer,
            wsServer: server.ip || config.wsServer,
            wsPort: server.port
        });
    }

    res.status(200).json({});
    return;
});

app.post('/heartbeat', (req, res) => {
    const { tsServer, WSServerIP, WSServerPort } = req.body;

    if (!WSServerPort || (tsServer && tsServer !== config.tsServer) || (WSServerIP && WSServerIP !== config.wsServer)) {
        res.status(400).json({ error: "Invalid parameters" });
        return;
    }

    res.status(200).json({});
    return;
});

app.get('/version', (req, res) => {
    res.status(200).json({
        "minVersion": "1.5.6",
        "currentVersion": "1.5.6",
        "defaultUpdateMessage": "Une mise à jour du plugin TokoVOIP est nécessaire ! Vous trouverez les infos pour mettre à jour sur le Discord de LP !",
        "minVersionWarningMessage": null
    });
    return;
});

app.get('/verify', (req, res) => {
    const { address } = req.query;

    if (address !== config.tsServer) {
        res.status(400).json({ error: "Invalid Server IP" });
        return;
    }

    let ipAddress = req.socket.remoteAddress;
    
    if (!ipAddress) {
        res.status(400).json({ error: "Invalid Client IP" });
        return;
    }

    ipAddress = ipAddress.replace('::ffff:', '');

    res.status(200).json({ ip: ipAddress });
    return;
});

app.get('/handshake', (req, res) => {
    const { ip: clientIP } = req.query;

    let foundClient = undefined;
    for (const voipClient of voipClients) {
        if (voipClient.clientIP === clientIP) {
            foundClient = voipClient;
            break;
        }
    }

    if (!foundClient) {
        res.status(403).json({ error: "Access denied" });
        return;
    }

    res.status(200).json({
        server: {
            ip: foundClient.wsServer,
            port: foundClient.wsPort
        }
    });
    return;
});

app.listen(config.port, () => {
    console.log(`Node server started running on port ${config.port}`);
});
