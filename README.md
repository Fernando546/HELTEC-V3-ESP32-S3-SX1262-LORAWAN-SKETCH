# Heltec V3 (ESP32-S3/SX1262) Stable LoRaWAN + TTN Example

This repository provides a minimal, stable Arduino sketch to connect a Heltec WiFi LoRa 32 V3 (or Wireless Stick V3) to The Things Network (TTN) using the **RadioLib** library.

This code is the result of extensive debugging and solves several critical, "show-stopping" bugs.

## The Problems This Code Solves

If you are here, you may faced these issues:
1.  **SHA-256 Error:** Using the official Heltec library (`LoRaWan_APP.h`) causes a `SHA-256 comparison failed` boot loop when the code is large.
2.  **License Error:** Using the official library after an "Erase Flash" causes a `Please provide a correct license!` error.
3.  **Join Error -1116 (MIC Mismatch):** Using the **RadioLib** library *seems* to work, but the device never joins and the Heltec console shows `Error: -1116` (Connection Failed. Error Code: -1116).
4.  **Join Error 0 (Timeout):** The device successfully joins TTN *once*, but after a reset, it never joins again and the console shows `Error: 0`.

This code and setup fix **all** of these issues.

---

## The Solution (3-Part Fix)

The solution requires a specific Arduino IDE setup, a specific code configuration, and a specific TTN setting.

### Part 1: Arduino IDE Setup (The `SHA-256` Fix)

Do **NOT** use the "Heltec WiFi LoRa 32(V3)" board definition. It causes the `SHA-256` error by mismanaging the large code size.

Use the generic **ESP32S3 Dev Module** definition instead:

1.  Go to **Tools > Board > esp32 > ESP32S3 Dev Module**.
2.  I set the following options in the **Tools** menu:
    * **USB CDC On Boot:** `Disabled` (Crucial for seeing `Serial.print` output in my case).
    * **Flash Mode:** `DIO 80MHz` (More stable than QIO).
    * **Partition Scheme:** `Huge APP (3MB No OTA/1MB SPIFFS)`.
    * **Upload Speed:** `115200`.

### Part 2: TTN Device Setup (The `1.0.3` Fix)

The `-1116` error is a version mismatch. RadioLib (by default, or in older versions) tries to join using LoRaWAN 1.0.3, but TTN defaults to 1.1.0.

1.  In your TTN Application, register your device manually.
2.  **Crucially:** Set the **LoRaWAN version** to **`LoRaWAN Specification 1.0.3`**.
3.  Set **Regional Parameters version** to **`RP001 Regional Parameters 1.0.3 revision A`**.
4.  Copy your `DevEUI` (MSB/normal format) and `AppKey` (MSB/array format).

### Part 3: The Code (The `-1116` & Hardware Fix)

This `.ino` file includes two critical hardware fixes that are missing from standard examples:

1.  **RF Switch Power Fix:** The Heltec V3 board has a design flaw where the Radio RF Switch (which switches between TX/RX) is **not powered** when using a generic board definition. This code manually powers it by setting **GPIO 14 to HIGH**. This fixes the "TX works, but RX fails" problem.
2.  **RF Switch Control Fix:** It tells RadioLib to use the radio's `DIO2` pin to control the now-powered RF switch (`radio.setDio2AsRfSwitch(true)`).

Follow the instructions in the `.ino` file to paste your keys.
