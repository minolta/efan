#include <Arduino.h>
#include <unity.h>
#include "Configfile.h"
#include <LITTLEFS.h>
#include <HTTPClient.h>
#include <Update.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
Configfile testconfig("/testconfig");
Configfile cfg("/config.cfg");
AsyncWebServer server(80);
const char configfile_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>Config</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 6px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 3px}
    input:checked+.slider {background-color: #b30000}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}

#customers {
    /* font-family: 'Karla', Tahoma, Varela, Arial, Helvetica, sans-serif; */
    border-collapse: collapse;
    width: 100%%;
    /* font-size: 12px; */
}
#btn {
  border: 1px solid #777;
  background: #6e9e2d;
  color: #fff;
  font: bold 11px 'Trebuchet MS';
  padding: 4px;
  cursor: pointer;
  -moz-border-radius: 4px;
  -webkit-border-radius: 4px;
}
.button {
  background-color: #4CAF50; /* Green */
  border: none;
  color: white;
  padding: 15px 32px;
  text-align: center;
  text-decoration: none;
  display: inline-block;
  font-size: 16px;
}
#customers td,
#customers th {
    border: 1px solid #ddd;
    padding: 8px;
}


/* #customers tr:nth-child(even){background-color: #f2f2f2;} */

#customers tr:hover {
    background-color: #ddd;
}

#customers th {
    padding-top: 12px;
    padding-bottom: 12px;
    text-align: left;
    background-color: #4CAF50;
    color: white;
}
</style>

<script>
function deleteallconfig()
{
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/resetconfig", true); 
    xhr.send();
}
function remove(config)
{
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/removeconfig?configname="+config, true); 

     xhr.addEventListener("readystatechange", () => {
     console.log(xhr.readystate);
    if (xhr.readyState === 4 && xhr.status === 200) {
     console.log(xhr.responseText);
     location.reload();
     }
 });
    xhr.send();
}
function add()
{
  var xhr = new XMLHttpRequest();
  var input = document.getElementById('newconfigname');
  var value = document.getElementById('newvalue');
  xhr.open("GET", "/addconfig?configname="+input.value+"&value="+value.value, true); 
  xhr.addEventListener("readystatechange", () => {
     console.log(xhr.readystate);
    if (xhr.readyState === 4 && xhr.status === 200) {
     console.log(xhr.responseText);
     var o =  JSON.parse(xhr.responseText);
     var t = document.getElementById('customers');
     var row = t.insertRow();
     row.innerHTML = "<td>"+o.setconfig+"</td><td>"+o.value+"</td><td><input value="+o.value+"></td>";
     }
 });
  xhr.send();
}
function setvalue(element,configname,value) {
  console.log("Call",element);
  var xhr = new XMLHttpRequest();
  var input = document.getElementById(configname);

  xhr.open("GET", "/addconfig?configname="+configname+"&value="+input.value, true); 
  xhr.addEventListener("readystatechange", () => {
     console.log(xhr.readystate);
    if (xhr.readyState === 4 && xhr.status === 200) {
     console.log(xhr.responseText);
    var o =  JSON.parse(xhr.responseText);
  var showvalue = document.getElementById(configname+'value');  
  console.log('Showvalue',showvalue);
  console.log('O',o);
  showvalue.innerHTML = o.value
    } else if (xhr.readyState === 4) {
     console.log("could not fetch the data");
     }
        });
  xhr.send();
}

setInterval(()=>{
  
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/info", true); 
  xhr.addEventListener("readystatechange", () => {
    if (xhr.readyState === 4 && xhr.status === 200) {
    console.log(xhr.responseText);
    var o =  JSON.parse(xhr.responseText);
    console.log('O',o);
    var tmp1 = document.getElementById("tmp1"); 
    tmp1.innerHTML = o.t 
    var tmp2 = document.getElementById("tmp2"); 
    tmp2.innerHTML = o.t1 
    var duty = document.getElementById("duty"); 
    duty.innerHTML = o.duty 
    var runmodename = document.getElementById("runmodename"); 
    runmodename.innerHTML = o.runmodename
    var uptime = document.getElementById("uptime"); 
    uptime.innerHTML = o.uptime 
    } else if (xhr.readyState === 4) {
     console.log("could not fetch the data");
     }
    });
  xhr.send();
  console.log('Call refresh');
}
, 500); // 3000 milliseconds = 3 seconds
</script>
  </head><body>
 <table id="customers">
  <tr>
  <td>Config</td><td>value</td><td>Set</td><td>#</td><td>x</td>
  </tr>
  %CONFIG%
 </table>
<hr>
New Config <input id=newconfigname> <input id=newvalue> <button  id=btn onClick="add()">add </button>
<hr>
<button id=btn onClick="deleteallconfig()">Reset Config</button>
<table id="customers">
 <tr>
  <td>uptime</td><td><label id="uptime">0</label></td>
  </tr>
  <tr>
  <td>TMP1</td><td><label id="tmp1">0</label></td>
  </tr>
  <tr>
  <td>TMP2</td><td><label id="tmp2">0</label></td>
  </tr>
    <tr>
  <td>DUTY</td><td><label id="duty">0</label></td>
  </tr>
     <tr>
  <td>Mode</td><td><label id="runmodename">0</label></td>
  </tr>
 </table>
</body></html>)rawliteral";
void setUp(void)
{
  // set stuff up here
}

