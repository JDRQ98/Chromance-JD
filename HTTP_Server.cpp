#include <WiFi.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <string>  
using namespace std;

#include "HTTP_Server.h" 
#include "ripple.h"
#include "EEP.h"
#include "SimpleJson.h"


// WiFi stuff - CHANGE FOR YOUR OWN NETWORK!
const char* ssid = "TP-Link-150";
const char* password = "Cenote#150";

const IPAddress ip(192, 168, 0, 241);  // IP address that THIS DEVICE should request
const IPAddress gateway(192, 168, 0, 1);  // Your router
const IPAddress subnet(255, 255, 255, 0);  // Your subnet mask (find it from your router's admin panel)

#define server hueBridge.webServer //Open port number 80 (HTTP)

unsigned long currentTime = 0;
unsigned long previousTime = 0;
const long timeout = 2000;

// Auxiliar variables to store the current output state
GlobalParameters_struct GlobalParameters = { 
  .loop_MasterFireRippleEnabled = 1,
  .loop_CenterFireRippleEnabled = 0,
  .loop_CubeFireRippleEnabled = 0,
  .loop_QuadFireRippleEnabled = 0,
  .loop_BorderFireRippleEnabled = 0,
  .loop_RandomEffectEnabled = 1,
  .currentNumberofRipples = NUMBER_OF_RIPPLES,
  .currentNumberofColors = 7,
  .currentBehavior = feisty,
  .currentDirection = ALL_DIRECTIONS,
  .currentDelayBetweenRipples = 3000, /* in milliseconds */
  .currentRainbowDeltaPerTick = 200, /* units: hue */
  .currentRippleLifeSpan = 3000, /* in milliseconds */
  .currentRippleSpeed = 0.5, 
  .currentDecay = 0.985,  // Multiply all LED's by this amount each tick to create fancy fading tails 0.972 good value for rainbow
};
boolean manualFireRipple = 0;
unsigned int currentProfile = 0;

