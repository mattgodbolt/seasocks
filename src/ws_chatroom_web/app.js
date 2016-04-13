var ws;

function add(text) {
    var chat = $('#chat-area');
    chat.text(chat.text() + "\n" + text);
}

$(function begin() {
    $('#form').submit(function onSubmit(e) {
        var ci = $('#chat-input');
        ws.send(ci.val());
        ci.val("");
        e.preventDefault();
    });
    ws = new WebSocket('ws://' + document.location.host + '/chat');
    ws.onopen = function () {
        console.log('onopen');
    };
    ws.onclose = function () {
        add('Lost connection.');
        console.log('onclose');
    };
    ws.onmessage = function (message) {
        add(message.data);
    };
    ws.onerror = function (error) {
        add("ERROR: " + error);
    };
});
