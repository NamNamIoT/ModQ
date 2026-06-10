
#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "kitdevesp32.h"
ModbusMaster node;
/************************* WiFi Access Point *********************************/

#define WLAN_SSID       "YOUR_WIFI_SSID"
#define WLAN_PASS       "YOUR_WIFI_PASSWORD"


/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "YOUR_MQTT_SERVER"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "YOUR_MQTT_USERNAME"
#define AIO_KEY         "YOUR_MQTT_KEY"

/************ Global State (you don't need to change this!) ******************/
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
/****************************** Feeds ***************************************/
Adafruit_MQTT_Publish fb = Adafruit_MQTT_Publish(&mqtt,"fb0");
void MQTT_connect();
void setup() 
{
  Serial.begin(115200);
  delay(10);
  Serial.println(F("Adafruit MQTT demo"));
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: "); 
  Serial.println(WiFi.localIP());
  Serial_kitdev.begin(9600, SERIAL_8N1); 
  node.begin(1, Serial_kitdev);
}
uint32_t x=0,pretime=0; int count_connect_fail=0;
void loop() 
{
  if(WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    count_connect_fail++;
    Serial.println("fail");
    if(count_connect_fail>=20) ESP.restart();
  }
  MQTT_connect();
  Adafruit_MQTT_Subscribe *subscription;
  
  Serial.println("");
  Serial.println("Wait Read");
  uint8_t result;
  result = node.readHoldingRegisters(1, 1);//Read 40001
  delay(10);
  if (result == node.ku8MBSuccess) //Read ok
  {
    uint16_t data[1];
    data[0]=node.getResponseBuffer(0);
    Serial.printf("\r\nValue 40001: %d",data[0]);
    String data_modbus=String(data[0]);
    fb.publish(data_modbus.c_str());
  }
  else Serial.print("Read Fail");
  delay(500); 
}
void MQTT_connect() {
  int8_t ret;
  if (mqtt.connected()) 
  {
    return;
  }
  Serial.print("Connecting to MQTT... ");
  uint8_t retries = 5;
  while ((ret = mqtt.connect()) != 0) 
  {
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         ESP.restart();
       }
  }
  Serial.println("MQTT Connected!");
}
