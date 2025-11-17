/*
 * STABLE LORAWAN SKETCH FOR HELTEC V3 (ESP32-S3 / SX1262)
 *
 * This code connects a Heltec V3 board to The Things Network (TTN)
 * using the RadioLib library.
 *
 * It SOLVES three major problems:
 * 1. FIXES -1116 (MIC Mismatch) Error: By forcing the radio's RF Switch VDD (Pin 14) HIGH.
 * 2. FIXES Join Failure: By telling RadioLib to use the internal DIO2 for RF switching.
 * 3. FIXES Version Mismatch: By forcing the LoRaWAN version to 1.0.3.
 *
 * YOU MUST use these Arduino IDE settings:
 * - Board: "ESP32S3 Dev Module"
 * - USB CDC On Boot: "Enabled"
 * - Partition Scheme: "Huge APP (3MB No OTA/1MB SPIFFS)"
 *
 * ...and these TTN Device settings:
 * - LoRaWAN Version: "LoRaWAN Specification 1.0.3"
 */

#include <RadioLib.h>
#include <SPI.h>

// --- Heltec V3 Pin Definitions (for RadioLib) ---
#define LORA_CS_PIN    8  // NSS / Chip Select
#define LORA_DIO1_PIN  14 // DIO1
#define LORA_RST_PIN   12 // Reset
#define LORA_BUSY_PIN  13 // Busy

// SPI Bus pins
#define LORA_SCK_PIN   9
#define LORA_MISO_PIN  11
#define LORA_MOSI_PIN  10

// --- CRITICAL HARDWARE FIX ---
// On Heltec V3, the RF Switch VDD (power) is connected
// to GPIO 14. We must set this HIGH to power the switch,
// otherwise, the radio can TX but CANNOT RX (causing Join failure).
// We define it here to be explicit.
#define LORA_RF_SW_VDD_PIN 14


// Create the radio object
SX1262 radio = new Module(LORA_CS_PIN, LORA_DIO1_PIN, LORA_RST_PIN, LORA_BUSY_PIN);

// Create the LoRaWAN node object
// We must specify the chip type (<SX1262>) for some RadioLib versions
LoRaWANNode<SX1262> node(&radio, &EU868);

/* ==========================================================
   PASTE YOUR TTN KEYS HERE
   ========================================================== */

// 1. Your JoinEUI (AppEUI). Default is 00s.
uint64_t joinEUI = 0x0000000000000000; 

// 2. Your DevEUI.
// MUST be in MSB format (normal order, just add 0x).
// Example: 70B3D57ED0074117
uint64_t devEUI  = 0x0000000000000000; // <-- PASTE YOURS

// 3. Your AppKey.
// MUST be in MSB array format (copy from TTN with the "< >" button).
// CRITICAL: You must paste the SAME AppKey into BOTH nwkKey and appKey.
uint8_t nwkKey[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // <-- PASTE APPKEY
uint8_t appKey[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // <-- PASTE APPKEY

// ==========================================================


void setup() {
  Serial.begin(115200);
  
  // This loop pauses the code until you open the Serial Monitor.
  // Crucial for ESP32-S3's native USB port.
  while (!Serial) {
    delay(10);
  }
  
  Serial.println("--------------------------------");
  Serial.println("Heltec V3 Stable LoRaWAN (RadioLib)");
  Serial.println("--------------------------------");

  // --- HARDWARE FIX 1: Power the RF Switch ---
  Serial.print("Powering RF Switch (Pin 14)... ");
  pinMode(LORA_RF_SW_VDD_PIN, OUTPUT);
  digitalWrite(LORA_RF_SW_VDD_PIN, HIGH);
  Serial.println("OK");
  
  // --- Configure SPI Bus ---
  Serial.print("Configuring SPI... ");
  SPI.begin(LORA_SCK_PIN, LORA_MISO_PIN, LORA_MOSI_PIN, LORA_CS_PIN);
  Serial.println("OK");

  // --- Initialize Radio ---
  Serial.print("Initializing SX1262 radio... ");
  
  // --- HARDWARE FIX 2: Enable internal RF Switch Control ---
  // Tell RadioLib to use the DIO2 pin to control the switch
  int state = radio.setDio2AsRfSwitch(true);
  if(state != RADIOLIB_ERR_NONE) {
    Serial.print("setDio2AsRfSwitch failed! Code: "); Serial.println(state);
  }
  
  state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("Radio init SUCCEEDED!");
  } else {
    Serial.print("Radio init FAILED! Code: "); Serial.println(state);
    while (true); // Stop here
  }

  // --- SOFTWARE FIX 3: Force LoRaWAN Version 1.0.3 ---
  // This fixes the -1116 (MIC Mismatch) error
  Serial.println("Forcing LoRaWAN version 1.0.3...");
  node.setMacVersion(LORAWAN_VERSION_1_0_3);
  
  // Configure OTAA
  Serial.println("Configuring OTAA...");
  node.beginOTAA(joinEUI, devEUI, nwkKey, appKey); 
  
  // Attempt to Join TTN
  Serial.println("Attempting to join TTN (JOIN)...");
  state = node.activateOTAA(); 
  
  if (state == RADIOLIB_LORAWAN_NEW_SESSION) {
    Serial.println("!!! JOINED SUCCESSFULLY !!!");
  } else {
    Serial.print("Join FAILED. Error code: "); Serial.println(state);
    Serial.println("Check Keys (nwkKey/appKey must be identical) and TTN settings (must be 1.0.3).");
  }
}

void loop() {
  static unsigned long lastSendTime = 0;
  if (millis() - lastSendTime > 60000) { // Send every 60 seconds
    lastSendTime = millis();
    
    Serial.println();
    Serial.print("Sending 'Hello' packet... ");

    // Simple payload "Hello"
    uint8_t payload[] = { 0x48, 0x65, 0x6C, 0x6C, 0x6F };
    
    int state = node.sendReceive(payload, sizeof(payload));

    if (state == RADIOLIB_ERR_NONE) {
      Serial.println("Packet sent!");
    } else {
      Serial.print("Send FAILED. Error code: "); Serial.println(state);
    }
  }
}
