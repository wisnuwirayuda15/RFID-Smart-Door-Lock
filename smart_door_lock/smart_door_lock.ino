/*
 * This ESP32 code is created by esp32io.com
 *
 * This ESP32 code is released in the public domain
 *
 * For more detail (instruction and wiring diagram), visit https://esp32io.com/tutorials/esp32-rfid-nfc-door-lock-system
 */
#define BLYNK_TEMPLATE_ID "your_template_id"
#define BLYNK_TEMPLATE_NAME "your_template_name"
#define BLYNK_AUTH_TOKEN "your_blynk_token"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <SPI.h>
#include <MFRC522.h>

char ssid[] = "wifi_ssid";
char pass[] = "wifi_password";
char auth[] = BLYNK_AUTH_TOKEN;

#define SS_PIN 5  // ESP32 pin GPIO5 
#define RST_PIN 22 // ESP32 pin GPIO27 
#define RELAY_PIN 26 // ESP32 pin GPIO32 connects to relay
#define VIRTUAL_PIN_SWITCH V1 // Virtual pin for Blynk switch
#define VIRTUAL_PIN_LABEL V0  // Virtual pin for Blynk label
#define VIRTUAL_PIN_STATUS V2  // Virtual pin for Blynk label

MFRC522 rfid(SS_PIN, RST_PIN);

struct UID {
  byte uid[10];  // Maximum UID size for MFRC522 is 10 bytes
  byte size;
};

UID authorizedUIDs[] = {
  // put your card id's here...
};

void setup() {
  Serial.begin(9600);
  SPI.begin(); // init SPI bus
  rfid.PCD_Init(); // init MFRC522
  pinMode(RELAY_PIN, OUTPUT); // initialize pin as an output.
  digitalWrite(RELAY_PIN, HIGH); // lock the door

  WiFi.begin(ssid, pass); // connect to WiFi

  // Attempt to connect to WiFi for 10 seconds
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(100);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Blynk.begin(auth, ssid, pass);
    Serial.println("WiFi connected");
  } else {
    Serial.println("WiFi not connected, running offline");
  }

  Serial.println("Tap an RFID/NFC tag on the RFID-RC522 reader");
}

bool checkUID() {
  for (int i = 0; i < sizeof(authorizedUIDs) / sizeof(authorizedUIDs[0]); i++) {
    if (rfid.uid.size == authorizedUIDs[i].size && 
        memcmp(rfid.uid.uidByte, authorizedUIDs[i].uid, rfid.uid.size) == 0) {
      return true;
    }
  }
  return false;
}

void door() {
  if (rfid.PICC_IsNewCardPresent()) { // new tag is available
    if (rfid.PICC_ReadCardSerial()) { // NUID has been read
      MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
      
      // Convert UID to string
      String uidStr = "";
      for (int i = 0; i < rfid.uid.size; i++) {
        if (rfid.uid.uidByte[i] < 0x10) uidStr += "0";
        uidStr += String(rfid.uid.uidByte[i], HEX);
      }
      uidStr.toUpperCase();
      Serial.println(uidStr);
      Blynk.virtualWrite(VIRTUAL_PIN_LABEL, uidStr); // Send UID to Blynk label

      if (checkUID()) {
        Serial.println("Access is granted");
        Blynk.virtualWrite(VIRTUAL_PIN_STATUS, "Access is granted");
        digitalWrite(RELAY_PIN, LOW);  // unlock the door for 2 seconds
        Blynk.virtualWrite(VIRTUAL_PIN_SWITCH, 1); // Set Blynk switch to 'on'
        delay(2000);
        digitalWrite(RELAY_PIN, HIGH); // lock the door
        Blynk.virtualWrite(VIRTUAL_PIN_SWITCH, 0); // Set Blynk switch to 'off'
      } else {
        Serial.print("Access denied, UID:");
        Blynk.virtualWrite(VIRTUAL_PIN_STATUS, "Access is denied");
        Blynk.virtualWrite(VIRTUAL_PIN_SWITCH, 0); // Set Blynk switch to 'off'
        digitalWrite(RELAY_PIN, HIGH); // lock the door
        for (int i = 0; i < rfid.uid.size; i++) {
          Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
          Serial.print(rfid.uid.uidByte[i], HEX);
        }
        Serial.println();
      }
      // rfid.PICC_HaltA(); // halt PICC
      // rfid.PCD_StopCrypto1(); // stop encryption on PCD
    }
  }
}

BLYNK_WRITE(VIRTUAL_PIN_SWITCH) {
  int pinValue = param.asInt(); // Get the value of the switch
  if (pinValue == 1) {
    Serial.println("Blynk: Unlocking the door");
    digitalWrite(RELAY_PIN, LOW);  // Unlock the door
  } else {
    Serial.println("Blynk: Locking the door");
    digitalWrite(RELAY_PIN, HIGH); // Lock the door
  }
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    Blynk.run();
  }
  door();
}
