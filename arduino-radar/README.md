# Ultrasonic Radar

Arduino/MEGA2560 + HC-SR04 + Servo → radar visualization in Processing.

## Wiring

```
MEGA2560          HC-SR04
--------          -------
5V       ------→  VCC
GND      ------→  GND
Pin 9    ------→  TRIG
Pin 10   ------→  ECHO

MEGA2560          Servo SG90
--------          ----------
5V       ------→  RED wire (VCC)
GND      ------→  BROWN wire (GND)
Pin 11   ------→  ORANGE wire (Signal)
```

### Physical setup

Mount the ultrasonic sensor on top of the servo horn (use hot glue, tape, or a small bracket).
The sensor should face outward so when the servo rotates, the sensor scans the area.

```
        ┌─────────┐
        │ HC-SR04  │  ← sensor faces forward
        │  ○   ○   │
        └────┬────┘
             │
        ┌────┴────┐
        │  SERVO  │  ← rotates 0-180°
        └─────────┘
```

## Upload Arduino Code

1. Open `radar.ino` in Arduino IDE
2. Select board: **Arduino Mega 2560** (Tools → Board)
3. Select correct port (Tools → Port)
4. Upload

## Run Processing Visualization

1. Install [Processing](https://processing.org/download)
2. Open `radar_processing/radar_processing.pde`
3. **Important:** Check the console output for serial ports and update the port index in `setup()` if needed:
   ```java
   String portName = Serial.list()[0];  // change index if needed
   ```
4. Click Run

## What You'll See

- Green radar display with distance arcs (50, 100, 150, 200 cm)
- Green sweep line following the servo rotation
- Red dots where objects are detected
- HUD showing current angle and distance

## Troubleshooting

| Problem | Solution |
|---------|----------|
| No data in Processing | Check serial port index, close Arduino Serial Monitor (only one app can use the port) |
| Servo jitters | Use external 5V power supply for servo instead of Arduino 5V |
| Readings are 0 | Check TRIG/ECHO wiring, ensure sensor faces open area |
| Processing crashes on start | Install Processing Serial library (Sketch → Import Library → Serial) |
