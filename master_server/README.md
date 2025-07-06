# TokoVOIP Master Server

## API Endpoints

### Health Check
- `GET /health` - Returns server status and uptime information

### Client Management
- `POST /register` - Register a client for handshake coordination
- `GET /handshake` - Retrieve handshake information for the requesting IP

### Server Management
- `POST /heartbeat` - Send heartbeat from WebSocket servers to maintain registration
- `GET /verify` - Verify if a TeamSpeak server is registered and active

### Version Control
- `GET /version` - Get current plugin version information and update messages

## Installation

1. Install dependencies:
```bash
npm install
```

2. Start the server:
```bash
npm start
```

## Automatic Cleanup

The server runs automatic cleanup jobs every 5 minutes to remove:
- Handshakes that haven't been updated in 2 minutes
- WebSocket servers that haven't sent heartbeats in 10 minutes
- Empty TeamSpeak server entries
