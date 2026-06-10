#include "Nelektrik_board.h"
ModbusMaster node;

void setup()
{
  pinMode(25, OUTPUT);//led 1
  pinMode(27, OUTPUT);//led 2
  Serial.begin(115200);
  Serial.print("\r\n*****************KIT DEV ESP32*******************");
  Nelektrik_Serial.begin(9600, SERIAL_8N1); 
  node.begin(1, Nelektrik_Serial); //ID node 1
}
void loop()
{ 
///////////////////////////////////////////////DOC INPUT//////////////////////////////////////////
  Serial.println("");
  Serial.println("Wait Read");
  uint8_t result;
  result = node.readInputRegisters(2, 3);//Read 30002, 30003, 30004
  delay(10);
  if (result == node.ku8MBSuccess) //Read ok
  {
    uint16_t data[3];
    data[0]=node.getResponseBuffer(0);
    data[1]=node.getResponseBuffer(1);
    data[2]=node.getResponseBuffer(2);
    Serial.printf("\r\nValue 30002: %d",data[0]);  // readInputRegisters(2, ...) bat dau tu 30002
    Serial.printf("\r\nValue 30003: %d",data[1]);
    Serial.printf("\r\nValue 30004: %d",data[2]);
  }
  else Serial.print("Read input fail");
  
///////////////////////////////////////////////DOC HOLDING//////////////////////////////////////////
  result = node.readHoldingRegisters(1, 3);//Read 40001, 40002, 40003
  if (result == node.ku8MBSuccess) //Read ok
  {
    uint16_t data[3];
    data[0]=node.getResponseBuffer(0);
    data[1]=node.getResponseBuffer(1);
    data[2]=node.getResponseBuffer(2);
    Serial.printf("\r\nValue 40001: %d",data[0]);
    Serial.printf("\r\nValue 40002: %d",data[1]);
    Serial.printf("\r\nValue 40003: %d",data[2]);
  }
  else Serial.print("Read holding fail");
  
////////////////////////////////////////////////////GHI////////////////////////////////////////////////
  result = node.writeSingleRegister(1,5);//FC06, write 40001 value 5,
  if (result == node.ku8MBSuccess)
  {
    Serial.printf("\r\nSucces");
  }
  else
  {
    Serial.printf("\r\nFail");
  }
}
