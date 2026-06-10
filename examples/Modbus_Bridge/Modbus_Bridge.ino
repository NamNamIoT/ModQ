#include <WiFi.h>
#include "ETH_MB.h"
#include "ModbusIP_ESP8266.h"
ModbusIP mb;
extern bool eth_connected;
#include "kitdevesp32.h"
ModbusMaster node;

void setup()
{
  Serial.begin(115200);
  Serial.print("\r\n*****************KIT DEV ESP32*******************");
  Serial_kitdev.begin(9600, SERIAL_8N1); 
  node.begin(1, Serial_kitdev); //ID node 1
  pinMode(32, OUTPUT);//led1
  pinMode(33, OUTPUT);//led2
  pinMode(14, OUTPUT);//pin enable ethernet
  digitalWrite(32, HIGH);
  digitalWrite(33, HIGH);
  digitalWrite(14, HIGH); 
  
//  if(WiFi.status() != WL_CONNECTED&&!eth_connected)
//    {
//      IPAddress local_IP(192, 168, 1, 123);
//      IPAddress gateway(192, 168, 1, 1);
//      IPAddress subnet(255, 255, 255, 0);
//      IPAddress primaryDNS(8, 8, 8, 8); //optional
//      IPAddress secondaryDNS(8, 8, 4, 4); //optional
//      if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
//        Serial.printf("\r\nWiFi Failed to configure");
//        }
//    }
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
  mb.addHreg(1, 0);//Tao thanh ghi 1, gia tri mac dinh 0, FC03 040001, modbus TCP/IP
  mb.addHreg(2, 0);//Tao thanh ghi 2, gia tri mac dinh 0, FC03 040002, modbus TCP/IP
  mb.addIreg(1, 0);//Tao thanh ghi 1, gia tri mac dinh 0, FC04 030001, modbus TCP/IP
  mb.addIreg(2, 0);//Tao thanh ghi 2, gia tri mac dinh 0, FC04 030002, modbus TCP/IP
}
void loop()
{ 
  mb.task();
  Serial.printf("\r\nWait Read  RTU");
  uint8_t result;
  result = node.readHoldingRegisters(1, 2);//Doc thanh ghi holding, doc 2 thanh ghi bat dau tu thanh ghi 1 (nghia la 40001 và 40002), read modbus RTU
  delay(10);
  
  if (result == node.ku8MBSuccess) //Read ok
  {
    uint16_t data[2];
    digitalWrite(32, !digitalRead(32));//blink led
    data[0]=node.getResponseBuffer(0);
    data[1]=node.getResponseBuffer(1);  // Fix: lấy đúng data[1] từ buffer
    Serial.printf("\n\rValue 40001: ");
    Serial.print(data[0]);
    Serial.printf("\n\rValue 40002: ");
    Serial.print(data[1]);
    mb.Hreg(1, data[0]);//Move data from "read modbus RTU" to "modbus TCP/IP"
    mb.Hreg(2, data[1]);//Move data from "read modbus RTU" to "modbus TCP/IP"
  }
  else Serial.print("Fail read");
  delay(1000); 
  digitalWrite(33, !digitalRead(33));
}
