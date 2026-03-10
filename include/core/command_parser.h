#ifndef S3_WIFI_CORE_COMMAND_PARSER_H_
#define S3_WIFI_CORE_COMMAND_PARSER_H_

#include <Arduino.h>

enum ParsedCommandType {
  PARSED_NONE,
  PARSED_SERIAL_ADD,
  PARSED_SERIAL_START,
  PARSED_MQTT_ID_0,
  PARSED_MQTT_ID_1,
  PARSED_MQTT_ID_2,
  PARSED_MQTT_ID_3,
  PARSED_MQTT_ID_4
};

struct ParsedCommand {
  ParsedCommandType type;
  uint32_t timeValue;
  String textId;
  uint8_t channels[4];
  uint32_t times[4];
};

bool command_parser_parse_serial(const String& input, ParsedCommand& out_cmd);
bool command_parser_parse_mqtt(const String& message, ParsedCommand& out_cmd);

#endif
