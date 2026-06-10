#include <WiFi.h>
#include "MQTT_WIFI.h"
#include "Settings.h"
#include "kitdevesp32.h"
extern Settings MySettings;
ModbusMaster node;
/*********************************************************/
//enum { RUN_DISABLE, RUN_WIFI, RUN_SIM};
uint32_t PreTimeWaitWifi = 0;
enum {S_DisConnectedAll, S_SimConnected, S_SimConnecting, S_WiFiConnected, S_WiFiConnecting};
const char* PrintState[5] = {"S_DisConnectedAll", "S_SimConnected", "S_SimConnecting", "S_WiFiConnected", "S_WiFiConnecting"};
uint8_t StateMQTT = S_WiFiConnecting;
uint8_t MQTTmodeRunCurrent = RUN_DISABLE;
void MQTTPublishBy(uint8_t RunMode);
void MQTTstateMachine();
bool Read_modbus();
void Restoredefault();

MQTT_WIFI MQTT_WiFi;
#define LED1 2
#define LED2 15
#define ETH 14
int PORT=1883;
String SERVER="YOUR_MQTT_SERVER";
String USER="YOUR_MQTT_USERNAME";
String PASS="YOUR_MQTT_PASSWORD";

String SSID_WIFI="YOUR_WIFI_SSID";
String PASS_WIFI="YOUR_WIFI_PASSWORD";

long Add_MB[12]; 

