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

OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature sensors(&oneWire);

long lastPublishTimer = 0;
long lastReadTempTimer = 0;
long coolingRunTimer = 0;
long coolingRestartTimer = 0;

void connect_wifi(const char * ssid, const char * wifi_pwd);
void mqtt_subscription_callback(String &topic, String &payload);
void connect_mqtt(const char * host, const char * user, const char * pwd);
void create_subscriptions(const char * topic);
void publish_status();
void read_temp_sensors();
void set_cooling_state(bool enable);
void verify_temperature();
int get_cooling_state();
bool calculate_cooling_state();

float currentTemperature = 30.0f;
float targetTemperature = 6.0f;

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  Serial.begin(115200);
  sensors.begin();
}

void loop() {

  if (millis()-lastReadTempTimer >= READ_TEMP_INTERVAL) {
    read_temp_sensors();
    lastReadTempTimer = millis();
    verify_temperature();
    publish_status();
  }

  if (!wifi.connected()) {
    connect_wifi(WIFI_SSID, WIFI_PWD);
    delay(250);
  }
  if (!mqtt.connected()) {
    connect_mqtt(MQTT_HOST, MQTT_USER, MQTT_PWD);
    mqtt.subscribe(TOPIC_CONFIG);
    delay(250);
  } else {
    mqtt.loop();
  }
}

void connect_wifi(const char * ssid, const char * wifi_pwd) {
  serial_printf("Connecting to Wifi with SSID: %c\n", ssid);

  int increment_delay = 500;
  int max_increments = WIFI_CONNECTION_RETRY_TIMEOUT / increment_delay;

  WiFi.begin(ssid, wifi_pwd);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    if (max_increments-- == 0) {
      Serial.println("Unable to create wifi connection. Retry on next iteration");
      return;
    }
    delay(increment_delay);
  }

  serial_printf("Wifi connected| IP: %s\n", WiFi.localIP());
}

void connect_mqtt(const char * host, const char * user, const char * pwd) {  
  mqtt.begin(host, wifi);
  mqtt.onMessage(mqtt_subscription_callback);

  int increment_delay = 500;
  int max_increments = MQTT_CONNECTION_RETRY_TIMEOUT / increment_delay;

  while (!mqtt.connect(host, user, pwd)) {
    Serial.print("*");
    if (max_increments-- == 0) {
      Serial.println("Unable to create mqtt connection. Retry on next iteration");
      return;
    }
    delay(increment_delay);
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

void verify_temperature() {
  if (currentTemperature >= targetTemperature && !get_cooling_state()) {
    set_cooling_state(calculate_cooling_state());
    return;
  } 

  set_cooling_state(false);
}

int get_cooling_state() {
  return digitalRead(RELAY_PIN) == HIGH ? 1 : 0;
}

bool calculate_cooling_state() {

  if (coolingRunTimer == 0 && coolingRestartTimer == 0) {
    return true;
  }

  if ((millis() - coolingRunTimer) >= MAX_COOLING_INTERVAL && coolingRunTimer != 0) {
    return false;
  }

  if ((millis() - coolingRestartTimer) >= COOLING_INTERVAL_RESUME_DELAY && coolingRestartTimer != 0) {
    return true;
  }

  return false;
}

void set_cooling_state(bool enable) {
  bool currentState = get_cooling_state();
  if (enable && !currentState) {
    coolingRunTimer = millis();
    coolingRestartTimer = 0;
    digitalWrite(RELAY_PIN, HIGH);
  } else if (!enable && currentState) {
    coolingRunTimer = 0;
    coolingRestartTimer = millis();
    digitalWrite(RELAY_PIN, LOW);
  }
}

void read_temp_sensors() {
  sensors.requestTemperatures();
  float tempS0 = sensors.getTempCByIndex(0);
    
  serial_printf("Temperature: %fC\n", tempS0);

  currentTemperature = tempS0;
}

void publish_status() {
  if (millis()-lastPublishTimer >= PUBLISH_STATUS_INTERVAL) {
    DynamicJsonDocument doc(1024);
    doc["current_temperature"] = currentTemperature;
    doc["cooling_active"] = get_cooling_state();
    String payload;
    serializeJson(doc, payload);
    mqtt.publish(TOPIC_STATUS_PUB, payload);
    lastPublishTimer = millis();
  }
}