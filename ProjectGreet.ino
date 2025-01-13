#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <map>
#include <time.h>

const char* ssid = "the_tech_academy";
const char* password = "TtaisThebest1";

const char* scriptURL = "https://script.google.com/macros/s/AKfycby1dP55iOmh3-_Jrp5abLZZ6yz7VAKiwol9ZMQMHTzLZT4hB0THACxJFuKKVAEQHU6s/exec"; // Replace with your Google Apps Script URL

// NTP Configuration
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 21600;
const int daylightOffset_sec = 0;

#define RST_PIN 27
#define SS_PIN 5


MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);


struct CardData {
  String uid;
  String name;
};


CardData cards[] = {
  {"039D5F17", "RAHAT"},
  {"13870317", "REMON"},
  {"33E54D17", "SHAMS"},
  {"6314E816", "NAHIYAN"},
  {"238ECD1B", "MUHAIMIN"},
  {"A3D9EA16", "ANIKA"},
  {"13F15117", "ABONTEE"},
  {"93AA5E15", "DEBUG"}
};

std::map<String, String> cardEntryTimes;

String getCardID();
String findCardOwner(String uid);
String getTimestamp();
String calculateDuration(String entryTime, String exitTime);
void sendToGoogleSheet(String name, String uid, String entryTime, String exitTime, String duration);

void setup() {
  Serial.begin(115200);
  pinMode(13, OUTPUT);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Connected");


  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("Time Synchronized");


  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("RFID Ready");


  Wire.begin(4, 22); // Pin 4 SDA and Pin 22 SCL for LCD
  lcd.init();
  lcd.backlight();
  lcd.print("Ready to Scan...");
}

void loop() {

  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String uid = getCardID();
    Serial.println("UID: " + uid);
    
  
    String owner = findCardOwner(uid);

    if (owner != "") {

      String timeStamp = getTimestamp();

    
      if (cardEntryTimes.find(uid) == cardEntryTimes.end()) {
        // Entry Case
        cardEntryTimes[uid] = timeStamp;
        //beep a buzzer
        digitalWrite(13, HIGH);
        delay(100);
        digitalWrite(13, LOW);
        lcd.clear();
        lcd.print("Welcome, ");
        lcd.setCursor(0, 1);
        lcd.print(owner);
        Serial.println("Entry Recorded: " + timeStamp);
      } else {
  // Exit case
  String entryTime = cardEntryTimes[uid];
  String exitTime = timeStamp;
        
  //Duration of Stay
  String duration = calculateDuration(entryTime, exitTime);

  // Check if it's less than 1 minutes
  int hours, minutes, seconds;
  sscanf(duration.c_str(), "%d:%d:%d", &hours, &minutes, &seconds);
  int totalSeconds = hours * 3600 + minutes * 60 + seconds;

  if (totalSeconds < 60) { // Less than 1 minute
    lcd.clear();
    lcd.print("Already Scanned!");
    Serial.println("Card scanned too soon after entry. Ignored.");
  } else {
    //beep a buzzer
    digitalWrite(13, HIGH);
    delay(100);
    digitalWrite(13, LOW);
    lcd.clear();
    lcd.print("See You Next");
    lcd.setCursor(0, 1);
    lcd.print("Time, ");
    lcd.setCursor(7, 1);
    lcd.print(owner);

    // Send data to Google Sheet
    sendToGoogleSheet(owner, uid, entryTime, exitTime, duration);

    Serial.println("Exit Recorded: " + exitTime);
    Serial.println("Duration: " + duration);


    cardEntryTimes.erase(uid);
  }
    } 
  }
    else {
    
      Serial.println("Unknown Card");
      lcd.clear();
      lcd.print("Unknown Card");
    }

  
    delay(3000);
    lcd.clear();
    lcd.print("Ready to Scan...");

    // Halt the RFID reader
    mfrc522.PICC_HaltA();
  }
}


String getCardID() {
  String cardID = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) {
      cardID += "0"; //for single hex digits
    }
    cardID += String(mfrc522.uid.uidByte[i], HEX);
  }
  cardID.toUpperCase();
  return cardID;
}


String findCardOwner(String uid) {
  for (int i = 0; i < sizeof(cards) / sizeof(cards[0]); i++) {
    if (cards[i].uid == uid) {
      return cards[i].name;
    }
  }
  return ""; // Return empty string if card not found
}


String getTimestamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return "N/A";
  }

  char timeString[20];
  strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(timeString);
}


String calculateDuration(String entryTime, String exitTime) {
  // Parse the timestamps into time structures
  struct tm entryTm, exitTm;
  if (strptime(entryTime.c_str(), "%Y-%m-%d %H:%M:%S", &entryTm) == nullptr ||
      strptime(exitTime.c_str(), "%Y-%m-%d %H:%M:%S", &exitTm) == nullptr) {
    return "Invalid Time";
  }

  
  time_t entryTimeT = mktime(&entryTm);
  time_t exitTimeT = mktime(&exitTm);

  
  double difference = difftime(exitTimeT, entryTimeT);


  int hours = difference / 3600;
  int minutes = ((int)difference % 3600) / 60;
  int seconds = (int)difference % 60;


  char durationString[10];
  snprintf(durationString, sizeof(durationString), "%02d:%02d:%02d", hours, minutes, seconds);

  return String(durationString);
}



void sendToGoogleSheet(String name, String uid, String entryTime, String exitTime, String duration) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(scriptURL);
    http.addHeader("Content-Type", "application/json");

    
    String payload = "{\"name\":\"" + name + "\",\"uid\":\"" + uid + "\",\"entryTime\":\"" + entryTime + "\",\"exitTime\":\"" + exitTime + "\",\"duration\":\"" + duration + "\"}";
    
    int httpResponseCode = http.POST(payload);
    if (httpResponseCode > 0) {
      Serial.println("Data Sent: " + payload);
    } else {
      Serial.println("Error Sending Data: " + String(httpResponseCode));
    }
    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }
}


