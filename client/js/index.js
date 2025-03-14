const maxSetpoint = 2 * 255; // Max setpoint value = 2^8 - 1 (8 bits for analogWrite)
const halfMaxSetpoint = maxSetpoint / 2; // Half of the max setpoint value, used for update sliders
const percHalfMaxSP = halfMaxSetpoint / 100;
let lastMouseSampleTime = 0; // Limit the number the mouse move event is executed

let socket = null; // Websocket connection to the rover
const wssPort = 82; // Match WSS_PORT in firmware/src/config.h
const camPort = 81; // Check from ESP32 CAM sr232 output

const ssEl = document.getElementById("socketStatus");
ssEl.innerHTML = "Disconnected";
ssEl.style.color = "red";

const motorSPs = document.getElementById("motorSPs");
motorSPs.style.display = "none";


const connectWSS = ip => { // Try to connect to the websocket server running on board
    socket = new WebSocket(`ws://${ip}:${wssPort}`, ['arduino']); 
    
    socket.onopen = () => {
        console.log("WSS connection stablished");
        ssEl.innerHTML = "Connected";
        ssEl.style.color = "green";
        motorSPs.style.display = "block";
    };

    socket.onclose = () => {
        console.log("WSS connection closed");
        ssEl.innerHTML = "Disconnected";
        ssEl.style.color = "red";
        motorSPs.style.display = "none";
    };

    socket.onmessage = event => {
        console.log('WSS: Received from server: ', event.data);
        const fbL = parseInt(event.data.slice(0,3));
        const fbR = parseInt(event.data.slice(3,6));
        updateFBSliders(fbL,fbR);
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

const sendSetpoints = (left, right) => { // Send the setpoints to the rover. Values go from 0 to mspsv
    const padLength = maxSetpoint.toString().length;
    const spL = padStart(left, padLength);
    const spR = padStart(right, padLength);
    console.log(`Sending setpoints: ${spL} ${spR}`);
    if(socket){ 
        if(socket.readyState === 1 && 
            spL >=0 && spR >=0 &&
            spL <= maxSetpoint && spR <= maxSetpoint
        ){
            const msg = `${spL}${spR}`;
            socket.send(msg);
            console.log(`Sent: left=${spL} right=${spR} msg=${msg}`)
        }else{
            console.log("Error sending setpoints");
            console.log(`Left: ${spL} Right: ${spR}`);
            console.log(`ReadyState: ${socket.readyState}`);
        }
    }else{
        console.log("Error sending setpoints. Socket not connected");
    }
};

const mouseMoveEvent = event => { // Update the setpoints and sliders based on the mouse position
    if(Date.now() - lastMouseSampleTime > 100){
        lastMouseSampleTime = Date.now();

        // x and y are values between 0 and 2*maxSetpoint
        const x = Math.floor(maxSetpoint * event.clientX / window.innerWidth);
        const y = Math.floor(maxSetpoint * event.clientY / window.innerHeight);
        const spL  = clamp(maxSetpoint / 2 + x - y, 0, maxSetpoint);
        const spR = clamp(1.5 * maxSetpoint - x - y, 0, maxSetpoint);
        const splPercentage = round(toPercentage(spL, 0, maxSetpoint) * 2 - 100);
        const sprPercentage = round(toPercentage(spR, 0, maxSetpoint) * 2 - 100);

        document.getElementById('leftSpeed').innerHTML  = `Left speed setpoint: ${splPercentage}%`;
        document.getElementById('rightSpeed').innerHTML = `Right speed setpoint: ${sprPercentage}%`;

        updateKnob(x, y);
        updateSPSliders(spL, spR);
        sendSetpoints(spL, spR);
    }
};

// Update the knob based on the mouse position
const updateKnob = (x, y) => { 
    const num = maxSetpoint/2 - x
    const den = y - maxSetpoint/2;
    const t = Math.atan2(num, den) / Math.PI * 50 + 50;
    $('.knob').val(t).trigger('change');
};

// Update the sliders based on the setpoints and feedback
const updateSPSliders = (spL, spR) => { // Setpoint sliders
    document.getElementById("spLB").value = (halfMaxSetpoint - spL) / percHalfMaxSP;
    document.getElementById("spLF").value = (spL - halfMaxSetpoint) / percHalfMaxSP;

    document.getElementById("spRB").value = (halfMaxSetpoint - spR) / percHalfMaxSP;
    document.getElementById("spRF").value = (spR - halfMaxSetpoint) / percHalfMaxSP;
};

const updateFBSliders = (fbL, fbR) => { // Feedback sliders
    document.getElementById("fbLB").value = (halfMaxSetpoint - fbL) / percHalfMaxSP;
    document.getElementById("fbLF").value = (fbL - halfMaxSetpoint) / percHalfMaxSP;

    document.getElementById("fbRB").value = (halfMaxSetpoint - fbR) / percHalfMaxSP;
    document.getElementById("fbRF").value = (fbR - halfMaxSetpoint) / percHalfMaxSP;
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