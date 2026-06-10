#include <WiFi.h>
#include "ETH_MB.h"
#include "ModbusIP_ESP8266.h"
ModbusIP mb;
extern bool eth_connected;
  
void setup() {
  pinMode(14, OUTPUT);
  digitalWrite(14, HIGH); 
  delay(1000);
  Serial.begin(115200);
  Serial.println("\r\nHello\r\n");
  if(WiFi.status() != WL_CONNECTED&&!eth_connected)
    {
      IPAddress local_IP(192, 168, 1, 123);
      IPAddress gateway(192, 168, 1, 1);
      IPAddress subnet(255, 255, 255, 0);
      IPAddress primaryDNS(8, 8, 8, 8); //optional
      IPAddress secondaryDNS(8, 8, 4, 4); //optional
      if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
        Serial.printf("\r\nWiFi Failed to configure");
        }
    }
  ETH_begin();
  delay(5000);
  WiFi.begin("TEN_WIFI", "PASS_WIFI");
  while (WiFi.status() != WL_CONNECTED && !eth_connected) {
    delay(500);
    Serial.print(".");
  }
 
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  if(!eth_connected)
  Serial.println(WiFi.localIP());
  else
  Serial.println(ETH.localIP());
  mb.begin();
  mb.addHreg(2, 0);//Tao thanh ghi 2, FC03 040002
  mb.addIreg(2, 0);//Tao thanh ghi 2, FC04 030002
}
 
void loop() {
   mb.task();
   mb.Hreg(2, 1);//Ghi gia tri 1 vao thanh ghi 040002
   mb.Ireg(2, 1);//Ghi gia tri 1 vao thanh ghi 030002
   delay(500);
   mb.Hreg(2, 0);//Ghi gia tri 0 vao thanh ghi 040002
   mb.Ireg(2, 1);//Ghi gia tri 0 vao thanh ghi 030002
   delay(500);
}
