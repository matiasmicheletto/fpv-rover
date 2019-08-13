
var socket = null; // Objeto para instanciar WebSocketServer
var lastSampleTime = 0; // Variable auxiliar para medir frecuencia de muestreo de pos del mouse

function readUrlAV(form) { // Conectar a la camara IP
    TextVar = form.inputbox1.value;
    VideoVar = "http://" + TextVar + ":8080/video";
    AudioVar = "http://" + TextVar + ":8080/audio.opus";
    document.getElementById("video").setAttribute('data', VideoVar);
    document.getElementById("audio").setAttribute('data', AudioVar);
}

function connectWSS(form) { // Iniciar wss
    // Iniciar websocket
    socket = new WebSocket('ws://'+form.inputbox2.value+':81/', ['arduino']);

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

// Enviar datos al server (poner dentro de un interval?)
function sendValues(x,y) {
    
    var formatStr = function(number, length) { // Convertir numero a string de longitud fija
        var str = '' + number;
        while (str.length < length) str = '0' + str;
        return str;
    };

    var sendx = formatStr(x, 4);
    var sendy = formatStr(y, 4);

    if(socket){
        socket.send(sendx + sendy);
        console.log('Cliente (envía): ' + sendx + sendy);
    }
}

var mouseMoveEvent = function(e){ // Para trackear movimiento del mouse por la pantalla
    if(Date.now() - lastSampleTime > 100){ // Muestrear posicion del mouse a 20 Hz
        lastSampleTime = Date.now(); // Actualizar instante de muestreo
        var x = Math.floor(2046*e.clientX/window.innerWidth);
        var y = Math.floor(2046*e.clientY/window.innerHeight);
        document.getElementById('mousepos').innerHTML = 'Posición: ' + x + ', ' + y;
        sendValues(x,y);
    }
};

document.addEventListener("mousemove",mouseMoveEvent);