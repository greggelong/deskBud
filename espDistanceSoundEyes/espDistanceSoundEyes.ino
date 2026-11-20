#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "BluetoothA2DPSource.h"
#include <math.h>

// --- OLED ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Ultrasonic (validated pins) ---
#define TRIG_PIN 5
#define ECHO_PIN 18

// --- Audio ---
BluetoothA2DPSource a2dp_source;
const char* SPEAKER_NAME = "A17";

// Consonant A Major Pentatonic scale (Hz): A3, B3, C#4, E4, F#4, A4, B4, C#5
const float pentatonic_scale[] = {
  220.00f, 246.94f, 277.18f, 329.63f,
  369.99f, 440.00f, 493.88f, 554.37f
};
const int scale_size = sizeof(pentatonic_scale) / sizeof(pentatonic_scale[0]);

volatile int distance_cm = 100;
volatile float target_freq = 220.0f;
volatile float target_brightness = 0.25f; // 0.0 (warm sine) .. 1.0 (brighter harmonic)
volatile float target_gain = 3800.0f;

float current_freq = 220.0f;
float current_brightness = 0.25f;
float current_gain = 3800.0f;
float phase = 0.0f;

unsigned long last_note_step_time = 0;
unsigned long last_display_update = 0;
int note_index = 0;
int note_direction = 1;

// Eye & blink animation state
int blinkState = 0;           // 0 = open, 1 = closing/closed
unsigned long lastBlinkTime = 0;
unsigned long nextBlinkInterval = 3200;

float gazeX = 0.0f;           // -1.0 (left) to +1.0 (right)
float gazeY = 0.0f;           // -1.0 (up) to +1.0 (down)
float targetGazeX = 0.0f;
float targetGazeY = 0.0f;
float crossEyeAmount = 0.0f;  // 0.0 (normal) to 1.0 (strongly crosseyed)
unsigned long lastSaccadeTime = 0;

// ----------------------------------------------------------------------------
int readUltrasonicDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration <= 0) return 150;

  int distance = (int)((duration * 0.0343f) / 2.0f);
  if (distance < 2) return 2;
  if (distance > 150) return 150;
  return distance;
}

// ----------------------------------------------------------------------------
// A2DP callback – runs at 44.1 kHz
int32_t get_sound_data(Frame *data, int32_t len) {
  const float sample_rate = 44100.0f;
  const float two_pi = 6.2831853f;

  float goal_freq = target_freq;
  float goal_bright = target_brightness;
  float goal_gain = target_gain;

  // Smooth per-sample slew rates to prevent clicks and harsh jumps
  const float freq_slew = 0.0015f;
  const float param_slew = 0.0005f;

  for (int i = 0; i < len; i++) {
    current_freq += (goal_freq - current_freq) * freq_slew;
    current_brightness += (goal_bright - current_brightness) * param_slew;
    current_gain += (goal_gain - current_gain) * param_slew;

    // Warm fundamental sine + gentle octave/fifth overtone
    float fund = sinf(phase);
    float overtone = 0.35f * sinf(2.0f * phase) + 0.15f * sinf(3.0f * phase);
    float wave = (1.0f - current_brightness * 0.65f) * fund + (current_brightness * 0.65f) * overtone;

    int32_t raw = (int32_t)(wave * current_gain);
    if (raw > 30000) raw = 30000;
    if (raw < -30000) raw = -30000;

    int16_t sample = (int16_t)raw;
    data[i].channel1 = sample;
    data[i].channel2 = sample;

    phase += (two_pi * current_freq) / sample_rate;
    if (phase >= two_pi) {
      phase -= two_pi;
    }
  }

  return len;
}

// ----------------------------------------------------------------------------
void updateEyeBehavior(int dist, unsigned long now) {
  // Natural blinking every ~2.5 - 5 seconds
  if (blinkState == 0 && (now - lastBlinkTime >= nextBlinkInterval)) {
    blinkState = 1;
    lastBlinkTime = now;
    nextBlinkInterval = random(2500, 5000);
  } else if (blinkState == 1 && (now - lastBlinkTime >= 140)) {
    blinkState = 0;
    lastBlinkTime = now;
  }

  // Crosseyed convergence ramps up smoothly when distance < 32 cm
  float target_cross = 0.0f;
  if (dist < 32) {
    target_cross = (32.0f - (float)dist) / 26.0f; // ~0.0 at 32cm up to ~1.0 at 6cm
    if (target_cross > 1.0f) target_cross = 1.0f;
    if (target_cross < 0.0f) target_cross = 0.0f;
  }
  crossEyeAmount += (target_cross - crossEyeAmount) * 0.25f;

  // Periodic subtle gaze shifts when not strongly crosseyed
  if (now - lastSaccadeTime > (unsigned long)random(700, 1600)) {
    lastSaccadeTime = now;
    if (crossEyeAmount > 0.5f) {
      // Slight curious upward glance while crosseyed
      targetGazeX = 0.0f;
      targetGazeY = -0.25f;
    } else {
      targetGazeX = random(-65, 66) / 100.0f;
      targetGazeY = random(-45, 46) / 100.0f;
    }
  }

  gazeX += (targetGazeX - gazeX) * 0.22f;
  gazeY += (targetGazeY - gazeY) * 0.22f;
}

