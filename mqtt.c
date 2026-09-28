#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "common.h"
#include "player.h"
#include "rtsp.h"
#include "utilities/string_utilities.h"

#include "rtp.h"

#ifdef CONFIG_DACP_CLIENT
#include "dacp.h"
#endif

#include "metadata/core.h"
#include "metadata/hub.h"
#include "metadata/pc_queue.h"

#include "mqtt.h"

#ifdef CONFIG_MQTT
// this is for receiving metadata

pc_queue metadata_mqtt_queue;
#define metadata_mqtt_queue_size 500
metadata_package metadata_mqtt_queue_items[metadata_mqtt_queue_size];
pthread_t metadata_mqtt_thread;

// this holds the mosquitto client
struct mosquitto *global_mosq = NULL;
static int connected = 0;
static int mqtt_lib_initialised = 0;
static int metadata_mqtt_queue_initialised = 0;
static pthread_mutex_t mqtt_state_mutex = PTHREAD_MUTEX_INITIALIZER;

static void mqtt_set_connected(int state) {
  pthread_mutex_lock(&mqtt_state_mutex);
  connected = state;
  pthread_mutex_unlock(&mqtt_state_mutex);
}

static int mqtt_is_connected(void) {
  int state;
  pthread_mutex_lock(&mqtt_state_mutex);
  state = connected;
  pthread_mutex_unlock(&mqtt_state_mutex);
  return state;
}

// mosquitto logging
void _cb_log(__attribute__((unused)) struct mosquitto *mosq, __attribute__((unused)) void *userdata,
             int level, const char *str) {
  switch (level) {
  case MOSQ_LOG_DEBUG:
    debug(3, "%s", str);
    break;
  case MOSQ_LOG_INFO:
    debug(3, "%s", str);
    break;
  case MOSQ_LOG_NOTICE:
    debug(3, "%s", str);
    break;
  case MOSQ_LOG_WARNING:
    inform("%s", str);
    break;
  case MOSQ_LOG_ERR: {
    warn("MQTT: Error: %s", str);
    break;
  }
  }
}

// mosquitto message handler
void on_message(__attribute__((unused)) struct mosquitto *mosq,
                __attribute__((unused)) void *userdata, const struct mosquitto_message *msg) {

  if ((msg == NULL) || (msg->payload == NULL) || (msg->payloadlen < 0)) {
    debug(1, "[MQTT]: received invalid message payload");
    return;
  }

  size_t payload_len = (size_t)msg->payloadlen;
  if (payload_len > 4096) {
    warn("[MQTT]: received oversized command payload (%zu bytes), ignoring", payload_len);
    return;
  }

  // null-terminate the payload
  char *payload = malloc(payload_len + 1);
  if (payload == NULL) {
    warn("[MQTT]: failed to allocate buffer for incoming command");
    return;
  }
  memcpy(payload, msg->payload, payload_len);
  payload[payload_len] = 0;

  char *command = payload;
  while (*command && isspace((unsigned char)*command))
    command++;
  char *command_end = command + strlen(command);
  while ((command_end > command) && isspace((unsigned char)command_end[-1]))
    *--command_end = '\0';

  if (*command == '\0') {
    free(payload);
    return;
  }

  debug(2, "[MQTT]: received Message on topic %s: %s\n", msg->topic, command);

  // All recognized commands
  char *commands[] = {"command",    "beginff",  "beginrew",   "mutetoggle",
                      "nextitem",   "previtem", "pause",      "playpause",
                      "play",       "stop",     "playresume", "shuffle_songs",
                      "volumedown", "volumeup", "disconnect", "queue_next",
                      NULL};

  int it = 0;

  // send command if it's a valid one
  while (commands[it] != NULL) {
    if (strcmp(command, commands[it]) == 0) {
      debug(2, "[MQTT]: Received Recognized Command: %s\n", commands[it]);
      if (strcmp(commands[it], "disconnect") == 0) {
        debug(2, "[MQTT]: Disconnect Command: %s\n", commands[it]);
        stop_play(); // stop any current session and don't replace it
      } else if (strcmp(commands[it], "queue_next") == 0) {
        warn("[MQTT]: queue_next command requires a track_id argument");
      } else {
        debug(2, "[MQTT]: DACP Command: %s\n", commands[it]);
#ifdef CONFIG_DACP_CLIENT
        send_simple_dacp_command(commands[it]);
#else
        warn("[MQTT]: command \"%s\" ignored because DACP client support is disabled", commands[it]);
#endif
      }
      break;
    }
    it++;
  }

  if (strncmp(command, "queue_next", strlen("queue_next")) == 0) {
    char next = command[strlen("queue_next")];
    if ((next == ' ') || (next == '\t')) {
      // command format is "queue_next <track_id>", where <track_id> is the hex track_id
      // string as published by shairport-sync itself.
      char *track_id = command + strlen("queue_next");
      while ((*track_id == ' ') || (*track_id == '\t'))
        track_id++;
      size_t track_id_len = strlen(track_id);
      while ((track_id_len > 0) && isspace((unsigned char)track_id[track_id_len - 1]))
        track_id[--track_id_len] = '\0';
      if (track_id_len == 0) {
        warn("[MQTT]: queue_next command received with no track_id -- ignoring.");
      } else {
        char dacp_command[256];
        snprintf(dacp_command, sizeof(dacp_command),
                 "cue?command=add&query='dmap.persistentid:0x%s'&mode=3", track_id);
        debug(2, "[MQTT]: Queue Next Command: %s\n", dacp_command);
#ifdef CONFIG_DACP_CLIENT
        send_simple_dacp_command(dacp_command);
#else
        warn("[MQTT]: queue_next ignored because DACP client support is disabled");
#endif
      }
      free(payload);
      return;
    }
  }

  if (commands[it] == NULL)
    debug(2, "[MQTT]: Unrecognised command payload ignored: %s", command);

  free(payload);
}

