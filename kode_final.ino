// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                            Import All Library
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                            WiFi Configuration
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* ssid = "hasyim";
const char* password = "bintangenam";

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                            MQTT Configuration
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* mqtt_server = "192.168.124.245";
const int mqtt_port = 1883;
const char* mqtt_client_id = "esp32_client";
const char* mqtt_username = "uas25_hasyim";
const char* mqtt_password = "uas25_hasyim";

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                MQTT Topics
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* led_topic = "esp32/led";
const char* pir_topic = "esp32/pir";
const char* sensor_topic = "esp32/sensor";
const char* status_topic = "esp32/status";
const char* lampu_topic = "esp32/lampu";
const char* kipas_topic = "esp32/kipas";
const char* listrik_topic = "esp32/listrik";

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//              Pin Configuration (gunakan GPIO number langsung)
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#define LED_PIN 2       // GPIO2, LED built-in pada beberapa board
#define PIR_PIN 18
#define FLAME_PIN 34
#define GASSMOKE_PIN 35
#define DHT_PIN 4
#define DHT_TYPE DHT11
#define LAMPU_PIN 17
#define KIPAS_PIN 16
#define IN1 14
#define IN2 27
#define IN3 26
#define IN4 25

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Ikon suhu 16x16 (termometer dengan bulb bulat dan skala)
const unsigned char tempIcon_16x16[] PROGMEM = {
  0x03, 0x80,  // ......111.......
  0x04, 0x40,  // .....1...1......
  0x04, 0x40,  // .....1...1......
  0x04, 0x40,  // .....1...1......
  0x05, 0x40,  // .....1.1.1......
  0x04, 0x40,  // .....1...1......
  0x05, 0x40,  // .....1.1.1......
  0x04, 0x40,  // .....1...1......
  0x05, 0x40,  // .....1.1.1......
  0x04, 0x40,  // .....1...1......
  0x0E, 0xE0,  // ....111.111.....
  0x1F, 0xF0,  // ...111111111....
  0x1F, 0xF0,  // ...111111111....
  0x1F, 0xF0,  // ...111111111....
  0x0E, 0xE0,  // ....111.111.....
  0x07, 0xC0   // .....11111......
};

// Ikon kelembapan 16x16 (tetes utama solid + tetes kecil outline di kiri atas)
const unsigned char waterIcon_16x16[] PROGMEM = {
  0x11, 0x80,  // ...1...11.......
  0x2B, 0xC0,  // ..1.1.111111....
  0x2B, 0xC0,  // ..1.1.111111....
  0x17, 0xE0,  // ...1.111111.....
  0x07, 0xE0,  // .....111111.....
  0x0F, 0xF0,  // ....11111111....
  0x0F, 0xF0,  // ....11111111....
  0x1F, 0xF8,  // ...1111111111...
  0x1F, 0xF8,  // ...1111111111...
  0x1F, 0xF8,  // ...1111111111...
  0x1F, 0xF8,  // ...1111111111...
  0x0F, 0xF0,  // ....11111111....
  0x0F, 0xF0,  // ....11111111....
  0x07, 0xE0,  // .....111111.....
  0x03, 0xC0,  // ......1111......
  0x00, 0x00   // ................
};

