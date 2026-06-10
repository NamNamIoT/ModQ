#include <WiFi.h>
#include "Settings.h"
#include "kitdevesp32.h"
Settings MySettings;
#include "ETH.h"
#include "ModbusIP_ESP8266_Canopus.h"
ModbusIP_Canopus mb;
ModbusIP_Canopus mb2;
extern bool eth_connected;
#include "modbus.h"
#include "modbusDevice.h"
#include "modbusRegBank.h"
#include "modbusSlave.h" 
modbusDevice regBank;
modbusSlave slave;
ModbusMaster node;
extern int our_board_is_master;
#include "esp_system.h"
const int wdtTimeout = 1000;
hw_timer_t *timer = NULL;
int ID_modbusTCP;
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
/*********************************************************/
bool eth_connected = false;
void WiFiEvent(WiFiEvent_t event)
{
  switch (event) {
    case SYSTEM_EVENT_ETH_START:
      Serial.printf("\r\nETH Started");
      //set eth hostname here
      ETH.setHostname("Canopus_Node");
      break;
    case SYSTEM_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;
    case SYSTEM_EVENT_ETH_GOT_IP:
      Serial.printf("\r\nETH MAC: ");
      Serial.print(ETH.macAddress());
      Serial.print(", IPv4: ");
      Serial.print(ETH.localIP());
      if (ETH.fullDuplex()) {
        Serial.print(", FULL_DUPLEX");
      }
      Serial.print(", ");
      Serial.print(ETH.linkSpeed());
      Serial.println("Mbps");
      eth_connected = true;
      break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
      Serial.printf("\r\nETH Disconnected");
      eth_connected = false;
      break;
    case SYSTEM_EVENT_ETH_STOP:
      Serial.print("\r\nETH Stopped");
      eth_connected = false;
      break;
    default:
      break;
  }
}

#define ETH_ENABLE 14

void ETH_begin(){
  pinMode(ETH_ENABLE, OUTPUT);
  digitalWrite(ETH_ENABLE, HIGH); //Enable ETH
  delay(10);
  WiFi.onEvent(WiFiEvent);
  ETH.begin();
}
bool Read_modbus();
#define LED1 2
#define LED2 15
extern String LocalIP[4],GatewayIP[4], Subnet, PrimaryDNS, SecondaryDNS;
int id_from_mbtcp=0, id1=0, id2=0, id3=0, id4=0;

