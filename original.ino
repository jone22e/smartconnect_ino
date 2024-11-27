//PROJETO BASE
#include <Arduino.h>
#include <Ethernet2.h>
#include <ModbusMaster.h>
#include <EEPROM.h>
#include <jsonlib.h>

#define DE 7
#define RE 6
#define RELE 5

ModbusMaster node;
static uint32_t timer;
char server[] = "spa.brasiltec.ind.br";
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
static int deviceId;
static uint32_t systemstatus = 0;
static int velocidadePorta = 9600;
static int tipodedados = SERIAL_8N1;
static int funcao = 0;
static int slaveId = 0;
static int tagId = 0;
static int enderecoId = 0;
static uint32_t writeValue = 0;
static int intervalo = 5000;
static int estadoRele = 0;
static int internetStatus = 0;

byte value;
byte randNumber;

EthernetClient client;

void writeStringToEEPROM(int addrOffset, const String &strToWrite)
{
  byte len = strToWrite.length();
  EEPROM.write(addrOffset, len);
  for (int i = 0; i < len; i++)
  {
    EEPROM.write(addrOffset + 1 + i, strToWrite[i]);
  }
}

String readStringFromEEPROM(int addrOffset)
{
  int newStrLen = EEPROM.read(addrOffset);
  char data[newStrLen + 1];
  for (int i = 0; i < newStrLen; i++)
  {
    data[i] = EEPROM.read(addrOffset + 1 + i);
  }
  data[newStrLen] = '\0';
  return String(data);
}

void configurar()
{
  deviceId = readStringFromEEPROM(0).toInt();
  intervalo = readStringFromEEPROM(15).toInt();
  if (intervalo<500) {
    intervalo = 5000;
  }
  if (intervalo>10000) {
    intervalo = 5000;
  }
  estadoRele = readStringFromEEPROM(20).toInt();
  if (estadoRele>1) {
    estadoRele = 1;
  }
}

void preTransmission()
{
  /*if (velocidadePorta<=2400) {
    delay(10);
  }*/
  digitalWrite(DE, 1);
  digitalWrite(RE, 1);
}

void postTransmission()
{
  /*
  if (velocidadePorta<=2400) {
    delay(10);
  }*/
  digitalWrite(DE, 0); //1 simulador funcionando
  digitalWrite(RE, 0); //0
}

//static void my_callback(byte status, word off, word len)
static void my_callback(String last)
{
  //Ethernet::buffer[off + 300] = 0;
  //char *token, *last;
  //last = token = strtok((const char *)Ethernet::buffer + off, "|");
  //for (; (token = strtok(NULL, "/")) != NULL; last = token)
  //  ;

  if (jsonExtract(last, "f") == "0")
  {
    //registrar
    writeStringToEEPROM(0, jsonExtract(last, "id"));
    configurar();
  }
  else if (jsonExtract(last, "f") == "9")
  {
    //desvincular
    deviceId = 0;
    writeStringToEEPROM(0, "0");
    configurar();
  }
  else if (jsonExtract(last, "f") == "1")
  {
    //solicitar leitura
    velocidadePorta = jsonExtract(last, "v").toInt();
    tipodedados = jsonExtract(last, "d").toInt();
    slaveId = jsonExtract(last, "e").toInt();
    tagId = jsonExtract(last, "t").toInt();
    enderecoId = jsonExtract(last, "g").toInt();
    funcao = jsonExtract(last, "x").toInt();
    writeValue = jsonExtract(last, "w").toInt();
    systemstatus = 1;
  }
  else if (jsonExtract(last, "f") == "2")
  {
    intervalo = jsonExtract(last, "i").toInt();
    if (intervalo < 500)
    {
      intervalo = 5000;
    }
    if (intervalo>10000) {
      intervalo = 5000;
    }
    writeStringToEEPROM(15, (String) intervalo);
  }
  else if (jsonExtract(last, "f") == "3")
  {
    estadoRele = jsonExtract(last, "w").toInt();
    writeStringToEEPROM(20, (String) estadoRele);
    digitalWrite(RELE, estadoRele);
  }
  else if (jsonExtract(last, "f") == "4")
  {
    systemstatus = 2;
  }
}

void getURL(String link, String params) {

  if (client.connect(server, 80)) {
    client.print("GET");
    client.print(link);
    client.print(params);
    client.println(" HTTP/1.1");
    client.println("Host: spa.brasiltec.ind.br");
    client.println("Connection: close");
    client.println();


    bool nextIsBody = false;
    String fullHeader = "";
    String fullBody = "";
    while(client.connected()) {
      while(client.available()) {
        char c = client.read();
        if (c=='|') { nextIsBody = true; } else if (nextIsBody==true) {
          fullBody += c;
        } else {
          fullHeader += c;
        }
      }
    }
    client.stop();
    my_callback(fullBody);
  } else {
    client.stop();
    internetStatus = 0;
  }
}

void sendData(const char *msg, uint16_t valor)
{
  char prefix[100];
  snprintf(prefix, 100, "?var=%s&var2=%03d&id=%d", msg, valor, tagId);
  //ether.browseUrl(PSTR("/s.php"), prefix, website, my_callback);
  //delay(100);
  getURL(" /s.php", prefix);
  delay(100);
}