void on_disconnect(__attribute__((unused)) struct mosquitto *mosq,
                   __attribute__((unused)) void *userdata, __attribute__((unused)) int rc) {
  mqtt_set_connected(0);
  debug(2, "[MQTT]: disconnected");
}

void on_connect(struct mosquitto *mosq, __attribute__((unused)) void *userdata,
                int rc) {
  if (rc != 0) {
    mqtt_set_connected(0);
    warn("[MQTT]: connect callback returned error code %d", rc);
    return;
  }

  mqtt_set_connected(1);
  debug(2, "[MQTT]: connected");

  // subscribe if requested
  if (config.mqtt_enable_remote && config.mqtt_topic) {
    char remotetopic[strlen(config.mqtt_topic) + 8];
    snprintf(remotetopic, strlen(config.mqtt_topic) + 8, "%s/remote", config.mqtt_topic);
    mosquitto_subscribe(mosq, NULL, remotetopic, 0);
  }

  // send autodiscovery messages if enabled
  if (config.mqtt_enable_autodiscovery && config.mqtt_publish_parsed) {
    send_autodiscovery_messages(mosq);
  }
}

// function to send autodiscovery messages for Home Assistant
void send_autodiscovery_messages(struct mosquitto *mosq) {
  if ((mosq == NULL) || (config.service_name == NULL) || (config.mqtt_topic == NULL))
    return;

  const char *device_name = config.service_name;
#ifdef CONFIG_AIRPLAY_2
  const char *device_id = config.airplay_device_id ? config.airplay_device_id : config.service_name;
#else
  const char *device_id = config.service_name;
#endif
  char *device_id_no_colons = str_replace(device_id, ":", "");
  if (device_id_no_colons == NULL) {
    warn("[MQTT]: failed to prepare autodiscovery device id");
    return;
  }
  const char *sw_version = get_version_string();
  const char *model = "shairport-sync";
  const char *model_friendly = "Shairport Sync";
  const char *manufacturer = "Mike Brady";
  const char *autodiscovery_prefix = (config.mqtt_autodiscovery_prefix != NULL)
                                         ? config.mqtt_autodiscovery_prefix
                                         : "homeassistant";

  char topic[512];
  char payload[1280];
  char device_payload[512];
  char id_string[128];

  snprintf(device_payload, sizeof(device_payload),
           "\"device\": {"
           "\"identifiers\": [\"%s\"],"
           "\"name\": \"%s\","
           "\"model\": \"%s\","
           "\"sw_version\": \"%s\","
           "\"manufacturer\": \"%s\""
           "}",
           device_id, device_name, model_friendly, sw_version, manufacturer);

  // when adding sensors here, be sure to also update sensor_names and icons below!
  const char *sensors[] = {"artist",
                           "album",
                           "title",
                           "genre",
                           "format",
                           "output_format",
                           "output_frame_rate",
                           "track_id",
                           "client_ip",
                           "client_mac_address",
                           "client_name",
                           "client_model",
                           "client_device_id",
                           "server_ip",
                           "volume",
                           "active",
                           "playing",
                           NULL};

  const char *sensor_names[] = {"Artist",
                                "Album",
                                "Title",
                                "Genre",
                                "Format",
                                "Output Format",
                                "Output Frame Rate",
                                "Track ID",
                                "Client IP",
                                "Client MAC Address",
                                "Client Name",
                                "Client Model",
                                "Client Device ID",
                                "Server IP",
                                "Volume",
                                "Active Session",
                                "Playing"};

  const char *icons[] = {
      "mdi:account-music",            // artist
      "mdi:album",                    // album
      "mdi:music",                    // title
      "mdi:music-box-multiple",       // genre
      "mdi:file",                     // format
      "mdi:file",                     // output format
      "mdi:file-chart",               // output frame rate
      "mdi:identifier",               // track ID
      "mdi:ip",                       // client IP
      "mdi:hexadecimal",              // client MAC address
      "mdi:cellphone-text",           // client name
      "mdi:cellphone-text",           // client model
      "mdi:hexadecimal",              // client device ID
      "mdi:ip-network",               // server IP
      "mdi:volume-high",              // volume
      "mdi:play-box-multiple",        // active
      "mdi:play-box-multiple-outline" // playing
  };

  for (int i = 0; sensors[i] != NULL; i++) {
    bool is_binary_sensor =
        (strcmp(sensors[i], "active") == 0 || strcmp(sensors[i], "playing") == 0);
    bool is_volume_sensor = strcmp(sensors[i], "volume") == 0;

    const char *entity_type = is_binary_sensor ? "binary_sensor" : "sensor";

    snprintf(topic, sizeof(topic), "%s/%s/%s_%s/%s/config", autodiscovery_prefix, entity_type,
             model, device_id_no_colons, sensors[i]);

    snprintf(id_string, sizeof(id_string), "%s_%s_%s", model, device_name, sensors[i]);

    snprintf(
        payload, sizeof(payload),
        "{"
        "\"name\": \"%s\","
        "\"state_topic\": \"%s/%s\","
        "\"icon\": \"%s\","
        "\"unique_id\": \"%s\","
        "\"object_id\": \"%s\","
        // As of Home Assistant 2025.10, `default_entity_id` replaces `object_id`.
        // Home Assistant 2026.4 will remove support for `object_id`,
        // so we add both for backward compatibility.
        "\"default_entity_id\": \"%s.%s\","
        "%s%s%s"
        "}",
        sensor_names[i],               // name
        config.mqtt_topic, sensors[i], // state_topic
        icons[i],                      // icon
        id_string,                     // unique_id
        id_string,                     // object_id
        entity_type, id_string,        // default_entity_id
        is_binary_sensor ? "\"payload_on\": \"1\",\"payload_off\": \"0\"," : "",
        is_volume_sensor
            ? "\"value_template\": \"{{ ((value | regex_findall_index("
              "find='^(.+?),', index=0, ignorecase=False) | float / 30 + 1) * 100) | round(0) }}\","
              "\"unit_of_measurement\": \"%\","
            : "",
        device_payload);

    mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, true);
    debug(2, "[MQTT]: published autodiscovery for %s", id_string);
  }

  free(device_id_no_colons);
}

