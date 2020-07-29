
# Introduction: TokoVOIP
TokoVOIP is a TeamSpeak plugin used along a FiveM script to add a custom proximity chat and radio system to [FiveM](https://fivem.net/)  

It includes radio effects thanks to the integration of the [RadioFX](https://www.myteamspeak.com/addons/f2e04859-d0db-489b-a781-19c2fab29def) plugin  

Downloads are available on the [releases](https://github.com/Itokoyamato/TokoVOIP_TS3/releases) page  

If you like TokoVOIP, give it a star ! It'd be much appreciated <3  

We have a [Discord](https://discord.gg/nbEprAw) server to discuss, or if you need help !  
[![Discord](https://img.shields.io/discord/728723568824680560?color=blue&label=Discord)](https://discord.gg/nbEprAw)

You can support my work:  
[![Patreon](https://img.shields.io/badge/Become%20a-patron-orange)](https://www.patreon.com/Itokoyamato)
[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=H2UXEZBF5KQBL&source=url)  

# Table of Contents
- [Introduction: TokoVOIP](#introduction-tokovoip)
- [Table of Contents](#table-of-contents)
- [Setting up TokoVOIP](#setting-up-tokovoip)
  - [(players) Setting up the ts3-plugin](#players-setting-up-the-ts3-plugin)
  - [(servers) Setting up the ws-server and fivem-script](#servers-setting-up-the-ws-server-and-fivem-script)
    - [Step 1: Setting up the ws-server](#step-1-setting-up-the-ws-server)
    - [Step 2: Setting up the fivem-script](#step-2-setting-up-the-fivem-script)
  - [(servers) Setting up ws-server as Standalone](#servers-setting-up-ws-server-as-standalone)
  - [(servers) Onesync Infinity](#servers-onesync-infinity)
- [How does it work ?](#how-does-it-work-)
- [Why do I need a ws-server ?](#why-do-i-need-a-ws-server-)
- [TROUBLESHOOTING (only 1.5.2)](#troubleshooting-only-152)
- [Terms and conditions](#terms-and-conditions)
- [Contributing](#contributing)
  - [Building the TS3 plugin](#building-the-ts3-plugin)
  - [Packaging the TS3 plugin](#packaging-the-ts3-plugin)
- [Dependencies and sources](#dependencies-and-sources)

# Setting up TokoVOIP
You can download the ws-server,  fivem-script & ts3-plugin on the [release](https://github.com/Itokoyamato/TokoVOIP_TS3/releases) page

## (players) Setting up the ts3-plugin
Every player must install Teamspeak 3 and the TokoVOIP plugin
* Install the plugin downloaded on the [release](https://github.com/Itokoyamato/TokoVOIP_TS3/releases) page
* Connect to the fivem server
* Connect to the TS3 server
* Join the right TS3 channel

The TS3 Plugin will connect only if you are already in-game  
The TS3 Plugin will try to connect if you join a TS3 channel containing 'tokovoip' in it's name  
You can use the buttons in `Plugins->TokoVoip` to manually connect/disconnect the TS3 Plugin  

## (servers) Setting up the ws-server and fivem-script

### Step 1: Setting up the ws-server
  * Download ws-server from the [release](https://github.com/Itokoyamato/TokoVOIP_TS3/releases) page
  * Extract it in your fivem resources folder
  * Open [ws_server/config.js](https://github.com/Itokoyamato/TokoVOIP_TS3/blob/master/ws_server/config.js)
  * Change "`TSServer`" to your Teamspeak server IPv4  
    Note: domain names are currently not supported. Please open an issue if that's something you would like to see available
  * start `ws_server` in your fivem server console
  * Copy the `IP:PORT` in the console after `Listening on` and save it for [**Step 2: Setting up the fivem-script**](#step-2-setting-up-the-fivem-script)

### Step 2: Setting up the fivem-script
  * Download tokovoip_script from the [release](https://github.com/Itokoyamato/TokoVOIP_TS3/releases) page
  * Extract it in your fivem resources folder
  * Open [tokovoip_script/c_config.lua](https://github.com/Itokoyamato/TokoVOIP_TS3/blob/master/fivem_script/tokovoip_script/c_config.lua)
    * Edit `wsServer` with the `IP:PORT` you copied from the ws-server console in [**Step 1: Setting up the ws-server**](#step-1-setting-up-the-ws-server)
    * Edit the `TSChannel` to match your Teamspeak configuration
    * Edit other settings to your preferences
  * Open and edit [s_config.lua](https://github.com/Itokoyamato/TokoVOIP_TS3/blob/master/fivem_script/tokovoip_script/s_config.lua) to your preference
  * Add `tokovoip` to your waiting channel name on your teamspeak server, it is [**case insensitive**](https://www.yourdictionary.com/case-insensitive)

  A documentation for the FiveM script is available [here](fivem_script)  

## (servers) Setting up ws-server as Standalone
It can be run as a standalone NodeJS application, on the same machine as your fivem server, or on a completely different machine
  * Download [Node.js](https://nodejs.org/en/)
  * Open [config.js](https://github.com/Itokoyamato/TokoVOIP_TS3/blob/master/ws_server/config.js)
    * Change "`TSServer`" to your Teamspeak server `IPv4`
    * If the ws-server is hosted on a separate machine:  
  * Open ws-server folder in cmd / terminal
  * Execute `npm i`
  * After its done run `node index.js`  
  A module such as `pm2` can be used to run the ws-server in the background

## (servers) Onesync Infinity
Onesync infinity is supported with TokoVOIP 1.5.0+  
By default, Teamspeak servers silence everyone when more than 100 users are in a channel  
Make sure your teamspeak server is configured properly:
* Right click your teamspeak server
* Press `Edit virtual server`
* Press `more`
* Open tab `Misc`
* Change the value of `Min clients in channel before silence`  

# How does it work ?
The system is based on websockets  
In-game data is sent through websockets to the TS3 plugin  
FiveM blocks websockets running on the local network, we must use a remote ws-server  

TS3 has no way to know on which fivem server you are currently on locally, a handshake system is required  
A master server is used to register handshakes  
That is it's only purpose, everything else is run on your own self-hosted ws-server and fivem-script  

* Phase 1 - Handshake:
  * fivem-script -> ws-server -> register for handshake (master server)
  * ts3-plugin -> look for handshake (master server) -> ws-server

Once the fivem websocket & ts3 websocket successfully handshaked, the master server is not used anymore

* Phase 2 - Communicate:
  * fivem-script -> ws-server -> ts3-plugin
  * fivem-script <- ws-server <- ts3-plugin

# Why do I need a ws-server ?
Read the following:
- [How does it work ?](#how-does-it-work-?)
- [TokoVOIP 1.5.0](https://github.com/Itokoyamato/TokoVOIP_TS3/pull/117)

# TROUBLESHOOTING (only 1.5.2)
**FiveM websocket keeps saying Not connected**:
- FiveM didn't successfuly connect to the websocket server.
- Make sure your websocket port is open
- Make sure firewall isn't blocking your port

**Ts3 websocket keeps saying Not connected**:
- Make sure your **waiting** channel has `tokovoip` in the name
- FiveM and Ts3 didn't successfuly handshake
- Make sure your websocket has proper teamspeak, fivem and ws ip
- Try manually using the connect button in **Plugins -> TokoVoip -> Connect**

**Could not find dependency yarn for resource ws_server**:
- Install yarn resource from [cfx-server-data repo](https://github.com/citizenfx/cfx-server-data/tree/master/resources/%5Bsystem%5D/%5Bbuilders%5D)
# Terms and conditions
A 'TokoVOIP' watermark must be visible on screen. You can move it, change it's design however you like.  
Just keep one somewhere. Thanks  
For the rest, refer to the [license](LICENSE.md)  

# Contributing
I am open to pull requests, feel free to build upon my work and improve it
Mind you, this is my only project done in C++ and have only done low level C projects, so I am certain a lot of improvements can be made

## Building the TS3 plugin

You will need the following installed:
- [Visual Studio IDE](https://visualstudio.microsoft.com/vs/)
- [Qt 5.12.7](https://download.qt.io/archive/qt/5.12.7/5.12.7/)
- [CMake](https://cmake.org/)

Clone the repo and don't forget to initialize the submodules:
```
git submodule update --init --recursive
```

Then move to the `ts3_plugin` folder, and generate the Visual Studio solution: (set the correct path to Qt)
```
mkdir build32
cd build32
cmake -G "Visual Studio 15 2017" -DCMAKE_PREFIX_PATH="<PATH_TO>/Qt/5.12.7/msvc2017" ..
cd ..
mkdir build64
cd build64
cmake -G "Visual Studio 15 2017 Win64"  -DCMAKE_PREFIX_PATH="<PATH_TO>/Qt/5.12.7/msvc2017_64" ..
```

The visual studio solutions are available in their platform specific folders.
You're ready to go !

## Packaging the TS3 plugin

Making a TS3 plugin package is very easy, you can use the template in `ts3_package` if you want.
You will need:
- `package.ini` file which gives some info about the plugin
- `.dll` files in a `plugin` folder

The `.dll` should have a suffix `_win32` or `_win64` matching their target platforms.

Then, archive the whole thing as a `.zip` file, and rename it to `.ts3_plugin`.

It's that simple.

Archive tree example:
```
.
+-- package.ini
+-- plugins
|   +-- tokovoip
|       +-- walkie_talkie16.png
|       +-- mic_click_off.wav
|       +-- mic_click_on.wav
|   +-- plugin_win32.dll
|   +-- plugin_win64.dll
```

# Dependencies and sources

- [RadioFX](https://github.com/thorwe/teamspeak-plugin-radiofx) by Thorwe
- [Simple-WebSocket-Server](https://gitlab.com/eidheim/Simple-WebSocket-Server) by eidheim
- [JSON for Modern C++](https://github.com/nlohmann/json.git) by nlohmann
- [cpp-httplib](https://github.com/yhirose/cpp-httplib) by yhirose
- [Task Force Arma 3 Radio](https://github.com/michail-nikolaev/task-force-arma-3-radio) by michail-nikolaev for the base concept and helping in figuring out a lot of TS3's stuff
