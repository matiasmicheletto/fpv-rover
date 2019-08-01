// Valores de los sliders
var x = document.getElementById('range1').value;
var y = document.getElementById('range2').value;
var socket;

function readUrlAV(form) { // Conectar a la camara IP
    TextVar = form.inputbox1.value;
    VideoVar = "http://" + TextVar + ":8080/video";
    AudioVar = "http://" + TextVar + ":8080/audio.opus";
    document.getElementById("video").setAttribute('data', VideoVar);
    document.getElementById("audio").setAttribute('data', AudioVar);
}

function connectWSS(form) { // Iniciar wss
    // Iniciar websocket
    socket = new WebSocket('ws://'+form.inputbox2.value+'/', ['arduino']);

    // Callbacks de websocket
    socket.onopen = function () {
        console.log("Conexión establecida.");
    };

    socket.onclose = function () {
        console.log("Conexión cerrada.");
    };

    socket.onmessage = function (event) {
        console.log('Servidor (recibe): ', event.data);
        // TODO: Actualizar indicadores visuales
    };

    socket.onerror = function (error) {
        console.log('WebSocket Error. Revisar conexión.');
        //console.log(error);
    };
}

// Callbacks de los sliders
function updateVal1() {
    x = document.getElementById('range1').value;
    document.getElementById('valores').innerHTML = 'Valores: ' + x + ', ' + y;
    sendValues();
}

function updateVal2() {
    y = document.getElementById('range2').value;
    document.getElementById('valores').innerHTML = 'Valores: ' + x + ', ' + y;
    sendValues();
}

function formatStr(number, length) { // Convertir numero a string de longitud fija
    var str = '' + number;
    while (str.length < length) str = '0' + str;
    return str;
}

// Enviar datos al server (poner dentro de un interval?)
function sendValues() {
    var sendx = formatStr(x, 4);
    var sendy = formatStr(y, 4);
    console.log('Cliente (envía): ' + sendx + sendy);
    socket.send(sendx + sendy);
}
