/*
    LDTool - read and write LEGO Dimensions tags
    Copyright (C) 2022  Simon Sievert

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * Author: Simon Sievert
 * 
 * A tool that can write and read ntag213 nfc tags for use with Lego Dimensions.
 * 
 * Takes commands via serial.
 * Use the serial monitor, set baud rate to 9600
 * and check below for available commands.
 * (should be self explanatory)
 * 
 * Things that work:
 * - password generation and authentification
 * - writing character tags with given id
 * - writing vehicle tags with given id
 * - identifying tag -> tag type + character id / vehicle id
 * 
 * Things that don't work:
 * - vehicle upgrades
 * 
 * 
 * Uses an RFID-RC522 reader
 * Uses MFRC522 Library (can be obtained through arduino library manager)
 * -----------------------------------------------------------------------------------------
 *             MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
 *             Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
 * Signal      Pin          Pin           Pin       Pin        Pin              Pin
 * -----------------------------------------------------------------------------------------
 * RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
 * SPI SS      SDA(SS)      10            53        D10        10               10
 * SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
 * SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
 * SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
 */

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

#include "LDTool.h"

#define RST_PIN 9   // Configurable, see typical pin layout above
#define SS_PIN 10   // Configurable, see typical pin layout above

//#define NO_CHIP_WRITE
#define NO_HELP_TEXT

MFRC522 nfc(SS_PIN, RST_PIN);

LDTool ld;

#define ADDR_UPGRADE_START 35
#define ADDR_CHARACTER_START 36
#define ADDR_TOKEN_TYPE 38
#define ADDR_PWD 43

#define SERIAL_BUF_SIZE 8

char serialBuf[SERIAL_BUF_SIZE];
uint16_t serialBufPos = 0;

// ##### available commands #####
const char* commandInfo = "info";
const char* commandDump = "dump";
const char* commandDumpNoAuth = "dumpna";
const char* commandCalculate = "calc";
const char* commandWriteCharacter = "wc";
const char* commandWritePasswordAndCharacter = "wpc";
const char* commandWriteVehicle = "wv";
const char* commandWritePasswordAndVehicle = "wpv";
const char* commandWritePassword = "wpwd";
const char* commandHelp = "help";

// ##### function definitions #####
void printHelp();
void printHex32(uint32_t* data, byte l);
void printHex(byte* data, byte l);
bool waitForChip();
bool verifyCorrectChip();
void dumpData(bool tryAuth);
bool auth(byte* pwd);
bool prepareChip();
bool prepareChipAuth();
void calculateStuff(uint32_t characterId);
bool writeCharacter(uint32_t characterId);
bool writeVehicle(uint32_t vehicleId, uint64_t upgrades);
bool writeData(int addr, byte* buf, byte pages);
bool writePwd();
void printInfo();

void setup() {
  Serial.begin(9600);
  SPI.begin();
  nfc.PCD_Init();
}

