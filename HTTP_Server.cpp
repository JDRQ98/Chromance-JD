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

const IPAddress ip(192, 168, 1, 241);  // IP address that THIS DEVICE should request
const IPAddress gateway(192, 168, 1, 250);  // Your router
const IPAddress subnet(255, 255, 255, 0);  // Your subnet mask (find it from your router's admin panel)

#define server hueBridge.webServer //Open port number 80 (HTTP)

unsigned long currentTime = 0;
unsigned long previousTime = 0;
const long timeout = 2000;

// Auxiliar variables to store the current output state
GlobalParameters_struct GlobalParameters = { 
  .loop_MasterFireRippleEnabled = 1,
  .loop_CenterFireRippleEnabled = 1,
  .loop_CubeFireRippleEnabled = 0,
  .loop_QuadFireRippleEnabled = 0,
  .loop_BorderFireRippleEnabled = 0,
  .loop_RandomEffectEnabled = 0,
  .currentNumberofRipples = HTTP_CURRENTNUMBEROFRIPPLES_DEFAULT,
  .currentNumberofColors = HTTP_CURRENTNUMBEROFCOLORS_DEFAULT,
  .currentBehavior = HTTP_CURRENTBEHAVIOR_DEFAULT,
  .currentDirection = HTTP_CURRENTDIRECTION_DEFAULT,
  .currentDelayBetweenRipples = HTTP_CURRENTDELAYBETWEENRIPPLES_DEFAULT, /* in milliseconds */
  .currentRainbowDeltaPerTick = HTTP_CURRENTRAINBOWDELTAPERTICK_DEFAULT, /* units: hue */
  .currentRippleLifeSpan = HTTP_CURRENTRIPPLELIFESPAN_DEFAULT, /* in milliseconds */
  .currentRippleSpeed = HTTP_CURRENTRIPPLESPEED_DEFAULT, 
  .currentDecay = HTTP_CURRENTDECAY_DEFAULT,  // Multiply all LED's by this amount each tick to create fancy fading tails 0.972 good value for rainbow
};
boolean manualFireRipple = 0;
unsigned int currentSelectedProfile = 0;
unsigned int currentLoadedProfile = -1;

