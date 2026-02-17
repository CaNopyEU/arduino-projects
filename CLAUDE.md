# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

Arduino projects for MEGA2560 (clone). Each project lives in its own directory with a `.ino` sketch and a `README.md` with wiring diagrams, component lists, and calibration instructions.

## Project Structure

- `arduino-radar/` — Ultrasonic radar: HC-SR04 + Servo, with Processing visualization (`radar_processing/radar_processing.pde`)
- `arduino-laser-alarm/` — Multi-sensor security system: laser tripwire + PIR + RFID RC522 + LCD1602 + RGB LED + buzzer
- `arduino-gesture-maze/` — Tilt-controlled maze game: GY-521 (MPU-6050) + MAX7219 8x8 LED matrix
- `components.md` — Hardware inventory (available sensors, modules, passive components). Update when new parts are added.

## Development

**Board:** Arduino Mega 2560 (Tools → Board in Arduino IDE)

**Upload workflow:** Open `.ino` in Arduino IDE → select board + port → Upload

**Libraries required:**
- `Servo.h` — built-in (radar)
- `SPI.h`, `MFRC522` — RFID (laser alarm)
- `LiquidCrystal` — built-in (laser alarm)
- `Wire.h` — built-in, I2C (gesture maze)
- `LedControl` — install via Library Manager (gesture maze)

**Processing:** The radar project has a companion Processing sketch. Requires [Processing IDE](https://processing.org/download). Serial port index in `setup()` must match the Arduino port.

## Conventions

- All serial communication runs at **9600 baud**
- Pin definitions are `const int` at the top of each sketch
- Calibration values (thresholds, delays) are grouped as named constants near the top
- Each README contains wiring tables, physical setup diagrams, and troubleshooting sections
- Commit format: `project(arduino): #N Description`

## Hardware Notes

- RFID RC522 runs on **3.3V** (not 5V) — SPI connection on MEGA pins 50-53
- PIR sensor (HC-SR501) needs ~60s stabilization after power-on
- Laser alarm `LASER_THRESHOLD` and gesture maze `TILT_THRESHOLD` need per-setup calibration
- GY-521 uses I2C: SDA=Pin 20, SCL=Pin 21 on MEGA (A4/A5 on UNO)