void loop() {
  while(Serial.available() > 0){
    char c = Serial.read();
    if(c != '\n'){
      if(serialBufPos < SERIAL_BUF_SIZE){
        serialBuf[serialBufPos] = c;
        serialBufPos++;
      }else{
        serialBufPos = 0;
        Serial.println("command too large for buffer");
      }
    }else{
      // search for first space in buffer
      int spacePos = -1;
      for(int i=0; i<serialBufPos; i++){
        if(serialBuf[i] == ' '){
          spacePos = i;
          break;
        }
      }

      int commandLength = serialBufPos;
      if(spacePos != -1){
        commandLength = spacePos;
      }

      char* command = new char[commandLength + 1];
      command[commandLength] = 0;
      memcpy(command, serialBuf, commandLength);

      int argumentLength = serialBufPos - commandLength - 1;
      bool hasArgument = argumentLength > 0;
      char* argument = new char[argumentLength + 1];
      argument[argumentLength] = 0;
      if(argumentLength > 0){
        memcpy(&argument[0], &serialBuf[commandLength + 1], argumentLength);
      }

      if(strcmp(command, commandInfo) == 0){
        Serial.println("> get info");
        printInfo();
      }else if(strcmp(command, commandDump) == 0){
        Serial.println("> dump chip memory");
        dumpData(true);
      }else if(strcmp(command, commandDumpNoAuth) == 0){
        Serial.println("> dump chip memory without auth");
        dumpData(false);
      }else if(strcmp(command, commandCalculate) == 0){
        Serial.println("> calculate encrypted caracter id");
        if(hasArgument){
          uint32_t characterId = atoi(argument);
          Serial.print("characterId: ");
          Serial.println(characterId);
          calculateStuff(characterId);
        }else{
          Serial.println("missing argument");
        }
      }else if(strcmp(command, commandWriteCharacter) == 0){
        Serial.println("> write character");
        if(hasArgument){
          uint32_t characterId = atoi(argument);
          Serial.print("characterId: ");
          Serial.println(characterId);
          writeCharacter(characterId);
        }else{
          Serial.println("missing argument");
        }
      }else if(strcmp(command, commandWritePasswordAndCharacter) == 0){
        Serial.println("> write password and character");
        if(hasArgument){
          Serial.println("> write password");
          if(writePwd()){
            Serial.println("> write character");
            uint32_t characterId = atoi(argument);
            Serial.print("characterId: ");
            Serial.println(characterId);
            writeCharacter(characterId);
          }
        }else{
          Serial.println("missing argument");
        }
      }else if(strcmp(command, commandWriteVehicle) == 0){
        Serial.println("> write vehicle");
        if(hasArgument){
          uint32_t vehicleId = atoi(argument);
          Serial.print("vehicleId: ");
          Serial.println(vehicleId);
          writeVehicle(vehicleId, 0);
        }else{
          Serial.println("missing argument");
        }
      }else if(strcmp(command, commandWritePasswordAndVehicle) == 0){
        Serial.println("> write password and vehicle");
        if(hasArgument){
          Serial.println("> write password");
          if(writePwd()){
            Serial.println("> write vehicle");
            uint32_t vehicleId = atoi(argument);
            Serial.print("vehicleId: ");
            Serial.println(vehicleId);
            writeVehicle(vehicleId, 0);
          }
        }else{
          Serial.println("missing argument");
        }
      }else if(strcmp(command, commandWritePassword) == 0){
        Serial.println("> write password");
        writePwd();
      }else if(strcmp(command, commandHelp) == 0){
        Serial.println("> print help");
        printHelp();
      }else{
        Serial.println("> unknown command");
        printHelp();
      }

      delete[] command;
      delete[] argument;

      // reset serial buf pos
      serialBufPos = 0;
    }
  }
  delay(100);
}

void printHelp(){
#ifndef NO_HELP_TEXT
  Serial.println("available commands:");
  Serial.println("info - get uid, password and character-/vehicleId (if programmed) from a chip");
  Serial.println("wpwd - write password");
  Serial.println("wc [characterId] - write character (password needs to already be written)");
  Serial.println("wpc [characterID] - write password and character");
  Serial.println("wv [vehicleID] - write vehicle (password needs to already be written)");
  Serial.println("wpv [vehicleID] - write password and vehicle");
  Serial.println("calc [characterID] - calculate encrypted character id for given chip");
  Serial.println("dump - dump entire chip memory (will attempt to authenticate with password)");
  Serial.println("dumpna - dump entire chip memory without authenticating with password first");
  Serial.println("help - print this help text");
#endif
}

void printHex32(uint32_t* data, byte l){
  for(byte i=0; i<l; i++){

    if(i > 0 && i%2 == 1){
      Serial.print(" ");
    }
    
    if(data[i] < 16){
      Serial.print("0000000");
    }else if(data[i] < 256){
      Serial.print("000000");
    }else if(data[i] < 4096){
      Serial.print("00000");
    }else if(data[i] < 65536){
      Serial.print("0000");
    }else if(data[i] < 1048576){
      Serial.print("000");
    }else if(data[i] < 16777216){
      Serial.print("00");
    }else if(data[i] < 268435456){
      Serial.print("0");
    }
    Serial.print(data[i], HEX);
  }
  Serial.println();
}