String SendHTML_Dashboard(void) {
// Display the HTML web page
String ptr = "<!DOCTYPE html> <html>\n";
ptr += "<html>\n";
ptr += "<head>\n";
ptr += "  <meta http-equiv='content-type' content='text/html; charset=windows-1252'>\n";
ptr += "  <meta name='viewport' content='width=device-width, initial-scale=1'>\n";
ptr += "  <link rel='icon' href='data:,'>\n";
ptr += "  <style>\n";
ptr += "    html {\n";
ptr += "      font-family: Helvetica;\n";
ptr += "      display: inline-block;\n";
ptr += "      margin: 0px auto;\n";
ptr += "      text-align: center;\n";
ptr += "    }\n";
ptr += "\n";
ptr += "    .button {\n";
ptr += "      background-color: #4CAF50;\n";
ptr += "      border: none;\n";
ptr += "      color: white;\n";
ptr += "      padding: 16px 40px;\n";
ptr += "      text-decoration: none;\n";
ptr += "      font-size: 30px;\n";
ptr += "      margin: 2px;\n";
ptr += "      cursor: pointer;\n";
ptr += "    }\n";
ptr += "\n";
ptr += "    .button2 {\n";
ptr += "      background-color: #555555;\n";
ptr += "    }\n";
ptr += "\n";
ptr += "    .InactiveProfile {\n";
ptr += "      background-color: gray;\n";
ptr += "      font-size: 30px;\n";
ptr += "      color: white;\n";
ptr += "      padding: 25px 25px;\n";
ptr += "      margin: 5px;\n";
ptr += "      border: none;\n";
ptr += "      cursor: pointer;\n";
ptr += "    }\n";
ptr += "\n";
ptr += "    .ActiveProfile {\n";
ptr += "      background-color: #4CAF50;\n";
ptr += "      font-size: 30px;\n";
ptr += "      color: white;\n";
ptr += "      padding: 25px 25px;\n";
ptr += "      margin: 5px;\n";
ptr += "      border: none;\n";
ptr += "      cursor: pointer;\n";
ptr += "    }\n";
ptr += "\n";
ptr += "    .SelectedProfile {\n";
ptr += "      background-color: green;\n";
ptr += "      font-size: 30px;\n";
ptr += "      color: white;\n";
ptr += "      padding: 30px 30px;\n";
ptr += "      margin: 5px;\n";
ptr += "      border: none;\n";
ptr += "      cursor: pointer;\n";
ptr += "    }\n";
ptr += "\n";
ptr += "    form div {\n";
ptr += "      display: flex;\n";
ptr += "      justify-content: space-between;\n";
ptr += "      margin: 10px 0;\n";
ptr += "    }\n";
ptr += "\n";
ptr += "    form div label {\n";
ptr += "      flex: 1;\n";
ptr += "      text-align: left;\n";
ptr += "    }\n";
ptr += "\n";
ptr += "    form div input {\n";
ptr += "      flex: 1;\n";
ptr += "      text-align: right;\n";
ptr += "    }\n";
ptr += "  </style>\n";
ptr += "</head>\n";
ptr += "<body>\n";
ptr += "  <h1>ESP32 Web Server</h1>\n";
ptr += "  <p>Fire Manual Ripple</p>\n";
ptr += "  <p><a href='http://192.168.0.241/ManualRipple'><button class='button'>Fire!</button></a></p>\n";
/****************** BEGINNING OF CHECKBOXES ******************/
  ptr += "<section id=\"checkboxes\"\n>";
  /* Display ON/OFF checkbox for loop_MasterFireRippleEnabled */
  if (GlobalParameters.loop_MasterFireRippleEnabled) {
    ptr += "<p><input type=\"checkbox\" id=\"master-auto-ripple-checkbox\" name=\"master-auto-ripple\" value=\"1\" onchange=\"toggleMasterAutoRipple(this)\" checked><label for=\"master-auto-ripple-checkbox\">Master Enable Automatic Ripples</label></p>\n";
  } else {
    ptr += "<p><input type=\"checkbox\" id=\"master-auto-ripple-checkbox\" name=\"master-auto-ripple\" value=\"1\" onchange=\"toggleMasterAutoRipple(this)\"><label for=\"master-auto-ripple-checkbox\">Master Enable Automatic Ripples</label></p>\n";
  }
  /* Display ON/OFF checkbox for loop_CenterFireRippleEnabled */
  if (GlobalParameters.loop_CenterFireRippleEnabled) {
    ptr += "<p><input type=\"checkbox\" id=\"center-auto-ripple-checkbox\" name=\"center-auto-ripple\" value=\"1\" onchange=\"toggleCenterAutoRipple(this)\" checked><label for=\"center-auto-ripple-checkbox\">Enable Ripples in Center Node</label></p>\n";
  } else {
    ptr += "<p><input type=\"checkbox\" id=\"center-auto-ripple-checkbox\" name=\"center-auto-ripple\" value=\"1\" onchange=\"toggleCenterAutoRipple(this)\"><label for=\"center-auto-ripple-checkbox\">Enable Ripples in Center Node</label></p>\n";
  }
  /* Display ON/OFF checkbox for loop_CubeFireRippleEnabled */
  if (GlobalParameters.loop_CubeFireRippleEnabled) {
    ptr += "<p><input type=\"checkbox\" id=\"cube-auto-ripple-checkbox\" name=\"cube-auto-ripple\" value=\"1\" onchange=\"toggleCubeAutoRipple(this)\" checked><label for=\"cube-auto-ripple-checkbox\">Enable Ripples in Cube Nodes</label></p>\n";
  } else {
    ptr += "<p><input type=\"checkbox\" id=\"cube-auto-ripple-checkbox\" name=\"cube-auto-ripple\" value=\"1\" onchange=\"toggleCubeAutoRipple(this)\"><label for=\"cube-auto-ripple-checkbox\">Enable Ripples in Cube Nodes</label></p>\n";
  }
  /* Display ON/OFF checkbox for loop_QuadFireRippleEnabled */
  if (GlobalParameters.loop_QuadFireRippleEnabled) {
    ptr += "<p><input type=\"checkbox\" id=\"quad-auto-ripple-checkbox\" name=\"quad-auto-ripple\" value=\"1\" onchange=\"toggleQuadAutoRipple(this)\" checked><label for=\"quad-auto-ripple-checkbox\">Enable Ripples in Quad Nodes</label></p>\n";
  } else {
    ptr += "<p><input type=\"checkbox\" id=\"quad-auto-ripple-checkbox\" name=\"quad-auto-ripple\" value=\"1\" onchange=\"toggleQuadAutoRipple(this)\"><label for=\"quad-auto-ripple-checkbox\">Enable Ripples in Quad Nodes</label></p>\n";
  }
  /* Display ON/OFF checkbox for loop_BorderFireRippleEnabled */
  if (GlobalParameters.loop_BorderFireRippleEnabled) {
    ptr += "<p><input type=\"checkbox\" id=\"border-auto-ripple-checkbox\" name=\"border-auto-ripple\" value=\"1\" onchange=\"toggleBorderAutoRipple(this)\" checked><label for=\"border-auto-ripple-checkbox\">Enable Ripples in Border Nodes</label></p>\n";
  } else {
    ptr += "<p><input type=\"checkbox\" id=\"border-auto-ripple-checkbox\" name=\"border-auto-ripple\" value=\"1\" onchange=\"toggleBorderAutoRipple(this)\"><label for=\"border-auto-ripple-checkbox\">Enable Ripples in Border Nodes</label></p>\n";
  }
  /* Display ON/OFF checkbox for loop_RandomEffectEnabled */
  if (GlobalParameters.loop_RandomEffectEnabled) {
    ptr += "<p><input type=\"checkbox\" id=\"Random-Effect-checkbox\" name=\"Random-ripple\" value=\"1\" onchange=\"toggleRandomEffect(this)\" checked><label for=\"Random-Effect-checkbox\">Enable Random effect</label></p>\n";
  } else {
    ptr += "<p><input type=\"checkbox\" id=\"Random-Effect-checkbox\" name=\"Random-ripple\" value=\"1\" onchange=\"toggleRandomEffect(this)\"><label for=\"Random-Effect-checkbox\">Enable Random effect</label></p>\n";
  }
  ptr += "</section\n>";
/****************** END OF CHECKBOXES ******************/
ptr += "  <section id='userinput'>\n";
ptr += "    <form action='/updateInternalVariables' method='post'>\n";
ptr += "      <div class='rangeslider'> \n";
ptr += "        <label for='NumberofRipples'>Enter the number of ripples [1 - 100]:</label>\n";
ptr += "        <input type='range' id='currentNumberofRipples' name='NumberofRipples' min='1' max='99' value='99'>\n";
ptr += "        <span id='currentNumberofRipples_display'></span>\n";
ptr += "      </div>\n";
ptr += "      <div class='rangeslider'> \n";
ptr += "        <label for='currentDelayBetweenRipples'>Enter delay between ripples in ms [1 - 20000]:</label>\n";
ptr += "        <input type='range' id='currentDelayBetweenRipples' name='currentDelayBetweenRipples' min='1' max='20000' value='6980'>\n";
ptr += "        <span id='currentDelayBetweenRipples_display'></span>\n";
ptr += "      </div>\n";
ptr += "      <div class='rangeslider'> \n";
ptr += "        <label for='currentRainbowDeltaPerTick'>Enter rainbow delta per tick in hue [1 - 20000]:</label>\n";
ptr += "        <input type='range' id='currentRainbowDeltaPerTick' name='currentRainbowDeltaPerTick' min='1' max='20000' value='200'>\n";
ptr += "        <span id='currentRainbowDeltaPerTick_display'></span>\n";
ptr += "      </div>\n";
ptr += "      <div class='rangeslider'> \n";
ptr += "        <label for='currentRippleLifeSpan'>Enter ripple life span in ms [1 - 20000]:</label>\n";
ptr += "        <input type='range' id='currentRippleLifeSpan' name='currentRippleLifeSpan' min='1' max='20000' value='3000'>\n";
ptr += "        <span id='currentRippleLifeSpan_display'></span>\n";
ptr += "      </div>\n";
ptr += "      <div class='rangeslider'> \n";
ptr += "        <label for='currentRippleSpeed'>Enter ripple speed (float) [0.01 - 10]:</label>\n";
ptr += "        <input type='range' id='currentRippleSpeed' name='currentRippleSpeed' min='1' max='1000' value='1'>\n";
ptr += "        <span id='currentRippleSpeed_display'></span>\n";
ptr += "      </div>\n";
ptr += "      <div class='rangeslider'> \n";
ptr += "        <label for='currentNumberofColors'>Enter the desired number of colors [1 - 25]:</label>\n";
ptr += "        <input type='range' id='currentNumberofColors' name='currentNumberofColors' min='1' max='25' value='7'>\n";
ptr += "        <span id='currentNumberofColors_display'></span>\n";
ptr += "      </div>\n";
ptr += "      <div>\n";
ptr += "        <label for='currentDecay'>Enter decay per tick [0.5 - 1]:</label>\n";
ptr += "        <input type='range' id='currentDecay' name='currentDecay' min='500' max='1000' value='500'>\n";
ptr += "        <span id='currentDecay_display'></span>\n";
ptr += "      </div>\n";
ptr += "      <div>\n";
ptr += "        <label for='currentBehavior'>Enter the desired behavior [0-2 = less to more aggro; 3 = alwaysRight; 4 = alwaysLeft]:</label>\n";
ptr += "        <select id='currentBehavior' name='currentBehavior'>\n";
ptr += "          <option value='0'>Mild</option>\n";
ptr += "          <option value='1' selected='selected'>Normal</option>\n";
ptr += "          <option value='2'>Agressive</option>\n";
ptr += "          <option value='3'>Always turns right</option>\n";
ptr += "          <option value='4'>Always turns left</option>\n";
ptr += "        </select>\n";
ptr += "      </div>\n";
ptr += "      <div>\n";
ptr += "        <label for='currentDirection'>Enter ripple direction [-1 = All directions; 0-5 = direction clockwise starting at 12:00; 6 = random direction]:</label>\n";
ptr += "        <select id='currentDirection' name='currentDirection'>\n";
ptr += "          <option value='-1' selected='selected'>All directions</option>\n";
ptr += "          <option value='0'>0°</option>\n";
ptr += "          <option value='1'>60</option>\n";
ptr += "          <option value='2'>120°</option>\n";
ptr += "          <option value='3'>180°</option>\n";
ptr += "          <option value='4'>-120°</option>\n";
ptr += "          <option value='5'>-60°</option>\n";
ptr += "          <option value='6'>Random direction</option>\n";
ptr += "        </select>\n";
ptr += "      </div>      \n";
ptr += "      <div>\n";
ptr += "        <button type='button' onclick='sendData()'>Submit</button>\n";
ptr += "      </div>\n";
ptr += "    </form>\n";
ptr += "  </section>\n";
/****************** BEGINNING OF PROFILE MANAGEMENT ******************/
  ptr += "<section id=\"userinput\"\n>";
  ptr += "<h1> Profiles </h1\n>";
    ptr += "<p> Current profile: " + String(currentProfile) + "</p\n>";
    ptr += "<div\n>";
      ptr += "<div id=\"profiles\"\n>";
        /* button 1*/
        ptr += "<a href=\"http://192.168.0.241/RestoreProfile_1\"><button ";
        if( (currentProfile == 0) && (ProfilesAvailable[0] == 1U)) ptr += "class=\"SelectedProfile\"";
        else if (ProfilesAvailable[0] == 1U) ptr += "class=\"ActiveProfile\"";
        else ptr += "class=\"InactiveProfile\"";
        ptr += ">#1</button></a>\n";
        /* button 2*/
        ptr += "<a href=\"http://192.168.0.241/RestoreProfile_2\"><button ";
        if( (currentProfile == 1) && (ProfilesAvailable[1] == 1U)) ptr += "class=\"SelectedProfile\"";
        else if (ProfilesAvailable[1] == 1U) ptr += "class=\"ActiveProfile\"";
        else ptr += "class=\"InactiveProfile\"";
        ptr += ">#2</button></a>\n";
        /* button 3*/
        ptr += "<a href=\"http://192.168.0.241/RestoreProfile_3\"><button ";
        if( (currentProfile == 2) && (ProfilesAvailable[2] == 1U)) ptr += "class=\"SelectedProfile\"";
        else if (ProfilesAvailable[2] == 1U) ptr += "class=\"ActiveProfile\"";
        else ptr += "class=\"InactiveProfile\"";
        ptr += ">#3</button></a>\n";
        /* button 4*/
        ptr += "<a href=\"http://192.168.0.241/RestoreProfile_4\"><button ";
        if( (currentProfile == 3) && (ProfilesAvailable[3] == 1U)) ptr += "class=\"SelectedProfile\"";
        else if (ProfilesAvailable[3] == 1U) ptr += "class=\"ActiveProfile\"";
        else ptr += "class=\"InactiveProfile\"";
        ptr += ">#4</button></a>\n";
        /* button 5*/
        ptr += "<a href=\"http://192.168.0.241/RestoreProfile_5\"><button ";
        if( (currentProfile == 4) && (ProfilesAvailable[4] == 1U)) ptr += "class=\"SelectedProfile\"";
        else if (ProfilesAvailable[4] == 1U) ptr += "class=\"ActiveProfile\"";
        else ptr += "class=\"InactiveProfile\"";
        ptr += ">#5</button></a>\n";
      ptr += "</div\n>";
      ptr += "<div\n>";
        ptr += "<a href=\"http://192.168.0.241/StoreProfile\"><button class=\"button\">Store Profile</button></a\n>";
        ptr += "<a href=\"http://192.168.0.241/DeleteProfile\"><button class=\"button\">Delete Profile</button></a\n>";
      ptr += "</div\n>";
    ptr += "</div\n>";
  ptr += "</section\n>";
/****************** END OF PROFILE MANAGEMENT ******************/
ptr += "  <p><a href='http://192.168.0.241/SWreset'><button class='button'>Software reset</button></a></p>\n";
ptr += "  <script>\n";
ptr += "\n";
ptr += "// START OF SLIDER SCRIPTS\n";
ptr += "var currentNumberofRipples_slider = document.getElementById('currentNumberofRipples'); \n";
ptr += "var currentNumberofRipples_var = currentNumberofRipples_slider.value;      \n";
ptr += "var outpcurrentNumberofRipples_display = document.getElementById('currentNumberofRipples_display'); \n";
ptr += "outpcurrentNumberofRipples_display.innerHTML = currentNumberofRipples_var;\n";
ptr += "currentNumberofRipples_slider.oninput = function() {\n";
ptr += "  currentNumberofRipples_var = this.value;\n";
ptr += "  outpcurrentNumberofRipples_display.innerHTML = currentNumberofRipples_var;\n";
ptr += "}\n";
ptr += "  \n";
ptr += "var currentDelayBetweenRipples_slider = document.getElementById('currentDelayBetweenRipples'); \n";
ptr += "var currentDelayBetweenRipples_var = currentDelayBetweenRipples_slider.value;      \n";
ptr += "var outpcurrentDelayBetweenRipples_display = document.getElementById('currentDelayBetweenRipples_display'); \n";
ptr += "outpcurrentDelayBetweenRipples_display.innerHTML = currentDelayBetweenRipples_var;\n";
ptr += "currentDelayBetweenRipples_slider.oninput = function() {\n";
ptr += "  currentDelayBetweenRipples_var = this.value;\n";
ptr += "  outpcurrentDelayBetweenRipples_display.innerHTML = currentDelayBetweenRipples_var;\n";
ptr += "}\n";
ptr += "                   \n";
ptr += "var currentRainbowDeltaPerTick_slider = document.getElementById('currentRainbowDeltaPerTick'); \n";
ptr += "var currentRainbowDeltaPerTick_var = currentRainbowDeltaPerTick_slider.value;      \n";
ptr += "var outpcurrentRainbowDeltaPerTick_display = document.getElementById('currentRainbowDeltaPerTick_display'); \n";
ptr += "outpcurrentRainbowDeltaPerTick_display.innerHTML = currentRainbowDeltaPerTick_var;\n";
ptr += "currentRainbowDeltaPerTick_slider.oninput = function() {\n";
ptr += "  currentRainbowDeltaPerTick_var = this.value;\n";
ptr += "    outpcurrentRainbowDeltaPerTick_display.innerHTML = currentRainbowDeltaPerTick_var;\n";
ptr += "}\n";
ptr += "                 \n";
ptr += "var currentRippleLifeSpan_slider = document.getElementById('currentRippleLifeSpan'); \n";
ptr += "var currentRippleLifeSpan_var = currentRippleLifeSpan_slider.value;      \n";
ptr += "var outpcurrentRippleLifeSpan_display = document.getElementById('currentRippleLifeSpan_display'); \n";
ptr += "outpcurrentRippleLifeSpan_display.innerHTML = currentRippleLifeSpan_var;\n";
ptr += "currentRippleLifeSpan_slider.oninput = function() {\n";
ptr += "  currentRippleLifeSpan_var = this.value;\n";
ptr += "  outpcurrentRippleLifeSpan_display.innerHTML = currentRippleLifeSpan_var;\n";
ptr += "}\n";
ptr += "\n";
ptr += "var currentRippleSpeed_slider = document.getElementById('currentRippleSpeed'); \n";
ptr += "var currentRippleSpeed_var = currentRippleSpeed_slider.value;                     \n";
ptr += "var outpcurrentRippleSpeed_display = document.getElementById('currentRippleSpeed_display'); \n";
ptr += "outpcurrentRippleSpeed_display.innerHTML = currentRippleSpeed_var/100;\n";
ptr += "currentRippleSpeed_slider.oninput = function() {\n";
ptr += "  currentRippleSpeed_var = this.value/100; //scale value down by 100\n";
ptr += "    outpcurrentRippleSpeed_display.innerHTML = currentRippleSpeed_var;\n";
ptr += "}\n";
ptr += "\n";
ptr += "var currentNumberofColors_slider = document.getElementById('currentNumberofColors'); \n";
ptr += "var currentNumberofColors_var = currentNumberofColors_slider.value;             \n";
ptr += "var outpcurrentNumberofColors_display = document.getElementById('currentNumberofColors_display'); \n";
ptr += "outpcurrentNumberofColors_display.innerHTML = currentNumberofColors_var;\n";
ptr += "currentNumberofColors_slider.oninput = function() {\n";
ptr += "  currentNumberofColors_var = this.value;\n";
ptr += "    outpcurrentNumberofColors_display.innerHTML = currentNumberofColors_var;\n";
ptr += "}\n";
ptr += "\n";
ptr += "var currentDecay_slider = document.getElementById('currentDecay'); \n";
ptr += "var currentDecay_var = currentDecay_slider.value;                        \n";
ptr += "var outpcurrentDecay_display = document.getElementById('currentDecay_display'); \n";
ptr += "outpcurrentDecay_display.innerHTML = currentDecay_var/1000;\n";
ptr += "currentDecay_slider.oninput = function() {\n";
ptr += "  currentDecay_var = this.value/1000; //scale value down by 1000\n";
ptr += "    outpcurrentDecay_display.innerHTML = currentDecay_var;\n";
ptr += "}\n";
ptr += "// END OF SLIDER SCRIPTS\n";
ptr += "\n";
ptr += "  // used to update internal variables\n";
ptr += "  function sendData() {\n";
ptr += "    // Include the logic to send the data to the server\n";
ptr += "    // You can access the values with document.getElementById('elementId').value\n";
ptr += "    data = {};\n";
ptr += "    // Get the value of the sliders\n";
ptr += "    data.currentNumberofRipples = parseInt(currentNumberofRipples_slider.value); \n";
ptr += "    data.currentDelayBetweenRipples = parseInt(currentDelayBetweenRipples_slider.value); \n";
ptr += "    data.currentRainbowDeltaPerTick = parseInt(currentRainbowDeltaPerTick_slider.value); \n";
ptr += "    data.currentRippleLifeSpan = parseInt(currentRippleLifeSpan_slider.value); \n";
ptr += "    data.currentRippleSpeed = parseFloat(currentRippleSpeed_slider.value); \n";
ptr += "    data.currentNumberofColors = parseInt(currentNumberofColors_slider.value); \n";
ptr += "    data.currentDecay = parseFloat(currentDecay_slider.value); \n";
ptr += "    data.currentBehavior = parseInt(document.getElementById('currentBehavior').value); \n";
ptr += "    data.currentDirection = parseInt(document.getElementById('currentDirection').value); \n";
ptr += "    \n";
ptr += "\n";
ptr += "    var xhr = new XMLHttpRequest();\n";
ptr += "    xhr.open('POST', '/updateInternalVariables', true);\n";
ptr += "    xhr.setRequestHeader('Content-Type', 'application/json');\n";
ptr += "    xhr.send(JSON.stringify(data));\n";
ptr += "  }\n";
ptr += "\n";
ptr += "    function toggleMasterAutoRipple(checkbox) {\n";
ptr += "      if (checkbox.checked) {\n";
ptr += "        fetch('/MasterFireRippleEnabled/on');\n";
ptr += "      } else {\n";
ptr += "        fetch('/MasterFireRippleEnabled/off');\n";
ptr += "      }\n";
ptr += "    }\n";
ptr += "    function toggleCenterAutoRipple(checkbox) {\n";
ptr += "      if (checkbox.checked) {\n";
ptr += "        fetch('/CenterFireRippleEnabled/on');\n";
ptr += "      } else {\n";
ptr += "        fetch('/CenterFireRippleEnabled/off');\n";
ptr += "      }\n";
ptr += "    }\n";
ptr += "    function toggleCubeAutoRipple(checkbox) {\n";
ptr += "      if (checkbox.checked) {\n";
ptr += "        fetch('/CubeFireRippleEnabled/on');\n";
ptr += "      } else {\n";
ptr += "        fetch('/CubeFireRippleEnabled/off');\n";
ptr += "      }\n";
ptr += "    }\n";
ptr += "    function toggleQuadAutoRipple(checkbox) {\n";
ptr += "      if (checkbox.checked) {\n";
ptr += "        fetch('/QuadFireRippleEnabled/on');\n";
ptr += "      } else {\n";
ptr += "        fetch('/QuadFireRippleEnabled/off');\n";
ptr += "      }\n";
ptr += "    }\n";
ptr += "    function toggleBorderAutoRipple(checkbox) {\n";
ptr += "      if (checkbox.checked) {\n";
ptr += "        fetch('/BorderFireRippleEnabled/on');\n";
ptr += "      } else {\n";
ptr += "        fetch('/BorderFireRippleEnabled/off');\n";
ptr += "      }\n";
ptr += "    }\n";
ptr += "    function toggleRandomEffect(checkbox) {\n";
ptr += "      if (checkbox.checked) {\n";
ptr += "        fetch('/RandomEffectEnabled/on');\n";
ptr += "      } else {\n";
ptr += "        fetch('/RandomEffectEnabled/off');\n";
ptr += "      }\n";
ptr += "    }\n";
ptr += "  </script>\n";
ptr += "</body>\n";
ptr += "</html>\n";
ptr += "\n";
return ptr;
}

