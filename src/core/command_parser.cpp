#include "core/command_parser.h"

#include <ArduinoJson.h>

static void reset_out_cmd(ParsedCommand& out_cmd) {
  out_cmd.type = PARSED_NONE;
  out_cmd.timeValue = 0;
  out_cmd.textId = "";
  for (int i = 0; i < 4; i++) {
    out_cmd.channels[i] = 0;
    out_cmd.times[i] = 0;
  }
}

bool command_parser_parse_serial(const String& input, ParsedCommand& out_cmd) {
  reset_out_cmd(out_cmd);

  if (input.startsWith("command:add")) {
    int time_idx = input.indexOf("time:");
    int id_idx = input.indexOf("id:");
    if (time_idx > 0 && id_idx > 0) {
      int comma_after_time = input.indexOf(',', time_idx);
      if (comma_after_time == -1) comma_after_time = input.length();
      String time_str = input.substring(time_idx + 5, comma_after_time);
      out_cmd.timeValue = time_str.toInt();

      String id_str = input.substring(id_idx + 3);
      int comma_after_id = id_str.indexOf(',');
      if (comma_after_id != -1) id_str = id_str.substring(0, comma_after_id);
      out_cmd.textId = id_str;

      out_cmd.type = PARSED_SERIAL_ADD;
      return true;
    }
    return false;
  }

  if (input.startsWith("command:start")) {
    int id_idx = input.indexOf("id:");
    if (id_idx > 0) {
      out_cmd.textId = input.substring(id_idx + 3);
      out_cmd.type = PARSED_SERIAL_START;
      return true;
    }
    return false;
  }

  return false;
}

bool command_parser_parse_mqtt(const String& message, ParsedCommand& out_cmd) {
  reset_out_cmd(out_cmd);

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, message);
  if (error) return false;

  const char* id_str = doc["id"];
  if (id_str == nullptr) return false;

  if (strcmp(id_str, "0") == 0) {
    out_cmd.type = PARSED_MQTT_ID_0;
    return true;
  }
  if (strcmp(id_str, "2") == 0) {
    out_cmd.type = PARSED_MQTT_ID_2;
    return true;
  }
  if (strcmp(id_str, "3") == 0) {
    out_cmd.type = PARSED_MQTT_ID_3;
    return true;
  }
  if (strcmp(id_str, "4") == 0) {
    out_cmd.type = PARSED_MQTT_ID_4;
    return true;
  }
  if (strcmp(id_str, "1") == 0) {
    JsonArray channel = doc["channel"];
    JsonArray times = doc["time"];
    if (channel.size() == 4 && times.size() == 4) {
      for (int i = 0; i < 4; i++) {
        out_cmd.channels[i] = channel[i];
        out_cmd.times[i] = times[i];
      }
      out_cmd.type = PARSED_MQTT_ID_1;
      return true;
    }
  }

  return false;
}
