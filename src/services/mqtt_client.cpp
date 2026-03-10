#include "services/mqtt_client.h"

#include <WiFi.h>
#include <PubSubClient.h>

static WiFiClient s_esp_client;
static PubSubClient s_client(s_esp_client);
static MqttCommandHandler s_command_handler = nullptr;

static void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) message += (char)payload[i];
  if (strcmp(topic, "/command") == 0 && s_command_handler != nullptr) {
    s_command_handler(message);
  }
}

void mqtt_client_init(const char* ssid, const char* password, const char* host, uint16_t port, MqttCommandHandler handler) {
  s_command_handler = handler;

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".....");
    delay(500);
  }
  Serial.println("WiFi connected");

  s_client.setServer(host, port);
  s_client.setCallback(mqtt_callback);
}

void mqtt_client_ensure_connected() {
  if (!s_client.connected()) {
    if (s_client.connect("ESP_MQTT", "ESP_MQTT", "ESP_MQTT")) {
      s_client.subscribe("/command");
    }
  }
}

void mqtt_client_loop() {
  s_client.loop();
}

bool mqtt_client_publish_data(const String& payload) {
  return s_client.publish("/data", payload.c_str());
}
