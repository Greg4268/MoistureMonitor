#include <Adafruit_BusIO_Register.h>
#include <Adafruit_GenericDevice.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_I2CRegister.h>
#include <Adafruit_SPIDevice.h>
#include <Adafruit_Si7021.h>
#include <LiquidCrystal.h>
#include <WiFiS3.h>
#include <ArduinoJson.h>

WiFiSSLClient client;

char ssid[] = "****";
char pass[] = "********";

const char* server = "web-production-6160.up.railway.app"; 
const int port = 443;
const char* path = "/update-alert";

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

// Keep track of current and previous status to detect changes
AlertStatus currentStatus = GOOD;
AlertStatus previousStatus = GOOD;  // Initialize to same as current to avoid initial alert

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

// Forward declare all functions
void connectToWiFi();
void sendAlertToServer(AlertStatus status);
bool isHighHumidity(int currHumidity, bool measureIndoor);
char isHumidityGoodBadOrBetweenTerrarium(int currHumidity);
char isHumidityGoodBadOrBetweenIndoor(int currHumidity);
void lowHumidityWarningLCD();
void closeHumidityWarningLCD();
void highHumidityWarningLCD();
void triggerLEDAndBuzzer(char condition);

bool measureIndoor = true;
bool measureTerrarium = !measureIndoor;

//buzzer 
const int buzzer = 6;

// humidity and temperature sensor 
Adafruit_Si7021 sensor = Adafruit_Si7021();
bool enableHeater = false;
uint8_t loopCnt = 0;

//  for status tracking and sending
bool firstRun = true;  // Flag to track first run
unsigned long lastStatusSentTime = 0;
const unsigned long statusSendInterval = 300000;  // Send status update every 5 minutes (300,000 ms) 

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
  Serial.println("Found Si7021");
  delay(500);

  // connect to WiFi
  connectToWiFi();

  // start lcd 
  Serial.println("Starting LCD connection");
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("Humidity Monitor");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  delay(2000);
  lcd.clear();
}

void loop() {
  Serial.print("Humidity:    ");
  Serial.print(sensor.readHumidity(), 2);
  Serial.print("\tTemperature: ");
  Serial.println(sensor.readTemperature(), 2);

  // Toggle heater enabled state every 30 seconds to clean sensors
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

  // Save the previous status before updating
  previousStatus = currentStatus;

  // Update the current status based on the condition
  if(condition == 'b'){
    if(isHighHumidity(currHumidity, measureIndoor)) {
      // humidity came back true == high humidity 
      triggerLEDAndBuzzer(condition);
      highHumidityWarningLCD();
      currentStatus = WARNING_BAD;
      Serial.print("WARNING: Humidity too high: ");
      Serial.println(currHumidity);
    }
    else {
      // humidity came back false == low humidity 
      triggerLEDAndBuzzer(condition);
      lowHumidityWarningLCD();
      currentStatus = WARNING_BAD;
      Serial.print("WARNING: Humidity too low: ");
      Serial.println(currHumidity);
    }
  }
  else if (condition == 'c') {
    triggerLEDAndBuzzer(condition);
    closeHumidityWarningLCD();
    currentStatus = WARNING_APPROACHING;
    Serial.print("WARNING: Humidity is getting out of ideal range: ");
    Serial.println(currHumidity);
  }
  else {
    triggerLEDAndBuzzer(condition);
    currentStatus = GOOD;
    lcd.clear();
    lcd.print("Humidity: GOOD");
    lcd.setCursor(0, 1);
    lcd.print("Value: ");
    lcd.print(currHumidity);
    lcd.print("%");
  }

  // Send alert only if status has changed or it's the first run or periodic update time
  unsigned long currentTime = millis();
  if (firstRun || currentStatus != previousStatus || 
      (currentTime - lastStatusSentTime >= statusSendInterval)) {
    
    sendAlertToServer(currentStatus);
    lastStatusSentTime = currentTime;
    
    if (firstRun) {
      firstRun = false;
      Serial.println("Initial status sent");
    } else if (currentStatus != previousStatus) {
      Serial.println("Status changed - alert sent");
    } else {
      Serial.println("Periodic status update sent");
    }
  }

  delay(5000);  // adjust for updat speed
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
    delay(50);
  }
  lcd.print("Humidity LOW!");
  lcd.setCursor(0, 1);
  lcd.print("Value: ");
  lcd.print(sensor.readHumidity(), 1);
  lcd.print("%");
}

