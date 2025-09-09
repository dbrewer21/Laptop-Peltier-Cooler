/*
 * Enhanced Temperature Control System with Comprehensive Error Handling
 * Code for Simpler Version, during Roommate situation.
 *     - Based on using a single temp sensor, placed in the cold side airflow.
 * 
 * T_dewpt = T_air - ((100-RelHum%)/5)  //Temperatures in Celsius
 * 
 * Indoor Temp = 70 F / 21.1 C
 * Indoor Humidity = 30 %
 * Indoor Dew Point = 37 F / 3 C
 * DP at 25% = 33 F / 0 C
 * DP at 35% = 41 F / 5 C
 * 
 * DPs at 25%/30%/35% at 65 F (18.3 C) = 29 / -2, 33 / 1, 47 / 3
 * DPs at 25%/30%/35% at 75 F (23.9 C) = 37 / 3, 42 / 5, 46 / 8
 */

#include "DHT.h"
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#define DHT11_PIN 2
#define degsym ((char)223) // Defines the degree symbol character compatible with the LCD display.
#define Temp_buffer 3 // in degrees Celsius
#define Pwr_fan 1 // fan will always be on

// System status tracking variables
unsigned long loop_count = 0;
unsigned long sensor_failures = 0;
unsigned long lcd_failures = 0;
unsigned long last_successful_reading = 0;
unsigned long system_start_time = 0;

// Temperature control variables
float T_Dewpt = 0;
float TLim_low = 0;
float TLim_high = 0;
int Pwr_pelt = 0; // variable controlling relay to peltier module.
int relay_pelt = 0; // variable for controlling relay signal. Maybe not needed?
const int relay_pin = 3;

// Previous values for change detection
float prev_tempC = -999;
float prev_humi = -999;
int prev_Pwr_pelt = -1;

LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht11(DHT11_PIN, DHT11);

// Function prototypes
bool checkI2C();
void printSystemStatus();
void printSensorData(float humi, float tempC, float tempF);
void printCalculatedValues();
void printRelayStatus();
bool updateLCD_Safe(float tempC);
void printBootMessage();

void setup() {
  Serial.begin(9600);
  system_start_time = millis();
  
  printBootMessage();
  
  Serial.println("=== SYSTEM INITIALIZATION ===");
  Serial.println("Initializing DHT11 sensor...");
  dht11.begin();
  Serial.println("✓ DHT11 sensor initialized");

  Serial.println("Initializing I2C and LCD...");
  Wire.begin();
  if (checkI2C()) {
    lcd.init();
    lcd.backlight();
    Serial.println("✓ LCD initialized successfully");
    // Display startup message
    lcd.setCursor(0, 0);
    lcd.print("System Starting");
    lcd.setCursor(0, 1);
    lcd.print("Please wait...");
  } else {
    Serial.println("⚠ LCD not detected - continuing without display");
  }

  Serial.println("Configuring relay pin...");
  pinMode(relay_pin, OUTPUT);
  digitalWrite(relay_pin, LOW); // Start with relay off
  Serial.print("✓ Relay pin ");
  Serial.print(relay_pin);
  Serial.println(" configured as output");

  Serial.println("=== INITIALIZATION COMPLETE ===");
  Serial.println("System ready - starting main loop");
  Serial.println("=====================================");
  delay(2000); // Give time to read startup messages
}

void loop() {
  loop_count++;
  unsigned long loop_start_time = millis();
  
  Serial.println();
  Serial.print("=== LOOP #");
  Serial.print(loop_count);
  Serial.print(" | Runtime: ");
  Serial.print((millis() - system_start_time) / 1000);
  Serial.println(" seconds ===");

  // Wait between measurements
  delay(2000);

  // Read sensor data with error handling
  Serial.println("Reading DHT11 sensor...");
  float humi = dht11.readHumidity();
  float tempC = dht11.readTemperature();
  float tempF = dht11.readTemperature(true);

  // Validate sensor readings
  if (isnan(humi) || isnan(tempC) || isnan(tempF)) {
    sensor_failures++;
    Serial.println("❌ SENSOR ERROR: Failed to read from DHT11 sensor!");
    Serial.print("Total sensor failures: ");
    Serial.println(sensor_failures);
    
    // Use previous values if available
    if (prev_tempC != -999) {
      Serial.println("⚠ Using previous sensor values for this iteration");
      tempC = prev_tempC;
      humi = prev_humi;
    } else {
      Serial.println("⚠ No previous values available - skipping this loop iteration");
      return;
    }
  } else {
    last_successful_reading = millis();
    printSensorData(humi, tempC, tempF);
    
    // Check for significant changes
    if (abs(tempC - prev_tempC) > 1.0) {
      Serial.print("🌡️ Significant temperature change detected: ");
      Serial.print(prev_tempC, 1);
      Serial.print("°C → ");
      Serial.print(tempC, 1);
      Serial.println("°C");
    }
    
    // Store current values
    prev_tempC = tempC;
    prev_humi = humi;
  }

  // Calculate dew point and limits
  T_Dewpt = tempC - ((100 - humi) / 5);
  TLim_low = T_Dewpt + Temp_buffer;
  TLim_high = tempC - Temp_buffer;
  
  printCalculatedValues();

  // Peltier control logic with detailed logging
  Serial.println("--- PELTIER CONTROL LOGIC ---");
  Serial.print("Current temp: ");
  Serial.print(tempC, 1);
  Serial.print("°C | Low limit: ");
  Serial.print(TLim_low, 1);
  Serial.print("°C | High limit: ");
  Serial.print(TLim_high, 1);
  Serial.println("°C");

  int new_Pwr_pelt;
  
  if (tempC <= TLim_low) {
    digitalWrite(relay_pin, LOW);
    new_Pwr_pelt = 0;
    Serial.println("🔵 DECISION: Temperature at/below dew point limit → Peltier OFF");
  } 
  else if (tempC >= TLim_low && tempC <= TLim_high) {
    digitalWrite(relay_pin, HIGH);
    new_Pwr_pelt = 1;
    Serial.println("🟡 DECISION: Temperature in optimal range → Peltier ON");
  } 
  else { // tempC > TLim_high
    digitalWrite(relay_pin, HIGH);
    new_Pwr_pelt = 1;
    Serial.println("🔴 DECISION: Temperature above high limit → Peltier ON");
  }

  // Check for relay state changes
  if (new_Pwr_pelt != prev_Pwr_pelt && prev_Pwr_pelt != -1) {
    Serial.print("⚡ RELAY STATE CHANGE: ");
    Serial.print(prev_Pwr_pelt ? "ON" : "OFF");
    Serial.print(" → ");
    Serial.println(new_Pwr_pelt ? "ON" : "OFF");
  }
  
  Pwr_pelt = new_Pwr_pelt;
  prev_Pwr_pelt = Pwr_pelt;
  
  printRelayStatus();

  // Update LCD with error handling
  Serial.println("--- LCD UPDATE ---");
  if (!updateLCD_Safe(tempC)) {
    lcd_failures++;
    Serial.print("❌ LCD update failed (Total failures: ");
    Serial.print(lcd_failures);
    Serial.println(")");
  } else {
    Serial.println("✓ LCD updated successfully");
  }

  // Print system status summary
  printSystemStatus();
  
  unsigned long loop_duration = millis() - loop_start_time;
  Serial.print("Loop completed in ");
  Serial.print(loop_duration);
  Serial.println(" ms");
  Serial.println("=====================================");
}

