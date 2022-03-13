#include <WiFi.h>

unsigned long currentTime = 0;
unsigned long previousTime = 0;
const long timeout = 2000;

// Auxiliar variables to store the current output state
String output26State = "off";
String output27State = "off";
int loopFireRippleEnabled = 1;
int manualFireRipple = 0;

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
            
            // turns the GPIOs on and off
            if (header.indexOf("GET /ManualRipple") >= 0) {
              Serial.println("Firing Manual Ripple");
              manualFireRipple = 1;
            } else if (header.indexOf("GET /FireRippleEnabled/on") >= 0) {
              Serial.println("loopFireRippleEnabled on");
              loopFireRippleEnabled = 1;
            } else if (header.indexOf("GET /FireRippleEnabled/off") >= 0) {
              Serial.println("loopFireRippleEnabled off");
              loopFireRippleEnabled = 0;
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
               
            // Display current state, and ON/OFF buttons for loopFireRippleEnabled     
            if (loopFireRippleEnabled) {
              client.println("<p> Automatic Ripples ON </p>");
              client.println("<p><a href=\"/FireRippleEnabled/off\"><button class=\"button button2\">Turn off</button></a></p>");
            } else {
              client.println("<p> Automatic Ripples OFF </p>");
              client.println("<p><a href=\"/FireRippleEnabled/on\"><button class=\"button\">Turn on</button></a></p>");
            }
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
