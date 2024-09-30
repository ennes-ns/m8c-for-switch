// serial.h

#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

int init_serial(const int verbose, const char *preferred_device);
int close_serial_port(void);
int disconnect(void);
int enable_and_reset_display(void);
int reset_display(void);
int send_msg_controller(uint8_t input);
int send_msg_keyjazz(uint8_t note, uint8_t velocity);
int serial_read(uint8_t *buffer, int len);
int check_serial_port(void);
int list_devices(void);

#endif // SERIAL_H