// helper function to publish under a topic and automatically append the main topic
void mqtt_publish(char *topic, char *data_in, uint32_t length_in) {
  if ((global_mosq == NULL) || (topic == NULL) || (config.mqtt_topic == NULL))
    return;

  char *data = data_in;
  uint32_t length = length_in;

  if ((length == 0) && (config.mqtt_empty_payload_substitute != NULL)) {
    length = strlen(config.mqtt_empty_payload_substitute);
    data = config.mqtt_empty_payload_substitute;
  }

  if (length > INT_MAX) {
    warn("[MQTT]: Publish failed: payload too large");
    return;
  }

  char fulltopic[strlen(config.mqtt_topic) + strlen(topic) + 3];
  snprintf(fulltopic, strlen(config.mqtt_topic) + strlen(topic) + 2, "%s/%s", config.mqtt_topic,
           topic);
  debug(2, "[MQTT]: publishing under %s", fulltopic);

  int rc;
  if ((rc = mosquitto_publish(global_mosq, NULL, fulltopic, (int)length, data, 0,
                              config.mqtt_publish_retain)) != MOSQ_ERR_SUCCESS) {
    switch (rc) {
    case MOSQ_ERR_NO_CONN:
      debug(1, "[MQTT]: Publish failed: not connected to broker");
      break;
    default:
      debug(1, "[MQTT]: Publish failed: unknown error");
      break;
    }
  }
}

