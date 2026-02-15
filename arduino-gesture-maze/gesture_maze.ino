// Gesture Maze - GY-521 (MPU-6050) + MAX7219 8x8 LED Matrix
//
// Tilt the gyroscope module to move a dot through mazes.
// Buzzer sounds on wall hit, melody plays on level complete.
//
// Libraries needed:
//   - LedControl (install via Library Manager)
//
// Hardware:
//   - MEGA2560 / Arduino UNO
//   - GY-521 (MPU-6050) — I2C
//   - MAX7219 8x8 LED matrix module
//   - Active buzzer
//   - Button (optional: restart)

#include <Wire.h>
#include <LedControl.h>

// --- Pin definitions ---

// MAX7219 (directly directly directly directly directly directly directly directly
// directly directly directly bit-banged SPI, any digital pins work)
const int MAX_DIN = 12;  // DIN
const int MAX_CS  = 11;  // CS/LOAD
const int MAX_CLK = 10;  // CLK

// Buzzer
const int BUZZER_PIN = 3;

// Button (optional restart)
const int BTN_PIN = 2;

// MPU-6050 I2C address
const int MPU_ADDR = 0x68;

// --- Configuration ---
const int TILT_THRESHOLD = 3000;   // raw accel threshold to start moving
const int MOVE_DELAY_MS  = 120;    // ms between moves (game speed)
const int BRIGHTNESS     = 4;      // 0-15, LED brightness

// --- Game state ---
int playerX = 0;
int playerY = 0;
int goalX   = 0;
int goalY   = 0;
int currentLevel = 0;
bool goalVisible = true;
unsigned long lastMove   = 0;
unsigned long lastBlink  = 0;
bool gameWon = false;

// Accelerometer raw values
int16_t accelX, accelY;

// --- Objects ---
LedControl lc = LedControl(MAX_DIN, MAX_CLK, MAX_CS, 1);

// --- Maze definitions ---
// 8x8 bitmap: 1 = wall, 0 = empty
// Player starts at top-left open space, goal at bottom-right area

const int NUM_LEVELS = 5;

// Level 0: Open playground (no walls, just learn controls)
const byte PROGMEM maze_0[8] = {
  B00000000,
  B00000000,
  B00000000,
  B00000000,
  B00000000,
  B00000000,
  B00000000,
  B00000000
};

// Level 1: Simple corridors
const byte PROGMEM maze_1[8] = {
  B00000000,
  B01111010,
  B00000010,
  B01011110,
  B01010000,
  B01010110,
  B00010000,
  B00000000
};

// Level 2: Spiral
const byte PROGMEM maze_2[8] = {
  B00000000,
  B01111110,
  B00000010,
  B01111010,
  B01000010,
  B01011110,
  B01000000,
  B00000000
};

// Level 3: Zigzag
const byte PROGMEM maze_3[8] = {
  B00000000,
  B11111010,
  B00001010,
  B01101010,
  B01001010,
  B01011010,
  B01000010,
  B00000000
};

// Level 4: Chambers
const byte PROGMEM maze_4[8] = {
  B00000000,
  B01010110,
  B01010000,
  B00010110,
  B01110100,
  B00000100,
  B01110110,
  B00000000
};

const byte* const mazes[NUM_LEVELS] PROGMEM = {
  maze_0, maze_1, maze_2, maze_3, maze_4
};

// Start and goal positions for each level {startX, startY, goalX, goalY}
const byte levelConfig[NUM_LEVELS][4] = {
  {0, 0, 7, 7},  // Level 0
  {0, 0, 7, 7},  // Level 1
  {0, 0, 6, 6},  // Level 2
  {6, 0, 7, 7},  // Level 3
  {0, 0, 7, 7},  // Level 4
};

// Current maze in RAM
byte currentMaze[8];

// --- Setup ---

void setup() {
  Serial.begin(9600);

  // Button
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  // Init MAX7219
  lc.shutdown(0, false);
  lc.setIntensity(0, BRIGHTNESS);
  lc.clearDisplay(0);

  // Init MPU-6050
  Wire.begin();
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);  // PWR_MGMT_1
  Wire.write(0);     // wake up
  Wire.endTransmission(true);

  // Set accelerometer range to ±2g (most sensitive)
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1C);  // ACCEL_CONFIG
  Wire.write(0);     // ±2g
  Wire.endTransmission(true);

  Serial.println("=== GESTURE MAZE ===");
  Serial.println("Tilt the GY-521 to move.");
  Serial.println("Reach the blinking goal!");
  Serial.println();

  startupAnimation();
  loadLevel(0);
}

// --- Main loop ---

