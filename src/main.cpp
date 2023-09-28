#include <Arduino.h>
#include <driver/adc.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include "ESp32OTA.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Ticker.h>
#include <Update.h>
#include <LITTLEFS.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "Configfile.h"
#include "Apmode.h"
void tickerfunction();
void setconfig();
void loadconfigtoram();
void connect();
void setupPort();
void setHttp();
String makestatus();
// put function declarations here:
AsyncWebServer server(80);
Ticker ticker;
Configfile cfg("/config.cfg");
ApMode ap("/config.cfg");
const int oneWireBus = 32;
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);
// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);

const String version = "1";
int apmode = 0;
long uptime = 0;
int jsonsize = 2048;
int duty = 0;
int toduty = 0; // เป็นตัวกำหนดให้ duty เท่ากับอันนี้
int torun = 0;
float t = 0;
String message;
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

</body></html>)rawliteral";
struct
{
  int apmode = 0; // บอกให้ run ap mode
  int apmodelimit = 30;
  int torundelay = 10;
  int haveds = 0;
  int r040 = 20;
  int r4160 = 40;
  int r6180 = 60;
  int r81100 = 100;
} configdata;
void setup()
{
  setupPort();
  Serial.begin(115200);
  ticker.attach(0.5, tickerfunction);
  setconfig();
  connect();
  setHttp();
  if (configdata.haveds)
    sensors.begin();
}

void setupPort()
{
  pinMode(2, OUTPUT);
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11); // สำหรับอ่าน แรงดัน batt
  adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_DB_11); // สำหรับ pressure sensor
  ledcSetup(0, 500, 10);
  ledcSetup(1, 500, 10);
  ledcAttachPin(17, 0);
  ledcAttachPin(GPIO_NUM_33, 1);
  ledcWrite(1, 0);
  ledcWrite(0, 0);
  pinMode(GPIO_NUM_25, OUTPUT);
  pinMode(GPIO_NUM_26, OUTPUT);
  pinMode(GPIO_NUM_2, OUTPUT);
}
void status(AsyncWebServerRequest *request)
{
  request->send(200, "application/json", makestatus());
}
void incpressure()
{
  if (duty < 1023)
  {
    duty += 20;
  }
  else
  {
    duty = 1023;
  }
}
void decpressure()
{
  if (duty > 0)
  {
    duty -= 20;
    if (duty < 20)
    {
      duty = 0;
    }
  }
}