// handler for incoming metadata
void mqtt_process_metadata(uint32_t type, uint32_t code, char *data, uint32_t length) {
  if (global_mosq == NULL || mqtt_is_connected() != 1) {
    debug(3, "[MQTT]: Client not connected, skipping metadata handling");
    return;
  }
  if (config.mqtt_publish_raw) {
    uint32_t val;
    char topic[] = "____/____";

    val = htonl(type);
    memcpy(topic, &val, 4);
    val = htonl(code);
    memcpy(topic + 5, &val, 4);
    mqtt_publish(topic, data, length);
  }
  if (config.mqtt_publish_parsed) {
    if (type == 'core') {
      int32_t r;
      char trackidstring[32];

      switch (code) {
      case 'asar':
        mqtt_publish("artist", data, length);
        break;
      case 'asal':
        mqtt_publish("album", data, length);
        break;
      case 'asfm':
        mqtt_publish("format", data, length);
        break;
      case 'asgn':
        mqtt_publish("genre", data, length);
        break;
      case 'minm':
        mqtt_publish("title", data, length);
        break;
      case 'mper':
        // publish the raw persistent-id bytes as hex, in the order received --
        // no byte-order or integer conversion, so this can't reverse the value.
        r = 0;
        for (uint32_t i = 0; i < length && r < (int32_t)sizeof(trackidstring) - 2; i++)
          r += snprintf(trackidstring + r, sizeof(trackidstring) - (size_t)r, "%02X",
                        (unsigned char)data[i]);
        mqtt_publish("track_id", trackidstring, r);
      }
    } else if (type == 'ssnc') {
      switch (code) {
      case 'abeg':
        mqtt_publish("active", "1", 1);
        mqtt_publish("active_start", data, length);
        break;
      case 'acre':
        mqtt_publish("active_remote_id", data, length);
        break;
      case 'aend':
        mqtt_publish("active", "0", 1);
        mqtt_publish("active_end", data, length);
        break;
      case 'asal':
        mqtt_publish("songalbum", data, length);
        break;
      case 'asdk':
        mqtt_publish("songdatakind", data,
                     length); // 0 seem to be a timed item, 1 an untimed stream
        break;
      case 'clip':
        mqtt_publish("client_ip", data, length);
        break;
      case 'cdid':
        mqtt_publish("client_device_id", data, length);
        break;
      case 'cmac':
        mqtt_publish("client_mac_address", data, length);
        break;
      case 'cmod':
        mqtt_publish("client_model", data, length);
        break;
      case 'daid':
        mqtt_publish("dacp_id", data, length);
        break;
      case 'phbt':
        mqtt_publish("frame_position_and_time", data, length);
        break;
      case 'phb0':
        mqtt_publish("first_frame_position_and_time", data, length);
        break;
      case 'ofmt':
        mqtt_publish("output_format", data, length);
        break;
      case 'ofps':
        mqtt_publish("output_frame_rate", data, length);
        break;
      case 'pbeg':
        mqtt_publish("playing", "1", 1);
        mqtt_publish("play_start", data, length);
        break;
      case 'pend':
        mqtt_publish("playing", "0", 1);
        mqtt_publish("play_end", data, length);
        break;
      case 'pfls':
        mqtt_publish("play_flush", data, length);
        break;
      case 'PICT':
        if (config.mqtt_publish_cover) {
          mqtt_publish("cover", data, length);
        }
        break;
      case 'prsm':
        mqtt_publish("playing", "1", 1);
        mqtt_publish("play_resume", data, length);
        break;
      case 'pvol':
        mqtt_publish("volume", data, length);
        break;
      case 'snam':
        mqtt_publish("client_name", data, length);
        break;
      case 'styp':
        mqtt_publish("stream_type", data, length);
        break;
      case 'svip':
        mqtt_publish("server_ip", data, length);
        break;
      case 'svna':
        mqtt_publish("service_name", data, length);
        break;
      }
    }
  }

  return;
}

