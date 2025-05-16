# 📦 Arduino Humidity Monitor Using Adafruit Si7021, LCD, and Arduino Uno R4 WiFi

> *This project uses the Arduino paired with the Adafruit Si7021 humidity and temperature sensor to track the humidity level of different environments (terrarium or indoor). It alerts users when humidity shifts out of the ideal range through an LCD display, buzzer alerts, and LED indicators (red, yellow, and green).*

---

## 📋 Table of Contents
- [About](#-about)
- [Features](#-features)
- [Getting Started](#-getting-started)
- [Hardware Setup](#hardware-setup)
- [Software Installation](#software-installation)
- [Usage](#-usage)
- [Configuration](#️-configuration)
- [Screenshots](#️-screenshots)
- [Contributing](#-contributing)
- [License](#-license)

---

## 🧩 About

### What problem does this solve?
For those interested in building their own terrarium or monitoring the humidity of their home, this project provides an effective solution. Maintaining proper humidity levels is critical for:

- **Terrarium environments:** Keeping plants and small creatures in optimal conditions
- **Indoor spaces:** Preventing mold growth while maintaining comfortable living conditions

This monitor eliminates guesswork by providing real-time feedback and alerts when humidity levels drift outside optimal ranges.

### How it works
The system monitors humidity using the Si7021 sensor and provides feedback through:
- Visual LCD display showing current status
- Color-coded LED system (green for ideal, yellow for approaching limits, red for out-of-range)
- Audible alerts with different tones for different conditions
- Serial monitoring capability for logging

### Technology used
- Arduino Uno R4 WiFi
- 16x2 LCD Display
- 3x LEDs (red, yellow, green)
- Adafruit Si7021 Humidity and Temperature Sensor
- Piezo buzzer
- 2x 220Ω Resistors
- 10kΩ Potentiometer
- Breadboard and jumper wires

---

## ✨ Features

- [x] **Live Humidity & Temperature Monitoring** — Continuously measures and displays environmental conditions with 2 decimal place precision
- [x] **Dual-Mode Operation** — Switch between terrarium mode (60-93% humidity) and indoor mode (30-60% humidity)
- [x] **Multi-Level Alert System** — Visual and audible notifications with increasing urgency:
  - Green LED: Ideal humidity range
  - Yellow LED + gentle tone: Approaching limits
  - Red LED + warning tone: Out of acceptable range
- [x] **LCD Status Display** — Shows current readings and warning messages
- [x] **Sensor Self-Cleaning** — Automatic heater cycling to maintain sensor accuracy
- [x] **Serial Output** — Real-time data logging via Serial Monitor
- [ ] **TODO: Cloud Integration** — Future feature to offload alerts to Arduino Cloud Agent or Personal Web Server with dashboard display and email notifications

---

## 🚀 Getting Started

### Prerequisites

- All components listed in the Technology section
- Arduino IDE installed on your computer
- Required libraries:
  - Adafruit_Si7021
  - Adafruit_BusIO
  - LiquidCrystal

### Hardware Setup

1. **LCD Display Connection:**
   - RS pin to Arduino pin 12
   - EN pin to Arduino pin 11
   - D4 pin to Arduino pin 5
   - D5 pin to Arduino pin 4
   - D6 pin to Arduino pin 3
   - D7 pin to Arduino pin 2
   - Connect potentiometer to LCD contrast pin
   
2. **LED Setup:**
   - Red LED to Arduino pin 10 (with 220Ω resistor)
   - Yellow LED to Arduino pin 9 (with 220Ω resistor)
   - Green LED to Arduino pin 8 (with 220Ω resistor)
   
3. **Buzzer Connection:**
   - Connect to Arduino pin 6
   
4. **Si7021 Sensor Connection:**
   - Connect VCC to 3.3V
   - Connect GND to ground
   - Connect SCL to Arduino SCL pin
   - Connect SDA to Arduino SDA pin

### Software Installation

```bash
# Clone the repo
git clone https://github.com/yourname/arduino-humidity-monitor.git
cd arduino-humidity-monitor

# Install required libraries in Arduino IDE
# Tools > Manage Libraries... > Search and install:
# - Adafruit Si7021 Library
# - Adafruit BusIO
# - LiquidCrystal (included with Arduino IDE)

# Open the project in Arduino IDE and upload to your Arduino board
```

---

## 📝 Usage

1. After uploading the code, the system will immediately begin monitoring humidity
2. The LCD will display current readings and status
3. LEDs and buzzer will provide immediate feedback on humidity conditions:
   - Green LED: Humidity is in ideal range
   - Yellow LED + beep: Humidity is approaching limits
   - Red LED + warning tone: Humidity is outside acceptable range
4. Open the Serial Monitor (115200 baud) to view detailed readings
5. To switch between indoor and terrarium modes, modify the `measureIndoor` variable in the code

---

## ⚙️ Configuration

The system has predefined humidity ranges for both terrarium and indoor environments:

### Terrarium Mode:
- Ideal range: 65-88% humidity
- Warning range: 60-65% or 88-93% humidity
- Alert range: <60% or >93% humidity

### Indoor Mode:
- Ideal range: 35-55% humidity
- Warning range: 30-35% or 55-60% humidity
- Alert range: <30% or >60% humidity

To adjust these ranges, modify the enum values in the code:

```cpp
// For terrarium environments
enum terrarium {
  TERRARIUM_LOW = 60, 
  TERRARIUM_CLOSE_LOW = 65, 
  TERRARIUM_CLOSE_HIGH = 88, 
  TERRARIUM_HIGH = 93
};

// For indoor environments
enum indoor {
  INDOOR_LOW = 30, 
  INDOOR_CLOSE_LOW = 35, 
  INDOOR_CLOSE_HIGH = 55, 
  INDOOR_HIGH = 60
};
```

---

## 🖼️ Screenshots
*coming next commit*

---

## 🤝 Contributing

1. Fork the project 
2. Create your feature branch (`git checkout -b feature/feature-name`)
3. Commit your changes (`git commit -m "Add feature"`)
4. Push to the branch (`git push origin feature/feature-name`)
5. Open a pull request

---

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.