bool checkI2C() {
  Wire.beginTransmission(0x27);
  byte error = Wire.endTransmission();
  return (error == 0);
}

void printBootMessage() {
  Serial.println();
  Serial.println("████████████████████████████████████████");
  Serial.println("█  PELTIER TEMPERATURE CONTROL SYSTEM  █");
  Serial.println("█            Enhanced Version           █");
  Serial.println("████████████████████████████████████████");
  Serial.println();
}

void printSensorData(float humi, float tempC, float tempF) {
  Serial.println("--- SENSOR READINGS ---");
  Serial.print("✓ DHT11 Humidity: ");
  Serial.print(humi, 2);
  Serial.print("% | Temperature: ");
  Serial.print(tempC, 1);
  Serial.print("°C (");
  Serial.print(tempF, 1);
  Serial.println("°F)");
}

void printCalculatedValues() {
  Serial.println("--- CALCULATED VALUES ---");
  Serial.print("Dew Point: ");
  Serial.print(T_Dewpt, 1);
  Serial.print("°C | Control Range: ");
  Serial.print(TLim_low, 1);
  Serial.print("°C to ");
  Serial.print(TLim_high, 1);
  Serial.println("°C");
}

void printRelayStatus() {
  Serial.println("--- SYSTEM STATUS ---");
  Serial.print("Fan Power: ");
  Serial.print(Pwr_fan ? "ON" : "OFF");
  Serial.print(" | Peltier: ");
  Serial.print(Pwr_pelt ? "ACTIVE" : "OFF");
  Serial.print(" | Relay Pin ");
  Serial.print(relay_pin);
  Serial.print(": ");
  Serial.println(digitalRead(relay_pin) ? "HIGH" : "LOW");
}

bool updateLCD_Safe(float tempC) {
  if (!checkI2C()) {
    Serial.println("⚠ I2C communication failed - LCD not responding");
    return false;
  }

  // Display temperature screen
  Serial.println("Writing temperature display...");
  lcd.setCursor(0, 0);
  lcd.print("T_Cold: ");
  lcd.print(tempC, 1);
  lcd.print(degsym);
  lcd.print("C   "); // Extra spaces to clear old data
  
  // Check I2C again after first operation
  if (!checkI2C()) {
    Serial.println("❌ I2C failed during temperature display");
    return false;
  }
  
  lcd.setCursor(0, 1);
  lcd.print("Delta_T: ");
  lcd.print(tempC - 21, 1);
  lcd.print(degsym);
  lcd.print("C   "); // Extra spaces to clear old data
  
  delay(3000);

  // Display status screen
  Serial.println("Writing status display...");
  
  // Check I2C before status display
  if (!checkI2C()) {
    Serial.println("❌ I2C failed before status display");
    return false;
  }
  
  lcd.setCursor(0, 0);
  lcd.print("Fans: ");
  lcd.print(Pwr_fan ? "On      " : "Off     ");
  
  lcd.setCursor(0, 1);
  lcd.print("Peltier: ");
  lcd.print(Pwr_pelt ? "Active  " : "Off     ");
  
  delay(2000);
  return true;
}

void printSystemStatus() {
  Serial.println("--- SYSTEM HEALTH ---");
  Serial.print("Uptime: ");
  Serial.print((millis() - system_start_time) / 1000);
  Serial.print("s | Loops: ");
  Serial.print(loop_count);
  Serial.print(" | Sensor failures: ");
  Serial.print(sensor_failures);
  Serial.print(" | LCD failures: ");
  Serial.println(lcd_failures);
  
  if (sensor_failures > 0) {
    float failure_rate = (float)sensor_failures / loop_count * 100;
    Serial.print("Sensor failure rate: ");
    Serial.print(failure_rate, 1);
    Serial.println("%");
  }
  
  Serial.print("Last successful sensor reading: ");
  if (last_successful_reading > 0) {
    Serial.print((millis() - last_successful_reading) / 1000);
    Serial.println(" seconds ago");
  } else {
    Serial.println("None yet");
  }
}