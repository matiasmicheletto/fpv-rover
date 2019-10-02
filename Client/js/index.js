var socket = null; // Objeto para instanciar WebSocketServer
var lastMouseSampleTime = 0; // Variable auxiliar para medir frecuencia de muestreo de pos del mouse
var lastEyetrackerSampleTime = 0; // Variable auxiliar para medir frecuencia de muestreo de pos del eyetracker

Eyetracker.init(); // Inicializar eyetracker (debe estar ejecutando el server)

var readUrlAV = function(form) { // Conectar a la camara IP
    TextVar = form.inputbox1.value;
    VideoVar = "http://" + TextVar + ":8080/video";
    AudioVar = "http://" + TextVar + ":8080/audio.opus";
    document.getElementById("video").setAttribute('src', VideoVar);
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
        // Convertir string en valores numericos de 4 digitos
        var pwL = parseInt(event.data.slice(0,4));
        var pwR = parseInt(event.data.slice(4,8));
        updatePwrSliders(pwL,pwR);
    };

    socket.onerror = function (error) {
        console.log('WebSocket Error. Revisar conexión.');
        //console.log(error);
    };
}

// Enviar datos al server (poner dentro de un interval?)
var sendValues = function(l,r) {
    
    var formatStr = function(number, length) { // Convertir numero a string de longitud fija
        var str = '' + number;
        while (str.length < length) str = '0' + str;
        return str;
    };

    var sendl = formatStr(l, 4);
    var sendr = formatStr(r, 4);

    if(socket){
        socket.send(sendl + sendr);
        console.log('Cliente (envía): ' + sendl + sendr);
    }
};

Eyetracker.onData = function(x,y) {
    if(Date.now() - lastEyetrackerSampleTime > 100){ // Muestrear posicion a 10 Hz
        lastEyetrackerSampleTime = Date.now(); // Actualizar instante de muestreo

        // Posicion en rango (0..2046,0..2046)
        var xx = Math.floor(2046*x);
        var yy = Math.floor(2046*y);

        var l = xx - yy + 1023;
        var r = 3069 - xx - yy;

        // Hacer clamp para que no se salga de rango
        l = l > 2046 ? 2046 : l < 0 ? 0 : l;
        r = r > 2046 ? 2046 : r < 0 ? 0 : r;

        updateKnob(xx, yy); // Actualizar slider circular central
        updateSPSliders(l, r); // Actualizar barras de progreso de set points
        sendValues(l, r); // Enviar valores al server (rover)
    }
};

var mouseMoveEvent = function(e){ // Para trackear movimiento del mouse por la pantalla
    if(Date.now() - lastMouseSampleTime > 100){ // Muestrear posicion del mouse a 10 Hz
        lastMouseSampleTime = Date.now(); // Actualizar instante de muestreo

        // Posicion del mouse en rango (0..2046,0..2046)
        var x = Math.floor(2046*e.clientX/window.innerWidth);
        var y = Math.floor(2046*e.clientY/window.innerHeight);

        // Mapeo de coordenadas: mouse (x,y) -> control (xp,yp) -> motores (l,r)
        //
        // Cambiar el rango de coordenadas de mouse a coordenadas de control
        // Rango componente horizontal (delta motores):  xp -> (-2046 .. 2046) ---> xp = 2*x - 2046
        // Rango componente vertical (promedio motores): yp -> (  0   .. 2046) ---> yp = 2046 - y
        //
        // Mapear coordenadas de control a potencia de motores 
        // Delta motores:     xp = l-r         Con la comp horizontal manejo diferencia de potencias
        // Promedio motores:  yp = (l+r)/2     Con la comp vertical manejo potencia de ambos a la vez
        //
        // Despeje
        // l = xp/2 + yp
        // r = yp - xp/2
        //
        // Finalmente reemplazar las ecuaciones de xp e yp con las posiciones del mouse
        var l = x - y + 1023;
        var r = 3069 - x - y;

        // Hacer clamp para que no se salga de rango
        l = l > 2046 ? 2046 : l < 0 ? 0 : l;
        r = r > 2046 ? 2046 : r < 0 ? 0 : r;

        updateKnob(x, y); // Actualizar slider circular central
        updateSPSliders(l, r); // Actualizar barras de progreso de set points
        document.getElementById('mousepos').innerHTML = 'Pwr: ' + l + ', ' + r; // Mostrar valores numericos
        sendValues(l, r); // Enviar valores al server (rover)
    }
};

document.addEventListener("mousemove",mouseMoveEvent);

// Indicador de direccion de avance
$(function() {$(".knob").knob({ // Slider rotativo
        readOnly: true, // No escucha inputs
        displayInput:false, // No muestra el valor que tiene (va de 0 a 100)
        fgColor: '#111111',
        bgColor: 'rgba(200,200,200,1)',
        cursor : 30,
        height: 500,
        width: 500,
        thickness: 0.2
    });
});

var updateKnob = function(x,y){ // Actualizar slider rotativo
    // Convertir x,y en angulo respecto del centro de la pantalla
    var t = Math.atan2(-(x-1023),(y-1023))/Math.PI*50+50;    
    $('.knob').val(t).trigger('change');
};

// Indicadores de potencia de motores
var updatePwrSliders = function(pwL, pwR){
    document.getElementById("pwLB").value = (1023-pwL)/1023*100;
    document.getElementById("pwLF").value = (pwL-1023)/1023*100;

    document.getElementById("pwRB").value = (1023-pwR)/1023*100;
    document.getElementById("pwRF").value = (pwR-1023)/1023*100;
};

// Indicadores de set points
var updateSPSliders = function(spL, spR){
    document.getElementById("spLB").value = (1023-spL)/1023*100;
    document.getElementById("spLF").value = (spL-1023)/1023*100;

    document.getElementById("spRB").value = (1023-spR)/1023*100;
    document.getElementById("spRF").value = (spR-1023)/1023*100;
};