void loop() {
  // Restart button
  if (digitalRead(BTN_PIN) == LOW) {
    delay(200);  // debounce
    loadLevel(0);
    return;
  }

  if (gameWon) {
    // Wait for button or auto-restart
    winAnimation();
    return;
  }

  unsigned long now = millis();

  // Read accelerometer
  readAccel();

  // Move player
  if (now - lastMove >= MOVE_DELAY_MS) {
    lastMove = now;
    movePlayer();
  }

  // Draw frame
  drawMaze();
  drawGoal(now);
  drawPlayer();

  // Check win
  if (playerX == goalX && playerY == goalY) {
    levelComplete();
  }
}

// --- MPU-6050 ---

void readAccel() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);  // ACCEL_XOUT_H
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 4, true);

  accelX = (Wire.read() << 8) | Wire.read();  // X
  accelY = (Wire.read() << 8) | Wire.read();  // Y
}

// --- Movement ---

void movePlayer() {
  int newX = playerX;
  int newY = playerY;

  // Map accelerometer to direction
  // Note: axes depend on how you orient the GY-521
  // X-axis tilt → left/right movement on matrix
  // Y-axis tilt → up/down movement on matrix
  if (accelX > TILT_THRESHOLD)  newY++;   // tilt forward  → down
  if (accelX < -TILT_THRESHOLD) newY--;   // tilt backward → up
  if (accelY > TILT_THRESHOLD)  newX--;   // tilt left     → left
  if (accelY < -TILT_THRESHOLD) newX++;   // tilt right    → right

  // Boundary check
  newX = constrain(newX, 0, 7);
  newY = constrain(newY, 0, 7);

  // Wall collision check
  if (isWall(newX, newY)) {
    // Hit wall — buzz and don't move
    tone(BUZZER_PIN, 150, 30);
    return;
  }

  // Move if position changed
  if (newX != playerX || newY != playerY) {
    playerX = newX;
    playerY = newY;
  }
}

bool isWall(int x, int y) {
  if (x < 0 || x > 7 || y < 0 || y > 7) return true;
  return bitRead(currentMaze[y], 7 - x);
}

// --- Level management ---

void loadLevel(int level) {
  currentLevel = level % NUM_LEVELS;
  gameWon = false;

  // Load maze from PROGMEM
  const byte* mazePtr = (const byte*)pgm_read_ptr(&mazes[currentLevel]);
  for (int i = 0; i < 8; i++) {
    currentMaze[i] = pgm_read_byte(&mazePtr[i]);
  }

  // Set positions
  playerX = levelConfig[currentLevel][0];
  playerY = levelConfig[currentLevel][1];
  goalX   = levelConfig[currentLevel][2];
  goalY   = levelConfig[currentLevel][3];

  Serial.print("Level ");
  Serial.print(currentLevel);
  Serial.println(" loaded!");

  lc.clearDisplay(0);

  // Short beep
  tone(BUZZER_PIN, 800, 100);
  delay(150);
  tone(BUZZER_PIN, 1000, 100);
  delay(150);
  noTone(BUZZER_PIN);
}

void levelComplete() {
  Serial.print("Level ");
  Serial.print(currentLevel);
  Serial.println(" complete!");

  // Victory melody
  int melody[] = {523, 659, 784, 1047};  // C5, E5, G5, C6
  for (int i = 0; i < 4; i++) {
    tone(BUZZER_PIN, melody[i], 150);
    delay(200);
  }
  noTone(BUZZER_PIN);

  delay(500);

  if (currentLevel < NUM_LEVELS - 1) {
    loadLevel(currentLevel + 1);
  } else {
    gameWon = true;
    Serial.println("ALL LEVELS COMPLETE!");
  }
}

// --- Drawing ---

void drawMaze() {
  for (int row = 0; row < 8; row++) {
    lc.setRow(0, row, currentMaze[row]);
  }
}

void drawPlayer() {
  lc.setLed(0, playerY, 7 - playerX, true);
}

void drawGoal(unsigned long now) {
  // Blink goal every 300ms
  if (now - lastBlink >= 300) {
    lastBlink = now;
    goalVisible = !goalVisible;
  }
  if (goalVisible) {
    lc.setLed(0, goalY, 7 - goalX, true);
  }
}

// --- Animations ---

void startupAnimation() {
  // Fill matrix row by row, then clear
  for (int r = 0; r < 8; r++) {
    lc.setRow(0, r, 0xFF);
    delay(50);
  }
  delay(200);
  for (int r = 7; r >= 0; r--) {
    lc.setRow(0, r, 0x00);
    delay(50);
  }
  delay(300);
}

void winAnimation() {
  // Alternating pattern
  unsigned long now = millis();
  bool tick = (now / 300) % 2;

  byte pattern = tick ? 0xAA : 0x55;
  for (int r = 0; r < 8; r++) {
    lc.setRow(0, r, (r % 2 == 0) ? pattern : ~pattern);
  }

  // Restart on button press
  if (digitalRead(BTN_PIN) == LOW) {
    delay(200);
    loadLevel(0);
  }
}
