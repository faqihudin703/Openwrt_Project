#include <WiFi.h>
#include <MQTT.h>
#include "DHT.h"
#include <MQ135.h>  // Include library MQ135
#include <NTPClient.h>
#include <WiFiUdp.h>

#define DHTPIN1 14 // pin untuk Sensor DHT22 pertama
#define DHTPIN2 4 // pin untuk Sensor DHT22 kedua
#define DHTTYPE DHT22
#define PIN_MQ135 35 // Pin analog untuk Sensor MQ-135

//Konfigurasi Wifi
const char* ssid = "nama-wifi";
const char* password = "password-wifi";

//Konfigurasi MQTT
const char* mqtt_server = "<ip-mqtt>"; // Ganti dengan IP MQTT broker
const int mqtt_port = 1883;
const char* mqtt_topic_dht22 = "sensor/dht22";
const char* mqtt_topic_mq135 = "sensor/mq135";
const char* mqtt_topic_time = "sensor/time";
const char* mqtt_user = "username-mqtt";   // Ganti dengan username MQTT Anda
const char* mqtt_password = "password-mqtt"; // Ganti dengan password MQTT Anda

// Daftar NTP Server
const char* ntpServers[] = {
  "0.id.pool.ntp.org",  // Server utama
  "1.id.pool.ntp.org",  // Server cadangan 1
  "2.id.pool.ntp.org",  // Server cadangan 2
  "time.google.com",    // Server alternatif
  "pool.ntp.org"        // Server global
};
const int numServers = sizeof(ntpServers) / sizeof(ntpServers[0]);

// Inisialisasi objek
WiFiClient net;
MQTTClient client;
DHT dht1(DHTPIN1, DHTTYPE); // Sensor DHT pertama
DHT dht2(DHTPIN2, DHTTYPE); // Sensor DHT kedua
MQ135 mq135_sensor(PIN_MQ135);  // Inisialisasi MQ135

// NTP Client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServers[0], 7 * 3600, 60000);  // menyesuaikan waktu lokal (GMT+7)

void connectWiFi() {
  Serial.print("Menghubungkan ke WiFi...");
  WiFi.begin(ssid, password);
  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    attempt++;
    if (attempt > 30) { // Timeout setelah 15 detik
      Serial.println("\nGagal terhubung ke WiFi! Periksa kredensial.");
      return;
    }
  }
  Serial.println("\nWiFi Terhubung!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void connectMQTT() {
  Serial.print("Menghubungkan ke MQTT broker...");
  int attempt = 0;
  while (!client.connect("ESP32Client", mqtt_user, mqtt_password)) {
    Serial.print(".");
    Serial.println(client.lastError()); // Debugging error MQTT
    attempt++;
    if (attempt > 10) { // Timeout setelah 10 percobaan
      Serial.println("\nGagal terhubung ke MQTT broker! Periksa kredensial.");
      return;
    }
    delay(5000);
  }
  Serial.println("\nTerhubung ke MQTT broker!");
}

// Fungsi untuk mencoba sinkronisasi waktu dengan beberapa NTP server
void syncTime() {
  for (int i = 0; i < numServers; i++) {
    Serial.print("🔄 Mencoba sinkronisasi dengan ");
    Serial.println(ntpServers[i]);

    timeClient.end();  // Hentikan NTP client saat ini
    timeClient = NTPClient(ntpUDP, ntpServers[i], 7 * 3600, 60000);
    timeClient.begin();
    timeClient.update();

    if (timeClient.getEpochTime() > 1700000000) {  // Validasi waktu (harus lebih dari tahun 2024)
      Serial.println("✅ Berhasil sinkronisasi!");
      return;
    }
  }

  Serial.println("🚨 Gagal sinkronisasi dengan semua server! Waktu mungkin tidak akurat.");
}

void setup() {
  Serial.begin(115200);
  dht1.begin();  // Inisialisasi sensor DHT pertama
  dht2.begin();  // Inisialisasi sensor DHT kedua
  connectWiFi();
  
  // Konfigurasi MQTT
  client.begin(mqtt_server, mqtt_port, net);
  connectMQTT();
}