int initialise_mqtt() {
  debug(1, "Initialising MQTT");
  if (config.mqtt_hostname == NULL) {
    debug(1, "[MQTT]: Not initialized, as the hostname is not set");
    return 0;
  }

  if (global_mosq != NULL)
    metadata_mqtt_close();

  int keepalive = 60;
  if (mqtt_lib_initialised == 0) {
    mosquitto_lib_init();
    mqtt_lib_initialised = 1;
  }

  if (!(global_mosq = mosquitto_new(config.service_name, true, NULL))) {
    warn("[MQTT]: Could not create mosquitto object!");
    return -1;
  }

  if (config.mqtt_cafile != NULL || config.mqtt_capath != NULL || config.mqtt_certfile != NULL ||
      config.mqtt_keyfile != NULL) {
    if (mosquitto_tls_set(global_mosq, config.mqtt_cafile, config.mqtt_capath, config.mqtt_certfile,
                          config.mqtt_keyfile, NULL) != MOSQ_ERR_SUCCESS) {
      warn("[MQTT]: TLS setup failed");
      metadata_mqtt_close();
      return -1;
    }
  }

  if (config.mqtt_username != NULL || config.mqtt_password != NULL) {
    if (mosquitto_username_pw_set(global_mosq, config.mqtt_username, config.mqtt_password) !=
        MOSQ_ERR_SUCCESS) {
      warn("[MQTT]: Username/password setup failed");
      metadata_mqtt_close();
      return -1;
    }
  }
  mosquitto_log_callback_set(global_mosq, _cb_log);

  if (config.mqtt_enable_remote) {
    mosquitto_message_callback_set(global_mosq, on_message);
  }

  mosquitto_disconnect_callback_set(global_mosq, on_disconnect);
  mosquitto_connect_callback_set(global_mosq, on_connect);
  if (mosquitto_connect(global_mosq, config.mqtt_hostname, config.mqtt_port, keepalive)) {
    inform("[MQTT]: Could not establish a mqtt connection");
    mqtt_set_connected(0);
  }
  if (mosquitto_loop_start(global_mosq) != MOSQ_ERR_SUCCESS) {
    inform("[MQTT]: Could not start MQTT main loop");
    metadata_mqtt_close();
    return -1;
  }

  return 0;
}

// metadata handling stuff
void metadata_mqtt_close(void) {
  mqtt_set_connected(0);
  if (global_mosq) {
    mosquitto_disconnect(global_mosq);
    mosquitto_loop_stop(global_mosq, true);
    mosquitto_destroy(global_mosq);
    global_mosq = NULL;
  }
}

void metadata_mqtt_thread_cleanup_function(__attribute__((unused)) void *arg) {
  // debug(2, "metadata_mqtt_thread_cleanup_function called");
  metadata_mqtt_close();
  // debug(2, "metadata_mqtt_thread_cleanup_function done");
}

