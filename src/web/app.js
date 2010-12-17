var ws;

$(function() {
    ws = new WebSocket('ws://' + document.location.host + '/dump');
    ws.onopen = function() {
        console.log('onopen');
    };
    ws.onclose = function() {
        $('#message').text('Lost connection.');
        console.log('onclose');
    };
    ws.onmessage = function(message) {
        eval(message.data);
    };
    ws.onerror = function(error) {
        console.log('onerror ' + error);
    };
});
