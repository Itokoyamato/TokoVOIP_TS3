module.exports = {
  //-- [REQUIRED] IPv4 Address of your teamspeak 3 server
  TSServer: "127.0.0.1",

  //-- [REQUIRED] Port of the ws_server
  //-- Make sure you open the port you specify below
  //-- Please use a port above 30k as some networks block those below it
  WSServerPort: 33250,

  //-- [OPTIONAL] IPv4 Address of the ws_server
  //-- Set by autoconfig
  // WSServerIP: "127.0.0.1",

  //-- [OPTIONAL] Enable connection/disconnection logs
  enableLogs: false,
};
