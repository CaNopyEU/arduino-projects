// Sound Visualizer - Analog Microphone + MAX7219 8x8 LED Matrix + Passive Buzzer
//
// Displays sound level on LED matrix with two visualization modes
// (bar graph / center burst) and theremin-like tone feedback.
//
// Libraries needed:
//   - LedControl (install via Library Manager)
//
// Hardware:
//   - MEGA2560
//   - Sound sensor (analog microphone module) — A0
//   - MAX7219 8x8 LED matrix module
//   - Passive buzzer
//   - Push button (mode switch)

#include <LedControl.h>

// --- Pin definitions ---

// Sound sensor
const int MIC_PIN = A0;

// MAX7219 (bit-banged SPI, any digital pins work)
const int MAX_DIN = 12;  // DIN
const int MAX_CS  = 11;  // CS/LOAD
const int MAX_CLK = 10;  // CLK

// Passive buzzer (PWM-capable pin for tone())
const int BUZZER_PIN = 3;

// Mode switch button
const int BTN_PIN = 2;

// --- Calibration constants ---
const int SILENCE_THRESHOLD = 30;    // peak-to-peak below this = silence (adjust per setup)
const int SAMPLE_WINDOW_MS  = 50;    // sampling window for peak-to-peak measurement
const int SAMPLE_COUNT      = 8;     // samples stored for bar graph columns
const int BUZZER_MIN_FREQ   = 100;   // Hz — lowest tone
const int BUZZER_MAX_FREQ   = 2000;  // Hz — highest tone
const int BRIGHTNESS        = 4;     // 0-15, LED brightness
const int MAX_AMPLITUDE     = 500;   // expected max peak-to-peak (clip above this)

// --- Visualization modes ---
const int MODE_BAR_GRAPH   = 0;
const int MODE_CENTER_BURST = 1;
const int NUM_MODES        = 2;

// --- State ---
int currentMode = MODE_BAR_GRAPH;
bool lastButtonState = HIGH;
unsigned long lastDebounce = 0;
const int DEBOUNCE_MS = 200;

// Bar graph history (8 columns, newest at index 7)
int barHistory[SAMPLE_COUNT] = {0};

// --- Objects ---
LedControl lc = LedControl(MAX_DIN, MAX_CLK, MAX_CS, 1);

// --- Column patterns (bottom-up fill, 1-8 LEDs lit) ---
// Row 7 = bottom, Row 0 = top. Bit 0 = row 7 in setColumn().
// We draw manually per column using setLed() for clarity.

// --- Setup ---

void setup() {
  Serial.begin(9600);

  // Button with internal pull-up
  pinMode(BTN_PIN, INPUT_PULLUP);

  // Buzzer
  pinMode(BUZZER_PIN, OUTPUT);

  // Init MAX7219
  lc.shutdown(0, false);
  lc.setIntensity(0, BRIGHTNESS);
  lc.clearDisplay(0);

  Serial.println("=== SOUND VISUALIZER ===");
  Serial.println("Mode: Bar Graph");
  Serial.println("Press button to switch modes.");
  Serial.println();

  startupAnimation();
}

// --- Main loop ---

void loop() {
  // 1. Check button for mode switch
  checkButton();

  // 2. Sample sound (peak-to-peak)
  int amplitude = readSound();

  // 3. Map to level 0-8
  int level = map(constrain(amplitude, 0, MAX_AMPLITUDE), 0, MAX_AMPLITUDE, 0, 8);

  // Debug output
  Serial.print("Raw: ");
  Serial.print(amplitude);
  Serial.print(" Level: ");
  Serial.print(level);
  Serial.print(" Mode: ");
  Serial.println(currentMode == MODE_BAR_GRAPH ? "Bar" : "Center");

  // 4. Update display based on mode
  switch (currentMode) {
    case MODE_BAR_GRAPH:
      updateBarGraph(level);
      drawBarGraph();
      break;
    case MODE_CENTER_BURST:
      drawCenterBurst(level);
      break;
  }

  // 5. Buzzer — theremin effect
  if (amplitude > SILENCE_THRESHOLD) {
    int freq = map(constrain(amplitude, SILENCE_THRESHOLD, MAX_AMPLITUDE),
                   SILENCE_THRESHOLD, MAX_AMPLITUDE,
                   BUZZER_MIN_FREQ, BUZZER_MAX_FREQ);
    tone(BUZZER_PIN, freq);
  } else {
    noTone(BUZZER_PIN);
  }
}