void *metadata_mqtt_thread_function(__attribute__((unused)) void *ignore) {
  //  #include <syscall.h>
  //  debug(1, "metadata_mqtt_thread_function PID %d", syscall(SYS_gettid));
  metadata_package pack;
  pthread_cleanup_push(metadata_mqtt_thread_cleanup_function, NULL);
  while (1) {
    pc_queue_get_item(&metadata_mqtt_queue, &pack);
    pthread_cleanup_push(metadata_pack_cleanup_function, (void *)&pack);
    if (config.mqtt_enabled) {
      if (pack.carrier) {
        debug(3,
              "                                        mqtt: type %x, code %x, length %u, message "
              "%d.",
              pack.type, pack.code, pack.length, pack.carrier->index_number);
      } else {
        debug(3, "                                        mqtt: type %x, code %x, length %u.",
              pack.type, pack.code, pack.length);
      }
      mqtt_process_metadata(pack.type, pack.code, pack.data, pack.length);
      debug(3, "                                        mqtt: done.");
    }

    pthread_cleanup_pop(1);
  }
  pthread_cleanup_pop(1); // will never happen
  pthread_exit(NULL);
}

void metadata_mqtt_queue_init() {
  if (metadata_mqtt_queue_initialised)
    return;

  // create a pc_queue for the MQTT handler
  pc_queue_init(&metadata_mqtt_queue, (char *)&metadata_mqtt_queue_items, sizeof(metadata_package),
                metadata_mqtt_queue_size, "mqtt");
  metadata_mqtt_queue_initialised = 1;
  if (named_pthread_create(&metadata_mqtt_thread, NULL, metadata_mqtt_thread_function, NULL,
                           "metadata mqtt") != 0) {
    metadata_mqtt_thread = 0;
    pc_queue_delete(&metadata_mqtt_queue);
    metadata_mqtt_queue_initialised = 0;
    debug(1, "Failed to create metadata mqtt thread!");
  }
}
void metadata_mqtt_queue_stop() {
  // debug(2, "metadata stop mqtt thread.");
  if (metadata_mqtt_thread) {
    pthread_cancel(metadata_mqtt_thread);
    pthread_join(metadata_mqtt_thread, NULL);
    metadata_mqtt_thread = 0;
  }

  if (metadata_mqtt_queue_initialised) {
    pc_queue_delete(&metadata_mqtt_queue);
    metadata_mqtt_queue_initialised = 0;
  }

  metadata_mqtt_close();
  // debug(2, "metadata stop mqtt done.");
}
int send_metadata_to_mqtt_queue(const uint32_t type, const uint32_t code, const char *data,
                                const uint32_t length, rtsp_message *carrier, int block) {
  return send_metadata_to_queue(&metadata_mqtt_queue, type, code, data, length, carrier, block);
}

#else

int initialise_mqtt() { return 0; }

void mqtt_process_metadata(__attribute__((unused)) uint32_t type,
                           __attribute__((unused)) uint32_t code,
                           __attribute__((unused)) char *data,
                           __attribute__((unused)) uint32_t length) {}

void mqtt_publish(__attribute__((unused)) char *topic, __attribute__((unused)) char *data_in,
                  __attribute__((unused)) uint32_t length_in) {}

void send_autodiscovery_messages(__attribute__((unused)) struct mosquitto *mosq) {}

void on_connect(__attribute__((unused)) struct mosquitto *mosq,
                __attribute__((unused)) void *userdata, __attribute__((unused)) int rc) {}

void on_disconnect(__attribute__((unused)) struct mosquitto *mosq,
                   __attribute__((unused)) void *userdata, __attribute__((unused)) int rc) {}

void on_message(__attribute__((unused)) struct mosquitto *mosq,
                __attribute__((unused)) void *userdata,
                __attribute__((unused)) const struct mosquitto_message *msg) {}

void _cb_log(__attribute__((unused)) struct mosquitto *mosq,
             __attribute__((unused)) void *userdata, __attribute__((unused)) int level,
             __attribute__((unused)) const char *str) {}

void metadata_mqtt_queue_init() {}

void metadata_mqtt_queue_stop() {}

int send_metadata_to_mqtt_queue(__attribute__((unused)) const uint32_t type,
                                __attribute__((unused)) const uint32_t code,
                                __attribute__((unused)) const char *data,
                                __attribute__((unused)) const uint32_t length,
                                __attribute__((unused)) rtsp_message *carrier,
                                __attribute__((unused)) int block) {
  return 0;
}

#endif
