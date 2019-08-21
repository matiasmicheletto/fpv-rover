
var socket = null; // Objeto para instanciar WebSocketServer
var lastSampleTime = 0; // Variable auxiliar para medir frecuencia de muestreo de pos del mouse

var readUrlAV = function(form) { // Conectar a la camara IP
    TextVar = form.inputbox1.value;
    VideoVar = "http://" + TextVar + ":8080/video";
    AudioVar = "http://" + TextVar + ":8080/audio.opus";
    document.getElementById("video").setAttribute('data', VideoVar);
    document.getElementById("audio").setAttribute('data', AudioVar);
}

var connectWSS = function(form) { // Iniciar wss
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
var sendValues = function(x,y) {
    
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
};

var updateKnob = function(x,y){ // Actualizar slider rotativo
    // Convertir x,y en angulo respecto del centro de la pantalla
    var t = Math.atan2(-(x-1023),(y-1023))/Math.PI*50+50;    
    $('.knob').val(t).trigger('change');
};

var mouseMoveEvent = function(e){ // Para trackear movimiento del mouse por la pantalla
    if(Date.now() - lastSampleTime > 100){ // Muestrear posicion del mouse a 10 Hz
        lastSampleTime = Date.now(); // Actualizar instante de muestreo

        // Posicion del mouse en rango (0..2046,0..2046)
        var x = Math.floor(2046*e.clientX/window.innerWidth);
        var y = Math.floor(2046*e.clientY/window.innerHeight);

        // Mapeo de coordenadas: mouse (x,y) -> control (xp,yp) -> motores (a,b)
        //
        // Cambiar el rango de coordenadas de mouse a coordenadas de control
        // Rango componente horizontal (delta motores):  xp -> (-2046 .. 2046) ---> xp = 2*x - 2046
        // Rango componente vertical (promedio motores): yp -> (  0   .. 2046) ---> yp = 2046 - y
        //
        // Mapear coordenadas de control a potencia de motores 
        // Delta motores:     xp = a-b         Con la comp horizontal manejo diferencia de potencias
        // Promedio motores:  yp = (a+b)/2     Con la comp vertical manejo potencia de ambos a la vez
        //
        // Despeje
        // a = xp/2 + yp
        // b = yp - xp/2
        //
        // Finalmente reemplazar las ecuaciones de xp e yp con las posiciones del mouse
        var a = x - y + 1023;
        var b = 3069 - x - y;

        // Hacer clamp para que no se salga de rango
        a = a > 2046 ? 2046 : a < 0 ? 0 : a;
        b = b > 2046 ? 2046 : b < 0 ? 0 : b;

        updateKnob(x,y);

        document.getElementById('mousepos').innerHTML = 'Pwr: ' + a + ', ' + b;
        sendValues(a,b);
    }
};


document.addEventListener("mousemove",mouseMoveEvent);

$(function() {$(".knob").knob({ // Slider rotativo
    'readOnly': true, // No escucha inputs
    'displayInput':false // No muestra el valor que tiene (va de 0 a 100)
});});