void tearDown(void)
{
  // clean stuff up here
}
void addCfg()
{
  testconfig.addConfig("0-40", "25");
  testconfig.addConfig("41-80", "80");
  testconfig.addConfig("81", "100");
}
void removeconfig(AsyncWebServerRequest *request)
{
  String v = request->arg("configname");
  cfg.remove(v);
  request->send(200, "application/json", "{\"remove\":\"" + v + "\"}");
}
String fillconfig(const String &var)
{
  // Serial.println(var);
  if (var == "CONFIG")
  {
    DynamicJsonDocument dy = cfg.getAll();
    JsonObject documentRoot = dy.as<JsonObject>();
    String tr = "";
    for (JsonPair keyValue : documentRoot)
    {
      String v = dy[keyValue.key()];
      String k = keyValue.key().c_str();
      tr += "<tr><td>" + k + "</td><td> <label id=" + k + "value>" + v + "</label> </td> <td> <input id = " + k + " value =\"" + v + "\"></td><td><button id=btn onClick=\"setvalue(this,'" + k + "','" + v + "')\">Set</button></td><td><button id=btn onClick=\"remove('" + k + "')\">Remove</button></td></tr>";
    }
    tr += "<tr><td>heap</td><td colspan=4>" + String(ESP.getFreeHeap()) + "</td></tr>";

    return tr;
  }
  return String();
}
void setwwwconfig(AsyncWebServerRequest *request)
{
  request->send_P(200, "text/html", configfile_html, fillconfig);
}
void info(AsyncWebServerRequest *request)
{
  DynamicJsonDocument doc(2000);
  doc["uptime"] = 1222;
  doc["torun"] = 20;
  doc["toduty"] = 3232;
  doc["duty"] = 32;
  doc["tmpduty"] =223;
  doc["t"] = 23.2;
  doc["t1"] = 24.3;
  doc["runmode"] = 1;
  doc["currentadc"] = 10;
  doc["runmodename"] = "TMP";
  char buf[2000];
  serializeJsonPretty(doc, buf, 2000);
  request->send(200, "application/json", buf);
}
void resettodefault(AsyncWebServerRequest *request)
{
  LITTLEFS.remove("/config.cfg");
  request->send(200, "application/json", "{\"resettodefault\":\"ok\"}");
  delay(1000);
  ESP.restart();
}
void addconfig(AsyncWebServerRequest *request)
{
  // delay(1000);
  String configname = request->arg("configname");
  String configvalue = request->arg("value");
  Serial.printf("Config name %s set to %s", configname, configvalue);
  cfg.addConfig(configname, configvalue);
  request->send(200, "application/json", "{\"addconfig\":\"ok\",\"setconfig\":\"" + configname + "\",\"value\":\"" + configvalue + "\"}");
}
void setHttp()
{
  server.on("/addconfig", HTTP_GET, addconfig);
  // server.on("/ota", HTTP_GET, updatedirect);
  // server.on("/on", HTTP_GET, on);
  server.on("/setconfig", HTTP_GET, setwwwconfig);
  server.on("/setconfigwww", HTTP_GET, setwwwconfig);
  // server.on("/checkin", HTTP_GET, checkinNow);
  // server.on("/off", HTTP_GET, poweroff);
  // server.on("/setvalue", HTTP_GET, setvalue);
  // server.on("/setwifi", HTTP_GET, setwifi);
  server.on("/info", HTTP_GET, info);
  // // server1.on("/scanwifiii", HTTP_GET, scanwifiII);
  // server.on("/get", HTTP_GET, get);
  // server.on("/restart", HTTP_GET, restart);
  server.on("/removeconfig", HTTP_GET, removeconfig);
  server.on("/resettodefault", HTTP_GET, resettodefault);

  // // server1.on("/runtime",HTTP_GET,runtime);
  server.begin();
}
void testAPMODE()
{
  const char *ssid = "TEST-ESP32-Access-Point";
  const char *password = "123456789";
  WiFi.softAP(ssid, password);
  cfg.openFile();
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  setHttp();
}
void testgetvalue()
{
  Configfile c("/a");
  c.openFile();
  c.addConfig("0-20", 10);
  c.addConfig("21-40", "40");
  c.addConfig("41-80", "60");
  c.addConfig("81-120", "100");
  int r1 = c.getIntConfig("0-20");
  int r2 = c.getIntConfig("21-40");
  int r3 = c.getIntConfig("41-80");
  int r4 = c.getIntConfig("81-120");
  float t = 30;
  int p = 0;
  if (t >= r1 && t < r2)
  {
    Serial.println(r1);
    p = r1;
  }
  else if (t >= r2 && t < r3)
  {
    Serial.println(r2);
    p = r2;
  }
  else if (t >= r3 && t < r4)
  {
    Serial.println(r3);
    p = r3;
  }
  else
    p = r4;

  TEST_ASSERT_EQUAL_INT(10, p);
}

void testDS18b20()
{
  const int oneWireBus = 32;
  // Setup a oneWire instance to communicate with any OneWire devices
  OneWire oneWire(oneWireBus);
  // Pass our oneWire reference to Dallas Temperature sensor
  DallasTemperature sensors(&oneWire);
  sensors.begin();
  sensors.requestTemperatures();
  int dc = sensors.getDeviceCount();
  float temperatureC = sensors.getTempCByIndex(0);
  float temperatureF = sensors.getTempFByIndex(0);
  Serial.printf(" Found %d %.2f %.2f\n", dc, temperatureC, temperatureF);
  float tc = sensors.getTempCByIndex(1);
  float tf = sensors.getTempFByIndex(1);
  Serial.printf(" Sensor 2 %.2f  %.2f\n", tc, tf);
}

void setup()
{
  pinMode(2, OUTPUT);
  Serial.begin(115200);
  delay(2000);
  UNITY_BEGIN();
  // RUN_TEST(testgetvalue);
  // RUN_TEST(testDS18b20);
  RUN_TEST(testAPMODE);
  UNITY_END();
}

void loop()
{
}