#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define TRIG_PIN 5
#define ECHO_PIN 18

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("=== TEST START ===");

  // --- Ultrasonic pins ---
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  Serial.println("Ultrasonic pins set.");

  // --- I2C scan ---
  Wire.begin(21, 22);
  Serial.println("Scanning I2C bus...");
  int found = 0;
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("  Found device at 0x");
      Serial.println(addr, HEX);
      found++;
    }
  }
  if (found == 0) Serial.println("  No I2C devices found!");
  else Serial.print("  Total devices: "); Serial.println(found);

  // --- OLED init ---
  Serial.println("Initializing OLED at 0x3C...");
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("  FAILED at 0x3C, trying 0x3D...");
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
      Serial.println("  FAILED at 0x3D too. OLED not working.");
    } else {
      Serial.println("  OLED OK at 0x3D");
    }
  } else {
    Serial.println("  OLED OK at 0x3C");
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("TEST");
  display.setTextSize(1);
  display.setCursor(0, 30);
  display.println("OLED works!");
  display.display();
  Serial.println("OLED test text drawn.");

  Serial.println("=== SETUP DONE ===");
}

void loop() {
  // --- Read ultrasonic ---
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  float distance = (duration == 0) ? -1.0 : (duration * 0.0343) / 2.0;

  Serial.print("Distance: ");
  if (distance < 0) Serial.println("NO ECHO (timeout)");
  else { Serial.print(distance, 1); Serial.println(" cm"); }

  // --- Update OLED ---
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Distance test");
  display.setTextSize(2);
  display.setCursor(0, 20);
  if (distance < 0) display.println("--- cm");
  else { display.print(distance, 1); display.println(" cm"); }
  display.display();

  delay(500);
}