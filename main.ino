//PROJETO BASE
#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
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
static unsigned long deviceId;
static uint32_t systemstatus = 0;
static int velocidadePorta = 9600;
static int tipodedados = SERIAL_8N1;
static int funcao = 0;
static int slaveId = 0;
static unsigned long tagId = 0;
static int enderecoId = 0;
static uint32_t writeValue = 0;
static int tamanho = 1;
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
  //deviceId = readStringFromEEPROM(0).toLong();
  deviceId = atol(readStringFromEEPROM(0).c_str());

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
    tagId = atol(jsonExtract(last, "t").c_str());
    enderecoId = jsonExtract(last, "g").toInt();
    funcao = jsonExtract(last, "x").toInt();
    writeValue = jsonExtract(last, "w").toInt();
    tamanho = jsonExtract(last, "s").toInt();
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

  if (Ethernet.linkStatus() != 1) {
    if (client) {
      client.stop();
    }
    internetStatus = 0;
  }

  // if (Ethernet.begin(mac) != 0) {

  // } else {
  //   if (client) {
  //     client.stop();
  //   }
  //   internetStatus = 0;
  // }
  
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
    if (client) {
      client.stop();
    }
    internetStatus = 0;
  }
}

void sendData(const char *msg, long valor)
{
  char prefix[200];
  snprintf(prefix, 200, "?var=%s&var2=%ld&id=%ld&tam=%d", msg, valor, tagId, tamanho);
  //ether.browseUrl(PSTR("/s.php"), prefix, website, my_callback);
  //delay(100);
  getURL(" /s.php", prefix);
  delay(100);
}


void getConfiguracao()
{
  char prefix[100];

  char mymac[20];

  snprintf(mymac, sizeof(mymac), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  snprintf(prefix, sizeof(prefix), "?id=%ld&mac=%s", deviceId, mymac);

  //snprintf(prefix, 100, "?id=%d", deviceId);
  //ether.browseUrl(PSTR(" /c.php"), prefix, website, my_callback);
  //delay(100);
  getURL(" /c.php", prefix);
  delay(100);
}

void sendReleyState()
{
  char prefix[100];
  snprintf(prefix, 100, "?state=%d&id=%ld", estadoRele, deviceId);
  //ether.browseUrl(PSTR("/r.php"), prefix, website, my_callback);
  getURL(" /r.php", prefix);
  delay(100);
  systemstatus = 0;
}

void captura()
{

  uint8_t result;
  uint16_t val0 = 0;
  uint16_t val1 = 0;
  uint16_t val2 = 0;
  uint16_t val3 = 0;
  uint16_t val4 = 0;
  uint16_t val5 = 0;
  uint16_t val6 = 0;
  uint16_t val7 = 0;
  char concatenatedStr[100]; 
  long concatenatedValue = 0;

  Serial.begin(velocidadePorta, tipodedados);

  while (!Serial)
  {
  }

  node.begin(slaveId, Serial);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  if (funcao == 1)
  {
    result = node.readCoils(enderecoId, tamanho);
  }
  else if (funcao == 2)
  {
    result = node.readDiscreteInputs(enderecoId, tamanho);
  }
  else if (funcao == 3)
  {
    result = node.readHoldingRegisters(enderecoId, tamanho);
  }
  else if (funcao == 4)
  {
    result = node.readInputRegisters(enderecoId, tamanho);
  }
  else if (funcao == 5) 
  {
    result = node.writeSingleCoil(enderecoId, writeValue);
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
    if (tamanho >= 1) val0 = node.getResponseBuffer(0);
    if (tamanho >= 2) val1 = node.getResponseBuffer(1);
    if (tamanho >= 3) val2 = node.getResponseBuffer(2);
    if (tamanho >= 4) val3 = node.getResponseBuffer(3);
    if (tamanho >= 5) val4 = node.getResponseBuffer(4);
    if (tamanho >= 6) val5 = node.getResponseBuffer(5);
    if (tamanho >= 7) val6 = node.getResponseBuffer(6);
    if (tamanho >= 8) val7 = node.getResponseBuffer(7);

    if (tamanho == 1) {
        snprintf(concatenatedStr, sizeof(concatenatedStr), "%d", val0);
    } else if (tamanho == 2) {
        snprintf(concatenatedStr, sizeof(concatenatedStr), "%d%d", val0, val1);
    } else if (tamanho == 4) {
        snprintf(concatenatedStr, sizeof(concatenatedStr), "%d%d%d%d", val0, val1, val2, val3);
    } else if (tamanho == 8) {
        snprintf(concatenatedStr, sizeof(concatenatedStr), "%d%d%d%d%d%d%d%d", val0, val1, val2, val3, val4, val5, val6, val7);
    }

    concatenatedValue = atol(concatenatedStr);

    sendData("1", concatenatedValue);
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
  // Tenta reconectar até 5 vezes com um delay de 1 segundo entre as tentativas.
  for(int attempt = 0; attempt < 5; attempt++) {
    Ethernet.begin(mac);
    if (Ethernet.linkStatus() == LinkON) {
      internetStatus = 1;
      return; // Sai da função se a reconexão for bem-sucedida.
    } else {
      delay(1000); // Espera um segundo antes de tentar novamente.
    }
  }
  internetStatus = 0; // Define o estado da internet como desconectado.
}


void setup()
{

  internetStatus = 0;
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

  //reconnectInternet();
}

void loop()
{
  // Verifica se o temporizador foi alcançado
  if (millis() > timer)
  {
    timer = millis() + intervalo;

    // Verifica o estado da internet
    if (internetStatus == 1) {
      // Gerencia as operações de acordo com systemstatus
      switch (systemstatus) {
        case 0:
          getConfiguracao();
          break;
        case 1:
          captura();
          break;
        case 2:
          sendReleyState();
          break;
        default:
          reconnectInternet();
          break;
      }
    } else {
      // Internet desconectada, tenta reconectar
      reconnectInternet();
    }
  }
}