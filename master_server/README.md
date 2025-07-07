# TokoVOIP Master Server

## Installation

1. **Install Node.js**
   Download and install the latest version of [Node.js](https://nodejs.org/en/).

2. **Install dependencies and start the server**

   Open a terminal or command prompt in the `master_server` directory and run the following commands:

   ```bash
   pnpm install
   pnpm start
   ```

3. **(Optional) Run in the background**

   To keep the server running continuously, use a process manager such as:

   * [`pm2`](https://pm2.keymetrics.io/)
   * `screen` (Linux-only)
   * `systemd` (Linux-only)

---

## Technical Details

### API Endpoints

#### Health Check

* `GET /health`
  Returns basic server status and uptime information.

#### Client Management

* `POST /register`
  Registers a client to coordinate the initial handshake process.

* `GET /handshake`
  Retrieves handshake data associated with the client's IP address.

#### Server Management

* `POST /heartbeat`
  Receives heartbeat signals from WebSocket servers to maintain active registration.

* `GET /verify`
  Verifies whether a TeamSpeak server is registered and currently active.

#### Version Control

* `GET /version`
  Returns the current plugin version along with any update notifications or messages.

---

## Automatic Cleanup

The server performs automatic cleanup tasks every 5 minutes. These include:

* Removing **handshake entries** that have not been updated in over **2 minutes**
* Removing **WebSocket servers** that have not sent a heartbeat in over **10 minutes**
* Deleting **TeamSpeak server entries** that are no longer in use (i.e., empty or stale)