/* HANDLER FUNCTIONS */

void handle_PostRequest() {
    Serial.println("received new POST request!");

    String body = server.arg("plain");
    DEBUG_MSG_HUE(body.c_str());
    Serial.println("");

    if (body.length() == 0){
        char response[strlen_P(HUE_ERROR_TEMPLATE) + server.uri().length() + 40];
        snprintf_P(
            response, sizeof(response),
            HUE_ERROR_TEMPLATE,
            5,
            server.uri().c_str(),
            "invalid/missing parameters in body");        
        server.send(400, "application/json", response);
    }else{
    SimpleJson json;
    json.parse(body);

    int NumberofRipples = json.hasPropery("currentNumberofRipples") ? json["currentNumberofRipples"].getInt() : GlobalParameters.currentNumberofRipples;
    short DelayBetweenRipples = json.hasPropery("currentDelayBetweenRipples") ? json["currentDelayBetweenRipples"].getInt() : GlobalParameters.currentDelayBetweenRipples;
    short RainbowDeltaPerTick = json.hasPropery("currentRainbowDeltaPerTick") ? json["currentRainbowDeltaPerTick"].getInt() : GlobalParameters.currentRainbowDeltaPerTick;
    unsigned long RippleLifeSpan = json.hasPropery("currentRippleLifeSpan") ? json["currentRippleLifeSpan"].getInt() : GlobalParameters.currentRippleLifeSpan;
    float RippleSpeed = json.hasPropery("currentRippleSpeed") ? (float) json["currentRippleSpeed"].getInt() : GlobalParameters.currentRippleSpeed;
    RippleSpeed = RippleSpeed/100; //scaling for proper display on HTML webpage
    int NumberofColors = json.hasPropery("currentNumberofColors") ? json["currentNumberofColors"].getInt() : GlobalParameters.currentNumberofColors;
    int Behavior = json.hasPropery("currentBehavior") ? json["currentBehavior"].getInt() : GlobalParameters.currentBehavior;
    int Direction = json.hasPropery("currentDirection") ? json["currentDirection"].getInt() : GlobalParameters.currentDirection;
    float Decay = json.hasPropery("currentDecay") ? (float) json["currentDecay"].getInt() : GlobalParameters.currentDecay;

    Serial.print("received new NumberofRipples from POST request: ");
    Serial.print(NumberofRipples);
    if(NumberofRipples > 0 && NumberofRipples <= NUMBER_OF_RIPPLES){ /* new value received */
      Serial.print(". New value accepted. Previous value:");
      Serial.println(GlobalParameters.currentNumberofRipples);
      GlobalParameters.currentNumberofRipples = NumberofRipples;
    } else {
      Serial.println(". New NumberofRipples not valid; discarded.");
    }

    Serial.print("received new DelayBetweenRipples from POST request: ");
    Serial.print(DelayBetweenRipples);
    if(DelayBetweenRipples >= 1 && DelayBetweenRipples <= 20000){ /* new value received */
      Serial.print(". New value accepted. Previous value:");
      Serial.println(GlobalParameters.currentDelayBetweenRipples);
      GlobalParameters.currentDelayBetweenRipples = DelayBetweenRipples;
    } else {
      Serial.println(". New DelayBetweenRipples not valid; discarded.");
    }

    Serial.print("received new RainbowDeltaPerTick from POST request: ");
    Serial.print(RainbowDeltaPerTick);
    if(RainbowDeltaPerTick >= 1 && RainbowDeltaPerTick <= 20000){ /* new value received */
      Serial.print(". New value accepted. Previous value:");
      Serial.println(GlobalParameters.currentRainbowDeltaPerTick);
      GlobalParameters.currentRainbowDeltaPerTick = RainbowDeltaPerTick;
    } else {
      Serial.println(". New RainbowDeltaPerTick not valid; discarded.");
    }

    Serial.print("received new RippleLifeSpan from POST request: ");
    Serial.print(RippleLifeSpan);
    if(RippleLifeSpan >= 1 && RippleLifeSpan <= 20000){ /* new value received */
      Serial.print(". New value accepted. Previous value:");
      Serial.println(GlobalParameters.currentRippleLifeSpan);
      GlobalParameters.currentRippleLifeSpan = RippleLifeSpan;
    } else {
      Serial.println(". New RippleLifeSpan not valid; discarded.");
    }

    if(RippleSpeed >= 0.01 && RippleSpeed <= 10){ /* new value received */
      Serial.print("received new Ripple Speed from POST request: ");
      Serial.print(String(RippleSpeed, 2));
      Serial.print(". New value accepted. Previous value:");
      Serial.println(String(GlobalParameters.currentRippleSpeed, 2));
      GlobalParameters.currentRippleSpeed = RippleSpeed;
    } else {
      Serial.println(". New Ripple Speed not valid; discarded.");
    }

    Serial.print("received new NumberofColors from POST request: ");
    Serial.print(NumberofColors);
    if(NumberofColors >= 1 && NumberofColors <= 25){ /* new value received */
      Serial.print(". New value accepted. Previous value:");
      Serial.println(GlobalParameters.currentNumberofColors);
      GlobalParameters.currentNumberofColors = NumberofColors;
    } else {
      Serial.println(". New NumberofColors not valid; discarded.");
    }

    Serial.print("received new Behavior from POST request: ");
    Serial.print(Behavior);
    if(Behavior >= 0 && Behavior <= 4){ /* new value received */
      Serial.print(". New value accepted. Previous value:");
      Serial.println(GlobalParameters.currentBehavior);
      GlobalParameters.currentBehavior = Behavior;
    } else {
      Serial.println(". New Behavior not valid; discarded.");
    }

    Serial.print("received new Direction from POST request: ");
    Serial.print(Direction);
    if(Direction >= -1 && Direction <= 6){ /* new value received */
      Serial.print(". New value accepted. Previous value:");
      Serial.println(GlobalParameters.currentDirection);
      GlobalParameters.currentDirection = Direction;
    } else {
      Serial.println(". New Direction not valid; discarded.");
    }

    Serial.print("received new Decay factor from POST request: ");
    Serial.print(String(Decay, 3));
    if(Decay >= 0.5 && Decay <= 1){ /* new value received */
      Serial.print(". New value accepted. Previous value:");
      Serial.println(String(GlobalParameters.currentDecay, 3));
      GlobalParameters.currentDecay = Decay;
    } else {
      Serial.println(". New Decay not valid; discarded.");
    }
    }

    //server.send(200, "text/html", SendHTML_Dashboard());
    server.send(200, "application/json", "{}");
}