void add_resgister();
void setup() 
{
  wifiMulti.addAP("YOUR_WIFI_SSID", "YOUR_WIFI_PASSWORD");
  //wifiMulti.addAP("LAN", "116117118");
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  Serial.begin(115200);
  MySettings.Init();
  MySettings.RestoreSettings();
  ETH_begin();
  if(our_board_is_master==0)
  init_MB();
  init_WDT();
  
  if(WiFi.status() != WL_CONNECTED&&!eth_connected)
    {
      IPAddress local_IP((uint8_t)(LocalIP[0].toInt()), (uint8_t)(LocalIP[1].toInt()), (uint8_t)(LocalIP[2].toInt()), (uint8_t)(LocalIP[3].toInt()));
      IPAddress gateway((uint8_t)(GatewayIP[0].toInt()), (uint8_t)(GatewayIP[1].toInt()), (uint8_t)(GatewayIP[2].toInt()), (uint8_t)(GatewayIP[3].toInt()));
      IPAddress subnet(255, 255, 255, 0);
      IPAddress primaryDNS(8, 8, 8, 8); //optional
      IPAddress secondaryDNS(8, 8, 4, 4); //optional
      if (!ETH.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) 
      {
        Serial.printf("\r\nETH Failed to configure");
      }
      if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) 
      {
        Serial.printf("\r\nWifi Failed to configure");
      }
    }

  mb.begin();
  add_resgister_TCP_RTU(); 
}
int count=0, count_fail=0;
void loop() 
{
  if(WiFi.status() != WL_CONNECTED&&!eth_connected)
  {
    digitalWrite(LED1,HIGH);
  }
  if(!eth_connected) wifiMulti.run();
  if(our_board_is_master==0)
  {
    slave.run();
    move_RTU_to_TCP(); 
  }
  mb.task();
  timerWrite(timer, 0);        
  MySettings.Configure();
  delay(10);
  count++;
  if(count%20==0)
  digitalWrite(LED2, !digitalRead(LED2));
  if(digitalRead(LED1)==HIGH)
  {
    count_fail++;
    if(count_fail>=12000)
    {
      Serial.printf("\r\nReset bcuz count over\r\n");
      esp_restart_noos();
    }
  }
    else
    count_fail=0;
  
} 
int FC=0;
extern String data_modbus;
int address_modbus, type_address_modbus, id_modbus;
bool Read_modbus( int option)
{
  if(data_modbus != "")
  {
  unsigned int tach=0, tach0=0, index0=0, index=0;
  String SubStr="";
  unsigned int leng=data_modbus.length();
  String data_id1, data_id2, data_id3, data_id4;
  for(int i0=0;i0<leng;i0++)
    {  
      if(data_modbus.charAt(i0)=='&')//tung ID
      {
        SubStr=data_modbus.substring(tach0,i0);
        tach0=i0+1; index0++;
        if((index0==1)&&(option==id1))
        {
          index=0;
          data_id1=SubStr;
           for(int i=0;i<leng;i++)
            {  
              if(data_id1.charAt(i)=='\n')
              {
                SubStr=data_id1.substring(tach,i);
                tach=i+1; index++;
                if(index>=1)
                {
                   unsigned int tach2=0, index2=0;
                   String SubStr2="";
                   unsigned int leng2=SubStr.length();
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
                          node.begin(id_modbus, Serial_kitdev);
                          uint8_t result;                    
                          if((address_modbus/10000==6)&&(FC==6))
                          {
                            //Serial.printf("\r\nWrite id %d, address %d", id_modbus, address_modbus); 
                            if(type_address_modbus==0)
                            node.writeSingleRegister(address_modbus%10000,mb.Hreg(address_modbus%10000));//FC06, write 40001 value 2,
                            else if(type_address_modbus==1)
                            {
                             node.writeSingleRegister(address_modbus%10000,mb.Hreg(address_modbus%10000));//FC06, write 40001 value 2,
                             node.writeSingleRegister(address_modbus%10000+1,mb.Hreg(address_modbus%10000+1));//FC06, write 40001 value 2, 
                            }
                          }
                          else
                          {
                            if(address_modbus/10000==4)
                            result = node.readHoldingRegisters(address_modbus%10000, type_address_modbus+1);
                            else if(address_modbus/10000==3)
                            result = node.readInputRegisters(address_modbus%10000, type_address_modbus+1);
                            delay(5); 
                            //Serial.printf("\r\nRead id %d, address %d. FC: %d", id_modbus, address_modbus, address_modbus/10000);
                            if (result == node.ku8MBSuccess) //Read ok
                            {
                              //Serial.printf("\r\nRead success"); 
                              digitalWrite(LED1, LOW);
                              if(type_address_modbus==0)
                              {
                                int int_value=node.getResponseBuffer(0);
                                if(address_modbus/10000==4)
                                  mb.Hreg(address_modbus%10000, int_value);
                                else if (address_modbus/10000==3)
                                  mb.Ireg(address_modbus%10000, int_value);
                              }
                              else if(type_address_modbus==1)
                              {
                                 uint16_t data[2];
                                 data[0]=node.getResponseBuffer(0);
                                 data[1]=node.getResponseBuffer(1); 
                                 if(address_modbus/10000==4)
                                 { 
                                  mb.Hreg(address_modbus%10000, data[0]);
                                  mb.Hreg(address_modbus%10000+1, data[1]);
                                 }
                                 else if (address_modbus/10000==3)
                                 {
                                  mb.Ireg(address_modbus%10000, data[0]);
                                  mb.Ireg(address_modbus%10000+1, data[1]);
                                 }
                              }  
                            }
                            else
                            {
                              digitalWrite(LED1, HIGH);
                              Serial.printf("\r\nRead fail address %d id %d", address_modbus, id_modbus); 
                              return false;
                            }
                          }
                        }
                      }   
                    }
                  }
              } 
          }
        }
        if((index0==2)&&(option==id2))
        {
          index=0;
          data_id1=SubStr;
           for(int i=0;i<leng;i++)
            {  
              if(data_id1.charAt(i)=='\n')
              {
                SubStr=data_id1.substring(tach,i);
                tach=i+1; index++;
                if(index>=1)
                {
                   unsigned int tach2=0, index2=0;
                   String SubStr2="";
                   unsigned int leng2=SubStr.length();
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
                            node.begin(id_modbus, Serial_kitdev);
                            uint8_t result;                    
                            if((address_modbus/10000==6)&&(FC==6))
                            {
                              //Serial.printf("\r\nWrite id %d, address %d", id_modbus, address_modbus); 
                              if(type_address_modbus==0)
                              node.writeSingleRegister(address_modbus%10000,mb.Hreg(address_modbus%10000));//FC06, write 40001 value 2,
                              else if(type_address_modbus==1)
                              {
                               node.writeSingleRegister(address_modbus%10000,mb.Hreg(address_modbus%10000));//FC06, write 40001 value 2,
                               node.writeSingleRegister(address_modbus%10000+1,mb.Hreg(address_modbus%10000+1));//FC06, write 40001 value 2, 
                              }
                            }
                            else
                            {
                              if(address_modbus/10000==4)
                              result = node.readHoldingRegisters(address_modbus%10000, type_address_modbus+1);
                              else if(address_modbus/10000==3)
                              result = node.readInputRegisters(address_modbus%10000, type_address_modbus+1);
                              delay(5);
                              //Serial.printf("\r\nRead id %d, address %d. FC: %d", id_modbus, address_modbus, address_modbus/10000);  
                              if (result == node.ku8MBSuccess) //Read ok
                              {
                                digitalWrite(LED1, LOW);
                                if(type_address_modbus==0)
                                {
                                  int int_value=node.getResponseBuffer(0);
                                  if(address_modbus/10000==4)
                                    mb.Hreg(address_modbus%10000, int_value);
                                  else if (address_modbus/10000==3)
                                    mb.Ireg(address_modbus%10000, int_value);
                                }
                                else if(type_address_modbus==1)
                                {
                                   uint16_t data[2];
                                   data[0]=node.getResponseBuffer(0);
                                   data[1]=node.getResponseBuffer(1); 
                                   if(address_modbus/10000==4)
                                   { 
                                    mb.Hreg(address_modbus%10000, data[0]);
                                    mb.Hreg(address_modbus%10000+1, data[1]);
                                   }
                                   else if (address_modbus/10000==3)
                                   {
                                    mb.Ireg(address_modbus%10000, data[0]);
                                    mb.Ireg(address_modbus%10000+1, data[1]);
                                   }
                                }  
                                //Serial.printf("\r\nRead success");  
                              }
                              else
                              {
                                digitalWrite(LED1, HIGH);
                                Serial.printf("\r\nRead fail address %d id %d", address_modbus, id_modbus); 
                                return false;
                              }
                            }
                          }
                        }   
                      }
                    }
                } 
            }
        }
        if((index0==3)&&(option==id3))
        {
          index=0;
          data_id1=SubStr;
           for(int i=0;i<leng;i++)
            {  
              if(data_id1.charAt(i)=='\n')
              {
                SubStr=data_id1.substring(tach,i);
                tach=i+1; index++;
                if(index>=1)
                {
                   unsigned int tach2=0, index2=0;
                   String SubStr2="";
                   unsigned int leng2=SubStr.length();
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
                            node.begin(id_modbus, Serial_kitdev);
                            uint8_t result;                    
                            if(address_modbus/10000==6)
                            {
                              if(type_address_modbus==0)
                              node.writeSingleRegister(address_modbus%10000,mb.Hreg(address_modbus%10000));//FC06, write 40001 value 2,
                              else if(type_address_modbus==1)
                              {
                               node.writeSingleRegister(address_modbus%10000,mb.Hreg(address_modbus%10000));//FC06, write 40001 value 2,
                               node.writeSingleRegister(address_modbus%10000+1,mb.Hreg(address_modbus%10000+1));//FC06, write 40001 value 2, 
                              }
                            }
                            else
                            {
                              if(address_modbus/10000==4)
                              result = node.readHoldingRegisters(address_modbus%10000, type_address_modbus+1);
                              else if(address_modbus/10000==3)
                              result = node.readInputRegisters(address_modbus%10000, type_address_modbus+1);
                              delay(5); 
                              if (result == node.ku8MBSuccess) //Read ok
                              {
                                digitalWrite(LED1, LOW);
                                if(type_address_modbus==0)
                                {
                                  int int_value=node.getResponseBuffer(0);
                                  if(address_modbus/10000==4)
                                    mb.Hreg(address_modbus%10000, int_value);
                                  else if (address_modbus/10000==3)
                                    mb.Ireg(address_modbus%10000, int_value);
                                }
                                else if(type_address_modbus==1)
                                {
                                   uint16_t data[2];
                                   data[0]=node.getResponseBuffer(0);
                                   data[1]=node.getResponseBuffer(1); 
                                   if(address_modbus/10000==4)
                                   { 
                                    mb.Hreg(address_modbus%10000, data[0]);
                                    mb.Hreg(address_modbus%10000+1, data[1]);
                                   }
                                   else if (address_modbus/10000==3)
                                   {
                                    mb.Ireg(address_modbus%10000, data[0]);
                                    mb.Ireg(address_modbus%10000+1, data[1]);
                                   }
                                }  
                              }
                              else
                              {
                                digitalWrite(LED1, HIGH);
                                Serial.printf("\r\nRead fail address %d id %d", address_modbus, id_modbus); 
                                return false;
                                
                              }
                            }
                          }
                        }   
                      }
                    }
                } 
            }
        }
        if((index0==4)&&(option==id4))
        {
          index=0;
          data_id1=SubStr;
           for(int i=0;i<leng;i++)
            {  
              if(data_id1.charAt(i)=='\n')
              {
                SubStr=data_id1.substring(tach,i);
                tach=i+1; index++;
                if(index>=1)
                {
                   unsigned int tach2=0, index2=0;
                   String SubStr2="";
                   unsigned int leng2=SubStr.length();
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
                            node.begin(id_modbus, Serial_kitdev);
                            uint8_t result;                    
                            if(address_modbus/10000==6)
                            {
                              if(type_address_modbus==0)
                              node.writeSingleRegister(address_modbus%10000,mb.Hreg(address_modbus%10000));//FC06, write 40001 value 2,
                              else if(type_address_modbus==1)
                              {
                               node.writeSingleRegister(address_modbus%10000,mb.Hreg(address_modbus%10000));//FC06, write 40001 value 2,
                               node.writeSingleRegister(address_modbus%10000+1,mb.Hreg(address_modbus%10000+1));//FC06, write 40001 value 2, 
                              }
                            }
                            else
                            {
                              if(address_modbus/10000==4)
                              result = node.readHoldingRegisters(address_modbus%10000, type_address_modbus+1);
                              else if(address_modbus/10000==3)
                              result = node.readInputRegisters(address_modbus%10000, type_address_modbus+1);
                              delay(5); 
                              if (result == node.ku8MBSuccess) //Read ok
                              {
                                digitalWrite(LED1, LOW);
                                if(type_address_modbus==0)
                                {
                                  int int_value=node.getResponseBuffer(0);
                                  if(address_modbus/10000==4)
                                    mb.Hreg(address_modbus%10000, int_value);
                                  else if (address_modbus/10000==3)
                                    mb.Ireg(address_modbus%10000, int_value);
                                }
                                else if(type_address_modbus==1)
                                {
                                   uint16_t data[2];
                                   data[0]=node.getResponseBuffer(0);
                                   data[1]=node.getResponseBuffer(1); 
                                   if(address_modbus/10000==4)
                                   { 
                                    mb.Hreg(address_modbus%10000, data[0]);
                                    mb.Hreg(address_modbus%10000+1, data[1]);
                                   }
                                   else if (address_modbus/10000==3)
                                   {
                                    mb.Ireg(address_modbus%10000, data[0]);
                                    mb.Ireg(address_modbus%10000+1, data[1]);
                                   }
                                }  
                              }
                              else
                              {
                                digitalWrite(LED1, HIGH);
                                Serial.printf("\r\nRead fail address %d id %d", address_modbus, id_modbus); 
                                return false;
                                
                              }
                            }
                          }
                        }   
                      }
                    }
                } 
            }
        }
        
      }
      //
    }
  }
  else
  {
    digitalWrite(LED1, !digitalRead(LED1));
    delay(1000);
  }
         
}
void add_resgister_TCP_RTU()
{
  if(data_modbus != "")
  {
  unsigned int tach=0, tach0=0, index0=0, index=0;
  String SubStr="";
  unsigned int leng=data_modbus.length();
  String data_id1, data_id2, data_id3, data_id4;
  for(int i0=0;i0<leng;i0++)
    {  
      if(data_modbus.charAt(i0)=='&')//tung ID
      {
        SubStr=data_modbus.substring(tach0,i0);
        tach0=i0+1; index0++;
        if(index0==1)
        {
          index=0;
          data_id1=SubStr;
           for(int i=0;i<leng;i++)
            {  
              if(data_id1.charAt(i)=='\n')
              {
                SubStr=data_id1.substring(tach,i);
                tach=i+1; index++;
                if(index>=1)
                {
                   unsigned int tach2=0, index2=0;
                   String SubStr2="";
                   unsigned int leng2=SubStr.length();
                   for(int i2=0;i2<leng2;i2++)
                    {  
                      if(SubStr.charAt(i2)=='@')
                      {
                        SubStr2=SubStr.substring(tach2,i2);
                        tach2=i2+1; index2++;
                        if(index2==1)
                        {
                          id_modbus=SubStr2.toInt();
                          id1=id_modbus;
                        }
                        else if(index2==2)
                        {
                          address_modbus=SubStr2.toInt();
                        }
                        else if(index2==3)
                        {
                         type_address_modbus=SubStr2.toInt();   
                        }
                      } 
                  }
                  if(address_modbus>=30000&&address_modbus<40000) 
                      {
                        if(type_address_modbus==0)
                        {
                          Serial.printf("\r\nAdd Ireg=%d", address_modbus%10000);
                          mb.addIreg(address_modbus%10000, 0);
                          regBank.add(address_modbus+1);
                        }
                        else
                        {
                          Serial.printf("\r\nAdd Ireg=%d", address_modbus%10000);
                          Serial.printf("\r\nAdd Ireg=%d", address_modbus%10000+1);
                          mb.addIreg(address_modbus%10000, 0);
                          mb.addIreg(address_modbus%10000+1, 0);
                          regBank.add(address_modbus+1);
                          regBank.add(address_modbus+2);
                        }
                      }
                      else if(address_modbus>=40000) 
                      {
                        if(type_address_modbus==0)
                        {
                          Serial.printf("\r\nAdd Hreg=%d, address_modbus: %d, index: %d", address_modbus%10000, address_modbus, index);
                          mb.addHreg(address_modbus%10000, 0);
                          regBank.add(address_modbus+1);
                        }
                        else
                        {
                          Serial.printf("\r\nAdd Hreg=%d", address_modbus%10000);
                          Serial.printf("\r\nAdd Hreg=%d", address_modbus%10000+1);
                          mb.addHreg(address_modbus%10000, 0);
                          mb.addHreg(address_modbus%10000+1, 0);
                          regBank.add(address_modbus+1);
                          regBank.add(address_modbus+2);
                        }
                      }
                }
              } 
            }
        }
        else if(index0==2)
        {
          index=0; tach=0;
          data_id2=SubStr;
           for(int i=0;i<leng;i++)
            {  
              if(data_id2.charAt(i)=='\n')
              {
                SubStr=data_id2.substring(tach,i);
                tach=i+1; index++;
                if(index>=1)
                {
                   unsigned int tach2=0, index2=0;
                   String SubStr2="";
                   unsigned int leng2=SubStr.length();
                   for(int i2=0;i2<leng2;i2++)
                    {  
                      if(SubStr.charAt(i2)=='@')
                      {
                        SubStr2=SubStr.substring(tach2,i2);
                        tach2=i2+1; index2++;
                        if(index2==1)
                        {
                          id_modbus=SubStr2.toInt();
                          id2=id_modbus;
                        }
                        else if(index2==2)
                        {
                          address_modbus=SubStr2.toInt();
                        }
                        else if(index2==3)
                        {
                         type_address_modbus=SubStr2.toInt();   
                        }
                      } 
                  }
                  if(address_modbus>=30000&&address_modbus<40000) 
                      {
                        if(type_address_modbus==0)
                        {
                          Serial.printf("\r\nAdd Ireg2=%d", address_modbus%10000);
                          mb.addIreg(address_modbus%10000, 0);
                          regBank.add(address_modbus+1);
                        }
                        else
                        {
                          Serial.printf("\r\nAdd Ireg2=%d", address_modbus%10000);
                          Serial.printf("\r\nAdd Ireg2=%d", address_modbus%10000+1);
                          mb.addIreg(address_modbus%10000, 0);
                          mb.addIreg(address_modbus%10000+1, 0);
                          regBank.add(address_modbus+1);
                          regBank.add(address_modbus+2);
                        }
                      }
                      else if(address_modbus>=40000) 
                      {
                        if(type_address_modbus==0)
                        {
                          Serial.printf("\r\nAdd Hreg2=%d, address_modbus: %d, index: %d", address_modbus%10000, address_modbus, index);
                          mb.addHreg(address_modbus%10000, millis());
                          regBank.add(address_modbus+1);
                        }
                        else
                        {
                          Serial.printf("\r\nAdd Hreg2=%d", address_modbus%10000);
                          Serial.printf("\r\nAdd Hreg2=%d", address_modbus%10000+1);
                          mb.addHreg(address_modbus%10000, 0);
                          mb.addHreg(address_modbus%10000+1, 0);
                          regBank.add(address_modbus+1);
                          regBank.add(address_modbus+2);
                        }
                      }
                }
              } 
            }
        }
        else if(index0==3)
        {
          index=0; tach=0;
          data_id2=SubStr;
           for(int i=0;i<leng;i++)
            {  
              if(data_id2.charAt(i)=='\n')
              {
                SubStr=data_id2.substring(tach,i);
                tach=i+1; index++;
                if(index>=1)
                {
                   unsigned int tach2=0, index2=0;
                   String SubStr2="";
                   unsigned int leng2=SubStr.length();
                   for(int i2=0;i2<leng2;i2++)
                    {  
                      if(SubStr.charAt(i2)=='@')
                      {
                        SubStr2=SubStr.substring(tach2,i2);
                        tach2=i2+1; index2++;
                        if(index2==1)
                        {
                          id_modbus=SubStr2.toInt();
                          id3=id_modbus;
                        }
                        else if(index2==2)
                        {
                          address_modbus=SubStr2.toInt();
                        }
                        else if(index2==3)
                        {
                         type_address_modbus=SubStr2.toInt();   
                        }
                      } 
                  }
                  if(address_modbus>=30000&&address_modbus<40000) 
                      {
                        if(type_address_modbus==0)
                        {
                          //Serial.printf("\r\nAdd Ireg3=%d", address_modbus%10000);
                          mb.addIreg(address_modbus%10000, 0);
                          regBank.add(address_modbus+1);
                        }
                        else
                        {
                          //Serial.printf("\r\nAdd Ireg3=%d", address_modbus%10000);
                          //Serial.printf("\r\nAdd Ireg3=%d", address_modbus%10000+1);
                          mb.addIreg(address_modbus%10000, 0);
                          mb.addIreg(address_modbus%10000+1, 0);
                          regBank.add(address_modbus+1);
                          regBank.add(address_modbus+2);
                        }
                      }
                      else if(address_modbus>=40000) 
                      {
                        if(type_address_modbus==0)
                        {
                          //Serial.printf("\r\nAdd Hreg3=%d, address_modbus: %d, index: %d", address_modbus%10000, address_modbus, index);
                          mb.addHreg(address_modbus%10000, millis());
                          regBank.add(address_modbus+1);
                        }
                        else
                        {
                          //Serial.printf("\r\nAdd Hreg3=%d", address_modbus%10000);
                          //Serial.printf("\r\nAdd Hreg3=%d", address_modbus%10000+1);
                          mb.addHreg(address_modbus%10000, 0);
                          mb.addHreg(address_modbus%10000+1, 0);
                          regBank.add(address_modbus+1);
                          regBank.add(address_modbus+2);
                        }
                      }
                }
              } 
            }
        }
        else if(index0==4)
        {
          index=0; tach=0;
          data_id2=SubStr;
           for(int i=0;i<leng;i++)
            {  
              if(data_id2.charAt(i)=='\n')
              {
                SubStr=data_id2.substring(tach,i);
                tach=i+1; index++;
                if(index>=1)
                {
                   unsigned int tach2=0, index2=0;
                   String SubStr2="";
                   unsigned int leng2=SubStr.length();
                   for(int i2=0;i2<leng2;i2++)
                    {  
                      if(SubStr.charAt(i2)=='@')
                      {
                        SubStr2=SubStr.substring(tach2,i2);
                        tach2=i2+1; index2++;
                        if(index2==1)
                        {
                          id_modbus=SubStr2.toInt();
                          id4=id_modbus;
                        }
                        else if(index2==2)
                        {
                          address_modbus=SubStr2.toInt();
                        }
                        else if(index2==3)
                        {
                         type_address_modbus=SubStr2.toInt();   
                        }
                      } 
                  }
                  if(address_modbus>=30000&&address_modbus<40000) 
                      {
                        if(type_address_modbus==0)
                        {
                          //Serial.printf("\r\nAdd Ireg4=%d", address_modbus%10000);
                          mb.addIreg(address_modbus%10000, 0);
                          regBank.add(address_modbus+1);
                        }
                        else
                        {
                          //Serial.printf("\r\nAdd Ireg4=%d", address_modbus%10000);
                          //Serial.printf("\r\nAdd Ireg4=%d", address_modbus%10000+1);
                          mb.addIreg(address_modbus%10000, 0);
                          mb.addIreg(address_modbus%10000+1, 0);
                          regBank.add(address_modbus+1);
                          regBank.add(address_modbus+2);
                        }
                      }
                      else if(address_modbus>=40000) 
                      {
                        if(type_address_modbus==0)
                        {
                          //Serial.printf("\r\nAdd Hreg4=%d, address_modbus: %d, index: %d", address_modbus%10000, address_modbus, index);
                          mb.addHreg(address_modbus%10000, millis());
                          regBank.add(address_modbus+1);
                        }
                        else
                        {
                          //Serial.printf("\r\nAdd Hreg4=%d", address_modbus%10000);
                          //Serial.printf("\r\nAdd Hreg4=%d", address_modbus%10000+1);
                          mb.addHreg(address_modbus%10000, 0);
                          mb.addHreg(address_modbus%10000+1, 0);
                          regBank.add(address_modbus+1);
                          regBank.add(address_modbus+2);
                        }
                      }
                }
              } 
            }
        }
      }
    }
        
  }       
}
void init_MB()
{
  Serial_kitdev.begin(MySettings.ModbusBaud.toInt(), MySettings.ModbusUARTConfig); 
  slave._device = &regBank;  
  delay(10);
  slave.setBaud(MySettings.ModbusBaud.toInt());
  regBank.setId(id_modbus);
}
void IRAM_ATTR resetModule() //timer watchdog overflow
{
    Serial.printf("\r\nReset bcuz WDT\r\n");
    esp_restart_noos();
}
void init_WDT()
{
  timer = timerBegin(0, 80, true); //timer 0, div 80
  timerAttachInterrupt(timer, &resetModule, true);
  timerAlarmWrite(timer,30000*wdtTimeout, false); //set time reset neu khong xoa dc co watchdog 
  timerAlarmEnable(timer); //enable interrupt
}
void move_RTU_to_TCP()
{
  if(data_modbus != "")
  {
  unsigned int tach=0, tach0=0, index0=0, index=0;
  String SubStr="";
  unsigned int leng=data_modbus.length();
  String data_id1, data_id2, data_id3, data_id4;
  for(int i0=0;i0<leng;i0++)
    {  
      if(data_modbus.charAt(i0)=='&')//tung ID
      {
        SubStr=data_modbus.substring(tach0,i0);
        tach0=i0+1; index0++;
        if(index0==1)
        {
          index=0;
          data_id1=SubStr;
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
                        }
                      } 
                  }
                  if(address_modbus>=30000&&address_modbus<40000) 
                      {
                        if(type_address_modbus==0)
                        {
                          mb.Ireg(address_modbus%10000, regBank.get(address_modbus+1));
                        }
                        else
                        {
                          mb.Ireg(address_modbus%10000, regBank.get(address_modbus+1));
                          mb.Ireg(address_modbus%10000+1, regBank.get(address_modbus+2));
                        }
                      }
                      else if(address_modbus>=40000) 
                      {
                        if(type_address_modbus==0)
                        {
                          mb.Hreg(address_modbus%10000, regBank.get(address_modbus+1));
                        }
                        else
                        {
                          mb.Hreg(address_modbus%10000, regBank.get(address_modbus+1));
                          mb.Hreg(address_modbus%10000+1, regBank.get(address_modbus+2));   
                        }
                      }
                }
              } 
            }
        }
        else if(index0==2)
        {
          index=0;
          data_id1=SubStr;
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
                        }
                      } 
                  }
                  if(address_modbus>=30000&&address_modbus<40000) 
                      {
                        if(type_address_modbus==0)
                        {
                          mb2.Ireg(address_modbus%10000, regBank.get(address_modbus+1));
                        }
                        else
                        {
                          mb2.Ireg(address_modbus%10000, regBank.get(address_modbus+1));
                          mb2.Ireg(address_modbus%10000+1, regBank.get(address_modbus+2));
                        }
                      }
                      else if(address_modbus>=40000) 
                      {
                        if(type_address_modbus==0)
                        {
                          mb2.Hreg(address_modbus%10000, regBank.get(address_modbus+1));
                        }
                        else
                        {
                          mb2.Hreg(address_modbus%10000, regBank.get(address_modbus+1));
                          mb2.Hreg(address_modbus%10000+1, regBank.get(address_modbus+2));   
                        }
                      }
                }
              } 
            }
        }
      }
    }
        
  }       
}
