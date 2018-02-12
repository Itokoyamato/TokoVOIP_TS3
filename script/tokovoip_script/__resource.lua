client_script "c_main.lua"
client_script "c_TokoVoip.lua"
client_script "c_utils.lua"

server_script "s_main.lua"
server_script "s_utils.lua"

ui_page "nui/index.html"

files({
    "nui/index.html",
    "nui/script.js",
})