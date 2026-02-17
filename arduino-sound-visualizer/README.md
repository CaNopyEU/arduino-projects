# Sound Visualizer

Sound-reactive LED display with theremin-like tone feedback. An analog microphone captures ambient sound, the MAX7219 8x8 LED matrix visualizes volume, and a passive buzzer maps loudness to pitch.

## How It Works

The microphone module outputs an analog signal proportional to sound pressure. The sketch samples this signal over a 50ms window, calculates peak-to-peak amplitude, and maps it to LED brightness/pattern and buzzer frequency. A button cycles between two visualization modes.

## Components

| Component | Qty |
|-----------|-----|
| MEGA2560 | 1 |
| Sound sensor (analog microphone module) | 1 |
| MAX7219 8x8 LED matrix module | 1 |
| Passive buzzer | 1 |
| Push button | 1 |
| Jumper wires | ~12 |

## Wiring

### Sound Sensor → MEGA2560

```
Sound Sensor     MEGA2560
────────────     ────────
VCC     ───────→ 5V
GND     ───────→ GND
A0/OUT  ───────→ A0 (analog output)
```

> Use the **analog output** pin on the sensor module (not the digital/threshold pin).

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

### Passive Buzzer + Button

```
Passive Buzzer   MEGA2560
──────────────   ────────
+       ───────→ Pin 3 (PWM)
-       ───────→ GND

Button           MEGA2560
──────           ────────
One leg ───────→ Pin 2
Other   ───────→ GND
(uses internal pull-up, no external resistor needed)
```

## Visualization Modes

Press the button to cycle between modes:

| Mode | Description |
|------|-------------|
| **Bar Graph** | 8 columns scrolling left-to-right, height = current volume. Creates a moving waveform effect. |
| **Center Burst** | Concentric rings expand from center outward proportional to volume. Silence = blank, loud = full matrix. |

## Buzzer (Theremin Effect)

The passive buzzer plays a tone whose frequency is proportional to detected sound level:
- **Quiet** (below `SILENCE_THRESHOLD`) → buzzer off
- **Soft sound** → low pitch (~100 Hz)
- **Loud sound** → high pitch (~2000 Hz)

## Install Library

In Arduino IDE: Sketch → Include Library → Manage Libraries → search **LedControl** → Install

## Calibration

Adjust these constants at the top of the sketch to match your environment:

```cpp
const int SILENCE_THRESHOLD = 30;    // peak-to-peak below this = silence
const int MAX_AMPLITUDE     = 500;   // expected max peak-to-peak (raise if clipping)
const int BUZZER_MIN_FREQ   = 100;   // lowest buzzer tone (Hz)
const int BUZZER_MAX_FREQ   = 2000;  // highest buzzer tone (Hz)
const int SAMPLE_WINDOW_MS  = 50;    // sampling window duration
const int BRIGHTNESS        = 4;     // LED brightness 0-15
```

### How to find good values

1. Upload the sketch and open **Serial Monitor** (9600 baud)
2. In a quiet room, note the `Raw:` values — set `SILENCE_THRESHOLD` just above these
3. Clap or speak loudly near the mic, note the peak `Raw:` values — set `MAX_AMPLITUDE` to roughly match
4. If the matrix barely lights up, lower `MAX_AMPLITUDE`
5. If the matrix is always fully lit, raise `MAX_AMPLITUDE`

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Matrix shows nothing | Check MAX7219 VCC is 5V, verify DIN/CS/CLK pins |
| No reaction to sound | Check sensor is connected to **A0**, use analog output pin (not digital) |
| Always fully lit | Raise `MAX_AMPLITUDE` or `SILENCE_THRESHOLD` |
| Barely lights up | Lower `MAX_AMPLITUDE`, or move mic closer to sound source |
| Buzzer hums constantly | Raise `SILENCE_THRESHOLD` to filter ambient noise |
| Buzzer doesn't sound | Verify it's a **passive** buzzer (active buzzers don't respond to `tone()`) |
| Button doesn't switch modes | Check button wiring — should connect pin 2 to GND when pressed |
| Serial shows 0 values | Check sensor power (VCC to 5V) and analog output wiring |
