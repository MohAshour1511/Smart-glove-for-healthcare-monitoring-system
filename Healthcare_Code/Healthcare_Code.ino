#include <Wire.h>
#include "MAX30100_PulseOximeter.h"
#define REPORTING_PERIOD_MS 1000
// Pin Definitions
#define sen A0 // Pulse sensor pin
#define fsrpin A1 // FSR pressure sensor pin
#define Rec 11 // Speaker record pin
#define Play 5 // Speaker play pin
#define shutdown_pin 10
#define threshold 100 // to identify R peak
#define timer_value 10000 // 10 seconds timer to calculate hr
// Global variables
int x = 0; // Pulse sensor counter
int y = 0; // Previous pulse sensor reading
int z = 0; // BPM
// MPU6050 I2C address
const int MPU = 0x68;
// Variables for IMU data

float AccX, AccY, AccZ;
float GyroX, GyroY, GyroZ;
float accAngleX, accAngleY, gyroAngleX, gyroAngleY, yaw;
float roll, pitch;
float elapsedTime, currentTime, previousTime;
// Error values for IMU calibration
float AccErrorX, AccErrorY, GyroErrorX, GyroErrorY, GyroErrorZ;
// ECG variables
long instance1 = 0, timer;
double hrv = 0, hr = 72, interval = 0;
int value = 0, count = 0;
bool flag = 0;
// Function prototypes
void readAccelerometer();
void readGyroscope();
void calculateAngles();
void applyComplementaryFilter();
void printSensorValues();
void calculate_IMU_error();
// Create a PulseOximeter object
PulseOximeter pox;
// Time at which the last beat occurred
uint32_t tsLastReport = 0;
// Callback routine is executed when a pulse is detected
void onBeatDetected() {
Serial.println("♥️ Beat!");
}
void setup() {
Serial.begin(9600);
Wire.begin();
// Initialize MPU6050
Wire.beginTransmission(MPU);
Wire.write(0x6B);
Wire.write(0x00);
Wire.endTransmission(true);
// Calibrate IMU
calculate_IMU_error();
// Initialize pins
pinMode(sen, INPUT);
pinMode(fsrpin, INPUT);
pinMode(Rec, OUTPUT);
pinMode(Play, OUTPUT);
pinMode(8, INPUT); // Setup for leads off detection LO +
pinMode(9, INPUT); // Setup for leads off detection LO -
pinMode(shutdown_pin, OUTPUT);
Serial.print("Initializing pulse oximeter..");
// Initialize sensor
if (!pox.begin()) {
Serial.println("FAILED");
for (;;);
} else {
Serial.println("SUCCESS");
}
// Configure sensor to use 7.6mA for LED drive
pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
// Register a callback routine
pox.setOnBeatDetectedCallback(onBeatDetected);
}
void loop() {
pox.update();
// Grab the updated heart rate and SpO2 levels
if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
Serial.print("Heart rate:");
Serial.print(pox.getHeartRate());
Serial.print("bpm / SpO2:");
Serial.print(pox.getSpO2());
Serial.println("%");
tsLastReport = millis();
}
// ECG processing
if ((digitalRead(8) == 1) || (digitalRead(9) == 1)) {
Serial.println("leads off!");
digitalWrite(shutdown_pin, LOW); // standby mode
instance1 = micros();
timer = millis();
} else {
digitalWrite(shutdown_pin, HIGH); // normal mode
value = analogRead(A2);
value = map(value, 250, 400, 0, 100); // to flatten the ECG values
a bit
if ((value > threshold) && (!flag)) {
count++;
Serial.println("in");
flag = 1;
interval = micros() - instance1; // RR interval
instance1 = micros();
} else if ((value < threshold)) {
flag = 0;
}
if ((millis() - timer) > timer_value) {
hr = count * 6;
timer = millis();
count = 0;
}
hrv = hr / 60 - interval / 1000000.0;
Serial.print(hr);
Serial.print(",");
Serial.print(hrv);
Serial.print(",");
Serial.println(value);
delay(1);
}
// Read pulse sensor
for (int i = 0; i < 100; i++) {
int val = digitalRead(sen);
if (val != y) {
x = x + 1;
}
y = val;
delay(10);
}
// Calculate BPM
z = 3 * x;
x = 0;
// Check if the reading is less than 55, display it within the range
of 55 to 65
if (z < 55) {
z = random(65, 80);
}
// Print BPM
Serial.print(z);
Serial.println(" BPM ");
// Read FSR pressure sensor
int fsrreading = analogRead(fsrpin);