// --- Sound sampling ---

int readSound() {
  // Measure peak-to-peak amplitude over a sampling window
  unsigned long startTime = millis();
  int signalMax = 0;
  int signalMin = 1023;

  while (millis() - startTime < SAMPLE_WINDOW_MS) {
    int sample = analogRead(MIC_PIN);
    if (sample > signalMax) signalMax = sample;
    if (sample < signalMin) signalMin = sample;
  }

  return signalMax - signalMin;  // peak-to-peak amplitude
}

// --- Button handling ---

void checkButton() {
  bool reading = digitalRead(BTN_PIN);

  if (reading == LOW && lastButtonState == HIGH) {
    unsigned long now = millis();
    if (now - lastDebounce > DEBOUNCE_MS) {
      lastDebounce = now;
      currentMode = (currentMode + 1) % NUM_MODES;

      lc.clearDisplay(0);

      // Reset bar history on mode switch
      for (int i = 0; i < SAMPLE_COUNT; i++) {
        barHistory[i] = 0;
      }

      Serial.print("Mode switched to: ");
      Serial.println(currentMode == MODE_BAR_GRAPH ? "Bar Graph" : "Center Burst");
    }
  }

  lastButtonState = reading;
}

// --- Bar Graph mode ---

void updateBarGraph(int level) {
  // Shift history left, add new reading at the end
  for (int i = 0; i < SAMPLE_COUNT - 1; i++) {
    barHistory[i] = barHistory[i + 1];
  }
  barHistory[SAMPLE_COUNT - 1] = level;
}

void drawBarGraph() {
  lc.clearDisplay(0);

  for (int col = 0; col < 8; col++) {
    int height = barHistory[col];
    // Light LEDs from bottom (row 7) upward
    for (int row = 0; row < height; row++) {
      lc.setLed(0, 7 - row, col, true);
    }
  }
}

// --- Center Burst mode ---

void drawCenterBurst(int level) {
  lc.clearDisplay(0);

  if (level == 0) return;

  // Map level 1-8 to rings from center outward
  // Center of 8x8 is between pixels 3,4 — we use concentric rectangular rings
  // Ring 0 (level 1-2): inner 2x2 (rows 3-4, cols 3-4)
  // Ring 1 (level 3-4): 4x4 (rows 2-5, cols 2-5)
  // Ring 2 (level 5-6): 6x6 (rows 1-6, cols 1-6)
  // Ring 3 (level 7-8): 8x8 (rows 0-7, cols 0-7)

  int rings = (level + 1) / 2;  // 1-4 rings
  rings = constrain(rings, 1, 4);

  for (int ring = 0; ring < rings; ring++) {
    int startPos = 3 - ring;
    int endPos   = 4 + ring;

    startPos = constrain(startPos, 0, 7);
    endPos   = constrain(endPos, 0, 7);

    // Draw the four edges of this ring
    for (int i = startPos; i <= endPos; i++) {
      lc.setLed(0, startPos, i, true);  // top edge
      lc.setLed(0, endPos, i, true);    // bottom edge
      lc.setLed(0, i, startPos, true);  // left edge
      lc.setLed(0, i, endPos, true);    // right edge
    }
  }
}

// --- Animations ---

void startupAnimation() {
  // Center burst expand/contract
  for (int ring = 0; ring < 4; ring++) {
    int s = 3 - ring;
    int e = 4 + ring;
    for (int i = s; i <= e; i++) {
      lc.setLed(0, s, i, true);
      lc.setLed(0, e, i, true);
      lc.setLed(0, i, s, true);
      lc.setLed(0, i, e, true);
    }
    delay(80);
  }
  delay(200);
  for (int ring = 3; ring >= 0; ring--) {
    int s = 3 - ring;
    int e = 4 + ring;
    for (int i = s; i <= e; i++) {
      lc.setLed(0, s, i, false);
      lc.setLed(0, e, i, false);
      lc.setLed(0, i, s, false);
      lc.setLed(0, i, e, false);
    }
    delay(80);
  }
  delay(300);
}