void setup() 
{
  pinMode(ETH, OUTPUT); 
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  digitalWrite(ETH, HIGH);
  Serial.begin(115200);
  MySettings.Init();
  MySettings.RestoreSettings();
  MQTT_WiFi.Init();  
  pinMode(5, INPUT);
}
uint32_t time_push=0, time_push_old=0;
int dem=0; extern String Client_ID;
int count_while=0;
void loop() 
{
 count_while=0;
 while(digitalRead(5)==LOW)
 {
  count_while++;
  delay(10);
  if(count_while>=800)
  Restoredefault();
 }
 MQTTstateMachine(); 
 time_push=millis();
 if(time_push-time_push_old>500||time_push<time_push_old)
 {
  dem++;
  if(dem%30==0)
  {
    MQTT_WiFi.PublishData_Retain((Client_ID+ "/Ping").c_str() , "1");
  }
  digitalWrite(LED2,!digitalRead(LED2));
  time_push_old=time_push;
  if(MQTTmodeRunCurrent==RUN_WIFI) 
  {
   Read_modbus();
  }
 }
} 
void MQTTstateMachine()
{
    switch(StateMQTT) 
     {
        case S_DisConnectedAll:
             StateMQTT=S_WiFiConnecting; 
             PreTimeWaitWifi=millis();
             Serial.printf("\r\nState Internet Machine: ");
             Serial.printf("%s",PrintState[(uint8_t)StateMQTT]);
        break;
         
        case S_WiFiConnecting:             
         if(MQTT_WiFi.ReconnectWIFI() == S_CONNECTED)
          {
            if(MQTT_WiFi.ReconnectMqtt() == S_CONNECTED) 
              { 
                StateMQTT=S_WiFiConnected;
                Serial.printf("\r\nState Internet Machine: ");
                Serial.printf("%s",PrintState[(uint8_t)StateMQTT]);
                MQTT_WiFi.Subscribe();
                MQTTmodeRunCurrent=RUN_WIFI; 
                MQTT_WiFi.PublishData((Client_ID+ "/FB").c_str() , ("Node online.\r\nConected to wifi "+ SSID_WIFI + ".\r\nReady for recieve configure.\r\n").c_str());
                break;
              }  
          }
          break;          
         case S_WiFiConnected:
           if((MQTT_WiFi.ReconnectWIFI() !=S_CONNECTED) || (MQTT_WiFi.ReconnectMqtt() !=S_CONNECTED))
            {
             StateMQTT=S_WiFiConnecting; 
             PreTimeWaitWifi=millis(); 
             MQTTmodeRunCurrent=RUN_DISABLE;
             Serial.printf("\r\nState Internet Machine: ");
             Serial.printf("%s",PrintState[(uint8_t)StateMQTT]);
            }
           break; 
     }
     switch(StateMQTT)
     {
        case S_DisConnectedAll:
        digitalWrite(LED1,LOW);
        break;
        case S_WiFiConnecting:
        digitalWrite(LED1,!digitalRead(LED1));
        break;
        case S_WiFiConnected:
         if(MQTTmodeRunCurrent==RUN_WIFI) 
         {
          digitalWrite(LED1,HIGH);
          MQTT_WiFi.MQTT_ReceiveData();  
         }
         break;
       }
}
extern String data_modbus;
int address_modbus, type_address_modbus, id_modbus;
bool Read_modbus()
{
  if(data_modbus != "")
  {
    
   unsigned int tach=0, index=0;
   String SubStr="";
   unsigned int leng=data_modbus.length();
   //Serial.printf("\r\ndata_modbus length=%d", leng);
   for(int i=0;i<leng;i++)
    {  
      if(data_modbus.charAt(i)=='\n')
      {
        SubStr=data_modbus.substring(tach,i);
        tach=i+1; index++;
        if(index>=1)
        {
           unsigned int tach2=0, index2=0;
           String SubStr2="";
           unsigned int leng2=SubStr.length();
           //Serial.printf("\r\nSubStr length=%d", leng2);
           for(int i2=0;i2<leng2;i2++)
            {  
              if(SubStr.charAt(i2)=='@')
              {
                 SubStr2=SubStr.substring(tach2,i2);
                  tach2=i2+1; index2++;
                  if(index2==1)
                  {
                    id_modbus=SubStr2.toInt();
                  }
                  else if(index2==2)
                  {
                    address_modbus=SubStr2.toInt();
                  }
                  else if(index2==3)
                  {
                    type_address_modbus=SubStr2.toInt();                   
                    Serial_kitdev.begin(MySettings.ModbusBaud.toInt(), MySettings.ModbusUARTConfig); 
                    node.begin(id_modbus, Serial_kitdev);//40001
                    uint8_t result;
                    if(address_modbus/10000==4)
                    result = node.readHoldingRegisters(address_modbus%10000, type_address_modbus+1);
                    else if(address_modbus/10000==3)
                    result = node.readInputRegisters(address_modbus%10000, type_address_modbus+1);
                    delay(10); 
                    if (result == node.ku8MBSuccess) //Read ok
                    {
                      String topic_push=String(id_modbus) + String(address_modbus);
                      Serial.printf("\r\nRead ID: %d address %d, ", id_modbus, address_modbus);
                      if(type_address_modbus==0)
                      {
                        int int_value=node.getResponseBuffer(0);
                        Serial.printf("value_I: %d",int_value);
                        MQTT_WiFi.PublishData(topic_push.c_str() , String(int_value).c_str() );
                      }
                      else if(type_address_modbus==1)
                      {
                         uint16_t data[2];
                         data[0]=node.getResponseBuffer(0);
                         data[1]=node.getResponseBuffer(1); 
                         uint32_t Hexvalue = ((uint32_t)data[0]<<16)|data[1];
                         float fl_value = *(float *)&Hexvalue;
                         Serial.printf("value_F: %0.4f", fl_value);//show value type float
                         MQTT_WiFi.PublishData(topic_push.c_str() , String(fl_value).c_str() );
                      }  
                    }
                    else 
                    {
                       Serial.printf("\r\nRead ID %d address %d FAIL", id_modbus, address_modbus);
                       String topic_push=String(id_modbus) + String(address_modbus);
                       MQTT_WiFi.PublishData(topic_push.c_str() , "N/A" );

                    }
                  }
                }   
              }
            }
        } 
    }
  }       
}
void Restoredefault()
{
  Serial.print("\r\nRestoredefault\r\n");
  // NOTE: Replace with your actual credentials before use
  String str_default = "1CANOPUS\n123456789\nYOUR_MQTT_SERVER\n1883\nYOUR_MQTT_USERNAME\nYOUR_MQTT_PASSWORD\n9600\n134217756\n000001\n10\n";
  MySettings.SaveAllSetting(&str_default);
  Serial.print("\r\nRESET\r\n");
  ESP.restart();
}
