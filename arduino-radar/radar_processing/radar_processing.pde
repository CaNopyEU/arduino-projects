// Ultrasonic Radar - Processing Visualization
// Reads "angle,distance" from Serial and draws radar display
//
// IMPORTANT: Change Serial.list()[0] below to match your Arduino port.
// Run with Processing IDE: https://processing.org/download

import processing.serial.*;

Serial myPort;

int currentAngle = 0;
int currentDist  = 0;

// Store distance for each degree 0-180
int[] distances = new int[181];

// Fade effect: store previous sweep lines
int[][] history = new int[181][2];  // [angle][0=dist, 1=age]

void setup() {
  size(1200, 700);
  smooth();

  // --- CHANGE THIS to your Arduino serial port ---
  // Print available ports to console:
  printArray(Serial.list());
  // Pick the correct index (usually 0 on Mac/Linux, may vary on Windows)
  String portName = Serial.list()[2];
  myPort = new Serial(this, portName, 9600);
  myPort.bufferUntil('\n');

  // Init history
  for (int i = 0; i <= 180; i++) {
    distances[i] = 0;
    history[i][0] = 0;
    history[i][1] = 0;
  }
}

void draw() {
  background(0, 4, 0);  // dark green-black
  drawRadar();
  drawSweepLine();
  drawDetectedObjects();
  drawHUD();
}

void serialEvent(Serial p) {
  String raw = p.readStringUntil('\n');
  if (raw == null) return;
  raw = trim(raw);

  String[] parts = split(raw, ',');
  if (parts.length == 2) {
    try {
      currentAngle = Integer.parseInt(parts[0]);
      currentDist  = Integer.parseInt(parts[1]);

      if (currentAngle >= 0 && currentAngle <= 180) {
        distances[currentAngle] = currentDist;
        history[currentAngle][0] = currentDist;
        history[currentAngle][1] = 255;  // full brightness
      }
    } catch (Exception e) {
      // skip malformed data
    }
  }
}

void drawRadar() {
  pushMatrix();
  translate(width / 2, height - 40);

  // Radar arcs
  noFill();
  strokeWeight(1);
  for (int r = 1; r <= 4; r++) {
    float radius = r * (height - 60) / 4.0;
    stroke(0, 80, 0);
    arc(0, 0, radius * 2, radius * 2, PI, TWO_PI);

    // Distance labels
    fill(0, 120, 0);
    noStroke();
    textSize(12);
    textAlign(CENTER);
    text((r * 50) + " cm", radius + 5, -5);
  }

  // Angle lines every 30 degrees
  stroke(0, 60, 0);
  strokeWeight(1);
  for (int a = 0; a <= 180; a += 30) {
    float rad = radians(a);
    float len = height - 60;
    line(0, 0, -cos(rad) * len, -sin(rad) * len);

    // Angle labels
    fill(0, 120, 0);
    noStroke();
    textSize(11);
    textAlign(CENTER);
    text(a + "°", -cos(rad) * (len + 15), -sin(rad) * (len + 15));
    stroke(0, 60, 0);
  }

  popMatrix();
}

void drawSweepLine() {
  pushMatrix();
  translate(width / 2, height - 40);

  float rad = radians(currentAngle);
  float len = height - 60;

  // Sweep trail (fading lines behind the sweep)
  for (int i = 1; i <= 20; i++) {
    int trailAngle = currentAngle - i;
    if (trailAngle < 0) trailAngle += 181;
    if (trailAngle > 180) continue;

    float trailRad = radians(trailAngle);
    int alpha = (int) map(i, 1, 20, 100, 0);
    stroke(0, 255, 0, alpha);
    strokeWeight(1);
    line(0, 0, -cos(trailRad) * len, -sin(trailRad) * len);
  }

  // Main sweep line
  stroke(0, 255, 0, 200);
  strokeWeight(2);
  line(0, 0, -cos(rad) * len, -sin(rad) * len);

  popMatrix();
}

void drawDetectedObjects() {
  pushMatrix();
  translate(width / 2, height - 40);

  float maxRadius = height - 60;

  for (int a = 0; a <= 180; a++) {
    int dist = distances[a];
    if (dist <= 0 || dist > 200) continue;

    // Age fade
    if (history[a][1] > 0) {
      history[a][1] -= 2;
    }
    int brightness = history[a][1];
    if (brightness <= 0) continue;

    float rad = radians(a);
    float r = map(dist, 0, 200, 0, maxRadius);

    // Object dot
    noStroke();
    fill(255, 50, 50, brightness);
    float px = -cos(rad) * r;
    float py = -sin(rad) * r;
    ellipse(px, py, 8, 8);

    // Dimmer halo
    fill(255, 0, 0, brightness / 4);
    ellipse(px, py, 16, 16);
  }

  popMatrix();
}

void drawHUD() {
  // Top-left info panel
  fill(0, 200, 0);
  noStroke();
  textSize(14);
  textAlign(LEFT);

  text("ULTRASONIC RADAR", 20, 30);
  text("Angle: " + currentAngle + "°", 20, 55);
  text("Distance: " + (currentDist > 0 ? currentDist + " cm" : "---"), 20, 75);

  // Status indicator
  fill(currentDist > 0 && currentDist <= 200 ? color(255, 50, 50) : color(0, 150, 0));
  ellipse(160, 25, 10, 10);
}
