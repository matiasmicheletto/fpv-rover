const maxSetpointValue = 255;
const mspv = maxSetpointValue*2;
let socket = null;
let lastMouseSampleTime = 0;
const wssPort = 82; // Match WSS_PORT in firmware/src/config.h
const camPort = 80; // Match CAM_PORT in firmware/src/config.h


const connectWSS = ip => { // Try to connect to the websocket server running on board
    const socket = new WebSocket(`ws://${ip}:${wssPort}`, ['arduino']); 
    
    socket.onopen = () => {
        console.log("WSS connection stablished");
    };

    socket.onclose = () => {
        console.log("WSS connection closed");
    };

    socket.onmessage = event => {
        console.log('WSS: Received from server: ', event.data);
        const pwL = parseInt(event.data.slice(0,4));
        const pwR = parseInt(event.data.slice(4,8));
        updatePwrSliders(pwL,pwR);
    };

    socket.onerror = error => {
        console.log('WSS Error. Check connection.');        
        console.error(error);
    };
};

const readUrlAV = ip => { // Read the video stream from the camera
    const videoSRC = `http://${ip}:${camPort}/stream`;    
    console.log("video src:", videoSRC);
    document.getElementById("video").setAttribute('src', videoSRC);
};

const connectToRover = form => { // Connect to the rover using the IP provided by the user
    const url = form.inputbox.value;
    localStorage.setItem('roverIP', url);
    console.log("Connecting to: " + url);
    connectWSS(url);
    readUrlAV(url);
};

const sendSetpoints = (left, right) => { // Send the setpoints to the rover
    const padLength = maxSetpointValue.toString().length;
    const setpointLeft = padStart(left, padLength);
    const setpointRight = padStart(right, padLength);
    if(socket){
        const msg = `${setpointLeft}${setpointRight}`;
        socket.send(msg);
        console.log('Sent to server: ' + msg);
    }
};

const mouseMoveEvent = event => { // Update the setpoints and sliders based on the mouse position
    if(Date.now() - lastMouseSampleTime > 100){
        lastMouseSampleTime = Date.now();

        const x = Math.floor(mspv * event.clientX / window.innerWidth);
        const y = Math.floor(mspv * event.clientY / window.innerHeight);

        const setpointLeft = clamp(x - y + maxSetpointValue, 0, mspv);
        const setpointRight = clamp(3 * maxSetpointValue - x - y, 0, mspv);

        const splPercentage = round(toPercentage(setpointLeft, 0, mspv)*2 - 100);
        const sprPercentage = round(toPercentage(setpointRight, 0, mspv)*2 - 100);

        document.getElementById('mousepos').innerHTML = `Setpoints: ${splPercentage}% , ${sprPercentage}%`;
        
        updateKnob(x, y);
        updateSPSliders(setpointLeft, setpointRight);
        sendSetpoints(setpointLeft, setpointRight);
        
        
    }
};

// Update the knob based on the mouse position
const updateKnob = (x,y) => { 
    const t = Math.atan2(-(x - maxSetpointValue),(y - maxSetpointValue))/Math.PI * 50 + 50;    
    $('.knob').val(t).trigger('change');
};

// Update the sliders based on the setpoints
const updatePwrSliders = (pwL, pwR) => { // Feedback from rover
    document.getElementById("pwLB").value = (maxSetpointValue-pwL)/maxSetpointValue*100;
    document.getElementById("pwLF").value = (pwL-maxSetpointValue)/maxSetpointValue*100;

    document.getElementById("pwRB").value = (maxSetpointValue-pwR)/maxSetpointValue*100;
    document.getElementById("pwRF").value = (pwR-maxSetpointValue)/maxSetpointValue*100;
};

const updateSPSliders = (spL, spR) => { // Setpoints sent to rover
    document.getElementById("spLB").value = (maxSetpointValue-spL)/maxSetpointValue*100;
    document.getElementById("spLF").value = (spL-maxSetpointValue)/maxSetpointValue*100;

    document.getElementById("spRB").value = (maxSetpointValue-spR)/maxSetpointValue*100;
    document.getElementById("spRF").value = (spR-maxSetpointValue)/maxSetpointValue*100;
};


// Load IP from local storage
const storedIP = localStorage.getItem('roverIP');
if(storedIP){
    document.getElementById("ipInput").setAttribute('value', storedIP);
}

// Track mouse movements
document.addEventListener("mousemove", mouseMoveEvent);

// Knob settings
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