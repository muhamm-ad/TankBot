#include "secrets.h"
#include "thingProperties.h"

// -------------------------
#define ULTRASONIC_T_TRIG_PIN 5
#define ULTRASONIC_T_ECHO_PIN 18
#define ULTRASONIC_T_HEIGHT 145  // Height of the sensor in cm
#define TANK_HEIGHT 120  // Height of the tank in cm
#define TANK_DIAMETER 150  // Diameter of the tank in cm
// -------------------------
#define FILL_TRIGGER_PIN 27
#define FILL_TRIGGER_ON LOW
#define FILL_TRIGGER_OFF HIGH
// -------------------------
#define IS_FILLING_PIN 34
// -------------------------


void setup() {
  // Initialize serial and wait for port to open:
  Serial.begin(9600);
  // This delay gives the chance to wait for a Serial Monitor without blocking if none is found
  delay(1500); 

  // Defined in thingProperties.h
  initProperties();

  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

  // Set up pins
  pinMode(ULTRASONIC_T_TRIG_PIN, OUTPUT);
  pinMode(ULTRASONIC_T_ECHO_PIN, INPUT);
  pinMode(FILL_TRIGGER_PIN, OUTPUT);
  pinMode(IS_FILLING_PIN, INPUT);

  digitalWrite(FILL_TRIGGER_PIN, FILL_TRIGGER_OFF);
  tankTriggerFill = 0; // turn off
  tankIsFilling = 0; // turn off
}

void loop() {
  ArduinoCloud.update();
  liquidLevelling();
  tankIsFilling = checkFillingStatus();
  // delay(1000);  // Wait 1 second before next measurement
}

void liquidLevelling() {
  // Send pulse to trigger the sensor
  digitalWrite(ULTRASONIC_T_TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRASONIC_T_TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_T_TRIG_PIN, LOW);
  
  // Measure the pulse duration
  long duration = pulseIn(ULTRASONIC_T_ECHO_PIN, HIGH);
  // Calculate the distance from sensor to liquid (in cm)
  long distance = duration * 0.034 / 2;  // Speed of sound is 0.034 cm/us
  // Adjust the distance to account for the sensor height
  long liquidLevel = ULTRASONIC_T_HEIGHT - distance - (ULTRASONIC_T_HEIGHT - TANK_HEIGHT);
  // Calculate volume of liquid in cubic centimeters (cm³)
  float pi = 3.14159;
  float volumeCm3 = pi * pow(TANK_DIAMETER / 2, 2) * liquidLevel;

  // Update cloud variables
  tankFillPercentage = (float(liquidLevel) / TANK_HEIGHT) * 100;
  tankLiquidHeight = liquidLevel;  // Water height in cm
  tankVolumeLiters = volumeCm3 / 1000; // Convert volume to liters (1 liter = 1000 cubic cm)
  tankVolumeCubicMeters = volumeCm3 / 1000000; // Convert volume to cubic meters (1 m³ = 1,000,000 cm³)
}

bool checkFillingStatus() {
  const float AmpsRmsErro = 0.08;
  const float outVoltageACS712 = 2.5;
  int mVperAmp = 185;

  int readValue;
  int maxValue = 0;
  int minValue = 4096;

  uint32_t start_time = millis();
  while ((millis() - start_time) < 1000) { // Sample for 1 sec
    readValue = analogRead(IS_FILLING_PIN);
    if (readValue > maxValue) maxValue = readValue;
    if (readValue < minValue) minValue = readValue;
  }

  double voltage = ((maxValue - minValue) * outVoltageACS712) / 4096.0;
  double VRMS = (voltage / 2.0) * 0.707;
  float AmpsRms = ((VRMS * 1000) / mVperAmp) - AmpsRmsErro;
  // Serial.println("");
  // Serial.print(AmpsRms);
  // Serial.print(" Amps RMS  ---  ");

  /* note: 1.2 is my own empirically established calibration factor 
  as the voltage measured at D34 depends on the length of the OUT-to-D34 wire
  230 is the main AC power voltage – this parameter changes locally */
  // int watt = (AmpsRms * 230 / 1.2);
  // Serial.print(watt);
  // Serial.println(" Watts");

  return AmpsRms > 0;
}

void onTankTriggerFillChange()  {
  if (tankTriggerFill) {
      digitalWrite(FILL_TRIGGER_PIN, FILL_TRIGGER_ON);
   } else {
      digitalWrite(FILL_TRIGGER_PIN, FILL_TRIGGER_OFF);
   }
}