// Ikon api 16x16 
const unsigned char fireIcon_16x16[] PROGMEM = {
  0x00, 0x00,
  0x00, 0x00,
  0x01, 0x00,
  0x00, 0x80,
  0x01, 0x40,
  0x02, 0x80,
  0x0A, 0x80,
  0x09, 0x30,
  0x10, 0xC8,
  0x14, 0x28,
  0x12, 0x90,
  0x08, 0x10,
  0x04, 0x20,
  0x02, 0xC0,
  0x01, 0x00,
  0x00, 0x00
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                 Objects
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHT_PIN, DHT_TYPE);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//        Langkah Motor Stepper Berputar - Urutan 8 langkah half-step
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int halfStepSequence[8][4] = {
  {1, 0, 0, 0},
  {1, 1, 0, 0},
  {0, 1, 0, 0},
  {0, 1, 1, 0},
  {0, 0, 1, 0},
  {0, 0, 1, 1},
  {0, 0, 0, 1},
  {1, 0, 0, 1}
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                 Variables
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float temperature = 0; //Suhu
float humidity = 0; //Lembab
bool ledState = false; //Led
int baseline = 0; //Gas baseline
int flamePercent = 0; //Api
int gasPercent = 0; //Gas Percent
// int currentStep = 0;
unsigned long lastUpdate = 0;
int animationFrame = 0;
unsigned long lastSensorRead = 0;
unsigned long lastStatusPublish = 0;
unsigned long startTime = 0;
const unsigned long sensorInterval = 1000;
const unsigned long statusInterval = 1000;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                Fungsi Setup()
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void setup() {
  Serial.begin(115200);
  delay(10);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(LAMPU_PIN, OUTPUT);   // Pastikan relay awalnya OFF
  digitalWrite(LAMPU_PIN, LOW);
  pinMode(KIPAS_PIN, OUTPUT);   // Pastikan relay awalnya OFF
  digitalWrite(KIPAS_PIN, LOW);
  pinMode(PIR_PIN, INPUT);

  // PIN MODE untuk Motor Stepper Driver
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);


  dht.begin();
  startTime = millis();

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  Serial.println("ESP32 MQTT IoT Device Ready!");

  // Inisialisasi OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Gagal menginisialisasi OLED"));
    while (true);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Tampilkan splash screen
  showSplashScreen();
  delay(2000);

  display.setCursor(0, 0);
  display.println("Memanaskan MQ-2...");
  display.display();

  delay(60000); // Pemanasan 60 detik

  // Ambil baseline udara bersih
  long total = 0;
  for (int i = 0; i < 100; i++) {
    total += analogRead(GASSMOKE_PIN);
    delay(10);
  }
  baseline = total / 100;

  Serial.print("Baseline udara bersih: ");
  Serial.println(baseline);
  
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Sensor Siap");
  display.print("Baseline: ");
  display.println(baseline);
  display.display();
  delay(2000);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                            Fungsi Setup_WiFi()
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void setup_wifi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//  Fungsi Callback() - *Fungsi Dijalankan Otomatis saat ada Pesan Masuk ⬇️
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) message += (char)payload[i];

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(message);

  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, message);
  if (error) {
    Serial.println("Failed to parse JSON");
    return;
  }
  // deserializeJson(doc, message);

  bool state = doc["state"];

  String topicStr = String(topic);

  //LAMPU
  if (topicStr == lampu_topic) {
    digitalWrite(LAMPU_PIN, state ? HIGH : LOW);
    Serial.println("Lampu turned " + String(state ? "ON" : "OFF"));
  } 
  //KIPAS
  else if (topicStr == kipas_topic) {
    digitalWrite(KIPAS_PIN, state ? HIGH : LOW);
    Serial.println("Kipas turned " + String(state ? "ON" : "OFF"));
  } 
  //LISTRIK
  else if (topicStr == listrik_topic) {
    digitalWrite(LAMPU_PIN, state ? HIGH : LOW);
    digitalWrite(KIPAS_PIN, state ? HIGH : LOW);
    Serial.println("Listrik Master Switch: " + String(state ? "ON (All ON)" : "OFF (All OFF)"));

    // Publish ulang status ke lampu dan kipas topic agar sinkron
    publishLampuKipasState(state, state);
  }
  //PIR
  else if (topicStr == pir_topic) {
    bool gerakanTerdeteksi = doc["state"];
    
    if (gerakanTerdeteksi) {
        Serial.println("Gerakan terdeteksi dari MQTT! Menyalakan motor dan LED.");
        digitalWrite(LAMPU_PIN, HIGH);   // Nyalakan LED lewat relay
        aktifkanMotorStepper();              // Fungsi untuk nyalakan motor stepper
    } else {
        Serial.println("Tidak ada gerakan dari MQTT. Mematikan motor dan LED.");
        digitalWrite(LAMPU_PIN, LOW);    // Matikan LED lewat Relay
        matikanMotorStepper();               // Fungsi untuk matikan motor
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                      Fungsi Publish State Lampu & Kipas()
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void publishLampuKipasState(bool lampuState, bool kipasState) {
  // Publish status Lampu
  DynamicJsonDocument docLampu(64);
  docLampu["state"] = lampuState;
  String jsonLampu;
  serializeJson(docLampu, jsonLampu);
  client.publish(lampu_topic, jsonLampu.c_str());

  // Publish status Kipas
  DynamicJsonDocument docKipas(64);
  docKipas["state"] = kipasState;
  String jsonKipas;
  serializeJson(docKipas, jsonKipas);
  client.publish(kipas_topic, jsonKipas.c_str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                     Fungsi Publish Data Sensor ESP32()
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void publishSensorData() {
  int pirState = digitalRead(PIR_PIN);
  int flameValue = analogRead(FLAME_PIN); // 0–4095
  int gasValue = analogRead(GASSMOKE_PIN);
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Ubah ke skala 0–100% (semakin kecil nilai, semakin kuat api)
  int flamePercent = map(flameValue, 4095, 1800, 0, 100);
  flamePercent = constrain(flamePercent, 0, 100); // Batas aman

  // Ubah ke skala 0–100%
  gasPercent = map(gasValue, 10, 400, 0, 100); // asumsi nilai normal 300–1200
  gasPercent = constrain(gasPercent, 0, 100);

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("DHT sensor read failed!");
    return;
  }

  if (pirState == HIGH) {
    Serial.println("Gerakan Terdeteksi");
  } else {
    Serial.println("Tidak Ada Gerakan.");
  }

  // Tampilkan status api berdasarkan intensitas
  String fireStatus;
  if (flamePercent < 30) {
    fireStatus = "AMAN";
  } else if (flamePercent < 75) {
    fireStatus = "WASPADA";
  } else {
    fireStatus = "BAHAYA";
  }

  // Status Gas (berdasarkan gasPercent)
  String gasStatus;
  int baseline = 0; // contoh baseline, sesuaikan jika dibaca dari sensor
  if (gasValue > baseline + 200) {
    gasStatus = "BOCOR";
  } else {
    gasStatus = "NORMAL";
  }

  // JSON Document cukup besar untuk menampung semua data
  DynamicJsonDocument doc(512);
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["pir"] = (pirState == HIGH) ? "Gerakan Terdeteksi" : "Tidak Ada Gerakan";
  doc["flame_percent"] = flamePercent;
  doc["gas_percent"] = gasPercent;
  doc["flame_status"] = fireStatus;
  doc["gas_status"] = gasStatus;
  doc["timestamp"] = millis();

  String jsonString;
  serializeJson(doc, jsonString);

  if (client.publish(sensor_topic, jsonString.c_str())) {
    Serial.println("Published Sensor Data:");
    Serial.println(jsonString);
  } else {
    Serial.println("Failed to publish sensor data");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                     Fungsi Publish Status ESP32 Online()
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void publishStatusOnline() {
  DynamicJsonDocument doc(128);
  doc["status"] = "online";
  doc["uptime"] = (millis() - startTime) / 1000;
  doc["free_heap"] = ESP.getFreeHeap();
  doc["wifi_rssi"] = WiFi.RSSI();

  String jsonString;
  serializeJson(doc, jsonString);

  if (client.publish(status_topic, jsonString.c_str())) {
    Serial.println("Status sent:");
    Serial.println(jsonString);
  } else {
    Serial.println("Failed to publish status");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                    Fungsi Publish Status ESP32 Offline()
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void publishStatusOffline() {
  DynamicJsonDocument doc(128);
  doc["status"] = "offline";
  doc["uptime"] = (millis() - startTime) / 1000;

  String jsonString;
  serializeJson(doc, jsonString);
  client.publish(status_topic, jsonString.c_str(), true);

  Serial.println("Status sent: offline");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                            Fungsi Reconnect()
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    bool connected = false;

    if (strlen(mqtt_username) > 0 && strlen(mqtt_password) > 0) {
      Serial.print(" with authentication...");
      connected = client.connect(mqtt_client_id, mqtt_username, mqtt_password);
    } else {
      Serial.print(" without authentication...");
      connected = client.connect(mqtt_client_id);
    }

    if (connected) {
      Serial.println(" connected");
      client.subscribe(led_topic);
      client.subscribe(pir_topic);
      client.subscribe(sensor_topic);
      client.subscribe(status_topic);
      client.subscribe(lampu_topic);
      client.subscribe(kipas_topic);
      client.subscribe(listrik_topic);
      publishStatusOnline();
      // Serial.println("Subscribed to: " + String(led_topic));
    } else {
      Serial.print(" failed, reconnect=");
      Serial.print(client.state());
      Serial.println(" retry in 5 seconds");

      delay(5000);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                      Fungsi Mengaktifkan Motor Stepper()
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void aktifkanMotorStepper() {
  digitalWrite(KIPAS_PIN, HIGH); // Nyalakan relay
  Serial.println("Motor Aktif");

  // Lakukan 1 putaran penuh (512 half-step untuk 28BYJ-48)
  for (int i = 0; i < 512; i++) {
    for (int step = 0; step < 8; step++) {
      digitalWrite(IN1, halfStepSequence[step][0]);
      digitalWrite(IN2, halfStepSequence[step][1]);
      digitalWrite(IN3, halfStepSequence[step][2]);
      digitalWrite(IN4, halfStepSequence[step][3]);
      delay(1); // Sesuaikan kecepatan rotasi
    }
  }

  // Matikan motor setelah putaran
  // digitalWrite(IN1, LOW);
  // digitalWrite(IN2, LOW);
  // digitalWrite(IN3, LOW);
  // digitalWrite(IN4, LOW);
  // Serial.println("Motor Selesai 1 Putaran");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                      Fungsi Mematikan Motor Stepper()
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void matikanMotorStepper() {
  digitalWrite(KIPAS_PIN, LOW); // Matikan relay
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  Serial.println("Motor OFF");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//           Fungsi SplashScreen Singkat Tampilan pada OLED Display()
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void showSplashScreen() {
  display.clearDisplay();
  
  // Header dengan border
  display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
  display.drawRect(2, 2, 124, 60, SSD1306_WHITE);
  
  display.setTextSize(2);
  display.setCursor(25, 15);
  display.print(F("SENSOR"));
  
  display.setTextSize(1);
  display.setCursor(35, 35);
  display.print(F("MONITOR"));
  
  display.setCursor(25, 50);
  display.print(F("Starting..."));
  
  display.display();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//             Fungsi Pivot Semua Tampilan pada OLED Display()
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void displayModernLayout() {
  display.clearDisplay();
  
  // Header dengan animasi dot
  drawHeader();
  
  // Section Temperature & Humidity (bersebelahan)
  drawTempHumiditySection();
  
  // Section Fire & Gas
  drawFireGasSection();
  
  // Border dan dekorasi
  drawBorders();
  
  display.display();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//          Fungsi Tulisan Header SMART HOME pada OLED Display()
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void drawHeader() {
  display.setTextSize(1);
  display.setCursor(2, 1);
  display.print(F("SMART HOME"));
  
  // Animasi loading dots
  String dots = "";
  for (int i = 0; i < (animationFrame % 4); i++) {
    dots += ".";
  }
  display.setCursor(90, 1);
  display.print(dots);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//         Fungsi Gambar Icon Suhu dan Lembap 16x16 pada OLED Display()
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void drawTempHumiditySection() {
  // === TEMPERATURE (Kiri) ===
  // Ikon temperature 16x16 (geser kebawah untuk menghindari area kuning)
  display.drawBitmap(4, 16, tempIcon_16x16, 16, 16, SSD1306_WHITE);

  // Nilai suhu dengan font besar di samping ikon
  display.setTextSize(1.5);
  display.setCursor(22, 18);
  display.print(temperature, 1);
  
  // Satuan dengan font kecil
  // display.setTextSize(1);
  // display.setCursor(22, 27);
  display.drawCircle(display.getCursorX() + 2, display.getCursorY() + 2, 1, SSD1306_WHITE);
  display.print(F(" C"));

  // === HUMIDITY (Kanan) ===
  // Ikon humidity 16x16 (geser kebawah untuk menghindari area kuning)
  display.drawBitmap(70, 16, waterIcon_16x16, 16, 16, SSD1306_WHITE);
  
  // Nilai kelembapan dengan font besar di samping ikon
  display.setTextSize(1.5);
  display.setCursor(88, 18);
  display.print(humidity, 1);
  
  // Satuan dengan font kecil
  // display.setTextSize(1);
  // display.setCursor(88, 27);
  display.print(F("%"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//            Fungsi Gambar Icon Api 16x16 pada OLED Display()
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void drawFireGasSection() {
  // Ikon api 16x16 (di area biru)
  display.drawBitmap(2, 38, fireIcon_16x16, 16, 16, SSD1306_WHITE);

  // // Status Api (dari flameLevel → intensity)
  // int fireIntensity = map(fireLevel * 10, 1000, 1800, 100, 0); // konversi 1000–1800 ke 100–0
  // fireIntensity = constrain(fireIntensity, 0, 100);
  
  // Status Api (berdasarkan firePercent)
  String fireStatus;
  if (flamePercent < 30) {
    fireStatus = "AMAN";
  } else if (flamePercent < 75) {
    fireStatus = "WASPADA";
  } else {
    fireStatus = "BAHAYA";
  }

  // Status Gas (berdasarkan gasPercent)
  String gasStatus;
  int baseline = 400; // contoh baseline, sesuaikan jika dibaca dari sensor
  if ((gasPercent * 10) > baseline + 300) {
    gasStatus = "BOCOR";
  } else {
    gasStatus = "NORMAL";
  }

  // Data Fire dan Gas
  display.setTextSize(1);
  display.setCursor(22, 40);
  display.print(F("Api: "));
  display.print(flamePercent);
  display.print(F("% "));
  display.println(fireStatus);
  // display.print(fireLevel, 1);
  // display.print(F("%"));

  display.setCursor(22, 50);
  display.print(F("Gas: "));
  display.print(gasPercent);
  display.print(F("% "));
  display.println(gasStatus);
  // display.print(gasLevel, 1);
  // display.print(F("%"));

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                   Fungsi Gambar Border pada OLED Display()
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void drawBorders() {
  // Border utama
  display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
  
  // Garis pemisah horizontal (disesuaikan dengan posisi baru)
  display.drawLine(0, 11, 127, 11, SSD1306_WHITE);
  display.drawLine(0, 35, 127, 35, SSD1306_WHITE);
  
  // Garis pemisah vertikal antara temp dan humidity
  display.drawLine(63, 12, 63, 34, SSD1306_WHITE);
  
  // Highlight corners
  display.drawPixel(1, 1, SSD1306_WHITE);
  display.drawPixel(126, 1, SSD1306_WHITE);
  display.drawPixel(1, 62, SSD1306_WHITE);
  display.drawPixel(126, 62, SSD1306_WHITE);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                              Fungsi Loop()
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void loop() {
  // Cek Koneksi ke Server MQTT
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();

  // Mengecek apakah sudah waktunya mengirim data sensor ke MQTT
  if (now - lastSensorRead > sensorInterval) {
    lastSensorRead = now;
    publishSensorData();
  }

  // Mengecek apakah sudah waktunya mengirim data status ke MQTT
  if (now - lastStatusPublish > statusInterval) {
    lastStatusPublish = now;
    publishStatusOnline();
  }

  // Mengecek apakah WiFi masih terhubung atau tidak.
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost! Reconnecting...");
    setup_wifi();
  }

  // Deteksi PIR dan Motor Stepper (Kipas Angin)
  bool gerakan = digitalRead(PIR_PIN);

  if (gerakan) {
    Serial.println("Gerakan Terdeteksi dari PIR (loop)!");
    aktifkanMotorStepper();
  } else {
    matikanMotorStepper();
  }

  // Update animasi setiap 500ms
  if (millis() - lastUpdate > 500) {
    lastUpdate = millis();
    animationFrame++;
  }

  // Tampilkan layout modern
  displayModernLayout();
  delay(100); // Delay kecil agar tidak spam dan CPU tidak 100%
}