void handle_OnConnect() {
  Serial.println("New client connected!");
  server.send(200, "text/html", SendHTML_Dashboard());
}

void handle_ManualRipple() {
  Serial.println("Received manual ripple request");
  manualFireRipple = 1;
  server.send(200, "text/html", SendHTML_Dashboard());
}

void handle_RestoreProfile_1() {
  boolean EEP_return = 0U;
  currentProfile = 0;
  EEP_return = EEPROM_RestoreProfile(currentProfile);
  if(EEP_return == 1U){ 
    /* profile found */
  } else{

  }
  server.send(200, "text/html", SendHTML_Dashboard());
}

void handle_RestoreProfile_2() {
  boolean EEP_return = 0U;
  currentProfile = 1;
  EEP_return = EEPROM_RestoreProfile(currentProfile);
  if(EEP_return == 1U){ 
    /* profile found */
  } else{

  }
  server.send(200, "text/html", SendHTML_Dashboard());
}

void handle_RestoreProfile_3() {
  boolean EEP_return = 0U;
  currentProfile = 2;
  EEP_return = EEPROM_RestoreProfile(currentProfile);
  if(EEP_return == 1U){ 
    /* profile found */
  } else{

  }
  server.send(200, "text/html", SendHTML_Dashboard());
}

void handle_RestoreProfile_4() {
  boolean EEP_return = 0U;
  currentProfile = 3;
  EEP_return = EEPROM_RestoreProfile(currentProfile);
  if(EEP_return == 1U){ 
    /* profile found */
  } else{

  }
  server.send(200, "text/html", SendHTML_Dashboard());
}

