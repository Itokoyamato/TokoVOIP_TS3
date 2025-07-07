# TokoVoIP `ws-server`

The `ws-server` can also be run as an independent Node.js application. This is useful if you want to separate it from the FiveM runtime.

1. **Install Node.js**
   Download and install the latest version of [Node.js](https://nodejs.org/en/).

2. **Configure the server**
   Open the configuration file: [`ws_server/config.js`](https://github.com/Itokoyamato/TokoVoIP_TS3/blob/master/ws_server/config.js)

   - Set the `TSServer` value to the **IP address** of your TeamSpeak server.
   - If hosting on a separate machine, ensure the `ws-server` is accessible by the FiveM server.

3. **Install dependencies and start the server**

   Open a terminal or command prompt in the `ws-server` directory and run the following commands:

   ```bash
   pnpm install
   pnpm start
   ```

4. **(Optional) Run in the background**

   To keep the server running continuously, use a process manager such as:

   * [`pm2`](https://pm2.keymetrics.io/)
   * `screen` (Linux-only)
   * `systemd` (Linux-only)
