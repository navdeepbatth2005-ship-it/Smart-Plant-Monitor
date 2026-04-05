# 🌿 Smart Plant Monitoring System

> An IoT system that classifies plant stress in real-time using ESP32, environmental sensors, and a logistic regression model trained on 15 years of regional climate data — with instant Blynk mobile alerts.

![Platform](https://img.shields.io/badge/Platform-ESP32-blue?style=flat-square)
![Sensors](https://img.shields.io/badge/Sensors-DHT11%20%7C%20LDR%20%7C%20Soil%20Moisture-green?style=flat-square)
![Model](https://img.shields.io/badge/Model-Logistic%20Regression-orange?style=flat-square)
![Accuracy](https://img.shields.io/badge/Accuracy-79.5%25-yellowgreen?style=flat-square)
![Simulation](https://img.shields.io/badge/Simulation-Wokwi-purple?style=flat-square)
![Dashboard](https://img.shields.io/badge/Dashboard-Blynk-cyan?style=flat-square)

---

## 📌 Project Overview

“You have to speak for the trees, for the trees have no tongues.” – Dr. Seuss. This system continuously listens to the plant by continuous monitoring of soil moisture, ambient light (LDR), temperature and humidity (DHT11).Then computes a **Stress Index**, and classifies plant health into one of two states using logistic regression:


| Class |           Label           | Blynk Alert                                    |
|-------|---------------------------|------------------------------------------------|
| `0`   | Healthy / Moderate Stress | ✅ *No action needed* or *Monitor closely*     |
| `1`   | Critical Stress           | 🚨 *Water immediately* / *Keep it under shade* |

The model was trained on **NASA POWER DAV** climate data from the **Ludhiana, Punjab region (2010–2025)**, making the stress thresholds regionally calibrated — not generic.

---

## 🏗️ System Architecture

```
[ DHT11 Sensor ]──┐
[ LDR Sensor   ]──┤──► [ ESP32 ] ──► Stress Index ──► Logistic Regression ──► Blynk App
[ Soil Moisture]──┘          │                                                     │
                             └──────────────── Serial Monitor ◄───────────────────┘
```

---

## 🧠 The Model

### Data Source
- **NASA POWER DAV API** — Ludhiana, Punjab, India
- **Time range:** 2010 – 2025 (15 years of daily climate records)
- **Parameters used:** Temperature (T2M), Relative Humidity (RH2M), and surface radiation proxies

### Stress Index Formula
A composite Stress Index was derived from the climate variables to label each observation:

```
float rawPSI = (w1 * lux) + (w2 * temperature) + (w3 * humidity) + (w4 * soil_moisture) + b;
```

Thresholds applied to generate binary class labels for supervised training.

### Model: Logistic Regression

```
Training size : 117 samples
Test size     :  39 samples
```

**Classification Report (Test Set):**

```
              precision    recall  f1-score   support

           0       0.82      0.93      0.88        30    ← Healthy/Moderate
           1       0.60      0.33      0.43         9    ← Critical Stress

    accuracy                           0.79        39
   macro avg       0.71      0.63      0.65        39
weighted avg       0.77      0.79      0.77        39
```

### ⚠️ Honest Limitations

- **Small dataset (39 test samples):** Metrics may not generalise well. A larger labeled field dataset is needed for production confidence.
- **Class imbalance (30:9):** The model is biased toward Class 0. Class 1 recall of 0.33 means it misses ~67% of critical stress events — a known risk in imbalanced classification.
- **Binary classification:** The system currently outputs two states. Multi-class extension (Healthy / Moderate / Critical) is planned as future work.
- **DHT11 vs DHT22:** Hardware prototype uses DHT11 (±2°C, ±5% RH); Wokwi simulation uses DHT22 (±0.5°C, ±2% RH) for higher fidelity simulation.

---

## 🔧 Hardware Components

| Component | Role |
|-----------|------|
| ESP32 Dev Board | Microcontroller + Wi-Fi |
| DHT11 | Temperature & Humidity sensing |
| LDR (Light Dependent Resistor) | Ambient light measurement |
| Soil Moisture Sensor | Soil water content (analog) |
| Resistors (10kΩ) | Pull-down for LDR circuit |
| Jumper Wires + Breadboard | Prototyping |

> 📷 *See `/components/hardwares.jpg` for the physical build.*

---

## 📱 Blynk Dashboard

The Blynk app receives real-time sensor readings and displays:
- Temperature, Humidity, Light level, Soil Moisture (value display)
- Computed Stress Index (gauge widgets)
- **One-liner recommendation:** contextual text pushed via `Blynk.virtualWrite()`

**Alert examples:**
```
🔴 "Critical stress: Water immediately!"
🟡 "Moderate stress: Check water & sunlight"
🟢 "Healthy: No stress."
```

> 📷 *See `/screenshots/` for Critical, Moderate, and Healthy state captures.*

---

## 💻 Wokwi Simulation

The full circuit is simulated on Wokwi with DHT22 (higher precision than DHT11 for simulation accuracy).

🔗 **[Open Wokwi Simulation]([https://wokwi.com/projects/457568602667820033])** 

> 📷 *See `/simulation/wokwi_screenshot.png`*

---

## 📁 Repository Structure

```
smart-plant-monitor/
├── README.md
├── src/
│   └── main.ino                  # Arduino/ESP32 firmware
├── model/
│   ├── stress_model.py           # Logistic regression training script
│   ├── nasa_data_2010_2025.csv   # Raw NASA POWER DAV dataset
│   └── model_analysis.ipynb      # EDA, training, evaluation notebook
├── hardware/
│   ├── components.jpg            # Physical hardware photo
│   └── circuit_diagram.png       # Schematic
├── simulation/
│   ├── wokwi_link.md             # Wokwi project link
│   └── wokwi_screenshot.png
└── screenshots/
    ├── healthy_state.png
    ├── moderate_stress.png
    ├── critical_condition.png
    └── blynk_dashboard.png
```

---

## 🚀 Getting Started

### 1. Flash the firmware
```bash
# Open src/main.ino in Arduino IDE
# Install required libraries:
#   - DHT sensor library (Adafruit)
#   - BlynkSimpleEsp32
#   - WiFi (built-in)
```

### 2. Configure credentials
```cpp
// In main.ino, update:
char auth[] = "YOUR_BLYNK_AUTH_TOKEN";
char ssid[] = "YOUR_WIFI_SSID";
char pass[] = "YOUR_WIFI_PASSWORD";
```

### 3. Upload model thresholds
The logistic regression coefficients are hardcoded in firmware based on the trained model. To retrain:
```bash
cd model/
pip install pandas scikit-learn matplotlib jupyter
jupyter notebook model_analysis.ipynb
```

---

## 📊 Serial Monitor Output

```
<img width="710" height="241" alt="Screenshot 2026-04-05 193754" src="https://github.com/user-attachments/assets/3f216abe-09e8-46b2-b1cd-0a0601cc70fc" />
```

## 🔮 Future Work

- [ ] Expand to 3-class model (Healthy / Moderate / Critical) with more labeled data
- [ ] Handle class imbalance with SMOTE or class weighting
- [ ] Add automated irrigation relay trigger on Critical state
- [ ] OTA (Over-the-Air) firmware updates via ESP32
- [ ] Historical data logging to Google Sheets via IFTTT/Webhooks
- [ ] Validate model against real field sensor readings

---

## 👨‍💻 Author

**[Navdeep Kaur Bath]**
B.E [Biotechnology] — [Chandigarh University]
📍 Mohali, Punjab, India

---

## 📜 License

MIT License — free to use, modify, and distribute with attribution.
