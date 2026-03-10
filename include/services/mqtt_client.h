#ifndef S3_WIFI_SERVICES_MQTT_CLIENT_H_
#define S3_WIFI_SERVICES_MQTT_CLIENT_H_

#include <Arduino.h>

typedef void (*MqttCommandHandler)(const String& message);

void mqtt_client_init(const char* ssid, const char* password, const char* host, uint16_t port, MqttCommandHandler handler);
void mqtt_client_ensure_connected();
void mqtt_client_loop();
bool mqtt_client_publish_data(const String& payload);

#endif
