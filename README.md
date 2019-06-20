# TokoVoip
TokoVoip is a TeamSpeak plugin used along a FiveM script to add a custom proximity chat and radio system to [FiveM](https://fivem.net/)

It includes radio effects thanks to the integration of the [RadioFX](https://www.myteamspeak.com/addons/f2e04859-d0db-489b-a781-19c2fab29def) plugin

The TS3 plugin does not require any extra setting up, you just have to install it

The tokovoip_script must be running on the server with the right configuration

A documentation for the FiveM script is available [here](fivem_script)

Downloads are available [here](https://github.com/Itokoyamato/TokoVOIP_TS3/releases)

This system was originally developed for **Revolution RP** by https://rmog.us, check it out for some FiveM RP ;)

If you like TokoVOIP, give it a star ! It'd be much appreciated <3  

If you're feeling generous  
[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=H2UXEZBF5KQBL&source=url)

# Terms and conditions
I require only the following to anyone that plan on using this system: to leave the copyrights included as is, and not alter the displays of the TokoVOIP name in any other way than changing their position on screen.

For the rest, refer to the [license](LICENSE.md)

# How does it work ?
TS3 has an extensive and accessible API that allows the manipulation of the local clients. The issue was to access this API via FiveM, and allow a script to pass data to TS3

FiveM scripts run in a fairly contained sandbox, to the exception of one part: `NUI`

With `NUI`, we have access to a full browser, which opens a lot of possibilities  
One of them: websockets

The TS3 plugin runs a local websocket server, to which the FiveM script connects to via javascript.

The lua script handles all the processing of positions, volume, muting, enabling/disabling radio effects
and then sends it to NUI as JSON,
following that, NUI simply sends the data to the TS3 plugin via websockets over the local network.

To be completely honest, I have never refactored the code since my very first prototypes  
Ideally, I would like to build the system in a way to allow direct access to TS3's API in the lua script itself (with limitations of course)  
But that never happened  
It works, and that's enough for now

# Contributing
I am open to pull requests, feel free to build upon my work and improve it  
Mind you, this is my only project done in C++ and have only done low level C projects, so I am certain a lot of improvements can be made

# Building the TS3 plugin

You will need the following installed:
- [Visual Studio IDE](https://visualstudio.microsoft.com/vs/)
- [Qt 5.6.0](https://download.qt.io/archive/qt/5.6/5.6.0/)
- [CMake](https://cmake.org/)

Clone the repo and don't forget to initialize the submodules:
```
git submodule update --init --recursive`
```

Then move to the `ts3_plugin` folder, and generate the Visual Studio solution: (set the correct path to Qt)
```
mkdir build32
cd build32
cmake -G "Visual Studio 15 2017" -DCMAKE_PREFIX_PATH="<PATH_TO>/Qt/5.12.2/msvc2017" ..
cd ..
mkdir build64
cd build64
cmake -G "Visual Studio 15 2017 Win64"  -DCMAKE_PREFIX_PATH="<PATH_TO>/Qt/5.12.2/msvc2017_64" ..
```

The visual studio solutions are available in their platform specific folders.
You're ready to go !

# Packaging the TS3 plugin

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
+-- plugin
|   +-- assets
|       +-- img1.png
|       +-- img2.png
|   +-- plugin_win32.dll
|   +-- plugin_win64.dll
```

# Dependencies and sources

- [RadioFX](https://github.com/thorwe/teamspeak-plugin-radiofx) by Thorwe
- [Simple-Web-Server](https://gitlab.com/eidheim/Simple-Web-Server) by eidheim
- [JSON for Modern C++](https://github.com/nlohmann/json.git) by nlohmann
- [CURL](https://github.com/curl/curl)
- [Task Force Arma 3 Radio](https://github.com/michail-nikolaev/task-force-arma-3-radio) by michail-nikolaev for the base concept and helping in figuring out a lot of TS3's stuff
