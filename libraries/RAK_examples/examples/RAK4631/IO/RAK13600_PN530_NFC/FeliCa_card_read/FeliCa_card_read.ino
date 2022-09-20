/**
   @file FeliCa_card_read.ino
   @author rakwireless.com
   @brief This example will attempt to connect to an FeliCa card or tag and retrieve some basic information about it that can be used to determine what type of card it is.
   @version 0.1
   @date 2021-10-14
   @copyright Copyright (c) 2021
**/

#include <Wire.h>
#include <SPI.h>
#include <RAK13600-PN532.h> // Click here to get the library: http://librarymanager/All#RAK13600-PN532

// If using the breakout or shield with I2C, define just the pins connected
#define PN532_IRQ   (WB_IO6)
#define PN532_RESET (WB_IO5)  // Not connected by default on the NFC Shield

// Or use this line for a breakout or shield with an I2C connection:
NFC_PN532 nfc(PN532_IRQ, PN532_RESET);

uint8_t        _prevIDm[8];
unsigned long  _prevTime;

void PrintHex8(const uint8_t d) {
  Serial.print(" ");
  Serial.print( (d >> 4) & 0x0F, HEX);
  Serial.print( d & 0x0F, HEX);
}

void setup(void)
{
  Serial.begin(115200);
  pinMode(WB_IO2, OUTPUT);
  digitalWrite(WB_IO2, HIGH);
  delay(300);
  while (!Serial) delay(10);
  Serial.println("Hello!");

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata)
  {
    Serial.print("Didn't find PN53x board");
    while (1) {
      delay(10);
    };// halt
  }

  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. "); Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata >> 8) & 0xFF, DEC);

  // Set the max number of retry attempts to read from a card
  // This prevents us from waiting forever for a card, which is
  // the default behaviour of the PN532.
  nfc.setPassiveActivationRetries(0xFF);
  nfc.SAMConfig();

  memset(_prevIDm, 0, 8);
}

void loop(void)
{
  uint8_t ret;
  uint16_t systemCode = 0xFFFF;
  uint8_t requestCode = 0x01;       // System Code request
  uint8_t idm[8];
  uint8_t pmm[8];
  uint16_t systemCodeResponse;

  // Wait for an FeliCa type cards.
  // When one is found, some basic information such as IDm, PMm, and System Code are retrieved.
  Serial.println("Waiting for an FeliCa card...  ");
  ret = nfc.felica_Polling(systemCode, requestCode, idm, pmm, &systemCodeResponse, 5000);

  if (ret != 1)
  {
    Serial.println("Could not find a card");
    delay(500);
    return;
  }

  if ( memcmp(idm, _prevIDm, 8) == 0 ) {
    if ( (millis() - _prevTime) < 3000 ) {
      Serial.println("Same card");
      delay(500);
      return;
    }
  }

#ifdef PN532DEBUG
    PN532DEBUGPRINT.print("Unhandled number of targets inlisted. NbTg: ");
    PN532DEBUGPRINT.print((byte)pn532_packetbuffer[14], HEX);
    PN532DEBUGPRINT.println();
    return -3;
#endif

  Serial.println("Found a card!");
  Serial.print("  IDm: ");
  nfc.PrintHex(idm, 8);
  Serial.print("  PMm: ");
  nfc.PrintHex(pmm, 8);
  Serial.print("  System Code: ");
  Serial.print(systemCodeResponse, HEX);
  Serial.print("\n");

  memcpy(_prevIDm, idm, 8);
  _prevTime = millis();

  uint8_t blockData[3][16];
  uint16_t serviceCodeList[1];
  uint16_t blockList[3];

  Serial.println("Write Without Encryption command ");
  serviceCodeList[0] = 0x0009;
  blockList[0] = 0x8000;
  /*unsigned long now = millis();
    blockData[0][3] = now & 0xFF;
    blockData[0][2] = (now >>= 8) & 0xFF;
    blockData[0][1] = (now >>= 8) & 0xFF;
    blockData[0][0] = (now >>= 8) & 0xFF;
  */

  blockData[0][3] = 0x55 & 0xFF;
  blockData[0][2] = 0x66 & 0xFF;
  blockData[0][1] = 0x77 & 0xFF;
  blockData[0][0] = 0x88 & 0xFF;
  
  Serial.print("   Writing current millis (");
  PrintHex8(blockData[0][0]);
  PrintHex8(blockData[0][1]);
  PrintHex8(blockData[0][2]);
  PrintHex8(blockData[0][3]);
  Serial.print(" ) to Block 0 -> ");
  ret = nfc.felica_WriteWithoutEncryption(1, serviceCodeList, 1, blockList, blockData);
  if (ret != 1)
  {
    Serial.println("Write Error");
  } else {
    Serial.println("Write OK!");
  }
  memset(blockData[0], 0, 16);
  Serial.print("Read Without Encryption command -> ");
  serviceCodeList[0] = 0x000B;
  blockList[0] = 0x8000;
  blockList[1] = 0x8001;
  blockList[2] = 0x8002;
  ret = nfc.felica_ReadWithoutEncryption(1, serviceCodeList, 3, blockList, blockData);
  if (ret != 1)
  {
    Serial.println("Read Error");
  } else {
    Serial.println("Read OK!");
    for (int i = 0; i < 3; i++ ) {
      Serial.print("  Block no. "); Serial.print(i, DEC); Serial.print(": ");
      nfc.PrintHex(blockData[i], 16);
    }
  }
  // Wait 1 second before continuing
  Serial.println("Card access completed!\n");
  delay(1000);
}
