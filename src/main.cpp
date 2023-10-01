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
#define TMPMODE 1
#define DIRMODE 2
#define MMODE 3
#define ENABLEPORT 26 // สำหรับตัดเข้าระบบเราถ้า ESP ทำงาน ok

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
int tmpduty = 0;
int readtimetime = 0;
int currentvalue = 0;
float t = 0;
float t1 = 0;
String message;
int runmode = TMPMODE;
int maxtmp = 0;
String runmodename = "";
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
function run()
{
  var xhr = new XMLHttpRequest();
  var input = document.getElementById('timetorun');
  var value = document.getElementById('level');
  xhr.open("GET", "/run?delay="+input.value+"&p="+value.value, true); 
  xhr.addEventListener("readystatechange", () => {
     console.log(xhr.readystate);
    if (xhr.readyState === 4 && xhr.status === 200) {
     console.log(xhr.responseText);
    //  var o =  JSON.parse(xhr.responseText);
    //  var t = document.getElementById('customers');
    //  var row = t.insertRow();
    //  row.innerHTML = "<td>"+o.setconfig+"</td><td>"+o.value+"</td><td><input value="+o.value+"></td>";
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
Directrun  <input id=timetorun> <input id=level> <button  id=btn onClick="run()">Run </button>
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
struct
{
  int apmode = 0; // บอกให้ run ap mode
  int apmodelimit = 30;
  int torundelay = 10;
  int haveds = 0;
  int haveapmode = 1;
  int r040 = 20;
  int r4160 = 40;
  int r6180 = 60;
  int r81100 = 100;
  int readtmptime = 30;
  int printadc = 1;
  int printruntmp = 1;
  int servermode = 1;
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
  {
    readtimetime = 500; //
    sensors.begin();
  }
}

void setupPort()
{
  pinMode(2, OUTPUT);
  pinMode(ENABLEPORT, OUTPUT);
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
  runmode = DIRMODE;
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
    Serial.println("DIRECT mode");
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
  doc["tmpduty"] = tmpduty;
  doc["t"] = t;
  doc["t1"] = t1;
  doc["runmode"] = runmode;
  doc["currentadc"] = currentvalue;
  doc["runmodename"] = runmodename;
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
  server.on("/", HTTP_GET, setwwwconfig);
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
  server.on("/info", HTTP_GET, status);
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
  configdata.r040 = cfg.getIntConfig("0-20", 20);
  configdata.r4160 = cfg.getIntConfig("21-40", 40);
  configdata.r6180 = cfg.getIntConfig("41-80", 80);
  configdata.r81100 = cfg.getIntConfig("81-100", 100);
  configdata.haveapmode = cfg.getIntConfig("haveapmode", 1);
  configdata.printadc = cfg.getIntConfig("printadc", 1);
  configdata.printruntmp = cfg.getIntConfig("printruntmp", 1);
  configdata.servermode = cfg.getIntConfig("servermode", 1);
}
void tickerfunction()
{
  digitalWrite(2, !digitalRead(2));
  uptime += 2;
  if (torun > 0)
  {
    torun--;
    if (torun <= 0)
      runmode = TMPMODE;
  }
  readtimetime++;
}
void servermode()
{
  String ssid = cfg.getConfig("servername", "EFAN");
  String password = cfg.getConfig("serverpassword", "123456789");
  WiFi.softAP(ssid.c_str(), password.c_str());
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  // setHttp();
}
void connect()
{
  if (!configdata.servermode)
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
    if (apmode && configdata.haveapmode)
    {
      ap.setrestartmode(cfg.getIntConfig("aprestart", 1)); // บอกให้ ap restart
      ap.setapmodetime(cfg.getIntConfig("aptime", 3));     // เวลาที่เปิด AP mode

      Serial.println("Ap Mode");
      ap.run();
    }
  }

  else
  {
    servermode();
  }
}

void directrun()
{
  if (toduty != duty && runmode == DIRMODE)
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

  if (configdata.haveds && runmode == TMPMODE && readtimetime >= configdata.readtmptime)
  {
    // t = 0;
    // t1 = 0;
    readtimetime = 0;
    float buf = 0;
    sensors.requestTemperatures();
    int sensorcount = sensors.getDeviceCount();
    if (sensorcount > 0)
    {
      buf = sensors.getTempCByIndex(0);
      if (buf > 0)
        t = buf;
      if (sensorcount > 1)
      {

        buf = sensors.getTempCByIndex(1);
        if (buf > 0)
          t1 = buf;
      }
    }
  }
}
int getTmp(float t1)
{
  int tt = 0;
  if (t1 <= configdata.r040)
  {
    tt = configdata.r040;
  }
  else if (t1 <= configdata.r4160)
  {
    tt = configdata.r4160;
  }
  else if (t1 <= configdata.r6180)
  {
    tt = configdata.r6180;
  }
  else if (t1 <= configdata.r81100)
  {
    tt = configdata.r81100;
  }

  return tt;
}
void runtmp()
{
  if (configdata.haveds && runmode == TMPMODE)
  {
    int tt = getTmp(t);
    int tt1 = getTmp(t1);
    maxtmp = 0;
    if (tt < tt1)
      maxtmp = tt1;
    else
      maxtmp = tt;

    if (maxtmp != 0)
    {
      int p = 1023 / 100 * maxtmp;
      if (configdata.printruntmp)
        Serial.printf("Run tmp %.2f P:%d == %d\n", t, p, tmpduty);
      if (p != tmpduty)
      {
        tmpduty = p;
      }
    }
  }
}
void to(int d, int t, String call)
{
  if (d < t)
  {
    duty++;
    Serial.printf("%s Inc Duty to t  %d/%d \n", call.c_str(), d, t);
    delay(20);
  }
  else if (d > t)
  {
    duty--;
    Serial.printf("%s Dec Duty to t  %d/%d \n", call.c_str(), d, t);
    delay(20);
  }
}
/**
 * @brief สำหรับ tmp
 *
 */
void updateduty()
{
  // ถ้า duty ไม่เท่ากับ tmpduty และ toduty =0 ระบบจะทำงาน
  if (runmode == TMPMODE && tmpduty != duty)
  {
    to(duty, tmpduty, "TMP MODE");
    digitalWrite(25, 1);
    runmodename = "TMP MODE";
  }
  else if (runmode == MMODE && currentvalue != duty)
  {
    int p = 1023 / 100 * currentvalue;
    to(duty, p, "MMODE");
    runmodename = "MMODE";
    digitalWrite(25, 0);
  }
  else if (runmode == DIRMODE && duty != toduty)
  {
    to(duty, toduty, "DIRMODE");
    runmodename = "DIR MODE";
    digitalWrite(25, 0);
  }
}
/***
 * บอกว่าเครื่องทำงานปกติ
 */
void checkRun()
{
  if (duty > 0)
    digitalWrite(ENABLEPORT, 1);
  else
    digitalWrite(ENABLEPORT, 0);
}
void readADC()
{
  float val = 0;
  int backuprunmode = runmode;
  for (int i = 0; i < 16; i++)
    val += adc1_get_raw(ADC1_CHANNEL_3);
  val /= 16;

  if (val > 20)
  {
    // Serial.printf("ADC VALUE %.2f\n", val);
    currentvalue = val / 4095 * 100;
    if (currentvalue != 0)
      runmode = MMODE;
    if (configdata.printadc)
    {
      Serial.printf("ADC: %.2f  Currentvalue:%d \n", val, currentvalue);
      delay(200);
    }
  }
  else if (runmode != DIRMODE)
  {
    runmode = TMPMODE; // กลับไป tmp mode ถ้าไม่อยู่ใน mode direct
  }
}

void loop()
{
  directrun();
  readTmp();
  runtmp();
  updateduty();
  checkRun();
  readADC();
  out(duty);
}
