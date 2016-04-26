Pebble.addEventListener("appmessage",
  function(e) {
    //console.log(e.payload[0]);
    if(e.payload.hello_msg == "whether" || e.payload.hello_msg == "outdoor"){
      //call the sendtowhether function when the request is whether or outdoor
      sendToWhether(e.payload.hello_msg);
    }else{
      //call the sendToServer function otherwise
      sendToServer(e.payload.hello_msg); //parameters      
    }
  }
);


var ipAddress = "158.130.111.87"; // Hard coded IP address

//send to server with the specified request
function sendToServer(parameter) {
  
  var req = new XMLHttpRequest();
  var port = "3001"; // Same port specified as argument to server
  var url = "http://" + ipAddress + ":" + port + "/" + parameter; 
  var method = "GET";
  var async = true;
  
  req.onload = function(e) {
      // see what came back
      var msg = "no response";
      var response = JSON.parse(req.responseText);
      if (response) {
        if (response.name) {
          msg = response.name;
        }
        else msg = "noname";
      }
      // sends message back to pebble
      Pebble.sendAppMessage({ "0": msg});
  };
  
  // Error handler: e.g. when server is not responsing..
  req.onerror = function () {
      Pebble.sendAppMessage({
            "0": "The server is unavailable."
        });
  };
    req.open(method, url, async);
    req.send(null);
}


/*
 * send message to our server without handling the response from server. 
 */
function sendToServerNoResponse(parameter) {
    var req = new XMLHttpRequest();
    var port = "3001"; // Same port specified as argument to server
    var url = "http://" + ipAddress + ":" + port + "/" + parameter;
    var method = "GET";
    var async = true;
    req.open(method, url, async);
    req.send(null);
}


/*
 * send message to query Philly's weather report and send result to our server or Pebble watch.
 */
function sendToWhether(parameter) {
    var req = new XMLHttpRequest();
    var port = "3001"; // Same port specified as argument to server
    var url = "http://api.openweathermap.org/data/2.5/weather?id=4560349&appid=ddf8f563b0c0f82b53d401753b9d71ab";
    var method = "GET";
    var async = true;

    req.onload = function(e) {
        // see what came back
        var msg = "No Response.";
        var response = JSON.parse(req.responseText);
        if (response) {
            if (response.main.temp) {
                var temp = parseFloat(response.main.temp) - 273.15;
                temp = (Math.round(temp * 100) / 100);
                msg = "Temperature: " + temp + " °C" ;
            } else msg = "No Weather Information";
        }
        if(parameter == "outdoor"){
          sendToServerNoResponse("whether:"+ temp);
        }else{
          Pebble.sendAppMessage({"0": msg});
        }
        // sends message back to pebble
    };
    // Error handler: e.g. when server is not responsing..
    req.onerror = function () {
      Pebble.sendAppMessage({
            "0": "Connection to whether API failed."
        });
    };
    req.open(method, url, async);
    req.send(null);
}