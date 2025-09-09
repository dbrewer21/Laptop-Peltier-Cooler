/*
 * Code for Simpler Version, during Roommate situation.
 *     - Based on using a single temp sensor, placed in the cold side airflow.
 * 
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
 * 
 */

#include "DHT.h"
#include <LiquidCrystal_I2C.h> // includes the I2C library

#define DHT11_PIN 2
#define degsym ((char)223) // Defines the degree symbol character compatible with the LCD display.

#define Temp_buffer 3 // in degrees Celsius
#define Pwr_fan 1 // fan will always be on

float T_Dewpt = 0;
float TLim_low = 0;
float TLim_high = 0;

int Pwr_pelt = 0; // variable controlling relay to peltier module.
int relay_pelt = 0; // variable for controlling relay signal. Maybe not needed?
const int relay_pin = 3;



LiquidCrystal_I2C lcd(0x27, 16, 2); //set the address for the I2C module, and the lines and characters per line.
DHT dht11(DHT11_PIN, DHT11);



void setup() {
  Serial.begin(9600);

  dht11.begin(); // initialize the sensor

  lcd.init(); // initialize the lcd
  lcd.backlight(); // turn on the backlight

  pinMode(relay_pin, OUTPUT); // initialize digital pin location as an output signal


}




void loop() {
  // wait a few seconds between measurements.
  delay(2000);

  // read humidity
  float humi  = dht11.readHumidity();
  // read temperature as Celsius
  float tempC = dht11.readTemperature();
  // read temperature as Fahrenheit
  float tempF = dht11.readTemperature(true);

  // check if any reads failed
  if (isnan(humi) || isnan(tempC) || isnan(tempF)) {
    Serial.println("Failed to read from DHT11 sensor!");
  } else {
    Serial.print("DHT11# Humidity: ");
    Serial.print(humi, 2); // Display with 2 decimal places
    Serial.print("%");

    Serial.print("  |  "); 

    Serial.print("Temperature: ");
    Serial.print(tempC, 1); // Display with 1 decimal place
    Serial.print("°C ~ ");
    Serial.print(tempF, 1); // Display with 1 decimal place
    Serial.println("°F");
  }
  

  /*//Check if cold temp is in or out of range, then trigger relay for peltier.
  Serial.print("Temp: ");
  Serial.print(tempC, 1);
  Serial.print("  | Added "); 
  Serial.print(tempC + TLim_dp, 1);
  Serial.print("  |  "); 
*/

  //Calculating current Dew Point.
  T_Dewpt = tempC - ((100-humi)/5);
  Serial.print("Dew Point: ");
  Serial.print(T_Dewpt, 1);
  Serial.print("  |  "); 

  //Defining temperature limits for having peltier active.
  TLim_low = T_Dewpt + Temp_buffer;
  TLim_high = tempC - Temp_buffer;


  // Logic for controlling the relay signal to peltier based on temperature reading and temperature limits above. 
  if (tempC <= TLim_low) {  // reading is at or below dew point limit
    digitalWrite(relay_pin, LOW); // turn off peltier cooler
    Pwr_pelt = 0; // sets status for peltier message
  } 
  else if (TLim_low <= tempC <= TLim_high) {  // reading is between the temperature limits
    digitalWrite(relay_pin, HIGH); // continue to have peltier turned on
    Pwr_pelt = 1; // sets status for peltier message
  } 
  else {   // tempC >= TLim_high   // reading is above the higher temp limit, either just turned on or not fully cooled to sub-ambient yet.
    digitalWrite(relay_pin, HIGH); // turn on peltier signal
    Pwr_pelt = 1; // sets status for peltier message
  }


  //Displaying temperature of cold side sensor.
  lcd.clear(); // clear display
  lcd.setCursor(0, 0); // move cursor to (0, 0)
  lcd.print("T_Cold:   "); // print message at (0, 0)
  lcd.print(tempC, 1); // Display with 1 decimal place
  lcd.print(degsym); // Displays the degree symbol defined above
  lcd.print("C");
  lcd.setCursor(0, 1); // move cursor to (2, 1)
  lcd.print("Delta_T: "); // print message at (2, 1)
  lcd.print(tempC-21, 1); // Display with 1 decimal place
  lcd.print(degsym); // Displays the degree symbol defined above
  lcd.print("C");
  delay(3000); // display the above for three seconds
  
  // Displaying status of peltier relay and fan.
  lcd.clear(); // clear display
  lcd.setCursor(0, 0); // move cursor to (0, 0)
  lcd.print("Fans: "); // print message at (0, 0)
  //lcd.print(Pwr_fan);
  if (Pwr_fan==1){
    lcd.print("On");
  } else {
    lcd.print("Off");
  }
  lcd.setCursor(0, 1); // move cursor to (2, 1)
  lcd.print("Peltier: "); // print message at (2, 1)
  if (Pwr_pelt==1){
    lcd.print("Active");
  } else {
    lcd.print("Off");
  }
  delay(2000); // display the above for two seconds

}
