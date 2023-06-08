#define SSID ""
#define WIFI_PWD ""
#define MQTT_HOST ""
#define MQTT_PORT 1883
#define MQTT_USER ""
#define MQTT_PWD ""
#define MQTT_CLIENT_NAME "fridgecontroller"

#define PUBLISH_STATUS_DELAY 5000
#define READ_TEMP_DELAY 1000
#define ONE_WIRE_BUS_PIN D4
#define RELAY_PIN D2
#define TARGET_TEMPERATURE_RESTART_OFFSET 1

#define TOPIC_CONFIG "fridgecontroller/config";
#define TOPIC_STATUS_PUB "fridgecontroller/status";