/*
 * 
 *
 * 
 *
 * 
 */

#include "DHT.h"
#include <LiquidCrystal_I2C.h> // includes the I2C library

#define DHT11_PIN 2
#define degsym ((char)223) // Defines the degree symbol character compatible with the LCD display.

LiquidCrystal_I2C lcd(0x27, 16, 2); //set the address for the I2C module, and the lines and characters per line.
DHT dht11(DHT11_PIN, DHT11);



void setup() {
  Serial.begin(9600);

  dht11.begin(); // initialize the sensor

  lcd.init(); // initialize the lcd
  lcd.backlight(); // turn on the backlight
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
  
  //Displaying temperatures of room and heatsink sensors.
  lcd.clear(); // clear display
  lcd.setCursor(0, 0); // move cursor to (0, 0)
  lcd.print("T_Rm:  "); // print message at (0, 0)
  lcd.print(tempF, 1); // Display with 1 decimal place
  lcd.print(degsym); // Displays the degree symbol defined above
  lcd.print("F");
  lcd.setCursor(0, 1); // move cursor to (2, 1)
  lcd.print("T_HS:  "); // print message at (2, 1)
  lcd.print(tempF, 1); // Display with 1 decimal place
  lcd.print(degsym); // Displays the degree symbol defined above
  lcd.print("F");
  delay(2000); // display the above for two seconds
  
  // Displaying status of water pump and peltier relays.
  lcd.clear(); // clear display
  lcd.setCursor(0, 0); // move cursor to (0, 0)
  lcd.print("Pump: Active ?"); // print message at (0, 0)
  /*if (power to pump is on){
    lcd.print("Active");
  } else {
    lcd.print("Off");
  }*/
  lcd.setCursor(0, 1); // move cursor to (2, 1)
  lcd.print("Pelt: ??"); // print message at (2, 1)
  /*if (power to peltier relay is on){
    lcd.print("Active");
  } else {
    lcd.print("Off");
  }*/
  delay(2000); // display the above for two seconds

}
