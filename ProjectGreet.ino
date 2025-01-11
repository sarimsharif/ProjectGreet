#include <SPI.h>
#include "MFRC522.h"

#define SS_PIN  5  // ESP32 pin GPIO5 
#define RST_PIN 27 // ESP32 pin GPIO27 

MFRC522 rfid(SS_PIN, RST_PIN);

// Array of recognized UUIDs
String UUIDs[] = {"B1EE9A06", 
                  "51312645", 
                  "935BA84B", 
                  "61E2CA45", 
                  "627DFD4B", 
                  "336C360E", 
                  "F1B31C06", 
                  "73FC5A4B", 
                  "23D53B0E", 
                  "B363E54B", 
                  "F1B6B706"};

const int UUID_COUNT = sizeof(UUIDs) / sizeof(UUIDs[0]);

void setup() {
  Serial.begin(9600);
  SPI.begin(); // init SPI bus
  rfid.PCD_Init(); // init MFRC522

  Serial.println("Tap an RFID/NFC tag on the RFID-RC522 reader");
}

void loop() {
  if (rfid.PICC_IsNewCardPresent()) { // new tag is available
    if (rfid.PICC_ReadCardSerial()) { // NUID has been read
      Serial.print("RFID/NFC Tag Type: ");
      Serial.println(rfid.PICC_GetTypeName(rfid.PICC_GetType(rfid.uid.sak)));

      // Extract and print UID in hexadecimal format
      String uuid = ""; // Variable to store UUID as a string
      for (int i = 0; i < rfid.uid.size; i++) {
        // Convert byte to hex and append to the string
        if (rfid.uid.uidByte[i] < 0x10) {
          uuid += "0";
        }
        uuid += String(rfid.uid.uidByte[i], HEX);
      }
      uuid.toUpperCase(); // Ensure the UUID is in uppercase for consistency

      Serial.print("UUID (as string): ");
      Serial.println(uuid); // Print the UUID as a single string

      // Check if the UUID is in the array
      bool recognized = false;
      for (int i = 0; i < UUID_COUNT; i++) {
        if (uuid == UUIDs[i]) {
          recognized = true;
          break;
        }
      }

      if (recognized) {
        Serial.println("Card Recognized!");
      } else {
        Serial.println("Card not recognized.");
      }

      rfid.PICC_HaltA(); // halt PICC
      rfid.PCD_StopCrypto1(); // stop encryption on PCD
    }
  }
}