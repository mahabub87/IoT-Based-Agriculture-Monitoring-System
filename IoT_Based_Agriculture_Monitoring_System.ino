#include <WiFi.h>
#include <DHT.h>
#include <LCD_I2C.h>
TaskHandle_t Core2;

// ======== BLYNK CONFIG =========
#define BLYNK_TEMPLATE_ID "TMPL6Y9HKRXLG"
#define BLYNK_TEMPLATE_NAME "IoT Based Agriculture Monitoring System"
#define BLYNK_AUTH_TOKEN "H2Tmk7HaoKhLfPy_2sc6SeveiQ1t0iNI"

char ssid[] = "DIU_Daffodil Smart City";  // Replace with your WiFi SSID
char pass[] = "diu123456789";             // Replace with your WiFi Password

#include <BlynkSimpleEsp32.h>

// ======== SENSOR & HARDWARE CONFIG =========
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

LCD_I2C lcd(0x27, 16, 2);

int sensorPin = 34;  // Soil Moisture Sensor
int relayPin = 26;   // Relay output

int Lowerlimit = 30;
int Upperlimit = 50;
bool pumpState = false;

int moisture = 0;
int t = 0, h = 0;

BlynkTimer timer;

// ======== BLYNK VIRTUAL PIN HANDLERS =========
BLYNK_WRITE(V4) {
  if (Lowerlimit == HIGH) {
    delay(1000);
  }
  Lowerlimit++;
  Lowerlimit = constrain(Lowerlimit, 0, 50);
}
BLYNK_WRITE(V5) {
  if (Lowerlimit == HIGH) {
    delay(1000);
  }
  Lowerlimit--;
  Lowerlimit = constrain(Lowerlimit, 0, 50);
}
BLYNK_WRITE(V6) {
  if (Upperlimit == HIGH) {
    delay(1000);
  }
  Upperlimit++;
  Upperlimit = constrain(Upperlimit, 30, 100);
}
BLYNK_WRITE(V7) {
  if (Upperlimit == HIGH) {
    delay(1000);
  }
  Upperlimit--;
  Upperlimit = constrain(Upperlimit, 30, 100);
}

void setup() {
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);  // Make sure pump is OFF
  delay(500);                   // Let ESP32 power stabilize
  Serial.begin(115200);
  lcd.begin();
  lcd.backlight();
  dht.begin();
  delay(1000);  // Wait before WiFi starts

  //ENABLE DUAL CORE MULTITASKING
  xTaskCreatePinnedToCore(coreTwo, "coreTwo", 10000, NULL, 0, &Core2, 0);
}

void coreTwo(void* pvParameters) {
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  //================= CORE0: LOOP (DUAL CORE MODE) ======================//
  for (;;) {
    Blynk.run();
    timer.run();
  }
}


void loop() {
  updateSystem();
  delay(3000);
}

// ======== SENSOR + LCD + LOGIC UPDATE =========
void updateSystem() {
  // === Read DHT11 ===
  h = dht.readHumidity();
  t = dht.readTemperature();
  h = constrain(h, 0, 100);
  t = constrain(t, 0, 70);

  // === Read Moisture ===
  moisture = analogRead(sensorPin);
  moisture = 4095 - moisture;
  moisture = map(moisture, 0, 4095, 0, 100);  // ESP32 ADC is 12-bit

  // === Serial Output ===
  Serial.printf("Temp: %d°C, Humidity: %d%%, Moisture: %d%%\n", t, h, moisture);

  // === Watering Control with Hysteresis ===
  if (!pumpState && moisture < Lowerlimit) {
    pumpState = true;
    digitalWrite(relayPin, HIGH);
  }
  if (pumpState && moisture > Upperlimit) {
    pumpState = false;
    digitalWrite(relayPin, LOW);
  }

  // === LCD Output ===
  lcd.setCursor(0, 0);
  lcd.printf("Tem Hum Mo. Pump");
  lcd.setCursor(0, 1);
  lcd.print(String(t) + "C " + String(h) + "% " + String(moisture) + "% ");

  // === LCD Display for Pump State ===
  lcd.setCursor(13, 1);
  lcd.print(pumpState ? "ON " : "OFF");

  // === Blynk Display ===
  Blynk.virtualWrite(V0, t);
  Blynk.virtualWrite(V1, h);
  Blynk.virtualWrite(V2, moisture);
  Blynk.virtualWrite(V3, pumpState ? "ON" : "OFF");
  Blynk.virtualWrite(V8, Lowerlimit);
  Blynk.virtualWrite(V9, Upperlimit);
}