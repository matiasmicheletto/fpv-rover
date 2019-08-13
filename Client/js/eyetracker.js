window.Eyetracker = (function(){
    return {
        tracking: false, // Estado midiendo/detenido        
        init: function(){ // Inicializar eyetracker
            TobiiWeb_Client.start({
                enable_state_machine_logs : true, 
                use_filter : "none", 
                services : [
                    {name : "GazeTracking_Service", page_url : window.location.href},
                    {name : "Log_Service"},
                    {name : "Echo_Service", prepend : "The server responded with: "},
                    {name : "Screenshot_Service"}
                ],
                onMessage : {
                    "GazeTracking_Service" : GazeCoordinatesReceiver({
                        filter: "none",
                        filter_size: 3,
                        onGazeCoordinates: function(x,y,t){ // Callback de datos recibidos
                            if(this.tracking)
                                this.onData(x/window.innerWidth, y/window.innerHeight);
                        }
                    }),
                    "Echo_Service" : function(message){
                        alert(message);
                    },
                    "Screenshot_Service" : function(message){
                        console.log("received: ");
                        console.log(message)
                    }
                }
              });
        },
        finish: function(){ // Detener adquisicion
            TobiiWeb_Client.stop();
        },
        start: function(){ // Habilitar callback
            this.tracking = true;
        },
        stop: function(){ // Deshabilitar callback
            this.tracking = false;
        },
        onData: function(x,y){ // Override
            console.log(x+"-"+y);
        }
     };
})()
