module.exports = {
  // IP address of your TeamSpeak 3 server
  TSServer: '152.53.181.170',

  // [OPTIONAL] IP address of the ws_server
  // Set automatically by autoconfig if left commented
  // WSServerIP: '127.0.0.1',

  // Port of the ws_server
  WSServerPort: 33250,

  // Enable or disable logging
  enableLogs: false,

  // Master server configuration (do not modify)
  masterServer: {
    registerUrl: 'https://master.chemnitzcityrp.de/register',
    heartbeatUrl: 'https://master.chemnitzcityrp.de/heartbeat'
  }
};
