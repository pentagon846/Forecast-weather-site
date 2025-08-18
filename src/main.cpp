#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

// ---------- WiFi ----------
const char* ssid = "WIFI_SSID";
const char* password = "WIFI_PASS";

// OPTIONAL: Static IP 
/*
IPAddress staticIP(192, 168, 1, 150);
IPAddress gateway (192, 168, 1, 1);
IPAddress subnet  (255, 255, 255, 0);
*/
  
// ---------- OLED ----------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ---------- Time (Rome summer = UTC+2 -> 7200) ----------
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7200, 60000); // 7200 = UTC+2 (літо)

// ---------- Pins ----------
#define RELAY_PIN 14   // D5
#define BUTTON_PIN 12  // D6



// ---------- Durations (minutes) ----------
int manualDuration  = 10;
int morningDuration = 10;
int eveningDuration = 10;

// ---------- Timers ----------
int  morningHour   = -1, morningMinute = -1;  bool morningEnabled = false;
int  eveningHour   = -1, eveningMinute = -1;  bool eveningEnabled = false;

// ---------- State machine ----------
enum Mode { IDLE, MANUAL, MORNING, EVENING };
Mode activeMode = IDLE;

bool          relayState      = false;
unsigned long startedAtEpoch  = 0;   // epoch seconds, коли стартував полив
int           activeDurationM = 0;   // хвилини поточного поливу

ESP8266WebServer server(80);

// ---------- Helpers ----------
unsigned long nowSec() { return timeClient.getEpochTime(); }

void relayOn()  { relayState = true;  digitalWrite(RELAY_PIN, HIGH); }
void relayOff() { relayState = false; digitalWrite(RELAY_PIN, LOW);  }

void startWatering(Mode m, int durationMin) {
  activeMode       = m;
  activeDurationM  = (durationMin < 0 ? 0 : durationMin);
  startedAtEpoch   = nowSec();
  relayOn();
}

void stopWatering() {
  relayOff();
  activeMode       = IDLE;
  activeDurationM  = 0;
  startedAtEpoch   = 0;
}

long remainingSeconds() {
  if (!relayState || activeDurationM <= 0) return 0;
  long elapsed = (long)nowSec() - (long)startedAtEpoch;
  long total   = (long)activeDurationM * 60L;
  long rem     = total - elapsed;
  return (rem > 0 ? rem : 0);
}

// ---------- OLED ----------
void drawDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);

  // Clock
  display.print("Time: ");
  display.println(timeClient.getFormattedTime());

  // Mode
  display.setCursor(0,10);
  display.print("Mode: ");
  switch (activeMode) {
    case IDLE:    display.println("IDLE");    break;
    case MANUAL:  display.println("MANUAL");  break;
    case MORNING: display.println("MORNING"); break;
    case EVENING: display.println("EVENING"); break;
  }

  // Relay + remaining
  display.setCursor(0,20);
  display.print("Relay: ");
  display.println(relayState ? "ON" : "OFF");

  if (relayState) {
    long rem = remainingSeconds();
    int mm = rem / 60;
    int ss = rem % 60;
    display.setCursor(0,30);
    display.print("Left: ");
    if (mm < 10) display.print('0'); display.print(mm);
    display.print(":");
    if (ss < 10) display.print('0'); display.print(ss);
  }

  // Morning
  display.setCursor(0,40);
  display.print("Morn: ");
  if (morningEnabled && morningHour >= 0 && morningMinute >= 0) {
    if (morningHour   < 10) display.print('0'); display.print(morningHour);
    display.print(":");
    if (morningMinute < 10) display.print('0'); display.print(morningMinute);
    display.print(" ("); display.print(morningDuration); display.print("m)");
  } else {
    display.print("off");
  }

  // Evening
  display.setCursor(0,50);
  display.print("Eve : ");
  if (eveningEnabled && eveningHour >= 0 && eveningMinute >= 0) {
    if (eveningHour   < 10) display.print('0'); display.print(eveningHour);
    display.print(":");
    if (eveningMinute < 10) display.print('0'); display.print(eveningMinute);
    display.print(" ("); display.print(eveningDuration); display.print("m)");
  } else {
    display.print("off");
  }

  display.display();
}

