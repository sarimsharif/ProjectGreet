#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include "MFRC522.h"
#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "the_tech_academy";
const char* password = "TtaisThebest1";

const char* scriptURL = "https://script.google.com/macros/s/AKfycbyquV8c5KEzFrceqAG1xoVX0UbZ9ecMuTaRSFraT4c0a_JBcNdc9sRIrkHY0wcAmJnR/exec"; // Replace with your script URL

#define RST_PIN 27
#define SS_PIN 5

MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(115200);

  // Wi-Fi Setup
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Connected");

  // RFID and LCD Setup
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("RFID Ready");

  lcd.init();
  lcd.backlight();
  lcd.print("Ready to Scan...");
}

void loop() {
  // Look for RFID card
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Get card ID
  String cardID = getCardID();
  Serial.println("Card ID: " + cardID);

  // Get the list of authorized card IDs
  String authorizedIDs = getAuthorizedCardIDs();
  
  // Check if the card ID is in the list
  bool isAuthorized = isCardAuthorized(cardID, authorizedIDs);

  // Display results
  lcd.clear();
  if (isAuthorized) {
    lcd.print("Access Granted");
    Serial.println("Access Granted");
  } else {
    lcd.print("Access Denied");
    Serial.println("Access Denied");
  }

  delay(3000); // Display result for 3 seconds
  lcd.clear();
  lcd.print("Ready to Scan...");
}

String getCardID() {
  String cardID = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) {
      cardID += "0"; // Add leading zero for single hex digits
    }
    cardID += String(mfrc522.uid.uidByte[i], HEX);
  }
  
  cardID.toUpperCase(); // Modify the cardID in place
  
  return cardID;
}


String getAuthorizedCardIDs() {
  String url = String(scriptURL);
  HTTPClient http;
  http.begin(url);

  // Allow redirects
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);  // Or HTTPC_FOLLOW_REDIRECTS

  int httpResponseCode = http.GET();
  
  String authorizedIDs = "";
  
  if (httpResponseCode > 0) {
    authorizedIDs = http.getString();
    Serial.println("Authorized IDs: " + authorizedIDs);
  } else {
    Serial.println("Error in HTTP request: " + String(httpResponseCode));
    lcd.clear();
    lcd.print("HTTP Error");
    delay(2000);
  }

  http.end();
  return authorizedIDs;
}



bool isCardAuthorized(String cardID, String authorizedIDs) {
  // Check if the card ID exists in the list of authorized IDs
  int index = authorizedIDs.indexOf(cardID);
  return index != -1; // If cardID is found, return true
}
