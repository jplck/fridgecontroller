#include <Arduino.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "conf.h"
#include "libs/helpers.h"
#include <ESP8266WiFi.h>
#include <MQTT.h>

WiFiClient wifi;
MQTTClient mqtt;

OneWire oneWire(ONE_WIRE_BUS_PIN);
DallasTemperature sensors(&oneWire);

long lastPublishTimer = 0;
long lastReadTempTimer = 0;

void connect_wifi(const char * ssid, const char * wifi_pwd);
void mqtt_subscription_callback(String &topic, String &payload);
void connect_mqtt(const char * host, const char * user, const char * pwd);
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
    connect_wifi(WIFI_SSID, WIFI_PWD);
    delay(1000);
  }
  if (!mqtt.connected()) {
    connect_mqtt(MQTT_HOST, MQTT_USER, MQTT_PWD);
    mqtt.subscribe(TOPIC_CONFIG);
    delay(1000);
  }

  read_temp_sensors();
  publish_status();
  enable_cooling_if_required();
  
  mqtt.loop();
}

void connect_wifi(const char * ssid, const char * wifi_pwd) {
  serial_printf("Connecting to Wifi with SSID: %c\n", ssid);

  WiFi.begin(ssid, wifi_pwd);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  serial_printf("Wifi connected| IP: %s\n", WiFi.localIP());
}

void connect_mqtt(const char * host, const char * user, const char * pwd) {  
  mqtt.begin(host, wifi);
  mqtt.onMessage(mqtt_subscription_callback);
  while (!mqtt.connect(host, user, pwd)) {
    Serial.print("*");
  }
  Serial.println("MQTT connected!");
}

void mqtt_subscription_callback(String &topic, String &payload) {
  serial_printf("Message arrived on topic: %s with payload: %s\n", topic, payload);

  if (String(topic) == TOPIC_CONFIG) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    targetTemperature = doc["target_temp"];
  } 
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
    
    serial_printf("Temperature: %fC\n", tempS0);

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