void closeHumidityWarningLCD() {
  lcd.clear();
  lcd.print("Out of ideal");
  delay(1000);
  lcd.clear();
  lcd.print("humidity range");
  lcd.setCursor(0, 1);
  lcd.print("Value: ");
  lcd.print(sensor.readHumidity(), 1);
  lcd.print("%");
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
  lcd.setCursor(0, 1);
  lcd.print("Value: ");
  lcd.print(sensor.readHumidity(), 1);
  lcd.print("%");
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

  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected; Attempting to reconnect...");
    connectToWiFi();
    if(WiFi.status() != WL_CONNECTED) {
      Serial.println("Failed to reconnect. Cannot send alert ;(");
      return;
    }
  }
  
  // Connect to server with timeout
  Serial.print("Connecting to server...");
  unsigned long connectionStartTime = millis();
  boolean connected = false;
  
  // Try to connect with timeout
  while (millis() - connectionStartTime < 5000) { // 5 second timeout
    if (client.connect(server, port)) {
      connected = true;
      break;
    }
    delay(100);
    Serial.print(".");
  }
  
  if (connected) {
    Serial.println("connected!");
    
    // Send the HTTP request
    client.println("POST " + String(path) + " HTTP/1.1");
    client.println("Host: " + String(server));
    client.println("Content-Type: application/json");
    client.println("Content-Length: " + String(jsonString.length()));
    client.println("Connection: close");
    client.println();
    client.println(jsonString);
    
    // Wait for response with timeout
    Serial.println("Waiting for response...");
    unsigned long responseStartTime = millis();
    boolean responseReceived = false;
    
    while (client.connected() && millis() - responseStartTime < 10000) {
      if (client.available()) {
        responseReceived = true;
        String line = client.readStringUntil('\n');
        Serial.println(line);
      }
    }
    
    if (!responseReceived) {
      Serial.println("No response received from server (timeout)");
    }
    
    client.stop();
    Serial.println("Connection closed");
  } else {
    Serial.println("connection failed!");
    Serial.print("WiFi status: ");
    Serial.println(WiFi.status());
    Serial.print("Signal strength (RSSI): ");
    Serial.println(WiFi.RSSI());
  }
}

void connectToWiFi() {
  // Print connecting message
  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);
  
  // Initialize connection counter
  int connectionAttempts = 0;
  const int maxAttempts = 20;  // Maximum number of attempts (10 seconds)
  
  // Start WiFi connection
  WiFi.begin(ssid, pass);
  
  // Check WiFi status properly with timeout
  while (WiFi.status() != WL_CONNECTED && connectionAttempts < maxAttempts) {
    delay(500);
    Serial.print(".");
    connectionAttempts++;
  }
  
  // Check if connected successfully
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi!");
    Serial.print("WiFi status: ");
    
    // Print status code in a human-readable form
    switch (WiFi.status()) {
      case WL_IDLE_STATUS:
        Serial.println("IDLE");
        break;
      case WL_NO_SSID_AVAIL:
        Serial.println("NO SSID AVAILABLE - Check WiFi name");
        break;
      case WL_CONNECT_FAILED:
        Serial.println("CONNECTION FAILED - Check password");
        break;
      case WL_DISCONNECTED:
        Serial.println("DISCONNECTED");
        break;
      default:
        Serial.println(WiFi.status());
        break;
    }
    
    // Wait a bit before trying again
    delay(3000);
    Serial.println("Retrying connection...");
    connectToWiFi();  
  }
}