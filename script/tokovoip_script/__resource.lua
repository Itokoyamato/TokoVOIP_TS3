client_script "src/c_utils.lua"
client_script "c_config.lua"
client_script "src/c_main.lua"
client_script "src/c_TokoVoip.lua"

server_script "s_config.lua"
server_script "src/s_main.lua"
server_script "src/s_utils.lua"

ui_page "nui/index.html"

files({
    "nui/index.html",
    "nui/script.js",
})

export "setPlayerData"
export "getPlayerData"
export "refreshAllPlayerData"
export "addPlayerToRadio"
export "removePlayerFromRadio"
export "clientRequestUpdateChannels"
export "isPlayerInChannel"