void loop() {
  // Periksa koneksi MQTT
  if (!client.connected()) {
    connectMQTT();
  }
  client.loop();

  timeClient.update();  // Perbarui waktu setiap loop

  unsigned long currentEpoch = timeClient.getEpochTime();  // Ambil waktu real

  Serial.print("⏳ Waktu Epoch (Unix Timestamp): ");
  Serial.println(currentEpoch);

  Serial.print("📅 Waktu Lokal (Indonesia): ");
  Serial.println(timeClient.getFormattedTime());  // Format HH:MM:SS

  // Ambil waktu real-time dalam format hh:mm:ss
  String realTime = timeClient.getFormattedTime();  // Contoh output: "14:25:30"

  // Kirim timestamp dan real-time ke MQTT
  String payload = String("{\"timestamp\":") + currentEpoch + 
                   ",\"real_time\":" + realTime +"}";
  client.publish(mqtt_topic_time, payload);
  Serial.println("📤 Data Timestamp dan Real-Time dikirim ke MQTT: " + payload);

  // Membaca data dari sensor DHT pertama
  Serial.println("📡 Membaca data sensor DHT pertama...");
  float temperature1 = dht1.readTemperature();
  float humidity1 = dht1.readHumidity();

  // Membaca data dari sensor DHT kedua
  Serial.println("📡 Membaca data sensor DHT kedua...");
  float temperature2 = dht2.readTemperature();
  float humidity2 = dht2.readHumidity();

  // Memeriksa apakah pembacaan data dari sensor gagal
  if (isnan(temperature1) || isnan(humidity1)) {
    Serial.println("❌ ERROR:Gagal membaca data dari sensor DHT pertama!");
  } else {
    Serial.print("Sensor DHT pertama - Suhu: ");
    Serial.print(temperature1);
    Serial.print(" °C  Kelembapan: ");
    Serial.print(humidity1);
    Serial.println(" %");
  }

  if (isnan(temperature2) || isnan(humidity2)) {
    Serial.println("❌ ERROR:Gagal membaca data dari sensor DHT kedua!");
  } else {
    Serial.print("Sensor DHT kedua - Suhu: ");
    Serial.print(temperature2);
    Serial.print(" °C  Kelembapan: ");
    Serial.print(humidity2);
    Serial.println(" %");
  }

  // Membaca sensor MQ-135 menggunakan library MQ135
  Serial.println("📡 Membaca data sensor MQ-135 Analog...");
  float rzero = mq135_sensor.getRZero();
  float correctedRZero1 = mq135_sensor.getCorrectedRZero(temperature1, humidity1);  // DHT1
  float correctedRZero2 = mq135_sensor.getCorrectedRZero(temperature2, humidity2);  // DHT2
  float resistance = mq135_sensor.getResistance();
  float ppm = mq135_sensor.getPPM();
  float correctedPPM1 = mq135_sensor.getCorrectedPPM(temperature1, humidity1);  // DHT1
  float correctedPPM2 = mq135_sensor.getCorrectedPPM(temperature2, humidity2);  // DHT2

  // Memeriksa apakah data sensor MQ135 DHT22 pertama valid
  if (isnan(rzero) || isinf(rzero) || isnan(correctedRZero1) || isinf(correctedRZero1) ||
    isnan(resistance) || isinf(resistance) || isnan(ppm) || isinf(ppm) ||
    isnan(correctedPPM1) || isinf(correctedPPM1)) {
    Serial.println("❌ ERROR: Pembacaan data dari sensor MQ135 DHT22 pertama gagal!");
  } else {
    Serial.print("MQ135 RZero: ");
    Serial.print(rzero);
    Serial.print("\t Corrected RZero 1: ");
    Serial.print(correctedRZero1);
    Serial.print("\t Resistance: ");
    Serial.print(resistance);
    Serial.print("\t PPM: ");
    Serial.print(ppm);
    Serial.print("ppm");
    Serial.print("\t Corrected PPM 1: ");
    Serial.print(correctedPPM1);
    Serial.println("ppm");
  }

  // Memeriksa apakah data sensor MQ135 DHT22 kedua valid
  if (isnan(rzero) || isinf(rzero) || isnan(correctedRZero2) || isinf(correctedRZero2) ||
    isnan(resistance) || isinf(resistance) || isnan(ppm) || isinf(ppm) ||
    isnan(correctedPPM2) || isinf(correctedPPM2)) {
    Serial.println("❌ ERROR: Pembacaan data dari sensor MQ135 DHT22 kedua gagal!");
  } else {
    Serial.print("MQ135 RZero: ");
    Serial.print(rzero);
    Serial.print("\t Corrected RZero 2: ");
    Serial.print(correctedRZero2);
    Serial.print("\t Resistance: ");
    Serial.print(resistance);
    Serial.print("\t PPM: ");
    Serial.print(ppm);
    Serial.print("ppm");
    Serial.print("\t Corrected PPM 2: ");
    Serial.print(correctedPPM2);
    Serial.println("ppm");
  }

  // Kirim data sensor pertama ke MQTT
  if (!isnan(temperature1) && !isnan(humidity1)) {
    String payload1 = String("{\"sensor1_temperature\":") + temperature1 +
                      ",\"sensor1_humidity\":" + humidity1 + "}";
    client.publish(mqtt_topic_dht22, payload1);
    Serial.println("📤 Data sensor pertama dikirim: " + payload1);
  } else {
    Serial.println("⏳ Data sensor DHT22 pertama tidak dikirim karena gagal membaca.");
  }

  // Kirim data sensor kedua ke MQTT
  if (!isnan(temperature2) && !isnan(humidity2)) {
    String payload2 = String("{\"sensor2_temperature\":") + temperature2 +
                      ",\"sensor2_humidity\":" + humidity2 + "}";
    client.publish(mqtt_topic_dht22, payload2);
    Serial.println("📤 Data sensor kedua dikirim: " + payload2);
  } else {
    Serial.println("⏳ Data sensor DHT22 kedua tidak dikirim karena gagal membaca.");
  }

  // Kirim data MQ-135 DHT22 pertama ke MQTT
  if (!isnan(rzero) && !isinf(rzero) && !isnan(correctedRZero1) && !isinf(correctedRZero1) &&
    !isnan(resistance) && !isinf(resistance) && !isnan(ppm) && !isinf(ppm) &&
    !isnan(correctedPPM1) && !isinf(correctedPPM1)) {
    String payload3 = String("{\"mq135_rzero\":") + rzero +
                      ",\"mq135_corrected_rzero_1\":" + correctedRZero1 +
                      ",\"mq135_resistance\":" + resistance +
                      ",\"mq135_ppm\":" + ppm +
                      ",\"mq135_corrected_ppm_1\":" + correctedPPM1 + "}";
    client.publish(mqtt_topic_mq135, payload3);
    Serial.println("📤 Data sensor MQ135 DHT22 pertama dikirim: " + payload3);
  } else {
    Serial.println("⏳ Data sensor MMQ135 DHT22 pertama tidak dikirim karena gagal membaca.");
  }

  // Kirim data MQ-135 DHT22 kedua ke MQTT
  if (!isnan(rzero) && !isinf(rzero) && !isnan(correctedRZero2) && !isinf(correctedRZero2) &&
    !isnan(resistance) && !isinf(resistance) && !isnan(ppm) && !isinf(ppm) &&
    !isnan(correctedPPM2) && !isinf(correctedPPM2)) {
    String payload4 = String("{\"mq135_rzero\":") + rzero +
                      ",\"mq135_corrected_rzero_2\":" + correctedRZero2 +
                      ",\"mq135_resistance\":" + resistance +
                      ",\"mq135_ppm\":" + ppm +
                      ",\"mq135_corrected_ppm_2\":" + correctedPPM2 + "}";
    client.publish(mqtt_topic_mq135, payload4);
    Serial.println("📤 Data sensor MQ135 DHT22 kedua dikirim: " + payload4);
  } else {
    Serial.println("⏳ Data sensor MQ-135 DHT22 kedua tidak dikirim karena gagal membaca.");
  }
  
  delay(5000); // Kirim data setiap 5 detik
  }