void handle_RestoreProfile_5() {
  boolean EEP_return = 0U;
  currentProfile = 4;
  EEP_return = EEPROM_RestoreProfile(currentProfile);
  if(EEP_return == 1U){ 
    /* profile found */
  } else{

  }
  server.send(200, "text/html", SendHTML_Dashboard());
}

void handle_StoreProfile() {
  EEPROM_StoreProfile(0U);
  server.send(200, "text/html", SendHTML_Dashboard());
}

void handle_DeleteProfile() {
  //EEPROM_RestoreProfile(0U);
  server.send(200, "text/html", SendHTML_Dashboard());
}

void handle_SWreset() {
  ESP.restart();
}

void handle_MasterFireRippleEnabled_On() {
  Serial.println("Master Automatic ripples: ON");
  GlobalParameters.loop_MasterFireRippleEnabled = 1;
}

void handle_MasterFireRippleEnabled_Off() {
  Serial.println("Master Automatic ripples: OFF");
  GlobalParameters.loop_MasterFireRippleEnabled = 0;
}

void handle_CenterFireRippleEnabled_On() {
  Serial.println("Center Automatic ripples: ON");
  GlobalParameters.loop_CenterFireRippleEnabled = 1;
}

void handle_CenterFireRippleEnabled_Off() {
  Serial.println("Center Automatic ripples: OFF");
  GlobalParameters.loop_CenterFireRippleEnabled = 0;
}

