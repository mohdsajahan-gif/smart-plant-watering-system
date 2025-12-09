# Smart Plant Watering System

IoT-based intelligent plant watering system using ESP32-C3 microcontroller and ESP RainMaker cloud platform.

## 🌱 Project Overview

Smart Plant is an automated watering system that monitors temperature and humidity in real-time, with remote control via mobile app and Amazon Alexa voice commands.

## ✨ Features

- ✅ Real-time temperature & humidity monitoring (DHT11 sensor)
- ✅ ESP RainMaker cloud integration
- ✅ Amazon Alexa voice control
- ✅ FreeRTOS multitasking (4 independent tasks)
- ✅ 0.96" OLED display for local monitoring
- ✅ Push notifications for critical conditions
- ✅ Remote control via RainMaker mobile app
- ✅ OTA firmware update support

## 🔧 Hardware Components

| Component | Model | Function |
|-----------|-------|----------|
| Microcontroller | ESP32-C3 (Seeed XIAO) | Main controller |
| Sensor | DHT11 | Temperature & humidity |
| Display | 0.96" I²C OLED | Local monitoring |
| Actuator | Buzzer | Pump simulator |
| Power | USB-C | 5V power supply |

## 📌 Pin Configuration

| Component | ESP32-C3 Pin | Notes |
|-----------|--------------|-------|
| DHT11 Data | GPIO 2 | Temperature/humidity sensor |
| Buzzer | GPIO 3 | Pump control output |
| OLED SDA | GPIO 6 | I²C data line |
| OLED SCL | GPIO 7 | I²C clock line |
| All GND | GND | Common ground |
| All VCC | 3.3V | Power supply |

## 💻 Software Architecture

**Development Environment:**
- ESP-IDF v5.5.1
- FreeRTOS
- C Programming Language
- Visual Studio Code

**FreeRTOS Tasks:**

1. **Task 1: Sensor Reading (Priority 5)**
   - Reads DHT11 every 3 seconds
   - Stores data in mutex-protected structure

2. **Task 2: Cloud Communication (Priority 4)**
   - Updates RainMaker every 5 seconds
   - Synchronizes device state with cloud

3. **Task 3: Alert & Notification (Priority 3)**
   - Monitors thresholds every 10 seconds
   - Sends push notifications when:
     - Temperature ≥ 35°C
     - Humidity ≤ 40%

4. **Task 4: Display Update (Priority 2)**
   - Refreshes OLED every 1 second
   - Shows temperature, humidity, pump status

## 🎯 Thresholds

| Parameter | Threshold | Action |
|-----------|-----------|--------|
| Temperature High | ≥ 35°C | Alert notification |
| Humidity Low | ≤ 40% | Alert notification |

## 🔗 Project Links

- 📄 **Full Report:** [Your Google Sites URL - add this later]
- 🎥 **Video Demo:** https://youtube.com/shorts/JrJXHjwujsc
- 📱 **Cloud Platform:** ESP RainMaker

## 🎓 Course Information

**Course:** EEM5043 Advanced Embedded Systems  
**Instructor:** Assoc Prof Dr. Mohd Zuki Yusoff  
**Institution:** Universiti Teknologi PETRONAS  
**Semester:** September 2025

## 📸 Project Photos

### Hardware Setup
[Add photos here later if you want]

### RainMaker App
Temperature and humidity monitoring with remote pump control.

### OLED Display
Real-time local monitoring display.

## 🚀 Setup Instructions

1. **Hardware Setup:**
   - Connect components according to pin configuration
   - Power via USB-C

2. **Software Setup:**
   - Install ESP-IDF v5.5.1
   - Clone this repository
   - Build and flash: `idf.py build flash monitor`

3. **Cloud Setup:**
   - Install ESP RainMaker app
   - Provision device via BLE
   - Link Amazon Alexa skill

## 📝 Key Technical Achievements

- Thread-safe data sharing using mutex
- Non-blocking task implementation
- Graceful sensor error handling
- Efficient FreeRTOS task scheduling
- Stable cloud connectivity
- Voice control integration

## 🔮 Future Enhancements

- Add soil moisture sensor
- Implement water level monitoring
- Add light sensor for day/night detection
- Replace buzzer with real water pump
- Machine learning for predictive watering
- Google Assistant integration

## 📄 License

This project is developed for educational purposes as part of EEM5043 coursework.

## 👤 Author

**[Your Name]**  
Student ID: [Your ID]  
Email: [Your Email - optional]

---

⭐ If you find this project helpful, please consider giving it a star!
```

---

## **STEP 5: Commit Changes**

1. Scroll down to bottom of edit page
2. In "Commit changes" box, type: **"Initial commit - Project documentation"**
3. Click green **"Commit changes"** button

---

## **STEP 6: Copy Your GitHub URL**

After committing, you'll see your repository with the README displayed.

**Copy the URL from browser address bar:**

It looks like:
```
https://github.com/YourUsername/smart-plant-watering-system
```

**SAVE THIS LINK!** You need it for submission!

---

## 📁 OPTIONAL: Upload Your Code (If You Have Time)

If you want to upload your actual code files:

1. Click **"Add file"** button
2. Click **"Upload files"**
3. Drag and drop these files:
   - `app_main.c`
   - `app_driver.c`
   - `app_priv.h`
   - Any other project files
4. Add commit message: **"Add source code"**
5. Click **"Commit changes"**

**But this is OPTIONAL** - the README alone is acceptable!

---

## ✅ UPDATE YOUR GOOGLE SITES WITH GITHUB LINK

Once you have your GitHub URL:

1. Go back to your Google Sites
2. Find the section with `[Insert GitHub URL]`
3. Replace it with your actual GitHub link
4. Click **Publish** again to update

---

## 📋 WHAT YOU NOW HAVE

- ✅ **YouTube Video:** https://youtube.com/shorts/JrJXHjwujsc
- ✅ **GitHub Repository:** https://github.com/YourUsername/smart-plant-watering-system
- ⏳ **Google Sites:** [URL after you publish]

---

## 🎯 FINAL SUBMISSION CHECKLIST

Once you have all 3 links, create your submission PDF with:
```
Smart Plant Watering System - Final Submission

Student: Mohammed Sajahan Bin Allapitchai
ID: 25014313
Date: 10-Dec-2025

---

1. GitHub Repository
Link: [Your GitHub URL]
Description: Complete project documentation with README, hardware 
specifications, pin configuration, software architecture, and FreeRTOS 
task details.

2. Web-Based Project Report  
Link: (https://sites.google.com/d/1GyyAtEeG98JHBqbb1jsJ5bjgvGJTgWPO/p/1C8xqgyhqPNePyKZt6_Oq-K-8RGxQGENI/edit)
Description: Comprehensive report including introduction, objectives, 
hardware design, software implementation, testing results, challenges, 
conclusions, and embedded video demonstration.

3. Video Demonstration
Link: https://youtube.com/shorts/JrJXHjwujsc
Description: Video demonstration showing complete system functionality 
including hardware setup, OLED display, RainMaker app, manual control, 
Alexa voice commands, and FreeRTOS tasks.

---

All links tested and verified as accessible.

Signature: sajahan
Date: 10-Dec-2025