// ---------- HTML (simple AJAX UI) ----------
const char PAGE_INDEX[] PROGMEM = R"HTML(
<!doctype html><html><head><meta charset="utf-8"/>
<title>Watering</title><meta name="viewport" content="width=device-width,initial-scale=1">
<style>
body{font-family:system-ui,Arial;margin:20px;line-height:1.4}
h2{margin:14px 0 8px}form,fieldset{margin:8px 0;padding:8px;border:1px solid #ddd;border-radius:8px}
.row{display:flex;gap:8px;flex-wrap:wrap;align-items:center}
input[type=number]{width:90px}
button, input[type=submit]{padding:6px 10px;border:1px solid #aaa;border-radius:8px;background:#f7f7f7;cursor:pointer}
.card{border:1px solid #ddd;border-radius:10px;padding:10px}
.badge{padding:2px 6px;border-radius:6px;background:#eee;display:inline-block}
</style></head>
<body>
<h1>Watering Controller</h1>

<div class="card">
  <h2>Status</h2>
  <div id="status">Loading...</div>
  <div class="row" style="margin-top:6px;">
    <form action="/api/stop" method="post"><button type="submit">Stop</button></form>
    <form action="/api/startManual" method="post"><button type="submit">Start Manual</button></form>
  </div>
</div>

<form class="card" action="/api/setManual" method="post">
  <h2>Manual Duration</h2>
  <div class="row">
    <label>Minutes <input type="number" name="duration" min="1" max="240"></label>
    <input type="submit" value="Save">
  </div>
</form>

<form class="card" action="/api/setMorning" method="post">
  <h2>Morning Timer</h2>
  <div class="row">
    <label>Hour <input type="number" name="h" min="0" max="23"></label>
    <label>Minute <input type="number" name="m" min="0" max="59"></label>
    <label>Duration <input type="number" name="d" min="1" max="240"></label>
    <label>Enabled <input type="checkbox" name="en" value="1"></label>
    <input type="submit" value="Save">
  </div>
</form>

<form class="card" action="/api/setEvening" method="post">
  <h2>Evening Timer</h2>
  <div class="row">
    <label>Hour <input type="number" name="h" min="0" max="23"></label>
    <label>Minute <input type="number" name="m" min="0" max="59"></label>
    <label>Duration <input type="number" name="d" min="1" max="240"></label>
    <label>Enabled <input type="checkbox" name="en" value="1"></label>
    <input type="submit" value="Save">
  </div>
</form>

<script>
async function load(){
  try{
    const r = await fetch('/status');
    const s = await r.json();
    document.getElementById('status').innerHTML =
      '<div>Time: <span class="badge">'+s.time+'</span></div>'+
      '<div>Mode: <span class="badge">'+s.mode+'</span></div>'+
      '<div>Relay: <span class="badge">'+(s.relay?'ON':'OFF')+'</span></div>'+
      (s.relay?('<div>Left: <span class="badge">'+s.left+'</span></div>'):'')+
      '<div>Manual: '+s.manual+'m</div>'+
      '<div>Morning: '+(s.morning.en?('ON '+s.morning.hhmm+' ('+s.morning.d+'m)'):'OFF')+'</div>'+
      '<div>Evening: '+(s.evening.en?('ON '+s.evening.hhmm+' ('+s.evening.d+'m)'):'OFF')+'</div>'+
      '<div>IP: '+s.ip+'</div>';
  }catch(e){document.getElementById('status').textContent='Error loading status';}
}
setInterval(load, 2000); load();
</script>
</body></html>
)HTML";

// ---------- HTTP helpers ----------
String modeToStr(Mode m){
  switch(m){ case IDLE: return "IDLE"; case MANUAL: return "MANUAL"; case MORNING: return "MORNING"; case EVENING: return "EVENING"; }
  return "-";
}

void handleIndex(){ server.send_P(200, "text/html", PAGE_INDEX); }

void handleStatus(){
  
  String json = "{";

  // time
  json += "\"time\":\"";
  json += timeClient.getFormattedTime();
  json += "\",";

  // mode
  json += "\"mode\":\"";
  json += modeToStr(activeMode);
  json += "\",";

  // relay
  json += "\"relay\":";
  json += (relayState ? "true" : "false");
  json += ",";

  // left
  long rem = remainingSeconds();
  int mm = rem / 60;
  int ss = rem % 60;
  String left;
  if (mm < 10) left += "0";
  left += String(mm);
  left += ":";
  if (ss < 10) left += "0";
  left += String(ss);
  json += "\"left\":\"";
  json += left;
  json += "\",";

  // manual
  json += "\"manual\":";
  json += String(manualDuration);
  json += ",";

  // morning
  json += "\"morning\":{";
  json += "\"en\":";
  json += (morningEnabled ? "true" : "false");
  json += ",";
  json += "\"hhmm\":\"";
  if (morningHour >= 0 && morningMinute >= 0) {
    if (morningHour < 10) json += "0";
    json += String(morningHour);
    json += ":";
    if (morningMinute < 10) json += "0";
    json += String(morningMinute);
  } else {
    json += "--:--";
  }
  json += "\",";
  json += "\"d\":";
  json += String(morningDuration);
  json += "},";

  // evening
  json += "\"evening\":{";
  json += "\"en\":";
  json += (eveningEnabled ? "true" : "false");
  json += ",";
  json += "\"hhmm\":\"";
  if (eveningHour >= 0 && eveningMinute >= 0) {
    if (eveningHour < 10) json += "0";
    json += String(eveningHour);
    json += ":";
    if (eveningMinute < 10) json += "0";
    json += String(eveningMinute);
  } else {
    json += "--:--";
  }
  json += "\",";
  json += "\"d\":";
  json += String(eveningDuration);
  json += "},";

  // ip
  IPAddress ip = WiFi.localIP();
  json += "\"ip\":\"";
  json += String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
  json += "\"}";

  server.send(200, "application/json", json);
}

int argToInt(const String& name, int def=0){ return server.hasArg(name)? server.arg(name).toInt(): def; }
bool argToBool(const String& name){ return server.hasArg(name) && (server.arg(name)=="1" || server.arg(name)=="on"); }

void handleStartManual(){ if (activeMode == IDLE) startWatering(MANUAL, manualDuration); server.sendHeader("Location","/"); server.send(303); }
void handleStop()       { stopWatering(); server.sendHeader("Location","/"); server.send(303); }

void handleSetManual(){
  int d = argToInt("duration", manualDuration);
  manualDuration = constrain(d, 1, 240);
  server.sendHeader("Location","/");
  server.send(303);
}

void handleSetMorning(){
  int h = argToInt("h", morningHour);
  int m = argToInt("m", morningMinute);
  int d = argToInt("d", morningDuration);
  bool en = argToBool("en");
  morningHour = constrain(h, 0, 23);
  morningMinute = constrain(m, 0, 59);
  morningDuration = constrain(d, 1, 240);
  morningEnabled = en;
  server.sendHeader("Location","/");
  server.send(303);
}

void handleSetEvening(){
  int h = argToInt("h", eveningHour);
  int m = argToInt("m", eveningMinute);
  int d = argToInt("d", eveningDuration);
  bool en = argToBool("en");
  eveningHour = constrain(h, 0, 23);
  eveningMinute = constrain(m, 0, 59);
  eveningDuration = constrain(d, 1, 240);
  eveningEnabled = en;
  server.sendHeader("Location","/");
  server.send(303);
}

// ---------- Setup / Loop ----------
void setup() {
  Serial.begin(115200);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { for(;;); }

  pinMode(RELAY_PIN, OUTPUT); relayOff();
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // OPTIONAL: Static IP (uncomment and set your values)
  // WiFi.config(staticIP, gateway, subnet);

  WiFi.begin(ssid, password);
  Serial.print("WiFi...");
  while (WiFi.status()!=WL_CONNECTED){ delay(400); Serial.print("."); }
  Serial.print("\nIP: "); Serial.println(WiFi.localIP());

  timeClient.begin();

  server.on("/",               handleIndex);
  server.on("/status",         handleStatus);
  server.on("/api/startManual",HTTP_POST, handleStartManual);
  server.on("/api/stop",       HTTP_POST, handleStop);
  server.on("/api/setManual",  HTTP_POST, handleSetManual);
  server.on("/api/setMorning", HTTP_POST, handleSetMorning);
  server.on("/api/setEvening", HTTP_POST, handleSetEvening);
  server.begin();
}

void loop() {
  server.handleClient();
  timeClient.update();

  // Button (toggle)
  static uint32_t lastBtn = 0;
  if (digitalRead(BUTTON_PIN)==LOW && (millis()-lastBtn>250)) {
    lastBtn = millis();
    if (activeMode==IDLE) startWatering(MANUAL, manualDuration);
    else stopWatering();
  }

  // Auto stop
  if (relayState && remainingSeconds()==0) {
    stopWatering();
  }

  // Schedule triggers once per minute edge
  static int lastMinute = -1;
  int h = timeClient.getHours();
  int m = timeClient.getMinutes();
  if (m != lastMinute) {
    lastMinute = m;
    if (activeMode==IDLE) {
      if (morningEnabled && h==morningHour && m==morningMinute) {
        startWatering(MORNING, morningDuration);
      } else if (eveningEnabled && h==eveningHour && m==eveningMinute) {
        startWatering(EVENING, eveningDuration);
      }
    }
  }

  drawDisplay();
  delay(250);
}
