#include <ESP32Servo.h>
#include "BluetoothSerial.h"

// --- KONFIGURASI PIN (Sesuai rangkaianmu) ---
const int trigPin = 14;
const int echoPin = 12;
const int servoPin = 26;

// Motor Kiri
const int ENA = 23;
const int IN1 = 22;
const int IN2 = 21;

// Motor Kanan
const int IN3 = 19;
const int IN4 = 18;
const int ENB = 5;

// --- VARIABEL ---
Servo myServo;
BluetoothSerial SerialBT;

int motorSpeed = 255; // Default Speed Max
int currentSpeed = 150;
int distance = 0;
int distanceLimit = 30; 
bool autoMode = false; 

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32_Robot_Aldan"); // Nama Bluetooth
  Serial.println("Bluetooth Siap!");

  // Setup Pin
  pinMode(ENA, OUTPUT); pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT); pinMode(ENB, OUTPUT);
  
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  myServo.attach(servoPin);
  myServo.write(90); 
}

void loop() {
  // Cek Data Bluetooth
  if (SerialBT.available()) {
    char command = SerialBT.read();
    Serial.println(command); // Untuk debugging

    // --- LOGIKA KONTROL MANUAL ---
    // Kita baca perintah manual HANYA jika TIDAK sedang mode auto
    // KECUALI perintah 'x' (matikan auto) atau 'X' (nyalakan auto)
    
    if (command == 'X') {
      autoMode = true;
      SerialBT.println("Mode OTOMATIS: ON");
    }
    else if (command == 'x') {
      autoMode = false;
      stopMove();
      SerialBT.println("Mode OTOMATIS: OFF (Manual)");
    }
    
    // Setting Kecepatan (0 - 9)
    else if (command >= '0' && command <= '9') {
      int speedVal = command - '0'; // Ubah char ke angka
      motorSpeed = map(speedVal, 0, 9, 0, 255);
      SerialBT.print("Kecepatan set ke: ");
      SerialBT.println(motorSpeed);
    }
    
    // Perintah Gerak (Hanya jalan kalau Manual Mode)
    else if (!autoMode) {
      switch (command) {
        case 'F': moveForward(); break;     // Maju
        case 'B': moveBackward(); break;    // Mundur
        case 'L': turnLeft(); break;        // Putar Kiri (Tajam)
        case 'R': turnRight(); break;       // Putar Kanan (Tajam)
        case 'G': moveForwardLeft(); break; // Serong Kiri Maju
        case 'H': moveForwardRight(); break;// Serong Kanan Maju
        case 'I': moveBackwardLeft(); break;// Serong Kiri Mundur
        case 'J': moveBackwardRight(); break;// Serong Kanan Mundur
        case 'S': stopMove(); break;        // Stop
      }
    }
  }

  // --- JALANKAN MODE OTOMATIS ---
  if (autoMode) {
    runAutoMode();
  }
}

// ==========================================
// BAGIAN FUNGSI GERAK (DRIVER L298N)
// ==========================================

void setMotor(int speedLeft, int speedRight, bool dirLeftMaju, bool dirRightMaju) {
  analogWrite(ENA, speedLeft);
  analogWrite(ENB, speedRight);

  // Motor Kiri
  if (dirLeftMaju) { digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW); }
  else             { digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH); }

  // Motor Kanan
  if (dirRightMaju) { digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW); }
  else              { digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH); }
}

void moveForward() {
  setMotor(motorSpeed, motorSpeed, true, true);
}

void moveBackward() {
  setMotor(motorSpeed, motorSpeed, false, false);
}

void turnLeft() { // Putar di tempat ke kiri
  setMotor(motorSpeed, motorSpeed, false, true);
}

void turnRight() { // Putar di tempat ke kanan
  setMotor(motorSpeed, motorSpeed, true, false);
}

void stopMove() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  analogWrite(ENA, 0); analogWrite(ENB, 0);
}

// --- Gerakan Halus (Serong) ---
// Caranya: Salah satu roda diperlambat
void moveForwardLeft() {
  setMotor(motorSpeed/4, motorSpeed, true, true);
}

void moveForwardRight() {
  setMotor(motorSpeed, motorSpeed/4, true, true);
}

void moveBackwardLeft() {
  setMotor(motorSpeed/4, motorSpeed, false, false);
}

void moveBackwardRight() {
  setMotor(motorSpeed, motorSpeed/4, false, false);
}

// ==========================================
// BAGIAN LOGIKA OTOMATIS (SENSOR)
// ==========================================

void runAutoMode() {
  distance = readDistance();
  
  if (distance < distanceLimit) {
    stopMove(); delay(200);
    moveBackward(); delay(300);
    stopMove();
    
    int rightDist = lookRight();
    delay(300);
    int leftDist = lookLeft();
    delay(300);

    // Pakai speed penuh saat menghindar
    int tempSpeed = motorSpeed; 
    motorSpeed = 200; // Pastikan cukup tenaga buat belok
    
    if (rightDist >= leftDist) {
      turnRight(); delay(500);
    } else {
      turnLeft(); delay(500);
    }
    
    stopMove();
    motorSpeed = tempSpeed; // Kembalikan kecepatan asal
  } else {
    moveForward();
  }
}

int readDistance() {
  digitalWrite(trigPin, LOW); delayMicroseconds(2);
  digitalWrite(trigPin, HIGH); delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  int cm = duration * 0.034 / 2;
  if (cm == 0) cm = 250;
  return cm;
}

int lookRight() {
  myServo.write(10); delay(500);
  int d = readDistance();
  myServo.write(90); return d;
}

int lookLeft() {
  myServo.write(170); delay(500);
  int d = readDistance();
  myServo.write(90); return d;
}
