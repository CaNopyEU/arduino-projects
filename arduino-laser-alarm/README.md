# Laser Security Alarm

Multi-layer security system with laser tripwire, PIR motion detection, and RFID access control.

## Features

- **Laser tripwire** - laser beam hits photoresistor; breaking the beam triggers alarm
- **PIR motion detection** - second detection layer
- **RFID arm/disarm** - scan authorized card to toggle system
- **LCD status display** - shows state, sensor readings, events
- **RGB LED** - green (disarmed), blue (armed), red (alarm)
- **Buzzer siren** - alternating tone alarm on intrusion
- **3-second arming countdown** - time to leave the area
- **Auto-reset** - alarm resets after 5 seconds, returns to armed state

## Components

| Component | From Kit | Qty |
|-----------|----------|-----|
| MEGA2560 | Starter kit | 1 |
| Laser module 5V | Sensor kit | 1 |
| Photoresistor | Both kits | 1 |
| HC-SR501 PIR sensor | Starter kit | 1 |
| RC522 RFID module | Starter kit | 1 |
| LCD1602 | Starter kit | 1 |
| RGB LED | Both kits | 1 |
| Active buzzer | Both kits | 1 |
| 10K resistor | Starter kit | 2 |
| 220 ohm resistor | Starter kit | 3 |
| Breadboard + wires | Starter kit | 1 |

## Wiring

### RFID RC522 → MEGA2560 (SPI)

```
RC522           MEGA2560
─────           ────────
SDA   ────────→ Pin 53
SCK   ────────→ Pin 52
MOSI  ────────→ Pin 51
MISO  ────────→ Pin 50
GND   ────────→ GND
RST   ────────→ Pin 5
3.3V  ────────→ 3.3V  ⚠️ NOT 5V!
```

### LCD1602 → MEGA2560 (4-bit)

```
LCD             MEGA2560
───             ────────
VSS   ────────→ GND
VDD   ────────→ 5V
V0    ────────→ potentiometer middle pin (contrast)
RS    ────────→ Pin 22
RW    ────────→ GND
E     ────────→ Pin 23
D4    ────────→ Pin 24
D5    ────────→ Pin 25
D6    ────────→ Pin 26
D7    ────────→ Pin 27
A     ────────→ 5V (backlight)
K     ────────→ GND (backlight)
```

> **Tip:** If you don't have a potentiometer for contrast (V0), connect V0 to GND through a 2K resistor, or just connect to GND directly and adjust from there.

### Sensors

```
Laser module         MEGA2560
────────────         ────────
S (signal)  ───────→ Pin 8
+           ───────→ 5V
-           ───────→ GND

Photoresistor circuit (voltage divider):

    5V ──── [Photoresistor] ──┬── [10K to GND]
                               │
                               └──→ A0

HC-SR501 PIR         MEGA2560
────────────         ────────
VCC         ───────→ 5V
OUT         ───────→ Pin 2
GND         ───────→ GND
```

### Outputs

```
RGB LED (common cathode)     MEGA2560
────────────────────         ────────
R  ── [220Ω] ─────────────→ Pin 9
G  ── [220Ω] ─────────────→ Pin 10
B  ── [220Ω] ─────────────→ Pin 11
GND ───────────────────────→ GND

Active Buzzer                MEGA2560
─────────────                ────────
+           ───────────────→ Pin 3
-           ───────────────→ GND
```

## Physical Setup

```
    [Laser]  ·  ·  ·  ·  ·  ·  · → [Photoresistor]
       ↑                                  ↑
    One side                         Other side
    of doorway                       of doorway

    Place PIR sensor nearby pointing at the passage area.
```

The laser beam should directly hit the photoresistor. Align them at the same height. When someone walks through, the beam breaks → alarm.

## Setup & Calibration

### 1. Find your RFID card UID

1. Upload the code
2. Open Serial Monitor (9600 baud)
3. Scan your RFID card/tag
4. Copy the UID printed (e.g. `A3:B2:C1:D0`)
5. Update `authorizedUIDs` array in the code:
   ```cpp
   byte authorizedUIDs[MAX_CARDS][4] = {
     {0xA3, 0xB2, 0xC1, 0xD0},  // your card
   };
   ```
6. Re-upload

### 2. Calibrate laser threshold

1. Arm the system, check Serial Monitor
2. LCD line 2 shows `L:XXX` — this is the light level on photoresistor
3. Note the value with laser ON and hitting the sensor (e.g. 800)
4. Note the value with laser blocked (e.g. 150)
5. Set `LASER_THRESHOLD` to a value between them (e.g. 300)

### 3. PIR sensitivity

The HC-SR501 has two potentiometers on the board:
- **Sensitivity** — turn clockwise to increase range (up to ~7m)
- **Time delay** — how long output stays HIGH after detection

Set time delay to minimum for faster response.

## Usage

| Action | Result |
|--------|--------|
| Scan authorized card | Toggle armed ↔ disarmed |
| Scan unknown card | "ACCESS DENIED" + orange flash |
| Break laser beam (armed) | ALARM — red flash + siren |
| Motion detected (armed) | ALARM — red flash + siren |
| After 5 sec alarm | Auto-resets to ARMED |

## Troubleshooting

| Problem | Solution |
|---------|----------|
| RFID not reading | Check 3.3V (not 5V!), check SPI wiring |
| LCD blank | Adjust contrast potentiometer / V0 resistor |
| Laser threshold wrong | Recalibrate — ambient light affects readings |
| PIR false triggers | Wait 60s after power-on, reduce sensitivity pot |
| Buzzer always on | Check pin polarity, ensure pin 3 is LOW at startup |