void getConfiguracao()
{
  char prefix[100];
  snprintf(prefix, 100, "?id=%d", deviceId);
  //ether.browseUrl(PSTR(" /c.php"), prefix, website, my_callback);
  //delay(100);
  getURL(" /c.php", prefix);
  delay(100);
}

void sendReleyState()
{
  char prefix[100];
  snprintf(prefix, 100, "?state=%d&id=%d", estadoRele, deviceId);
  //ether.browseUrl(PSTR("/r.php"), prefix, website, my_callback);
  getURL(" /r.php", prefix);
  delay(100);
  systemstatus = 0;
}

void captura()
{

  uint8_t result;

  Serial.begin(velocidadePorta, tipodedados);

  while (!Serial)
  {
  }

  node.begin(slaveId, Serial);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  if (funcao == 1)
  {
    result = node.readCoils(enderecoId, 1);
  }
  else if (funcao == 2)
  {
    result = node.readDiscreteInputs(enderecoId, 1);
  }
  else if (funcao == 3)
  {
    result = node.readHoldingRegisters(enderecoId, 1);
  }
  else if (funcao == 4)
  {
    result = node.readInputRegisters(enderecoId, 1);
  }
  else if (funcao == 6)
  {
    result = node.writeSingleRegister(enderecoId, writeValue);
  }
  else if (funcao == 10)
  {
    /*
    node.setTransmitBuffer(0, writeValue & 0xFFFF);
    node.setTransmitBuffer(1, (writeValue >> 16) & 0xFFFF);
    */
    node.setTransmitBuffer(0, lowWord(writeValue));
    node.setTransmitBuffer(1, highWord(writeValue));
    result = node.writeMultipleRegisters(enderecoId, 2);
  }
  else
  {
    sendData("10", 0);
  }

  if (result == node.ku8MBSuccess)
  {
    sendData("1", node.getResponseBuffer(0));
  }
  else
  {
    const char *tmpstr2;
    switch (result)
    {
    case node.ku8MBSuccess:
      tmpstr2 = "1";
      break;
    case node.ku8MBIllegalFunction:
      tmpstr2 = "2";
      break;
    case node.ku8MBIllegalDataAddress:
      tmpstr2 = "3";
      break;
    case node.ku8MBIllegalDataValue:
      tmpstr2 = "4";
      break;
    case node.ku8MBSlaveDeviceFailure:
      tmpstr2 = "5";
      break;
    case node.ku8MBInvalidSlaveID:
      tmpstr2 = "6";
      break;
    case node.ku8MBInvalidFunction:
      tmpstr2 = "7";
      break;
    case node.ku8MBResponseTimedOut:
      tmpstr2 = "8";
      break;
    case node.ku8MBInvalidCRC:
      tmpstr2 = "9";
      break;
    default:
      tmpstr2 = "10";
      break;
    }
    sendData(tmpstr2, 0);
  }

  node.clearResponseBuffer();

  Serial.flush();
  Serial.end();
  systemstatus = 0;
}

void reconnectInternet() {
  if (Ethernet.begin(mac) == 0) {
    //try again
    internetStatus = 0;
  } else {
    internetStatus = 1;
  }
}

void setup()
{
//  Serial.begin(9600);
//  while (!Serial) {
//    ;
//  }
//  Serial.println ("****MAC start***");
  randomSeed(analogRead(A5));//从悬空的A5引脚获取随机状态，用于随机数
  value = EEPROM.read(800);
  if(value==0xde){
//    Serial.println("There is already a MAC address.");
    }
  else{
    EEPROM.write(800, 0xde);
    for(int i=801;i<806;i++){
      randNumber = random(0, 255);
      EEPROM.write(i, randNumber);
      }
    }
  for(int i=0;i<6;i++){
      mac[i] = EEPROM.read(i+800);
//      Serial.print(mac[i],HEX);
//      Serial.print(":");
      }
//  Serial.println ("****end***");
//  sysClock(INT_OSC_32M);          // Set internal 32M oscillator
//  sysClockPrescale(SYSCLK_DIV_2); // set clock prescale 0 2 4 8 16 32 64 128
  //Serial.begin(velocidadePorta, tipodedados);
  pinMode(DE, OUTPUT);
  pinMode(RE, OUTPUT);
  pinMode(RELE, OUTPUT);
  postTransmission();

  configurar();

  digitalWrite(RELE, estadoRele);

  //ether.begin(sizeof Ethernet::buffer, mymac, SS);
  //ether.dhcpSetup();
  //ether.dnsLookup(website);

  reconnectInternet();
}

void loop()
{
  //ether.packetLoop(ether.packetReceive());
  if (millis() > timer)
  {
    timer = millis() + intervalo;

    if (internetStatus==1) {

      if (systemstatus == 0) {
        getConfiguracao();
      } else if (systemstatus == 1) {
        captura();
      } else if (systemstatus == 2) {
        sendReleyState();
      } else {
        //nothing
      }
    } else {
      reconnectInternet();
    }
  }


}
