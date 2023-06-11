#define WIFI_SSID ""
#define WIFI_PWD ""
#define MQTT_HOST ""
#define MQTT_PORT 1883
#define MQTT_USER ""
#define MQTT_PWD ""
#define MQTT_CLIENT_NAME "fridgecontroller"

//Intervals in milliseconds
#define PUBLISH_STATUS_INTERVAL 5000
#define READ_TEMP_INTERVAL 5000
#define MAX_COOLING_INTERVAL 15 * 60 * 1000
#define COOLING_INTERVAL_RESUME_DELAY 15 * 60 * 1000
#define WIFI_CONNECTION_RETRY_TIMEOUT 5 * 1000
#define MQTT_CONNECTION_RETRY_TIMEOUT 2 * 1000

#define TEMP_SENSOR_PIN D4
#define RELAY_PIN D2

#define TOPIC_CONFIG "fridgecontroller/config";
#define TOPIC_STATUS_PUB "fridgecontroller/status";