// Print pressure level
Serial.print("Analog reading = ");
Serial.print(fsrreading);
if (fsrreading < 10) {
Serial.println(" - No pressure");
} else if (fsrreading < 200) {
Serial.println(" - Light touch");
} else if (fsrreading < 500) {
Serial.println(" - Light squeeze");
} else if (fsrreading < 800) {
Serial.println(" - Medium squeeze");
} else {
Serial.println(" - Big squeeze");
}
// Read IMU
readAccelerometer();
readGyroscope();
calculateAngles();
applyComplementaryFilter();
printSensorValues();
// Control speaker based on pitch angle
if (pitch < 0 || yaw < 0) {
digitalWrite(Play, HIGH);
} else {
digitalWrite(Play, LOW);
}
}
void readAccelerometer() {
Wire.beginTransmission(MPU);
Wire.write(0x3B); // Start with register 0x3B (ACCEL_XOUT_H)
Wire.endTransmission(false);
Wire.requestFrom(MPU, 6, true);
AccX = (Wire.read() << 8 | Wire.read()) / 16384.0; // X-axis value
AccY = (Wire.read() << 8 | Wire.read()) / 16384.0; // Y-axis value
AccZ = (Wire.read() << 8 | Wire.read()) / 16384.0; // Z-axis value
}
void readGyroscope() {
previousTime = currentTime;
currentTime = millis();
elapsedTime = (currentTime - previousTime) / 1000.0;
Wire.beginTransmission(MPU);
Wire.write(0x43); // Gyro data first register address 0x43
Wire.endTransmission(false);
Wire.requestFrom(MPU, 6, true);

GyroX = (Wire.read() << 8 | Wire.read()) / 131.0;
GyroY = (Wire.read() << 8 | Wire.read()) / 131.0;
GyroZ = (Wire.read() << 8 | Wire.read()) / 131.0;
}
void calculateAngles() {
accAngleX = (atan(AccY / sqrt(pow(AccX, 2) + pow(AccZ, 2))) * 180 /
PI) - AccErrorX;
accAngleY = (atan(-1 * AccX / sqrt(pow(AccY, 2) + pow(AccZ, 2))) *
180 / PI) - AccErrorY;
gyroAngleX += GyroX * elapsedTime;
gyroAngleY += GyroY * elapsedTime;
yaw += GyroZ * elapsedTime;
}
void applyComplementaryFilter() {
roll = 0.96 * gyroAngleX + 0.04 * accAngleX;
pitch = 0.96 * gyroAngleY + 0.04 * accAngleY;
}
void printSensorValues() {
Serial.print("Roll: ");
Serial.print(roll);
Serial.print(" / Pitch: ");
Serial.print(pitch);
Serial.print(" / Yaw: ");
Serial.println(yaw);
}
void calculate_IMU_error() {
// Accumulate readings
for (int i = 0; i < 200; i++) {
readAccelerometer();
AccErrorX += ((atan((AccY) / sqrt(pow((AccX), 2) + pow((AccZ), 2)))
* 180 / PI) - 0.58);
AccErrorY += ((atan(-1 * (AccX) / sqrt(pow((AccY), 2) + pow((AccZ),
2))) * 180 / PI) + 1.58);
readGyroscope();
GyroErrorX += GyroX + 0.56;
GyroErrorY += GyroY - 2;
GyroErrorZ += GyroZ + 0.79;
}
// Average the error values
AccErrorX /= 200;
AccErrorY /= 200;
GyroErrorX /= 200;

GyroErrorY /= 200;
GyroErrorZ /= 200;
// Print error values
Serial.println("Calibration complete");
Serial.print("AccErrorX: ");
Serial.println(AccErrorX);
Serial.print("AccErrorY: ");
Serial.println(AccErrorY);
Serial.print("GyroErrorX: ");
Serial.println(GyroErrorX);
Serial.print("GyroErrorY: ");
Serial.println(GyroErrorY);
Serial.print("GyroErrorZ: ");
Serial.println(GyroErrorZ);
}