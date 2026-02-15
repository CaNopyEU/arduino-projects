# Gesture Maze

Tilt-controlled maze game on 8x8 LED matrix using GY-521 gyroscope/accelerometer.

## How It Works

Tilt the GY-521 module to navigate a glowing dot through mazes displayed on the LED matrix. Reach the blinking goal to advance to the next level. 5 levels from open playground to complex mazes.

## Components

| Component | Qty |
|-----------|-----|
| MEGA2560 | 1 |
| GY-521 (MPU-6050) | 1 |
| MAX7219 8x8 LED matrix module | 1 |
| Active buzzer | 1 |
| Push button | 1 |
| 10K resistor (for button, optional if using INPUT_PULLUP) | 1 |
| Jumper wires | ~15 |

## Wiring

### GY-521 (MPU-6050) → MEGA2560 (I2C)

```
GY-521          MEGA2560
──────          ────────
VCC    ───────→ 5V
GND    ───────→ GND
SCL    ───────→ Pin 21 (SCL)
SDA    ───────→ Pin 20 (SDA)
```

> On Arduino UNO: SCL = A5, SDA = A4

### MAX7219 LED Matrix → MEGA2560

```
MAX7219         MEGA2560
───────         ────────
VCC    ───────→ 5V
GND    ───────→ GND
DIN    ───────→ Pin 12
CS     ───────→ Pin 11
CLK    ───────→ Pin 10
```

### Buzzer + Button

```
Active Buzzer    MEGA2560
─────────────    ────────
+       ───────→ Pin 3
-       ───────→ GND

Button           MEGA2560
──────           ────────
One leg ───────→ Pin 2
Other   ───────→ GND
(uses internal pull-up, no external resistor needed)
```

## Physical Setup

Hold or mount the GY-521 flat in your hand. Tilting it controls the dot:

```
        Tilt backward → dot moves UP
                ↑
                │
Tilt left ←── [GY-521] ──→ Tilt right
                │
                ↓
        Tilt forward → dot moves DOWN
```

If directions feel inverted, swap the `>` and `<` signs for `accelX` / `accelY` in the `movePlayer()` function.

## Install Library

In Arduino IDE: Sketch → Include Library → Manage Libraries → search **LedControl** → Install

## Levels

| Level | Description |
|-------|-------------|
| 0 | Open field — learn the controls, no walls |
| 1 | Simple corridors |
| 2 | Spiral path |
| 3 | Zigzag |
| 4 | Chambers |

Bright dots = walls. Your dot moves through empty spaces. The blinking dot = goal.

## Controls

| Input | Action |
|-------|--------|
| Tilt GY-521 | Move the dot |
| Hit a wall | Short buzz, dot stays |
| Reach goal | Victory melody, next level |
| All levels done | Win animation (checkerboard) |
| Press button | Restart from level 0 |

## Calibration

If the dot drifts or is too sensitive/sluggish, adjust these values in the code:

```cpp
const int TILT_THRESHOLD = 3000;   // higher = less sensitive (default 3000)
const int MOVE_DELAY_MS  = 120;    // higher = slower movement (default 120ms)
```

## Custom Mazes

Each maze is an 8x8 bitmap in the code. `1` = wall, `0` = empty:

```cpp
const byte PROGMEM maze_X[8] = {
  B00000000,  // row 0 (top)
  B01111010,  // row 1
  B00000010,  // row 2
  B01011110,  // row 3
  B01010000,  // row 4
  B01010110,  // row 5
  B00010000,  // row 6
  B00000000   // row 7 (bottom)
};
```

Design your own mazes, add them to the `mazes[]` array, and increment `NUM_LEVELS`.

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Dot doesn't move | Check GY-521 I2C wiring (SDA=20, SCL=21 on MEGA) |
| Dot drifts without tilting | Lower `TILT_THRESHOLD` or place GY-521 on flat surface to test |
| Directions inverted | Swap `>` / `<` for accelX or accelY in `movePlayer()` |
| Matrix shows nothing | Check MAX7219 VCC is 5V, check DIN/CS/CLK pins |
| Matrix shows garbage | Verify LedControl library installed, check CS pin |
| Button doesn't restart | Check button wiring, should connect pin 2 to GND when pressed |
