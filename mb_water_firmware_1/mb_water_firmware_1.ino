 // ATTiny 1614, 8MHz max. for 3.3V
// USES DAC, so MUST be ATTiny1614, or ATTiny814
//              =================================

/*
 * Change in 1e - cope with variants of AHT10 that don't return 1 when begin() called.
 * Any problem getting t and h will be caught by tester anyway.
 */


#include "AHT10.h"

/* License - Released by MonkMakes under MIT license: https://github.com/monkmakes/plant_monitor_firmware/blob/main/LICENSE
 *  
 *  Protocol - single letter command all responses as a string in decimal
 *  w -> w=230\n    0..255 
 *  h -> h=50\n     0..100 
 *  t -> t=20.4\n   temperature in degrees C, decimal
 *  r -> r=22\n     raw uncalibrated sensor reading
 *  j -> {"wetness" : 230, "humidity":50, "temp":20}
 *  L -> LED shows wetness
 *  l -> LED off
 *  f -> firmware version
 *  
 */ 

const char* firmwareVersion = "1e";

const long dry = 13; // calibrate for new board design - dry / in water r (raw) readings
const long wet = 430;
const long span = wet - dry;

const int redThreshold = 5;       // % moisture
const int orangeThreshold = 30;

const int senseOutPinA = 8;
const int senseOutPinB = 3;
const int senseInPin = 9;

const int redPin = 1;
const int greenPin = 10;
const int bluePin = 0;
const int dacPin = 2;

const long samplePeriod = 500;
const int numSamples = 5;  // bigger number, less noise but slower.

AHT10 aht = AHT10(0x38, AHT10_SENSOR);

long lastSampleTime = 0;
boolean ledOn = true;
int moisture255 = 127;
int moisture100 = 50;

void setup() {
  DACReference(INTERNAL2V5);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(senseOutPinA, OUTPUT);
  pinMode(senseOutPinB, OUTPUT);
  Serial.begin(9600);
  selfTest();
  delay(500);
  aht.begin();
  Serial.println("OK");
}

void loop() {
  long now = millis();
  if (now > lastSampleTime + samplePeriod) {
    lastSampleTime = now;
    moisture255 = readMoisture(255);
    moisture100 = readMoisture(100);
    analogWrite(dacPin, moisture255);
    if (ledOn) {
      ledColorMoisture(moisture255);
    }
    else {
      setColor(0, 0, 0);
    }
    //blink(1);
  }
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 'w') {
      //blink(3);
      Serial.print("w="); Serial.println(moisture100);
      Serial.flush();
    }
    else if (cmd == 'h') {
      Serial.print("h="); Serial.println(aht.readHumidity());
      Serial.flush();
    }
    else if (cmd == 't') {
      Serial.print("t="); Serial.println(aht.readTemperature());
      Serial.flush();
    }
    else if (cmd == 'r') {
      Serial.print("r="); Serial.println(readMoisture());
      Serial.flush();
    }   
    else if (cmd == 'j') {
      // {"wetness" : 230, "humidity":50, "temp":20}
      Serial.print("{\"wetness\":"); Serial.print(moisture100);
      Serial.print(", \"humidity\":"); Serial.print(aht.readHumidity());
      Serial.print(", \"temp\":"); Serial.print(aht.readTemperature()); Serial.println("}");
    }
    else if (cmd == 'v') {
      Serial.println(firmwareVersion);
    } 
    else if (cmd == 'L') {
      ledOn = true;
    }
    else if (cmd == 'l') {
      ledOn = false;
    }
    else if (cmd == 'T') {
      selfTest();
    }
    //blink(1);
  }
}

int readMoisture1() {
  // measure charge time one way round
  // then set pins other way around for the 
  // same period to reverse any electrolysis
  digitalWrite(senseOutPinA, HIGH);
  digitalWrite(senseOutPinB, LOW);
  long t0 = micros();
  while (digitalRead(senseInPin) == LOW) { }; // wait for high
  long t1 = micros();
  long d = t1 - t0;
  digitalWrite(senseOutPinA, LOW);
  digitalWrite(senseOutPinB, HIGH);
  delayMicroseconds(d*10);
  return d;
}

int readMoisture() {
  long total = 0;
  for (int i = 0; i < numSamples; i++) {
    total += readMoisture1();
  }
  int m = total / numSamples;
  return m;
}

int readMoisture(int maximum) {
  int m = readMoisture();
  float k = float(maximum) / float(span);
  int v = int((m - dry) * k);
  if (v < 0) v = 0;
  if (v > maximum) v = maximum;
  return v;
}

void setColor(int r, int g, int b) {
  analogWrite(redPin, r);
  analogWrite(greenPin, g);
  analogWrite(bluePin, b);
}

void ledColorMoisture(int greenness) {
  int level = (greenness / 16) * 16;
  setColor((255-level), level, 0);
}

void selfTest() {
  setColor(255, 0, 0);
  delay(500);
  setColor(0, 255, 0);
  delay(500);
  setColor(0, 0, 255);
  delay(500);
  setColor(0, 0, 0);
  delay(500);
}

void error() {
  while (true) {
    setColor(255, 255, 255);
    delay(50);
    setColor(0, 0, 0);
    delay(50);  
  }
}

void blink(int n) {
  for (int i=0; i < n; i++) {
    setColor(255, 255, 255);
    delay(300);
    setColor(0, 0, 0);
    delay(300);
  }
}
