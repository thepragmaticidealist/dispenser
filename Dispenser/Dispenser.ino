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

void setup() {
    Serial.begin(9600);  // Initialize serial communications with the PC
    while (!Serial);     // Do nothing if no serial port is opened (for ATMEGA32U4-based Arduinos)
    SPI.begin();         // Init SPI bus
    mfrc522.PCD_Init();  // Init MFRC522

    pinMode(GREEN_LED, OUTPUT);
    pinMode(RED_LED, OUTPUT);

    lcd.begin(16, 2);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Please scan your");
    lcd.setCursor(0, 1);
    lcd.print("card to get item");
    delay(3000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Hold tag for 3s");
    lcd.setCursor(0, 1);
    lcd.print("to scan item");

    Serial.println(F("Scan PICC to see UID..."));
}

void loop() {
    unsigned long currentMillis = millis();

    // Reset read counts if timeout has passed
    for (int i = 0; i < MAX_CARDS; i++) {
        if (cardUUIDs[i] != "" && (currentMillis - lastReadTime[i]) > (TIMEOUT_MIN * 60000)) {
            cardReadCounts[i] = 0;
            lastReadTime[i] = currentMillis;
        }
    }

    // Reset the loop if no new card present on the sensor/reader
    if (!mfrc522.PICC_IsNewCardPresent()) {
        return;
    }

    // Select one of the cards
    if (!mfrc522.PICC_ReadCardSerial()) {
        digitalWrite(RED_LED, HIGH);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Please hold your");
        lcd.setCursor(0, 1);
        lcd.print("tag for 3 sec");
        Serial.println("Error reading card.");
        delay(2000);
        digitalWrite(RED_LED, LOW);
        return;
    }

    // Store the UUID in a variable
    String uuid = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        if (i > 0) uuid += "-"; // Add separator between bytes
        uuid += String(mfrc522.uid.uidByte[i], HEX);
    }

    // Check if UUID already exists in the list
    int index = -1;
    for (int i = 0; i < MAX_CARDS; i++) {
        if (cardUUIDs[i] == uuid) {
            index = i;
            break;
        } else if (cardUUIDs[i] == "") { // Store new UUID if there's an empty slot
            cardUUIDs[i] = uuid;
            cardReadCounts[i] = 0;
            maxReadsPerCard[i] = MAX_READS; // Set default max reads per card
            lastReadTime[i] = currentMillis;
            index = i;
            break;
        }
    }

    // If we found or added the card, increment its count
    if (index != -1) {
        lastReadTime[index] = currentMillis;
        cardReadCounts[index]++;
        if (cardReadCounts[index] > maxReadsPerCard[index]) {
            Serial.println("That's enough for the day");
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("You've already");
            lcd.setCursor(0, 1);
            lcd.print("scanned max today");
            digitalWrite(RED_LED, HIGH);
            delay(2000);
            digitalWrite(RED_LED, LOW);
            return;
        }
    } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("We're having");
        lcd.setCursor(0, 1);
        lcd.print("trouble reading");
        digitalWrite(RED_LED, HIGH);
        delay(2000);
        digitalWrite(RED_LED, LOW);
        return;
    }

    // Turn on the green LED if the card is read successfully
    digitalWrite(GREEN_LED, HIGH);

    // Print the UUID
    Serial.print("UUID: ");
    Serial.println(uuid);

    // Show LCD success message with marquee effect
    lcd.clear();
    String message = "Scanned " + String(cardReadCounts[index]) + " times today";
    for (int i = 0; i < message.length(); i++) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Your card has");
        lcd.setCursor(0, 1);
        lcd.print(message.substring(i));
        delay(300);
    }

    delay(3000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Hold tag for 3s");
    lcd.setCursor(0, 1);
    lcd.print("to scan item");

    digitalWrite(GREEN_LED, LOW);
    mfrc522.PICC_HaltA(); // Halt the current card
}

// Function to manually edit the max read count for a specific card
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
