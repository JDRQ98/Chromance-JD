#include <WiFi.h>


// WiFi stuff - CHANGE FOR YOUR OWN NETWORK!
const char* ssid = "TP-Link-150";
const char* password = "Cenote#150";

const IPAddress ip(192, 168, 0, 241);  // IP address that THIS DEVICE should request
const IPAddress gateway(192, 168, 0, 1);  // Your router
const IPAddress subnet(255, 255, 255, 0);  // Your subnet mask (find it from your router's admin panel)

WiFiServer server(80); //Open port number 80 (HTTP)

unsigned long currentTime = 0;
unsigned long previousTime = 0;
const long timeout = 2000;

// Auxiliar variables to store the current output state
String output26State = "off";
String output27State = "off";
int loopFireRippleEnabled = 1;
int manualFireRipple = 0;
int currentNumberofRipples = 9;

String header = ""; //Variable to store the HTTP request



void HandleHTTPRequest(WiFiClient client){                         // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeout) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // handle HTTP requests
            if (header.indexOf("GET /ManualRipple") >= 0) {
              Serial.println("Firing Manual Ripple");
              manualFireRipple = 1;
            } else if (header.indexOf("GET /FireRippleEnabled/on") >= 0) {
              Serial.println("loopFireRippleEnabled on");
              loopFireRippleEnabled = 1;
            } else if (header.indexOf("GET /FireRippleEnabled/off") >= 0) {
              Serial.println("loopFireRippleEnabled off");
              loopFireRippleEnabled = 0;
            } else if (header.indexOf("POST /updateVariable") >= 0) {
              Serial.println("received the following POST request (Raw): \n");
              Serial.println(header);
              int startPos = header.indexOf("variable=") + 9; // add 9 to move past "variable="
              int endPos = header.indexOf("&");
              Serial.println("startPos: \n");
              Serial.println(startPos);
              if(startPos != 8) { /* payload found */
                String variableValueStr = header.substring(startPos, endPos);
                int variableValue = variableValueStr.toInt();
                currentNumberofRipples = variableValue; // update the variable
              }
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println();
              client.println("Variable updated");
            }
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>ESP32 Web Server</h1>");
            
            // Display current state, and ON/OFF buttons for GPIO 26  
            client.println("<p> Fire Manual Ripple </p>");
            client.println("<p><a href=\"/ManualRipple\"><button class=\"button\">Fire!</button></a></p>");
               
            // Display ON/OFF checkbox for loopFireRippleEnabled
            if (loopFireRippleEnabled)
            {
              client.println("<p><input type=\"checkbox\" id=\"auto-ripple-checkbox\" name=\"auto-ripple\" value=\"1\" onchange=\"toggleAutoRipple(this)\" checked><label for=\"auto-ripple-checkbox\">Automatic Ripples</label></p>");
            }
            else
            {
              client.println("<p><input type=\"checkbox\" id=\"auto-ripple-checkbox\" name=\"auto-ripple\" value=\"1\" onchange=\"toggleAutoRipple(this)\"><label for=\"auto-ripple-checkbox\">Automatic Ripples</label></p>");
            }

            /* Text input for number of ripples */
            client.println("<form action=\"/updateVariable\" method=\"post\">");
            client.println("<label for=\"variable\">Enter a value for the variable:</label>");
            client.println("<input type=\"text\" id=\"variable\" name=\"variable\" value=\"" + String(currentNumberofRipples) + "\">");
            client.println("<button type=\"submit\">Submit</button>");
            client.println("</form>");
            
            
            client.println("<div> </div>");
            client.println("<p> Rainbow: <input type=\"checkbox\" id=\"checkboxRainbow\" data-toggle=\"toggle\" data-onstyle=\"default\"  data-width=\"500%\"> ");
            client.println("<div> </div>");
            
            client.println("<button onclick=\"saveData()\" id=\"buttonSave\">Save data</button> ");
            client.println("<a href=\"/SendConfiguration\"> <button id=\"buttonSend\">Send data</button> </a> ");
            
            //Javascript functions
            client.println("<script> function toggleAutoRipple(checkbox)");
            client.println("{if (checkbox.checked){");
                // checkbox is checked, turn on automatic ripples
                client.println("fetch('/FireRippleEnabled/on');}");
              client.println("else{");
                // checkbox is unchecked, turn off automatic ripples
                client.println("fetch('/FireRippleEnabled/off');}");
            client.println("}</script>");
                /*
                client.println("<script>");
                client.println("window.post = function(url) {");
                client.println("return fetch(url, {method: \"GET\", headers: {'Content-Type': 'application/json'} });");
                client.println("}");
                client.println("function sendData() {");
                client.println("post(\"/FireRippleEnabled/off\"");
                client.println("}");
                client.println("</script>");
                */

                client.println("</body></html>");

            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
}

void WiFi_MainFunction(void){
  WiFiClient client = server.available();   // Listen for incoming clients
  if (client) {                             // If a new client connects,
    Serial.println("New Client. Handling HTTP request");
    HandleHTTPRequest(client);
  }
}

  void WiFi_init(void){
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid, password);
      WiFi.config(ip, gateway, subnet);
      while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("Connection Failed! Rebooting...");
        delay(50000);
        ESP.restart();
      }
      server.begin();
      Serial.print("WiFi connected! IP = ");
      Serial.println(WiFi.localIP());
  }
