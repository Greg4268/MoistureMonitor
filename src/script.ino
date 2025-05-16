#include <Adafruit_BusIO_Register.h>
#include <Adafruit_GenericDevice.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_I2CRegister.h>
#include <Adafruit_SPIDevice.h>
#include <Adafruit_Si7021.h>
#include <LiquidCrystal.h>

// LCD Display pins 
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// LED pins 
const int RED_LED = 10, YELLOW_LED = 9, GREEN_LED = 8;

// humidity and temperature sensor 
Adafruit_Si7021 sensor = Adafruit_Si7021();
bool enableHeater = false;
uint8_t loopCnt = 0;

void setup() {
  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  // begin serial 
  Serial.begin(115200);
  delay(1000);
  
  if(!sensor.begin()){
    Serial.println("Did not find Si7021");
    while(true)
      ;
  }

  Serial.print("Found Si7021");
  delay(500);

  // start lcd 
  Serial.print("Starting LCD connection");
  lcd.begin(16, 2);
  delay(1000);
}

void loop() {
  Serial.print("Humidity:    ");
  Serial.print(sensor.readHumidity(), 2);
  Serial.print("\tTemperature: ");
  Serial.println(sensor.readTemperature(), 2);
  delay(1000);

  // Toggle heater enabled state every 30 seconds to clean sensors ? - this is from the example but might as well remain if it does not impact the reading or improves them 
  // An ~1.8 degC temperature increase can be noted when heater is enabled
  if (++loopCnt == 30) {
    enableHeater = !enableHeater;
    sensor.heater(enableHeater);
    Serial.print("Heater Enabled State: ");
    if (sensor.isHeaterEnabled())
      Serial.println("ENABLED");
    else
      Serial.println("DISABLED");
       
    loopCnt = 0;
  }

  int currHumidity = sensor.readHumidity();
  if(currHumidity >= 93) {
    highHumidityWarning(currHumidity);
    triggerLED(currHumidity);
  }
  else if (currHumidity <= 60) {
    lowHumidityWarning(currHumidity);
    triggerLED(currHumidity);
  }
  else {
    triggerLED(currHumidity);
  }
  delay(1000);
}


void lowHumidityWarning(int currHumidity) {
  Serial.print("WARNING: Humidity too low: ");
  Serial.println(currHumidity);
  for(byte i = 0; i < 3; i++){
    lcd.print("WARNING");
    delay(200);
    lcd.clear();
  }
  lcd.print("Humidity LOW!");
}

void highHumidityWarning(int currHumidity) {
  Serial.print("WARNING: Humidity too high: ");
  Serial.println(currHumidity);
  for(byte i = 0; i < 3; i++){
    lcd.print("WARNING");
    delay(200);
    lcd.clear();
  }
  lcd.print("Humidity HIGH!");
}

void triggerLED(int currHumidity) {
  if(currHumidity < 93 && currHumidity > 60){
    digitalWrite(GREEN_LED, HIGH);
    delay(100);
    digitalWrite(GREEN_LED, LOW);
  }
  if(currHumidity >= 93 || currHumidity <= 60){
    digitalWrite(RED_LED, HIGH);
    delay(100);
    digitalWrite(RED_LED, LOW);
  }
}