// ----------------------------------------------------------------------------
void drawEyes(int dist) {
  unsigned long now = millis();
  updateEyeBehavior(dist, now);

  display.clearDisplay();

  // Header & mood status
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("DIST: ");
  display.print(dist);
  display.print(" cm");

  display.setCursor(0, 10);
  if (dist >= 80) {
    display.print("MOOD: SERENE");
  } else if (dist >= 32) {
    display.print("MOOD: WARM DRIFT");
  } else {
    display.print("MOOD: LOCKED IN");
  }

  // Eye sclera dimensions
  const int leftEyeX = 22;
  const int rightEyeX = 76;
  const int eyeY = 24;
  const int eyeW = 30;
  const int eyeH = 34;
  const int radius = 7;

  if (blinkState == 1) {
    // Closed eyelid lines
    display.drawFastHLine(leftEyeX + 2, eyeY + eyeH / 2, eyeW - 4, SSD1306_WHITE);
    display.drawFastHLine(rightEyeX + 2, eyeY + eyeH / 2, eyeW - 4, SSD1306_WHITE);
  } else {
    // Draw eye whites (sclera)
    display.fillRoundRect(leftEyeX, eyeY, eyeW, eyeH, radius, SSD1306_WHITE);
    display.fillRoundRect(rightEyeX, eyeY, eyeW, eyeH, radius, SSD1306_WHITE);

    // Max pupil travel inside sclera
    const int maxOffsetX = 7;
    const int maxOffsetY = 6;

    // Convergence pulls left pupil inward (+X) and right pupil inward (-X)
    float convergeOffset = crossEyeAmount * (float)maxOffsetX;

    int leftPupilX = leftEyeX + (eyeW / 2) + (int)(gazeX * maxOffsetX * (1.0f - 0.6f * crossEyeAmount) + convergeOffset);
    int rightPupilX = rightEyeX + (eyeW / 2) + (int)(gazeX * maxOffsetX * (1.0f - 0.6f * crossEyeAmount) - convergeOffset);
    int pupilY = eyeY + (eyeH / 2) + (int)(gazeY * maxOffsetY);

    // Clamp pupils safely inside sclera
    int minLX = leftEyeX + 6;
    int maxLX = leftEyeX + eyeW - 6;
    int minRX = rightEyeX + 6;
    int maxRX = rightEyeX + eyeW - 6;
    int minY = eyeY + 6;
    int maxY = eyeY + eyeH - 6;

    if (leftPupilX < minLX) leftPupilX = minLX;
    if (leftPupilX > maxLX) leftPupilX = maxLX;
    if (rightPupilX < minRX) rightPupilX = minRX;
    if (rightPupilX > maxRX) rightPupilX = maxRX;
    if (pupilY < minY) pupilY = minY;
    if (pupilY > maxY) pupilY = maxY;

    // Draw dark pupils + tiny catchlight dot for life
    int pupilR = (crossEyeAmount > 0.5f) ? 5 : 6;
    display.fillCircle(leftPupilX, pupilY, pupilR, SSD1306_BLACK);
    display.fillCircle(rightPupilX, pupilY, pupilR, SSD1306_BLACK);

    display.drawPixel(leftPupilX - 2, pupilY - 2, SSD1306_WHITE);
    display.drawPixel(rightPupilX - 2, pupilY - 2, SSD1306_WHITE);
  }

  display.display();
}

// ----------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("=== HARMONIC SYNTH + EXPRESSIVE EYES START ===");

  randomSeed(analogRead(0));

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Wire.begin(21, 22);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 init failed");
    for (;;);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("BOOTING...");
  display.display();
  Serial.println("OLED OK");

  a2dp_source.set_data_callback_in_frames(get_sound_data);
  a2dp_source.set_volume(80);
  Serial.print("Starting A2DP -> ");
  Serial.println(SPEAKER_NAME);
  a2dp_source.start(SPEAKER_NAME);
}

// ----------------------------------------------------------------------------
void loop() {
  int dist = readUltrasonicDistance();
  distance_cm = dist;

  unsigned long now = millis();

  // Step interval & harmonic brightness respond to distance
  int step_interval;
  if (dist >= 80) {
    step_interval = map(constrain(dist, 80, 150), 80, 150, 320, 520);
    target_brightness = 0.20f;
    target_gain = 3600.0f;
  } else if (dist >= 32) {
    step_interval = map(dist, 32, 79, 170, 310);
    target_brightness = 0.55f;
    target_gain = 4200.0f;
  } else {
    step_interval = map(dist, 2, 31, 85, 165);
    target_brightness = 0.90f;
    target_gain = 4600.0f;
  }

  // Ping-pong through the consonant pentatonic scale so notes stay musical
  if (now - last_note_step_time >= (unsigned long)step_interval) {
    last_note_step_time = now;

    note_index += note_direction;
    if (note_index >= scale_size - 1) {
      note_index = scale_size - 1;
      note_direction = -1;
    } else if (note_index <= 0) {
      note_index = 0;
      note_direction = 1;
    }

    // Close range can add an octave leap on high notes for sparkle
    float freq = pentatonic_scale[note_index];
    if (dist < 25 && note_index >= 4) {
      freq *= 1.5f; // Consonant fifth/octave shimmer within key
    }
    target_freq = freq;
  }

  // Update OLED at ~20 Hz for smooth eyeball movement
  if (now - last_display_update >= 50) {
    last_display_update = now;
    drawEyes(dist);
  }

  static unsigned long lastStatus = 0;
  if (now - lastStatus >= 2000) {
    lastStatus = now;
    Serial.print("dist=");
    Serial.print(dist);
    Serial.print("cm  noteHz=");
    Serial.print(target_freq, 1);
    Serial.print("  bt=");
    Serial.println(a2dp_source.is_connected() ? "connected" : "searching");
  }

  delay(15);
}