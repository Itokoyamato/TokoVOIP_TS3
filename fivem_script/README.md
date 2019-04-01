# Setting up
The script must be placed in a folder named `tokovoip_script` (NUI restriction)

You can edit the voip configuration in `c_config.lua`

You can edit the radio channels in `s_config.lua`  
If the channel name contains 'Call' it will be displayed as a call on the ingame overlay

## API Reference
---------------------------------------------------------------

***Currently, the radio channels interaction must be done via the client***

## Client exports

- ### addPlayerToRadio(channel)

  Adds the player to a radio channel

  <table>
  <tr>
    <th>Params</th>
    <th>type</th>
    <th>Details</th>
  </tr>
  <tr>
    <td>channel</td>
    <td>number</td>
    <td>Id of the radio channel</td>
  </tr>
  </table>

- ### removePlayerFromRadio(channel)

  Removes the player from a radio channel

  <table>
  <tr>
    <th>Params</th>
    <th>type</th>
    <th>Details</th>
  </tr>
  <tr>
    <td>channel</td>
    <td>number</td>
    <td>Id of the radio channel</td>
  </tr>
  </table>

- ### isPlayerInChannel(channel)

  Returns true if the player is in the specified radio channel

  <table>
  <tr>
    <th>Params</th>
    <th>type</th>
    <th>Details</th>
  </tr>
  <tr>
    <td>channel</td>
    <td>number</td>
    <td>Id of the radio channel</td>
  </tr>
  </table>

- ### clientRequestUpdateChannels()

  Requests to update the local radio channels
  </table>

- ### setPlayerData(playerName, key, data, shared)

  Sets a data key on the specified player
  Note: if the data is not shared, it will only be available to the local player

  <table>
  <tr>
    <th>Params</th>
    <th>type</th>
    <th>Details</th>
  </tr>
  <tr>
    <td>playerName</td>
    <td>string</td>
    <td>Name of the player to apply the data to</td>
  </tr>
  <tr>
    <td>key</td>
    <td>string</td>
    <td>Name of the data key</td>
  </tr>
  <tr>
    <td>data</td>
    <td>any</td>
    <td>Data to save in the key</td>
  </tr>
  <tr>
    <td>shared</td>
    <td>boolean</td>
    <td>If set to true, will sync through network to all the players</td>
  </tr>
  </table>

- ### getPlayerData(playerName, key)

  Returns the data of matching player and key

  <table>
  <tr>
    <th>Params</th>
    <th>type</th>
    <th>Details</th>
  </tr>
  <tr>
    <td>playerName</td>
    <td>string</td>
    <td>Name of the player to retrieve the data from</td>
  </tr>
  <tr>
    <td>key</td>
    <td>string</td>
    <td>Name of the data key</td>
  </tr>
  </table>

- ### refreshAllPlayerData(toEveryone)

  Effectively syncs the shared data

  <table>
  <tr>
    <th>Params</th>
    <th>type</th>
    <th>Details</th>
  </tr>
  <tr>
    <td>toEveryone</td>
    <td>boolean</td>
    <td>If set to true, the data will be synced to all players, otherwise only to the client who requested it</td>
  </tr>
  </table>
