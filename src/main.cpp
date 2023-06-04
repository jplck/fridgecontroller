#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <MQTT.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define PUBLISH_STATUS_DELAY 5000
#define READ_TEMP_DELAY 1000
#define ONE_WIRE_BUS_PIN D4
#define RELAY_PIN 2
#define TARGET_TEMPERATURE_RESTART_OFFSET 1

const char * SSID = "YOUR-SSID";
const char* WIFI_PWD = "YOUR-PWD";
const char* MQTT_HOST = "YOUR-MQTT-HOST";
const int MQTT_PORT = 1883;
const char* MQTT_USER = "mqtt";
const char* MQTT_PWD = "YOUR-PWD";
const char* MQTT_CLIENT_NAME = "consumptioncontroller";

const char* TOPIC_CONFIG = "fridgecontroller/config";
const char* TOPIC_STATUS_PUB = "fridgecontroller/status";

WiFiClient wifi;
MQTTClient mqtt;
OneWire oneWire(ONE_WIRE_BUS_PIN);
DallasTemperature sensors(&oneWire);

long lastPublishTimer = 0;
long lastReadTempTimer = 0;

void connect_wifi();
void mqtt_callback(String &topic, String &payload);
void connect_mqtt();
void create_subscriptions(const char * topic);
void publish_status();
void read_temp_sensors();
void set_cooling_state(bool enable);
void enable_cooling_if_required();

float currentTemperature = 30.0f;
float targetTemperature = 6.0f;
bool coolingState = false;

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  Serial.begin(115200);
  sensors.begin();
}

void loop() {
  if (!wifi.connected()) {
    connect_wifi();
    delay(1000);
  }
  if (!mqtt.connected()) {
    connect_mqtt();
    delay(1000);
  }

  read_temp_sensors();
  publish_status();
  enable_cooling_if_required();
  
  mqtt.loop();
}

void enable_cooling_if_required() {
  if (currentTemperature >= targetTemperature + TARGET_TEMPERATURE_RESTART_OFFSET && !coolingState) {
    set_cooling_state(true);
  } else if (currentTemperature <= targetTemperature && coolingState) {
    set_cooling_state(false);
  }
}

void set_cooling_state(bool enable) {
  digitalWrite(RELAY_PIN, enable ? HIGH : LOW);
  coolingState = enable;
}

void read_temp_sensors() {
  if (millis()-lastReadTempTimer >= READ_TEMP_DELAY) {
    sensors.requestTemperatures();
    float tempS0 = sensors.getTempCByIndex(0);
    Serial.print("Celsius temperature: ");
    Serial.println(tempS0);

    currentTemperature = tempS0;

    lastReadTempTimer = millis();
  }
}

void publish_status() {
  if (millis()-lastPublishTimer >= PUBLISH_STATUS_DELAY) {
    DynamicJsonDocument doc(1024);
    doc["current_temperature"] = currentTemperature;
    doc["cooling_active"] = coolingState;
    String payload;
    serializeJson(doc, payload);
    mqtt.publish(TOPIC_STATUS_PUB, payload);
    lastPublishTimer = millis();
  }
}

void connect_wifi() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(SSID);
  WiFi.begin(SSID, WIFI_PWD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void connect_mqtt() {  
  mqtt.begin(MQTT_HOST, wifi);
  mqtt.onMessage(mqtt_callback);
  while (!mqtt.connect(MQTT_HOST, MQTT_USER, MQTT_PWD)) {
    Serial.print("*");
  }
  Serial.println("MQTT connected!");

  create_subscriptions(TOPIC_CONFIG);

  delay(500);
}

void mqtt_callback(String &topic, String &payload) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  Serial.println(payload);

  if (String(topic) == TOPIC_CONFIG) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    targetTemperature = doc["target_temp"];
  } 

}

void create_subscriptions(const char * topic) {
  mqtt.subscribe(topic);
}