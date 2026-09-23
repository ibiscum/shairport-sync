#pragma once

extern volatile int command_execute_call_count;
extern volatile int command_execute_last_block;
extern const char *command_execute_last_command;

extern int named_pthread_create_call_count;

void reset_command_execute_stub(void);
void reset_thread_lifecycle_stubs(void);
void set_named_pthread_create_real_mode(int enabled);
