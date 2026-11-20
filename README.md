# deskBud
# ESP32 Distance-Reactive Harmonic Synth & Expressive OLED Eyes

An Arduino IDE project for an **ESP32 DevKitC** that reads distance from an ultrasonic sensor to drive:
1. **Bluetooth A2DP Audio:** Plays a pleasant A major pentatonic synth whose note speed, brightness, and shimmer change with distance.
2. **0.96" OLED Display:** Renders animated eyes with natural blinking, gaze shifts, and crosseyed convergence when an object gets close.

---

## Hardware Required

* **ESP32 DevKitC** (classic ESP32 with Bluetooth Classic support)
* **0.96" I2C OLED Display** (128x64, SSD1306)
* **HC-SR04 Ultrasonic Sensor**
* **Bluetooth A2DP Speaker / Headphones**
* Jumper wires & breadboard

---

## Wiring & Power Notes

> **Important – Protecting Your ESP32 Pins:** The ESP32's GPIO pins are **not 5V-tolerant**. Powering the HC-SR04 from **3.3V** (or your ~3.5V rail, staying well below 5V) helps keep the `ECHO` signal safe for GPIO 18 without risking pin damage. Note that running an HC-SR04 below 5V can reduce maximum detection range or occasionally cause missed echoes—if range is too short or inconsistent, use a proper voltage divider on `ECHO` (e.g., 1kΩ / 2kΩ) rather than feeding 5V straight into GPIO 18.

| Component Pin | ESP32 DevKitC Pin | Notes |
| :--- | :--- | :--- |
| **OLED `VCC`** | `3V3` | Check your OLED module silkscreen (most support 3.3V–5V) |
| **OLED `GND`** | `GND` | Common ground |
| **OLED `SDA`** | `GPIO 21` | `Wire.begin(21, 22)` |
| **OLED `SCL`** | `GPIO 22` | `Wire.begin(21, 22)` |
| **HC-SR04 `VCC`** | `3V3` (or your ~3.5V supply) | Kept low to protect ESP32 GPIO pins |
| **HC-SR04 `TRIG`** | `GPIO 5` | Trigger output |
| **HC-SR04 `ECHO`** | `GPIO 18` | Echo input (safe from 5V overvoltage at ~3.3V–3.5V) |
| **HC-SR04 `GND`** | `GND` | Common ground |

---

## Arduino IDE Setup

1. **Install ESP32 Board Support:**
   * Open **Arduino IDE** → **File** → **Preferences**.
   * Add the ESP32 boards manager URL if not already added, then open **Boards Manager**, search for **esp32**, and install the official Espressif package.
   * Select your **ESP32 Dev Module** (DevKitC) under **Tools → Board** and choose your **Port**.
2. **Install Required Libraries:**
   * Open **Sketch → Include Library → Manage Libraries...** and install:
     * **Adafruit GFX Library**
     * **Adafruit SSD1306**
   * Install **ESP32-A2DP** (`BluetoothA2DPSource.h`) via Library Manager or from GitHub if not already installed.
3. **Partition Scheme (If Sketch Is Large):**
   * Bluetooth A2DP uses significant flash/DRAM. If compilation or upload complains about partition size, check **Tools → Partition Scheme** and select a scheme with enough app space (such as **Huge APP (3MB No OTA/1MB SPIFFS)**).

---

## Changing the Bluetooth Speaker Name

Before uploading, change `SPEAKER_NAME` in your sketch to match the **exact Bluetooth name** of your speaker or headphones:

```cpp
const char* SPEAKER_NAME = "A17";
