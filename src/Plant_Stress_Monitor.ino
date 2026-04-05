#define BLYNK_TEMPLATE_ID "Your_Blynk_ID"
#define BLYNK_TEMPLATE_NAME "Your_Template_Name"
#define BLYNK_AUTH_TOKEN "Your_AUTH_TOKEN"

#include <Arduino.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>

// -------- WIFI --------
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Your Wife name";
char pass[] = "Wifi password";

// -------- PINS --------
#define DHTPIN 33
#define DHTTYPE DHT11
#define SOIL_PIN 35
#define LDR_PIN  32
#define BUZZER   15
#define LED      2    // Yellow/Red — Moderate/Critical
#define GREEN_LED 4   // Green — Normal/Healthy

// -------- BLYNK VIRTUAL PINS --------
// V0 = Temperature
// V1 = Humidity
// V2 = Soil Moisture (%)
// V3 = Light (%)
// V5 = Status Message
// V4 = PSI Value

// -------- OBJECTS --------
DHT dht(DHTPIN, DHTTYPE);
BlynkTimer timer;

// -------- GLOBAL VARIABLES --------
float currentPSI  = 0;
bool  greenState  = false;
bool  alertSent   = false;

// -------- MODEL WEIGHTS --------
// IMPORTANT: These weights were trained on RAW ADC values (0–4095).
// PSI output is therefore also in a raw scale — we normalise it below.
float w1 = -0.00771605f;   // lux (raw LDR ADC)
float w2 =  0.03646973f;   // temperature (°C)
float w3 =  0.01274697f;   // humidity (%)
float w4 =  1.14942529f;   // soil moisture (raw ADC)
float b  =  0.0f;

// Raw value ranges observed from your sensors (adjust if needed)
const float PSI_RAW_MIN = 0.0f;
const float PSI_RAW_MAX = 4500.0f;   // tune: Serial-print rawPSI on a dry plant

float predict_psi(float lux, float temperature, float humidity, float soil_moisture) {
  float rawPSI = (w1 * lux) + (w2 * temperature) + (w3 * humidity) + (w4 * soil_moisture) + b;

  // Normalise to 0–100 so thresholds (30 / 70) make sense
  float normalised = (rawPSI - PSI_RAW_MIN) / (PSI_RAW_MAX - PSI_RAW_MIN) * 100.0f;
  normalised = constrain(normalised, 0.0f, 100.0f);
  return normalised;
}

// -------- MAP ADC → PERCENTAGE (for display) --------
// Soil: HIGH ADC = DRY (less conductive) → invert so 100% = fully wet
float soilPercent(int raw) {
  return constrain(map(raw, 4095, 0, 0, 100), 0, 100);
}

// Light: HIGH ADC on LDR usually = bright (depends on circuit); adjust if inverted
float lightPercent(int raw) {
  return constrain(map(raw, 0, 4095, 0, 100), 0, 100);
}

// -------- SENSOR READ + BLYNK SEND --------
void sendToBlynk() {
  float temperature = dht.readTemperature();
  float humidity    = dht.readHumidity();
  int   soilRaw     = analogRead(SOIL_PIN);
  int   lightRaw    = analogRead(LDR_PIN);

  // --- Validate DHT ---
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("[ERROR] DHT11 read failed — check wiring on pin 33");
    return;
  }

  float soilPct  = soilPercent(soilRaw);
  float lightPct = lightPercent(lightRaw);
  float psi      = predict_psi(lightRaw, temperature, humidity, soilRaw);
  currentPSI     = psi;

  // --- Serial Monitor Output ---
  Serial.println("========================================");
  Serial.print("Temperature   : "); Serial.print(temperature); Serial.println(" °C");
  Serial.print("Humidity      : "); Serial.print(humidity);    Serial.println(" %");
  Serial.print("Soil (raw)    : "); Serial.print(soilRaw);
  Serial.print("  →  ");           Serial.print(soilPct);     Serial.println(" %");
  Serial.print("Light (raw)   : "); Serial.print(lightRaw);
  Serial.print("  →  ");           Serial.print(lightPct);    Serial.println(" %");
  Serial.print("PSI (0-100)   : "); Serial.println(psi);

  // --- Send sensor readings to Blynk ---
  Blynk.virtualWrite(V0, temperature);
  Blynk.virtualWrite(V1, humidity);
  Blynk.virtualWrite(V2, soilPct);
  Blynk.virtualWrite(V3, lightPct);
  Blynk.virtualWrite(V4, psi);         // PSI on V6

  // --- Status + Recommendation ---
  String statusMessage;
  if (psi < 30) {
    statusMessage = "Healthy: No stress";
  } else if (psi < 70) {
    statusMessage = "Moderate stress: Check water & sunlight";
  } else {
    statusMessage = "Critical stress: Water immediately!";
  }
  Blynk.virtualWrite(V5, statusMessage);

  Serial.print("Status        : "); Serial.println(statusMessage);
  Serial.println("========================================");
}

// -------- OUTPUT CONTROL (LEDs + Buzzer) --------
void controlOutputs() {

  // 🌱 NORMAL — blink green, everything else OFF
  if (currentPSI < 30) {
    greenState = !greenState;
    digitalWrite(GREEN_LED, greenState);
    digitalWrite(LED,    LOW);    // ← FIX: turn off alert LED
    digitalWrite(BUZZER, LOW);    // ← FIX: turn off buzzer
    alertSent = false;
  }

  // ⚠️ MODERATE — solid alert LED, no buzzer
  else if (currentPSI < 70) {
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(LED,    HIGH);
    digitalWrite(BUZZER, LOW);    // ← FIX: buzzer off in moderate

    if (!alertSent) {
      Blynk.logEvent("plant_alert",
        "Moderate Stress! Check soil moisture & adjust sunlight.");
      alertSent = true;
    }
  }

  // 🚨 CRITICAL — alert LED + buzzer
  else {
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(LED,    HIGH);
    digitalWrite(BUZZER, HIGH);

    if (!alertSent) {
      Blynk.logEvent("plant_alert",
        "CRITICAL! Water immediately & fix light/temperature.");
      alertSent = true;
    }
  }
}

// -------- SETUP --------
void setup() {
  Serial.begin(115200);
  delay(500);

  dht.begin();

  pinMode(BUZZER,    OUTPUT);
  pinMode(LED,       OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  // Safe initial state
  digitalWrite(BUZZER,    LOW);
  digitalWrite(LED,       LOW);
  digitalWrite(GREEN_LED, LOW);

  Serial.println("[BOOT] Connecting to Blynk...");
  Blynk.begin(auth, ssid, pass);
  Serial.println("[BOOT] Connected!");

  timer.setInterval(5000L, sendToBlynk);    // sensors + PSI every 5s
}

// -------- LOOP --------
void loop() {
  Blynk.run();
  timer.run();
}