void run(AsyncWebServerRequest *request)
{
  DynamicJsonDocument doc(jsonsize);
  char buf[jsonsize];

  if (request->hasParam("port") && request->getParam("port")->value().equals("test"))
  {
    Serial.println("Run test port");
    message = "test port ";

    while (duty < 1023)
    {
      incpressure();
      Serial.println(duty);
      delay(20);
    }
    doc["Runport"] = "Test";
    serializeJsonPretty(doc, buf, jsonsize);
    request->send(200, "application/json", buf);
    for (int i = 0; i < 40; i++)
    {

      delay(200);
    }
    while (duty > 20)
    {
      decpressure();
      Serial.println(duty);
      delay(20);
    }

    return;
  }
  else if (request->hasParam("delay"))
  {
    Serial.println("Open pump");
    int t = request->getParam("delay")->value().toInt();
    torun = t * 2;
    if (request->hasParam("p"))
    {

      toduty = request->getParam("p")->value().toInt();
      if (toduty < 101)
      {
        toduty = 1023 / 100 * toduty;
      }
    }
    message = "Run on method in " + String(t);

    doc["Runport"] = "Runontime";
    doc["p"] = toduty;
    doc["t"] = t;
    serializeJsonPretty(doc, buf, jsonsize);
    request->send(200, "application/json", buf);
  }
}
void removeconfig(AsyncWebServerRequest *request)
{
  String v = request->arg("configname");
  cfg.remove(v);
  loadconfigtoram();
  request->send(200, "application/json", "{\"remove\":\"" + v + "\"}");
}
String makestatus()
{
  DynamicJsonDocument doc(jsonsize);
  doc["uptime"] = uptime;
  doc["torun"] = torun;
  doc["toduty"] = toduty;
  doc["duty"] = duty;
  doc["t"] = t;
  char buf[jsonsize];
  serializeJsonPretty(doc, buf, jsonsize);
  return String(buf);
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
void allconfig(AsyncWebServerRequest *request)
{
  // delay(1000);
  DynamicJsonDocument doc = cfg.getAll();
  char buf[jsonsize];
  serializeJsonPretty(doc, buf, jsonsize);
  request->send(200, "application/json", buf);
}
/**
 * ส่งข้อมูลออกไป PWM
 * */
void out(int d)
{
  ledcWrite(0, d);
  ledcWrite(1, d);
}
void addconfig(AsyncWebServerRequest *request)
{
  // delay(1000);
  String configname = request->arg("configname");
  String configvalue = request->arg("value");
  Serial.printf("Config name %s set to %s", configname, configvalue);
  cfg.addConfig(configname, configvalue);
  loadconfigtoram();
  request->send(200, "application/json", "{\"addconfig\":\"ok\",\"setconfig\":\"" + configname + "\",\"value\":\"" + configvalue + "\"}");
}
void resettodefault(AsyncWebServerRequest *request)
{
  LITTLEFS.remove("/config.cfg");
  request->send(200, "application/json", "{\"resettodefault\":\"ok\"}");
  delay(1000);
  ESP.restart();
}
void setHttp()
{
  server.on("/", HTTP_GET, status);
  server.on("/status", HTTP_GET, status);
  server.on("/addconfig", HTTP_GET, addconfig);
  server.on("/allconfig", HTTP_GET, allconfig);
  server.on("/run", HTTP_GET, run);
  // server.on("/ota", HTTP_GET, updatedirect);
  // server.on("/on", HTTP_GET, on);
  server.on("/setconfig", HTTP_GET, setwwwconfig);
  server.on("/setconfigwww", HTTP_GET, setwwwconfig);
  // server.on("/checkin", HTTP_GET, checkinNow);
  // server.on("/off", HTTP_GET, poweroff);
  // server.on("/setvalue", HTTP_GET, setvalue);
  // server.on("/setwifi", HTTP_GET, setwifi);
  // server.on("/scanwifi", HTTP_GET, setwifi);
  // // server1.on("/scanwifiii", HTTP_GET, scanwifiII);
  // server.on("/get", HTTP_GET, get);
  // server.on("/restart", HTTP_GET, restart);
  server.on("/removeconfig", HTTP_GET, removeconfig);
  server.on("/resettodefault", HTTP_GET, resettodefault);

  // // server1.on("/runtime",HTTP_GET,runtime);
  server.begin();
}
void setconfig()
{

  cfg.setbuffer(jsonsize);
  if (!LITTLEFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
  }
  if (!cfg.openFile())
  {
    cfg.addConfig("ssid", "forpi");
    cfg.addConfig("password", "04qwerty");
    // initconfig();
    // Serial.println("Init config file");
  }
  loadconfigtoram();
}
void loadconfigtoram()
{
  configdata.torundelay = cfg.getIntConfig("torundelay", 10);
  configdata.haveds = cfg.getIntConfig("haveds", 0);
  configdata.r040 = cfg.getIntConfig("0-40",20);
  configdata.r4160 = cfg.getIntConfig("41-60",40);
  configdata.r6180 = cfg.getIntConfig("61-80",60);
  configdata.r81100 = cfg.getIntConfig("81-100",100);
}
void tickerfunction()
{
  digitalWrite(2, !digitalRead(2));
  uptime += 2;
  if (torun > 0)
    torun--;
}
void connect()
{
  int l = 0;
  int connectwifilimit = cfg.getIntConfig("connectwifilimit", 10);
  String u = cfg.getConfig("ssid");
  String p = cfg.getConfig("password");
  Serial.printf("\nconnect to %s with password %s\n", u.c_str(), p.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(cfg.getConfig("ssid", "forpi").c_str(), cfg.getConfig("password", "04qwerty").c_str());
  WiFi.setSleep(false);
  Serial.print("Connect");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");

    if (l > connectwifilimit)
    {
      apmode = 1;
      break;
    }
    delay(1000);
    l++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("************ Ip:");
    Serial.println(WiFi.localIP());
    // gateway = WiFi.gatewayIP();
  }
  if (apmode)
  {
    Serial.println("Ap Mode");
    ap.run();
  }
}

void directrun()
{
  if (toduty != duty)
  {
    if (duty < toduty)
    {
      // incpressure();
      duty++;
    }
    else if (duty > toduty)
    {
      // decpressure();
      duty--;
    }
    out(duty);
    delay(configdata.torundelay);
  }

  if (torun <= 0 && toduty > 0)
  {
    toduty = 0;
    Serial.println("Reset to run");
  }
}

void readTmp()
{
  if (configdata.haveds)
  {
    sensors.requestTemperatures();
    t = sensors.getTempCByIndex(0);
  }
}
void runtmp()
{
  if(configdata.haveds)
  {
  int tt = 0;
  if (t <= configdata.r040)
  {
    tt = configdata.r040;
  }
  else if (t <= configdata.r4160)
  {
    tt = configdata.r4160;
  }
  else if (t <= configdata.r6180)
  {
    tt = configdata.r6180;
  }
  else if (t <= configdata.r81100)
  {
    tt =configdata.r81100;
  }
  else
    tt = 0;

  int p = 1023 / 100 * tt;
  Serial.printf("Run tmp %.2f P:%d\n",t,p);
  out(p);
  delay(500);
  }
}
void loop()
{
  // put your main code here, to run repeatedly:
  //
  directrun();
  readTmp();
  runtmp();
  // delay(1000);
}