void handle_CubeFireRippleEnabled_On() {
  Serial.println("Cube Automatic ripples: ON");
  GlobalParameters.loop_CubeFireRippleEnabled = 1;
}

void handle_CubeFireRippleEnabled_Off() {
  Serial.println("Cube Automatic ripples: OFF");
  GlobalParameters.loop_CubeFireRippleEnabled = 0;
}

void handle_QuadFireRippleEnabled_On() {
  Serial.println("Quad Automatic ripples: ON");
  GlobalParameters.loop_QuadFireRippleEnabled = 1;
}

void handle_QuadFireRippleEnabled_Off() {
  Serial.println("Quad Automatic ripples: OFF");
  GlobalParameters.loop_QuadFireRippleEnabled = 0;
}

void handle_BorderFireRippleEnabled_On() {
  Serial.println("Border Automatic ripples: ON");
  GlobalParameters.loop_BorderFireRippleEnabled = 1;
}

void handle_BorderFireRippleEnabled_Off() {
  Serial.println("Border Automatic ripples: OFF");
  GlobalParameters.loop_BorderFireRippleEnabled = 0;
}

void handle_RandomEffectEnabled_On() {
  Serial.println("Border Automatic ripples: ON");
  GlobalParameters.loop_RandomEffectEnabled = 1;
}

