#ifndef AP2_EVENT_RECEIVER_H
#define AP2_EVENT_RECEIVER_H

#include "player.h"

ssize_t ap2_event_port_send_message(rtsp_conn_info *conn, char *data, size_t data_length);
#ifdef CONFIG_AIRPLAY_2
ssize_t ap2_event_port_post_command(rtsp_conn_info *conn, plist_t command);
#else
ssize_t ap2_event_port_post_command(rtsp_conn_info *conn, const void *command);
#endif

void *ap2_event_receiver(void *arg);

#endif // AP2_EVENT_RECEIVER_H