String SendHTML_Dashboard(void) {
// Display the HTML web page
String ptr = "<!DOCTYPE html> <html>\n";
ptr += "<html>\n";
ptr += "<head>\n";
ptr += "  <meta http-equiv='content-type' content='text/html; charset=UTF-8'>\n";
ptr += "  <meta name='viewport' content='width=device-width, initial-scale=1'>\n";
ptr += "  <meta http-equiv='Cache-Control' content='no-cache, no-store, must-revalidate'>\n";
ptr += "  <meta http-equiv='Pragma' content='no-cache'>\n";
ptr += "  <meta http-equiv='Expires' content='0'>\n";
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
ptr += "    .preset {\n";
ptr += "      background-color: #4CAF50;\n";
ptr += "      border: none;\n";
ptr += "      color: white;\n";
ptr += "      padding: 8px 20px;\n";
ptr += "      text-decoration: none;\n";
ptr += "      font-size: 14px;\n";
ptr += "      margin: 2px;\n";
ptr += "      cursor: pointer;\n";
ptr += "    }\n";
ptr += "\n";
ptr += "    .button2 {\n";
ptr += "      background-color: #555555;\n";
ptr += "    }\n";
ptr += "\n";
ptr += "    .Profile {\n";
ptr += "      font-size: 30px;\n";
ptr += "      color: white;\n";
ptr += "      padding: 25px 25px;\n";
ptr += "      margin: 5px;\n";
ptr += "      border: none;\n";
ptr += "      cursor: pointer;\n";
ptr += "    }\n";
ptr += "    .InactiveProfile {\n";
ptr += "      background-color: gray;\n";
ptr += "    }\n";
ptr += "\n";
ptr += "    .ActiveProfile {\n";
ptr += "      background-color: #4CAF50;\n";
ptr += "    }\n";
ptr += "\n";
ptr += "    .SelectedProfile {\n";
ptr += "      padding: 30px 30px;\n";
ptr += "    }\n";
ptr += "\n";
ptr += "    .LoadedProfile {\n";
ptr += "      background-color: green;\n";
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
/****************** BEGINNING OF PROFILE MANAGEMENT ******************/
  ptr += "<section id=\"profileManagement\"\n>";
  ptr += "    <h1>User Profiles</h1>\n";
  ptr += "<p> Current profile selected: " + String(currentSelectedProfile + 1) + "</p>\n";
  if(currentLoadedProfile == -1) ptr += "<p> Last profile loaded: N/A </p\n>";
  else ptr += "<p> Last profile loaded: " + String(currentLoadedProfile + 1) + "</p>\n";
  ptr += "<div\n>";
  ptr += "<div id=\"profiles\"\n>";
  /* button 1*/
  ptr += "<button onclick='selectProfile(0)' class='Profile ";
  if (ProfilesAvailable[0] == 1U) ptr += "ActiveProfile ";
  else ptr += "InactiveProfile ";
  if (currentLoadedProfile == 0) ptr += "LoadedProfile ";
  if (currentSelectedProfile == 0) ptr += "SelectedProfile ";
  ptr += "'>#1</button></a>\n";
  /* button 2*/
  ptr += "<button onclick='selectProfile(1)' class='Profile ";
  if (ProfilesAvailable[1] == 1U) ptr += "ActiveProfile ";
  else ptr += "InactiveProfile ";
  if (currentLoadedProfile == 1) ptr += "LoadedProfile ";
  if (currentSelectedProfile == 1) ptr += "SelectedProfile ";
  ptr += "'>#2</button></a>\n";
  /* button 3*/
  ptr += "<button onclick='selectProfile(2)' class='Profile ";
  if (ProfilesAvailable[2] == 1U) ptr += "ActiveProfile ";
  else ptr += "InactiveProfile ";
  if (currentLoadedProfile == 2) ptr += "LoadedProfile ";
  if (currentSelectedProfile == 2) ptr += "SelectedProfile ";
  ptr += "'>#3</button></a>\n";
  /* button 4*/
  ptr += "<button onclick='selectProfile(3)' class='Profile ";
  if (ProfilesAvailable[3] == 1U) ptr += "ActiveProfile ";
  else ptr += "InactiveProfile ";
  if (currentLoadedProfile == 3) ptr += "LoadedProfile ";
  if (currentSelectedProfile == 3) ptr += "SelectedProfile ";
  ptr += "'>#4</button></a>\n";
  /* button 5*/
  ptr += "<button onclick='selectProfile(4)' class='Profile ";
  if (ProfilesAvailable[4] == 1U) ptr += "ActiveProfile ";
  else ptr += "InactiveProfile ";
  if (currentLoadedProfile == 4) ptr += "LoadedProfile ";
  if (currentSelectedProfile == 4) ptr += "SelectedProfile ";
  ptr += "'>#5</button></a>\n";
  ptr += "</div\n>";
  ptr += "      <div>\n";
  ptr += "          <button class='button' onclick = 'saveCurrentSelectedProfile()'> Save </button>\n";
  ptr += "          <button class='button' onclick = 'loadCurrentSelectedProfile()'> Load </button>\n";
  ptr += "          <button class='button' style='background-color: red;' onclick = 'deleteCurrentSelectedProfile()'> Delete </button>\n";
  ptr += "      </div>\n";
  ptr += "</div\n>";
    ptr += "    <h1> Presets</ h1>\n ";
  ptr += "      <div>\n";
  ptr += "        <button onclick='selectPreset(1)' class='preset'>Restore default settings</button>\n";
  ptr += "        <button onclick='selectPreset(2)' class='preset'>TBD</button>\n";
  ptr += "        <button onclick='selectPreset(3)' class='preset'>TBD</button>\n";
  ptr += "        <button onclick='selectPreset(0)' class='preset'>TBD</button>\n";
  ptr += "        <button onclick='selectPreset(0)' class='preset'>TBD</button>\n";
  ptr += "      </div>\n";
  ptr += "</section\n>";
  /****************** END OF PROFILE MANAGEMENT ******************/
  /****************** BEGINNING USER INPUT ******************/
  ptr += "  <section id='userinput'>\n";
ptr += "    <form action='/updateInternalVariables' method='post'>\n";
ptr += "      <div class='rangeslider'> \n";
ptr += "        <label for='NumberofRipples'>Enter the number of ripples [" + String(HTTP_CURRENTNUMBEROFRIPPLES_MIN) + " - " + String(HTTP_CURRENTNUMBEROFRIPPLES_MAX) + "]:</label>\n";
ptr += "        <input type='range' id='currentNumberofRipples' name='NumberofRipples' min='" + String(HTTP_CURRENTNUMBEROFRIPPLES_MIN) + "' max='" + String(HTTP_CURRENTNUMBEROFRIPPLES_MAX) + "' value='" + String(GlobalParameters.currentNumberofRipples) + "'>\n";
ptr += "        <span id='currentNumberofRipples_display'></span>\n";
ptr += "      </div>\n";
ptr += "      <div class='rangeslider'> \n";
ptr += "        <label for='currentDelayBetweenRipples'>Enter delay between ripples in ms [" + String(HTTP_CURRENTDELAYBETWEENRIPPLES_MIN) + " - " + String(HTTP_CURRENTDELAYBETWEENRIPPLES_MAX) + "]:</label>\n";
ptr += "        <input type='range' id='currentDelayBetweenRipples' name='currentDelayBetweenRipples' min='" + String(HTTP_CURRENTDELAYBETWEENRIPPLES_MIN) + "' max='" + String(HTTP_CURRENTDELAYBETWEENRIPPLES_MAX) + "' value='" + String(GlobalParameters.currentDelayBetweenRipples) + "'>\n";
ptr += "        <span id='currentDelayBetweenRipples_display'></span>\n";
ptr += "      </div>\n";
ptr += "      <div class='rangeslider'> \n";
ptr += "        <label for='currentRainbowDeltaPerTick'>Enter rainbow delta per tick in hue [" + String(HTTP_CURRENTRAINBOWDELTAPERTICK_MIN) + " - " + String(HTTP_CURRENTRAINBOWDELTAPERTICK_MAX) + "]:</label>\n";
ptr += "        <input type='range' id='currentRainbowDeltaPerTick' name='currentRainbowDeltaPerTick' min='" + String(HTTP_CURRENTRAINBOWDELTAPERTICK_MIN) + "' max='" + String(HTTP_CURRENTRAINBOWDELTAPERTICK_MAX) + "' value='" + String(GlobalParameters.currentRainbowDeltaPerTick) + "'>\n";
ptr += "        <span id='currentRainbowDeltaPerTick_display'></span>\n";
ptr += "      </div>\n";
ptr += "      <div class='rangeslider'> \n";
ptr += "        <label for='currentRippleLifeSpan'>Enter ripple life span in ms [" + String(HTTP_CURRENTRIPPLELIFESPAN_MIN) + " - " + String(HTTP_CURRENTRIPPLELIFESPAN_MAX) + "]:</label>\n";
ptr += "        <input type='range' id='currentRippleLifeSpan' name='currentRippleLifeSpan' min='" + String(HTTP_CURRENTRIPPLELIFESPAN_MIN) + "' max='" + String(HTTP_CURRENTRIPPLELIFESPAN_MAX) + "' value='" + String(GlobalParameters.currentRippleLifeSpan) + "'>\n";
ptr += "        <span id='currentRippleLifeSpan_display'></span>\n";
ptr += "      </div>\n";
ptr += "      <div class='rangeslider'> \n";
ptr += "        <label for='currentRippleSpeed'>Enter ripple speed (float) [" + String(HTTP_CURRENTRIPPLESPEED_MIN) + " - " + String(HTTP_CURRENTRIPPLESPEED_MAX) + "]:</label>\n";
ptr += "        <input type='range' id='currentRippleSpeed' name='currentRippleSpeed' min='" + String(HTTP_CURRENTRIPPLESPEED_MIN*100) + "' max='" + String(HTTP_CURRENTRIPPLESPEED_MAX*100) + "' value='" + String(GlobalParameters.currentRippleSpeed*100) + "'>\n";
ptr += "        <span id='currentRippleSpeed_display'></span>\n";
ptr += "      </div>\n";
ptr += "      <div class='rangeslider'> \n";
ptr += "        <label for='currentNumberofColors'>Enter the desired number of colors [" + String(HTTP_CURRENTNUMBEROFCOLORS_MIN) + " - " + String(HTTP_CURRENTNUMBEROFCOLORS_MAX) + "]:</label>\n";
ptr += "        <input type='range' id='currentNumberofColors' name='currentNumberofColors' min='" + String(HTTP_CURRENTNUMBEROFCOLORS_MIN) + "' max='" + String(HTTP_CURRENTNUMBEROFCOLORS_MAX) + "' value='" + String(GlobalParameters.currentNumberofColors) + "'>\n";
ptr += "        <span id='currentNumberofColors_display'></span>\n";
ptr += "      </div>\n";
ptr += "      <div>\n";
ptr += "        <label for='currentDecay'>Enter decay per tick [" + String(HTTP_CURRENTDECAY_MIN) + " - " + String(HTTP_CURRENTDECAY_MAX) + "]:</label>\n";
ptr += "        <input type='range' id='currentDecay' name='currentDecay' min='" + String(HTTP_CURRENTDECAY_MIN*1000) + "' max='" + String(HTTP_CURRENTDECAY_MAX*1000) + "' value='" + String(GlobalParameters.currentDecay*1000) + "'>\n";
ptr += "        <span id='currentDecay_display'></span>\n";
ptr += "      </div>\n";
ptr += "      <div>\n";
ptr += "        <label for='currentBehavior'>Enter the desired behavior:</label>\n";
ptr += "        <select id='currentBehavior' name='currentBehavior'>\n";
if(GlobalParameters.currentBehavior == 0)
  ptr += "          <option value='0' selected='selected'>Mild</option>\n";
else{
  ptr += "          <option value='0'>Mild</option>\n"; 
}
if(GlobalParameters.currentBehavior == 1)
  ptr += "          <option value='1' selected='selected'>Normal</option>\n";
else{
  ptr += "          <option value='1'>Normal</option>\n"; 
}
if(GlobalParameters.currentBehavior == 2)
  ptr += "          <option value='2' selected='selected'>Agressive</option>\n";
else{
  ptr += "          <option value='2'>Agressive</option>\n"; 
}
if(GlobalParameters.currentBehavior == 3)
  ptr += "          <option value='3' selected='selected'>Always turns right</option>\n";
else{
  ptr += "          <option value='3'>Always turns right</option>\n"; 
}
if(GlobalParameters.currentBehavior == 4)
  ptr += "          <option value='4' selected='selected'>Always turns left</option>\n";
else{
  ptr += "          <option value='4'>Always turns left</option>\n"; 
}
ptr += "        </select>\n";
ptr += "      </div>\n";
ptr += "      <div>\n";
ptr += "        <label for='currentDirection'>Enter ripple direction:</label>\n";
ptr += "        <select id='currentDirection' name='currentDirection'>\n";
if(GlobalParameters.currentDirection == -1)
  ptr += "          <option value='-1' selected='selected'>All directions</option>\n";
else{
  ptr += "          <option value='-1'>All directions</option>\n"; 
}
if(GlobalParameters.currentDirection == 0)
  ptr += "          <option value='0' selected='selected'>0°</option>\n";
else{
  ptr += "          <option value='0'>0°</option>\n"; 
}
if(GlobalParameters.currentDirection == 1)
  ptr += "          <option value='1' selected='selected'>-60°</option>\n";
else{
  ptr += "          <option value='1'>60°</option>\n"; 
}
if(GlobalParameters.currentDirection == 2)
  ptr += "          <option value='2' selected='selected'>-120°</option>\n";
else{
  ptr += "          <option value='2'>120°</option>\n"; 
}
if(GlobalParameters.currentDirection == 3)
  ptr += "          <option value='3' selected='selected'>180°</option>\n";
else{
  ptr += "          <option value='3'>180°</option>\n"; 
}
if(GlobalParameters.currentDirection == 4)
  ptr += "          <option value='4' selected='selected'>120°</option>\n";
else{
  ptr += "          <option value='4'>-120°</option>\n"; 
}
if(GlobalParameters.currentDirection == 5)
  ptr += "          <option value='5' selected='selected'>60°</option>\n";
else{
  ptr += "          <option value='5'>-60°</option>\n"; 
}
/* random direction not supported yet, TBD.
if(GlobalParameters.currentDirection == 6)
  ptr += "          <option value='6' selected='selected'>Random direction</option>\n";
else{
  ptr += "          <option value='6'>Random direction</option>\n"; 
}
*/
ptr += "        </select>\n";
ptr += "      </div>      \n";
ptr += "      <div>\n";
ptr += "      </div>\n";
ptr += "        <p><button class=\"button\" type='button' onclick='sendData()'>Submit</button></p>\n";
ptr += "    </form>\n";
ptr += "  </section>\n";
ptr += "  <p>Fire Manual Ripple</p>\n";
ptr += "  <p><a href='http://192.168.1.241/ManualRipple'><button class='button'>Fire!</button></a></p>\n";
/****************** END OF USER INPUT ******************/
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
  ptr += "  function selectProfile(profile) {\n";
  ptr += "    // Include the logic to send the data to the server\n";
  ptr += "    data = {};\n";
  ptr += "    if (profile >= 0) { data.selectProfile = profile; }\n";
  ptr += "\n";
  ptr += "    var xhr = new XMLHttpRequest();\n";
  ptr += "    xhr.open('POST', '/profileManagement', true);\n";
  ptr += "    xhr.setRequestHeader('Content-Type', 'application/json');\n";
  ptr += "    xhr.send(JSON.stringify(data));\n";
  ptr += "    setTimeout(()=> {window.location.href='http://192.168.1.241/dashboard';window.location.reload(true);} ,250);\n";
  ptr += "  }\n";
  ptr += "\n";
    ptr += "  function deleteCurrentSelectedProfile() {\n";
  ptr += "    // Include the logic to send the data to the server\n";
  ptr += "    data = {};\n";
  ptr += "    data.deleteProfile = 1;\n";
  ptr += "\n";
  ptr += "    var xhr = new XMLHttpRequest();\n";
  ptr += "    xhr.open('POST', '/profileManagement', true);\n";
  ptr += "    xhr.setRequestHeader('Content-Type', 'application/json');\n";
  ptr += "    xhr.send(JSON.stringify(data));\n";
  ptr += "    setTimeout(()=> {window.location.href='http://192.168.1.241/dashboard';window.location.reload(true);} ,250);\n";
  ptr += "  }\n";
  ptr += "\n";
    ptr += "\n";
    ptr += "  function saveCurrentSelectedProfile() {\n";
  ptr += "    // Include the logic to send the data to the server\n";
  ptr += "    data = {};\n";
  ptr += "    data.saveProfile = 1;\n";
  ptr += "\n";
  ptr += "    var xhr = new XMLHttpRequest();\n";
  ptr += "    xhr.open('POST', '/profileManagement', true);\n";
  ptr += "    xhr.setRequestHeader('Content-Type', 'application/json');\n";
  ptr += "    xhr.send(JSON.stringify(data));\n";
  ptr += "    setTimeout(()=> {window.location.href='http://192.168.1.241/dashboard';window.location.reload(true);} ,250);\n";
  ptr += "  }\n";
  ptr += "\n";
    ptr += "\n";
    ptr += "  function loadCurrentSelectedProfile() {\n";
  ptr += "    // Include the logic to send the data to the server\n";
  ptr += "    data = {};\n";
  ptr += "    data.loadProfile = 1;\n";
  ptr += "\n";
  ptr += "    var xhr = new XMLHttpRequest();\n";
  ptr += "    xhr.open('POST', '/profileManagement', true);\n";
  ptr += "    xhr.setRequestHeader('Content-Type', 'application/json');\n";
  ptr += "    xhr.send(JSON.stringify(data));\n";
  ptr += "    setTimeout(()=> {window.location.href='http://192.168.1.241/dashboard';window.location.reload(true);} ,250);\n";
  ptr += "  }\n";
  ptr += "\n";
    ptr += "  function selectPreset(preset) {\n";
  ptr += "    // Include the logic to send the data to the server\n";
  ptr += "    data = {};\n";
  ptr += "    if (preset >= 0) { data.selectPreset = preset; }\n";
  ptr += "\n";
  ptr += "    var xhr = new XMLHttpRequest();\n";
  ptr += "    xhr.open('POST', '/profileManagement', true);\n";
  ptr += "    xhr.setRequestHeader('Content-Type', 'application/json');\n";
  ptr += "    xhr.send(JSON.stringify(data));\n";
  // ptr += "    window.location.href='http://192.168.1.241/dashboard';window.location.reload(true);\n";
  ptr += "    setTimeout(()=> {window.location.href='http://192.168.1.241/dashboard';window.location.reload(true);} ,250);\n";
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
    Decay = Decay/1000; //scaling for proper display on HTML webpage

    Serial.print("received new NumberofRipples from POST request: ");
    Serial.print(NumberofRipples);
    if(NumberofRipples >= HTTP_CURRENTNUMBEROFRIPPLES_MIN && NumberofRipples <= HTTP_CURRENTNUMBEROFRIPPLES_MAX){ /* new value received */
      Serial.print(". New value accepted. Previous value:");
      Serial.println(GlobalParameters.currentNumberofRipples);
      GlobalParameters.currentNumberofRipples = NumberofRipples;
    } else {
      Serial.println(". New NumberofRipples not valid; discarded.");
    }

    Serial.print("received new DelayBetweenRipples from POST request: ");
    Serial.print(DelayBetweenRipples);
    if(DelayBetweenRipples >= HTTP_CURRENTDELAYBETWEENRIPPLES_MIN && DelayBetweenRipples <= HTTP_CURRENTDELAYBETWEENRIPPLES_MAX){ /* new value received */
      Serial.print(". New value accepted. Previous value:");
      Serial.println(GlobalParameters.currentDelayBetweenRipples);
      GlobalParameters.currentDelayBetweenRipples = DelayBetweenRipples;
    } else {
      Serial.println(". New DelayBetweenRipples not valid; discarded.");
    }

    Serial.print("received new RainbowDeltaPerTick from POST request: ");
    Serial.print(RainbowDeltaPerTick);
    if(RainbowDeltaPerTick >= HTTP_CURRENTRAINBOWDELTAPERTICK_MIN && RainbowDeltaPerTick <= HTTP_CURRENTRAINBOWDELTAPERTICK_MAX){ /* new value received */
      Serial.print(". New value accepted. Previous value:");
      Serial.println(GlobalParameters.currentRainbowDeltaPerTick);
      GlobalParameters.currentRainbowDeltaPerTick = RainbowDeltaPerTick;
    } else {
      Serial.println(". New RainbowDeltaPerTick not valid; discarded.");
    }

    Serial.print("received new RippleLifeSpan from POST request: ");
    Serial.print(RippleLifeSpan);
    if(RippleLifeSpan >= HTTP_CURRENTRIPPLELIFESPAN_MIN && RippleLifeSpan <= HTTP_CURRENTRIPPLELIFESPAN_MAX){ /* new value received */
      Serial.print(". New value accepted. Previous value:");
      Serial.println(GlobalParameters.currentRippleLifeSpan);
      GlobalParameters.currentRippleLifeSpan = RippleLifeSpan;
    } else {
      Serial.println(". New RippleLifeSpan not valid; discarded.");
    }

    Serial.print("received new Ripple Speed from POST request: ");
    Serial.print(String(RippleSpeed, 2));
    if(RippleSpeed >= HTTP_CURRENTRIPPLESPEED_MIN && RippleSpeed <= HTTP_CURRENTRIPPLESPEED_MAX){ /* new value received */
      Serial.print(". New value accepted. Previous value:");
      Serial.println(String(GlobalParameters.currentRippleSpeed, 2));
      GlobalParameters.currentRippleSpeed = RippleSpeed;
    } else {
      Serial.println(". New Ripple Speed not valid; discarded.");
    }

    Serial.print("received new NumberofColors from POST request: ");
    Serial.print(NumberofColors);
    if(NumberofColors >= HTTP_CURRENTNUMBEROFCOLORS_MIN && NumberofColors <= HTTP_CURRENTNUMBEROFCOLORS_MAX){ /* new value received */
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
    if(Decay >= HTTP_CURRENTDECAY_MIN && Decay <= HTTP_CURRENTDECAY_MAX){ /* new value received */
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

void handle_profileManagement() {
  Serial.println("received POST request for profile management");
  String body = server.arg("plain");
  DEBUG_MSG_HUE(body.c_str());

  /* local variables*/
  presetType preset = no_preset;
  bool deleteProfileRequest = 0U;
  bool saveProfileRequest = 0U;
  bool loadProfileRequest = 0U;
  bool loadProfileRequest_return = 0U;

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
    /* USER PROFILE MANAGEMENT */
    if(json.hasPropery("selectProfile")){
      if( (json["selectProfile"].getInt()) >= 0 && (json["selectProfile"].getInt() < EEPROM_SUPPORTED_PROFILES) ){
       currentSelectedProfile = json["selectProfile"].getInt();
        Serial.print("new profile selected: ");
        Serial.println(currentSelectedProfile);
      }else{
        Serial.print("received new INVALID profile: ");
        Serial.print(json["selectProfile"].getInt());
        Serial.print(". Discarding - current profile is still ");
        Serial.println(currentSelectedProfile);
      }
    }
    
    if(json.hasPropery("deleteProfile")){
      deleteProfileRequest = (bool) json["deleteProfile"].getInt();
      if(deleteProfileRequest == 1U){
        Serial.println("received request to delete current profile");
        EEPROM_InvalidateProfile(currentSelectedProfile); 
        if(currentLoadedProfile == currentSelectedProfile) currentLoadedProfile = -1; /*loaded profile was the one we just deleted! */
      }
    }

    if(json.hasPropery("loadProfile")){
      loadProfileRequest = (bool) json["loadProfile"].getInt();
      if(loadProfileRequest == 1U){
        Serial.println("received request to load current profile");
        loadProfileRequest_return = EEPROM_RestoreProfile(currentSelectedProfile);
        if(loadProfileRequest_return) currentLoadedProfile = currentSelectedProfile;
      }
    }

    if(json.hasPropery("saveProfile")){
      saveProfileRequest = (bool) json["saveProfile"].getInt();
      if(saveProfileRequest == 1U){
        Serial.println("received request to save current profile");
        EEPROM_StoreProfile(currentSelectedProfile);
        currentLoadedProfile = currentSelectedProfile;
      }
    }

    /* PRESET MANAGEMENT */
    if(json.hasPropery("selectPreset")){
      preset = (presetType) json["selectPreset"].getInt();
        Serial.print("new preset selected: ");
        Serial.println(preset);

      switch(preset){
      case default_preset:
        Serial.println("received request for preset Rainbow Trails");
        GlobalParameters.currentNumberofRipples = HTTP_CURRENTNUMBEROFRIPPLES_DEFAULT;
        GlobalParameters.currentNumberofColors = HTTP_CURRENTNUMBEROFCOLORS_DEFAULT;
        GlobalParameters.currentBehavior = HTTP_CURRENTBEHAVIOR_DEFAULT;
        GlobalParameters.currentDirection = HTTP_CURRENTDIRECTION_DEFAULT;
        GlobalParameters.currentDelayBetweenRipples = HTTP_CURRENTDELAYBETWEENRIPPLES_DEFAULT; /* in milliseconds */
        GlobalParameters.currentRainbowDeltaPerTick = HTTP_CURRENTRAINBOWDELTAPERTICK_DEFAULT; /* units: hue */
        GlobalParameters.currentRippleLifeSpan = HTTP_CURRENTRIPPLELIFESPAN_DEFAULT;           /* in milliseconds */
        GlobalParameters.currentRippleSpeed = HTTP_CURRENTRIPPLESPEED_DEFAULT;
        GlobalParameters.currentDecay = HTTP_CURRENTDECAY_DEFAULT;
        break;
      case RainbowTrails:
        Serial.println("received request for preset Rainbow Trails");
        break;
      case LongTrails:
        Serial.println("received request for  preset Long Trails");
        break;
      }
    }
  }
  server.send(200, "text/html", SendHTML_Dashboard());
}


void handle_SWreset() {
  ESP.restart();
}

/* checkbox handling */
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
/* end of checkbox handling*/

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
  server.on("/profileManagement", handle_profileManagement);
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
