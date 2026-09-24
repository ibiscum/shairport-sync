#pragma once

extern int command_execute_call_count;
extern int command_execute_last_block;
extern const char *command_execute_last_command;

void reset_command_execute_stub(void);
