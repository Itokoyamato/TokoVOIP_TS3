fx_version 'bodacious'
games { 'gta5' }

client_scripts {
	'src/c_utils.lua'
  'c_config.lua'
  'src/c_main.lua'
  'src/c_TokoVoip.lua'
  'src/nuiProxy.js'
}

server_scripts {
 's_config.lua'
 'src/s_main.lua'
 'src/s_utils.lua'

ui_page 'nui/index.html'

files {
    'nui/index.html',
    'nui/script.js',
}
