let socket = null;
let lastMouseSampleTime = 0;

const readUrlAV = function(form) {
    const TextVar = form.inputbox1.value;
    const VideoVar = "http://" + TextVar;// + ":81/stream";    
    console.log(VideoVar);
    document.getElementById("video").setAttribute('src', VideoVar);
}

const connectWSS = function(form) {
    socket = new WebSocket('ws://'+form.inputbox2.value, ['arduino']);
    
    socket.onopen = function () {
        console.log("Connection stablished");
    };

    socket.onclose = function () {
        console.log("Connection closed");
    };

    socket.onmessage = function (event) {
        console.log('Received: ', event.data);        
        const pwL = parseInt(event.data.slice(0,4));
        const pwR = parseInt(event.data.slice(4,8));
        updatePwrSliders(pwL,pwR);
    };

    socket.onerror = function (error) {
        console.log('WebSocket Error. Check connection.');        
        console.error(error);
    };
}

const sendValues = function(l,r) {
    const padStart = function(number, length) {
        let str = '' + number;
        while (str.length < length) str = '0' + str;
        return str;
    };
    const sendl = padStart(l, 4);
    const sendr = padStart(r, 4);
    if(socket){
        socket.send(sendl + sendr);
        console.log('Client (sent): ' + sendl + sendr);
    }
};

const mouseMoveEvent = function(e){
    if(Date.now() - lastMouseSampleTime > 100){
        lastMouseSampleTime = Date.now();

        const x = Math.floor(2046*e.clientX/window.innerWidth);
        const y = Math.floor(2046*e.clientY/window.innerHeight);
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
        let l = x - y + 1023;
        let r = 3069 - x - y;
        l = l > 2046 ? 2046 : l < 0 ? 0 : l;
        r = r > 2046 ? 2046 : r < 0 ? 0 : r;

        updateKnob(x, y);
        updateSPSliders(l, r);
        document.getElementById('mousepos').innerHTML = 'Pwr: ' + l + ', ' + r;
        sendValues(l, r);
    }
};

document.addEventListener("mousemove",mouseMoveEvent);

$(function() {$(".knob").knob({ 
        readOnly: true,
        displayInput:false,
        fgColor: '#111111',
        bgColor: 'rgba(200,200,200,1)',
        cursor : 30,
        height: 500,
        width: 500,
        thickness: 0.2
    });
});

const updateKnob = function(x,y){ 
    const t = Math.atan2(-(x-1023),(y-1023))/Math.PI*50+50;    
    $('.knob').val(t).trigger('change');
};

const updatePwrSliders = function(pwL, pwR){
    document.getElementById("pwLB").value = (1023-pwL)/1023*100;
    document.getElementById("pwLF").value = (pwL-1023)/1023*100;

    document.getElementById("pwRB").value = (1023-pwR)/1023*100;
    document.getElementById("pwRF").value = (pwR-1023)/1023*100;
};

const updateSPSliders = function(spL, spR){
    document.getElementById("spLB").value = (1023-spL)/1023*100;
    document.getElementById("spLF").value = (spL-1023)/1023*100;

    document.getElementById("spRB").value = (1023-spR)/1023*100;
    document.getElementById("spRF").value = (spR-1023)/1023*100;
};