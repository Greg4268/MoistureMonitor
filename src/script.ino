#include <Adafruit_BusIO_Register.h>
#include <Adafruit_GenericDevice.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_I2CRegister.h>
#include <Adafruit_SPIDevice.h>
#include <Adafruit_Si7021.h>
#include <LiquidCrystal.h>
#include <WiFiS3.h>
#include <ArduinoJson.h>

int status = WL_IDLE_STATUS;
WiFiClient client;

char ssid[] = "******";
char pass[] = "**********";

const char* server = "http://192.168.10.1";
const int port = 8080;
const char* path = "/update-info"; 

// LCD Display pins 
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// LED pins 
const int RED_LED = 10, YELLOW_LED = 9, GREEN_LED = 8;

// alert status for server
enum AlertStatus { 
  GOOD, 
  WARNING_APPROACHING, 
  WARNING_BAD
};

AlertStatus currentStatus = GOOD;

// moist terrarium humidity constants 
enum terrarium {
  TERRARIUM_LOW = 60, 
  TERRARIUM_CLOSE_LOW = 65, 
  TERRARIUM_CLOSE_HIGH = 88, 
  TERRARIUM_HIGH = 93
};

// humidity constants for indoors - not terrarium 
enum indoor {
  INDOOR_LOW = 30, 
  INDOOR_CLOSE_LOW = 35, 
  INDOOR_CLOSE_HIGH = 55, 
  INDOOR_HIGH = 60
};

bool measureIndoor = true;
bool measureTerrarium = !measureIndoor;

//buzzer 
const int buzzer = 6;

// humidity and temperature sensor 
Adafruit_Si7021 sensor = Adafruit_Si7021();
bool enableHeater = false;
uint8_t loopCnt = 0;

void setup() {
  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(buzzer, OUTPUT);
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

  // connect to WiFi
  connectToWiFi();

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
  char condition; 
  
  if(measureIndoor){
    condition = isHumidityGoodBadOrBetweenIndoor(currHumidity);
  } else {
    condition = isHumidityGoodBadOrBetweenTerrarium(currHumidity);
    // b == bad
    // c == close
    // g == good 
  }

  if(condition == 'b'){
    if(isHighHumidity(currHumidity, measureIndoor)) {
      // humidity came back true == high humidity 
      triggerLEDAndBuzzer(condition);
      highHumidityWarningLCD();
      currentStatus = WARNING_BAD;
      sendAlertToServer(currentStatus);
      Serial.print("WARNING: Humidity too high: ");
      Serial.println(currHumidity);
    }
    else {
      // humidity came back false == low humidity 
      triggerLEDAndBuzzer(condition);
      lowHumidityWarningLCD();
      currentStatus = WARNING_BAD;
      sendAlertToServer(currentStatus);
      Serial.print("WARNING: Humidity too low: ");
      Serial.println(currHumidity);
    }
  }
  else if (condition == 'c') {
    triggerLEDAndBuzzer(condition);
    closeHumidityWarningLCD();
    currentStatus = WARNING_APPROACHING;
    sendAlertToServer(currentStatus);
    Serial.print("WARNING: Humidity is getting out of ideal range: ");
    Serial.println(currHumidity);
  }
  else {
    triggerLEDAndBuzzer(condition);
    currentStatus = GOOD;
    sendAlertToServer(currentStatus);
    lcd.print("Humidity: GOOD");
  }
  delay(1000);
}

bool isHighHumidity(int currHumidity, bool measureIndoor) {
  if(!measureIndoor && currHumidity >= TERRARIUM_HIGH){
    return true;
  }
  else if (measureIndoor && currHumidity >= INDOOR_HIGH) {
    return true;
  }
  return false;
}

char isHumidityGoodBadOrBetweenTerrarium(int currHumidity){
  if(currHumidity >= TERRARIUM_HIGH || currHumidity <= TERRARIUM_LOW){
    return 'b';
  }
  if(currHumidity >= TERRARIUM_CLOSE_HIGH || currHumidity <= TERRARIUM_CLOSE_LOW){
    return 'c';
  }
  return 'g';
} 

char isHumidityGoodBadOrBetweenIndoor(int currHumidity){
  if(currHumidity >= INDOOR_HIGH || currHumidity <= INDOOR_LOW){
    return 'b';
  }
  if(currHumidity >= INDOOR_CLOSE_HIGH || currHumidity <= INDOOR_CLOSE_LOW){
    return 'c';
  }
  return 'g';
} 

void lowHumidityWarningLCD() {
  lcd.clear();
  for(byte i = 0; i < 3; i++){
    lcd.print("WARNING");
    delay(200);
    lcd.clear();
    delay(50);  // Added a small delay after clearing
  }
  lcd.print("Humidity LOW!");
}

void closeHumidityWarningLCD() {
  lcd.clear();
  lcd.print("Out of ideal");
  delay(1000);
  lcd.clear();
  lcd.print("humidity range");
}

void highHumidityWarningLCD() {
  lcd.clear();
  for(byte i = 0; i < 3; i++){
    lcd.print("WARNING");
    delay(200);
    lcd.clear();
    delay(50);  
  }
  lcd.print("Humidity HIGH!");
}

void triggerLEDAndBuzzer(char condition) {
  // Turn all LEDs off first
  digitalWrite(RED_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(GREEN_LED, LOW);
  noTone(buzzer);
  
  if(condition == 'g'){
    digitalWrite(GREEN_LED, HIGH);
    delay(200);
  }
  else if(condition == 'b'){
    digitalWrite(RED_LED, HIGH);
    tone(buzzer, 500);
    delay(200);
  }
  else if (condition == 'c') {
    digitalWrite(YELLOW_LED, HIGH);
    tone(buzzer, 800);
    delay(200);
}

void sendAlertToServer(AlertStatus status) {
  String statusString;

  switch(status) {
    case GOOD:
      statusString = "good";
      break;
    case WARNING_APPROACHING:
      statusString = "warning(approaching bad)";
      break;
    case WARNING_BAD:
      statusString = "warning(bad)";
      break;
  }

  Serial.print("Sending alert status: ");
  Serial.println(statusString);
  
  // Create JSON document
  StaticJsonDocument<200> doc;
  doc["status"] = statusString;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  // Connect to server
  Serial.print("Connecting to server...");
  if (client.connect(server, port)) {
    Serial.println("connected!");
    
    // Send HTTP POST request
    client.println("POST " + path + " HTTP/1.1");
    client.println("Host: " + String(server));
    client.println("Content-Type: application/json");
    client.println("Content-Length: " + String(jsonString.length()));
    client.println("Connection: close");
    client.println();
    client.println(jsonString);
    
    // Wait for response
    unsigned long timeout = millis();
    while (client.connected() && millis() - timeout < 10000) {
      if (client.available()) {
        String line = client.readStringUntil('\n');
        Serial.println(line);
      }
    }
    
    client.stop();
    Serial.println("Connection closed");
  } else {
    Serial.println("connection failed!");
  }
}

void connectToWiFi() {
  WiFi.begin(ssid, pass);
  while(status != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}