void handle_RandomEffectEnabled_Off() {
  Serial.println("Border Automatic ripples: OFF");
  GlobalParameters.loop_RandomEffectEnabled = 0;
}

/* to be called periodically inside loop() */
void WiFi_MainFunction(void){
  server.handleClient();
}

/* to be called once at startup */
void WiFi_init(void){
  /* Setup WiFi network */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.config(ip, gateway, subnet);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(1000);
    ESP.restart();
  }
  
  /* Setup REST API Handlers */
  server.on("/dashboard", handle_OnConnect);
  server.on("/ManualRipple", handle_ManualRipple);
  server.on("/RestoreProfile_1", handle_RestoreProfile_1);
  server.on("/RestoreProfile_2", handle_RestoreProfile_2);
  server.on("/RestoreProfile_3", handle_RestoreProfile_3);
  server.on("/RestoreProfile_4", handle_RestoreProfile_4);
  server.on("/RestoreProfile_5", handle_RestoreProfile_5);
  server.on("/StoreProfile", handle_StoreProfile);
  server.on("/DeleteProfile", handle_DeleteProfile);
  server.on("/SWreset", handle_SWreset);
  server.on("/MasterFireRippleEnabled/on", handle_MasterFireRippleEnabled_On);
  server.on("/MasterFireRippleEnabled/off", handle_MasterFireRippleEnabled_Off);
  server.on("/CenterFireRippleEnabled/off", handle_CenterFireRippleEnabled_Off);
  server.on("/CenterFireRippleEnabled/on", handle_CenterFireRippleEnabled_On);
  server.on("/CubeFireRippleEnabled/off", handle_CubeFireRippleEnabled_Off);
  server.on("/CubeFireRippleEnabled/on", handle_CubeFireRippleEnabled_On);
  server.on("/QuadFireRippleEnabled/off", handle_QuadFireRippleEnabled_Off);
  server.on("/QuadFireRippleEnabled/on", handle_QuadFireRippleEnabled_On);
  server.on("/BorderFireRippleEnabled/off", handle_BorderFireRippleEnabled_Off);
  server.on("/BorderFireRippleEnabled/on", handle_BorderFireRippleEnabled_On);
  server.on("/RandomEffectEnabled/off", handle_RandomEffectEnabled_Off);
  server.on("/RandomEffectEnabled/on", handle_RandomEffectEnabled_On);
  server.on("/updateInternalVariables", HTTP_POST, handle_PostRequest); 
  
  /* server already begun by hueBrdige */
  //server.begin();
  Serial.printf("Wifi connected, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
  //Serial.println(WiFi.localIP());

  // Setup Multicast DNS https://en.wikipedia.org/wiki/Multicast_DNS 
  // You can open http://hexagono.local in Chrome on a desktop
  Serial.println("Setup MDNS for http://hexagono.local");
  if (!MDNS.begin("hexagono"))
  {
    Serial.println("Error setting up MDNS responder!");
  }
}
