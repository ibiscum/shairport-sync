#ifndef MQTT_H
#define MQTT_H

#include <stdint.h>
#include "rtsp.h"

#ifdef CONFIG_MQTT
#include <mosquitto.h>
#else
struct mosquitto;
#endif

int initialise_mqtt(void);
void mqtt_process_metadata(uint32_t type, uint32_t code, const char *data, uint32_t length);
void mqtt_publish(const char *topic, const char *data, uint32_t length);

void metadata_mqtt_queue_init();
void metadata_mqtt_queue_stop();
int send_metadata_to_mqtt_queue(const uint32_t type, const uint32_t code, const char *data,
                                const uint32_t length, rtsp_message *carrier, int block);

#endif // MQTT_H
