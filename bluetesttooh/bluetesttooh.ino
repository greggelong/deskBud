#include <Arduino.h>
#include "BluetoothA2DPSource.h"

BluetoothA2DPSource a2dp_source;

// --- Tone generator state ---
float phase = 0.0;
const float sample_rate = 44100.0;
const float two_pi = 2.0 * PI;
float freq = 440.0;

// --- Frame-based audio callback (matches set_data_callback_in_frames) ---
int32_t get_sound_data(Frame *data, int32_t len) {
  for (int i = 0; i < len; i++) {
    float sample = sin(phase) * 0.3;          // 0.3 amplitude, avoid clipping
    int16_t s = (int16_t)(sample * 32767);
    data[i].channel1 = s;
    data[i].channel2 = s;
    phase += two_pi * freq / sample_rate;
    if (phase > two_pi) phase -= two_pi;
  }
  return len;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("=== A2DP TEST START ===");

  // Register the frame-based callback BEFORE starting
  a2dp_source.set_data_callback_in_frames(get_sound_data);
  a2dp_source.set_volume(80);

  // Replace with your speaker's exact Bluetooth name
  const char* speakerName = "A17";
  Serial.print("Starting A2DP, target: ");
  Serial.println(speakerName);

  a2dp_source.start(speakerName);

  Serial.println("A2DP started. Waiting for connection...");
  Serial.println("Make sure the speaker is on and in pairing mode.");
}

void loop() {
  // Report connection status once per second
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 1000) {
    lastPrint = millis();
    if (a2dp_source.is_connected()) {
      Serial.println("STATUS: CONNECTED - you should hear a 440 Hz tone");
    } else {
      Serial.println("STATUS: not connected yet");
    }
  }
  delay(10);
}