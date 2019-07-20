function doSendNuiMessage(event, payload) {
    SendNuiMessage(JSON.stringify({
        type: event,
        payload
    }));
}
exports('doSendNuiMessage', doSendNuiMessage)