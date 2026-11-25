#pragma once

// This handles the plist that comes in on the AirPlay 2 COMMAND endpoint.

#include "rtsp.h"

#ifdef CONFIG_AIRPLAY_2
#include <plist/plist.h>
void metadata_hub_handle_command_plist(rtsp_conn_info *conn, const plist_t command_dict);
#else
void metadata_hub_handle_command_plist(rtsp_conn_info *conn, const void *command_dict);
#endif
