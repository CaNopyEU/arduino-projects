// Ultrasonic Radar - Arduino / MEGA2560
// Servo sweeps 0-180°, HC-SR04 measures distance at each angle
// Sends "angle,distance\n" over Serial at 9600 baud

#include <Servo.h>

const int TRIG_PIN = 9;
const int ECHO_PIN = 10;
const int SERVO_PIN = 11;

const int MAX_DISTANCE_CM = 200;  // ignore readings beyond this
const int SWEEP_DELAY_MS = 30;    // delay between each degree step

Servo servo;

long measureDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);  // 30ms timeout
  long distance = duration * 0.034 / 2;            // cm

  if (distance == 0 || distance > MAX_DISTANCE_CM) {
    return 0;  // out of range
  }
  return distance;
}

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  servo.attach(SERVO_PIN);
  servo.write(90);
  delay(1000);
}

void loop() {
  // Sweep forward: 0 -> 180
  for (int angle = 0; angle <= 180; angle++) {
    servo.write(angle);
    delay(SWEEP_DELAY_MS);
    long dist = measureDistance();
    Serial.print(angle);
    Serial.print(",");
    Serial.println(dist);
  }

  // Sweep back: 180 -> 0
  for (int angle = 180; angle >= 0; angle--) {
    servo.write(angle);
    delay(SWEEP_DELAY_MS);
    long dist = measureDistance();
    Serial.print(angle);
    Serial.print(",");
    Serial.println(dist);
  }
}