void printHex(byte* data, byte l){
  for(byte i=0; i<l; i++){
    if(data[i] < 16){
      Serial.print("0");
    }
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}


bool waitForChip(){
  bool found = false;
  bool printedSomething = false;
  for(byte i=0; i<10; i++){
    if(nfc.PICC_IsNewCardPresent() && nfc.PICC_ReadCardSerial()){
      found = true;
      break;
    }else{
      if(i == 0){
         Serial.print("waiting for chip ");
         printedSomething = true;
      }
      delay(200);
      Serial.print(".");
    }
  }
  if(printedSomething){
    Serial.println();
  }
  if(!found){
    Serial.println("timeout");
  }
  return found;
}

bool verifyCorrectChip(){
  MFRC522::StatusCode status;
  
  byte buf[18];
  byte len = 18;
  memset(buf, 0, len);
  int block = 0;

  status = nfc.MIFARE_Read(block, buf, &len);
  if(status == MFRC522::STATUS_OK){

    // check if chip was manufactured by NXP Semiconductors
    if(buf[0] != 0x04){
      return false;
    }
    
    // check if chip has 144 byte memory
    if(buf[14] != 0x12){
      return false;
    }

    return true;
  }
  return false;
}

void dumpData(bool tryAuth){
  if(prepareChip()){
    if(tryAuth){
      byte pwd[4];
      ld.genPwd(nfc.uid.uidByte, pwd);
      Serial.print("pwd: ");
      printHex(pwd, 4);
      if(auth(pwd)){
        Serial.println("auth ok");
      }else{
        Serial.println("auth failed");
      }
    }else{
      Serial.println("auth disabled");
    }

    byte buf[18];
    byte len = 18;
    memset(buf, 0, len);
    int block = 0;

    bool keepReading = true;
    while(keepReading){
      memset(buf, 0, len);
      byte readLen = len;
      MFRC522::StatusCode status;
      status = nfc.MIFARE_Read(block, buf, &readLen);
      if(status == MFRC522::STATUS_OK){
        if(readLen > 0){
          for(byte i=0; i<readLen; i++){
            if(i >= 16){
              break;
            }
            int current = block + i/4;
            if(current <= 44){
              if(i%4 == 0){
                Serial.print(current);
                Serial.print("\t");
              }
              if(buf[i] < 16){
                Serial.print("0");
              }
              Serial.print(buf[i], HEX);
              if(i%4 == 3){
                Serial.println();
              }
            }
          }
          block += 4;
          if(block > 44){
            keepReading = false;
          }
        }else{
          keepReading = false;
        }
      }else{
        //Serial.println("failed to read from chip");
        keepReading = false;
        //Serial.print("at block: ");
        //Serial.println(block);
      }
    }
    Serial.println();
  }
}

bool auth(byte* pwd){
  byte response[2];
  memset(response, 0, 2);

  MFRC522::StatusCode status;
  status = nfc.PCD_NTAG216_AUTH(&pwd[0], response);
  if(status != MFRC522::STATUS_OK){
    Serial.print("authentication failed: ");
    Serial.println(nfc.GetStatusCodeName(status));
    return false;
  }
  return true;
}

bool prepareChip(){
  if(!waitForChip()){
    return false;
  }
  if(!verifyCorrectChip()){
    Serial.println("unknown chip");
    return false;
  }
  return true;
}

bool prepareChipAuth(){
  if(!prepareChip()){
    return false;
  }
  byte pwd[4];
  ld.genPwd(nfc.uid.uidByte, pwd);
  if(!auth(pwd)){
    return false;
  }
  return true;
}

void calculateStuff(uint32_t characterId){
  if(prepareChip()){
    // encrypt character id
    uint32_t buf[2];
    ld.encrypt(nfc.uid.uidByte, characterId, buf);
    
    Serial.print("encrypted character id: ");
    printHex32(buf, 2);
    Serial.println("should be on pages 36 and 37");
  }
}

bool writeCharacter(uint32_t characterId){
  if(!prepareChipAuth()){
    return false;
  }
  
  // encrypt character id
  uint32_t character[2];
  ld.encrypt(nfc.uid.uidByte, characterId, character);
  
  byte buf[16];
  memset(buf, 0, 16);

  // copy character to the place that we will write to pages 36 and 37
  for(byte i=0; i<2; i++){
    buf[i*4 + 4] = character[i] >> 24;
    buf[i*4 + 5] = character[i] >> 16;
    buf[i*4 + 6] = character[i] >> 8;
    buf[i*4 + 7] = character[i];
  }

  for(byte i=0; i<4; i++){
    printHex(&buf[i*4], 4);
  }
  
#ifdef NO_CHIP_WRITE
  Serial.println("not actually writing stuff");
  return false;
#endif

  // start writing from page 35
  if(!writeData(ADDR_UPGRADE_START, buf, 4)){
    Serial.println("write character failed");
    return false;
  }
  Serial.println("write character ok");
  return true;
}

bool writeVehicle(uint32_t vehicleId, uint64_t upgrades){
  if(!prepareChipAuth()){
    return false;
  }
  
  byte buf[16];
  memset(buf, 0, 16);

  // set vehicle id
  buf[4] = vehicleId;
  buf[5] = vehicleId >> 8;
  buf[6] = vehicleId >> 16;
  buf[7] = vehicleId >> 24;

  // set type to vehicle
  buf[13] = 0x01;

  // set upgrades
  // ToDo

  for(byte i=0; i<4; i++){
    printHex(&buf[i*4], 4);
  }
  
#ifdef NO_CHIP_WRITE
  Serial.println("not actually writing stuff");
  return false;
#endif

  // start writing from page 35
  if(!writeData(ADDR_UPGRADE_START, buf, 4)){
    Serial.println("write vehicle failed");
    return false;
  }
  Serial.println("write vehicle ok");
  return true;
}

// buf should hold pages * 4 bytes of data
bool writeData(int addr, byte* buf, byte pages){
  MFRC522::StatusCode status;
  for(byte i=0; i<pages; i++){
    // for some reason we have to tell it to write 16 bytes,
    // but it will only ever write 4 (only to the given page)
    // so this is ok and will not overwrite any config stuff
    // or random stuff from memory
    status = nfc.MIFARE_Write(addr + i, &buf[i*4], 16);
    if(status != MFRC522::STATUS_OK){
      return false;
    }
  }
  return true;
}

bool writePwd(){
  if(!prepareChip()){
    return false;
  }
  
  byte pwd[4];
  memset(pwd, 0, 4);
  ld.genPwd(nfc.uid.uidByte, pwd);

  Serial.print("pwd: ");
  printHex(pwd, 4);
  
#ifdef NO_CHIP_WRITE
  Serial.println("not actually writing stuff");
  return false;
#endif

  if(!writeData(ADDR_PWD, pwd, 1)){
    Serial.println("write password failed");
    return false;
  }
  Serial.println("write password ok");
  return true;
}

void printInfo(){
  if(!prepareChip()){
    return;
  }

  Serial.print("uid: ");
  printHex(nfc.uid.uidByte, 7);

  // authenticate
  {
    byte pwd[4];
    ld.genPwd(nfc.uid.uidByte, pwd);
    Serial.print("pwd: ");
    printHex(pwd, 4);
    if(!auth(pwd)){
      return;
    }
  }
  
  MFRC522::StatusCode status;
  
  byte buf[18];
  byte len = 18;
  memset(buf, 0, len);
  int block = 35;

  status = nfc.MIFARE_Read(block, buf, &len);
  if(status == MFRC522::STATUS_OK){
    bool isVehicle = buf[13] == 0x01;
    Serial.print("tag type: ");
    if(!isVehicle){
      Serial.println("character");
      uint32_t characterPages[2];
      for(byte i=0; i<2; i++){
        characterPages[i] = ((uint32_t)buf[i*4 + 4] << 24) | ((uint32_t)buf[i*4 + 5] << 16) | ((uint32_t)buf[i*4 + 6] << 8) | (uint32_t)buf[i*4 + 7];
      }
      uint32_t characterId = ld.decryptCharacterPages(nfc.uid.uidByte, characterPages);
      Serial.print("id: ");
      Serial.println(characterId);
    }else{
      Serial.println("vehicle");
      uint32_t vehicleId = (uint32_t)buf[4] | ((uint32_t)buf[5] << 8) | ((uint32_t)buf[6] << 16) | ((uint32_t)buf[7] << 24);
      Serial.print("id: ");
      Serial.println(vehicleId);
    }
  }else{
    Serial.println("failed to read page 35 to 38");
  }
}
