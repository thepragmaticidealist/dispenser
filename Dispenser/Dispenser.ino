#include <SPI.h>
#include <MFRC522.h>
#include <Arduino.h>
#include <LiquidCrystal.h>

#define RST_PIN      49
#define SS_PIN       53 
#define GREEN_LED    46
#define RED_LED      47
#define MAX_CARDS    10  // Maximum number of unique cards to track
#define MAX_READS    3   // Default maximum valid reads per card (can be edited)
#define TIMEOUT_MIN  5   // Timeout in minutes to reset read counts

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
LiquidCrystal lcd(7, 8, 9, 10, 11, 12); // LCD connected to digital pins

String cardUUIDs[MAX_CARDS];  // Array to store UUIDs
int cardReadCounts[MAX_CARDS] = {0}; // Array to track read counts
int maxReadsPerCard[MAX_CARDS]; // Array to allow setting max reads per card
unsigned long lastReadTime[MAX_CARDS] = {0}; // Store last read timestamps
unsigned long lastIdleMessageTime = 0;
const unsigned long idleMessageInterval = 3000; // Show idle message every 3 seconds

void scrollLCD(String line1, String line2, int delayTime = 200) {
    int maxLength = max(line1.length(), line2.length());
    for (int i = 0; i <= maxLength; i++) {
        lcd.clear();
        lcd.setCursor(0, 0);
        if (i < line1.length())
            lcd.print(line1.substring(i));
        lcd.setCursor(0, 1);
        if (i < line2.length())
            lcd.print(line2.substring(i));
        delay(delayTime);
    }
}

void setup() {
    Serial.begin(9600);  // Initialize serial communications with the PC
    while (!Serial);     // Do nothing if no serial port is opened (for ATMEGA32U4-based Arduinos)
    SPI.begin();         // Init SPI bus
    mfrc522.PCD_Init();  // Init MFRC522

    pinMode(GREEN_LED, OUTPUT);
    pinMode(RED_LED, OUTPUT);

    lcd.begin(16, 2);
    lcd.clear();
    scrollLCD("Please scan your", "card to get item");
    scrollLCD("Please scan your", "card to get item");
    scrollLCD("Hold tag for 3s", "to scan item");

    Serial.println(F("Scan PICC to see UID..."));
}

void loop() {
    unsigned long currentMillis = millis();

    // Show idle message every 3 seconds if no card is being scanned
    if (!mfrc522.PICC_IsNewCardPresent()) {
        if (currentMillis - lastIdleMessageTime >= idleMessageInterval) {
            scrollLCD("Hold tag for 3s", "to scan item");
            lastIdleMessageTime = currentMillis;
        }
        return;
    }

    for (int i = 0; i < MAX_CARDS; i++) {
        if (cardUUIDs[i] != "" && (currentMillis - lastReadTime[i]) > (TIMEOUT_MIN * 60000)) {
            cardReadCounts[i] = 0;
            lastReadTime[i] = currentMillis;
        }
    }

    if (!mfrc522.PICC_ReadCardSerial()) {
        digitalWrite(RED_LED, HIGH);
        scrollLCD("Please hold your", "tag for 3 sec");
        Serial.println("Error reading card.");
        delay(2000);
        digitalWrite(RED_LED, LOW);
        return;
    }

    String uuid = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        if (i > 0) uuid += "-";
        uuid += String(mfrc522.uid.uidByte[i], HEX);
    }

    int index = -1;
    for (int i = 0; i < MAX_CARDS; i++) {
        if (cardUUIDs[i] == uuid) {
            index = i;
            break;
        } else if (cardUUIDs[i] == "") {
            cardUUIDs[i] = uuid;
            cardReadCounts[i] = 0;
            maxReadsPerCard[i] = MAX_READS;
            lastReadTime[i] = currentMillis;
            index = i;
            break;
        }
    }

    if (index != -1) {
        lastReadTime[index] = currentMillis;
        cardReadCounts[index]++;
        if (cardReadCounts[index] > maxReadsPerCard[index]) {
            Serial.println("That's enough for the day");
            scrollLCD("You've already", "scanned max today");
            digitalWrite(RED_LED, HIGH);
            delay(2000);
            digitalWrite(RED_LED, LOW);
            return;
        }
    } else {
        scrollLCD("We're having", "trouble reading");
        digitalWrite(RED_LED, HIGH);
        delay(2000);
        digitalWrite(RED_LED, LOW);
        return;
    }

    digitalWrite(GREEN_LED, HIGH);

    Serial.print("UUID: ");
    Serial.println(uuid);

    String msgLine1 = "Your card has";
    String msgLine2 = "Scanned " + String(cardReadCounts[index]) + " times today";
    scrollLCD(msgLine1, msgLine2);

    delay(2000);
    digitalWrite(GREEN_LED, LOW);

    delay(1000); // Wait 1 more second to total 3s before clearing
    lcd.clear();
    scrollLCD("Hold tag for 3s", "to scan item");

    mfrc522.PICC_HaltA();
}

void setMaxReads(String uuid, int maxReads) {
    for (int i = 0; i < MAX_CARDS; i++) {
        if (cardUUIDs[i] == uuid) {
            maxReadsPerCard[i] = maxReads;
            Serial.print("Max reads updated for UUID ");
            Serial.print(uuid);
            Serial.print(": ");
            Serial.println(maxReads);
            return;
        }
    }
    Serial.println("UUID not found.");
}
