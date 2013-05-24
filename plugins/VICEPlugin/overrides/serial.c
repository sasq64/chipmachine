#include <stdlib.h>
#include "serial.h"
#include "traps.h"

static serial_t serialdevices[SERIAL_MAXDEVICES];

int serial_init(const struct trap_s *trap_list) { return 0; }
int serial_resources_init(void) { return 0; }
int serial_cmdline_options_init(void) { return 0; }
void serial_shutdown(void) {}
int serial_install_traps(void) { return 0; }
int serial_remove_traps(void) { return 0; }

void serial_trap_init(WORD tmpin) {}
int serial_trap_attention(void) { return 0; }
int serial_trap_send(void) { return 0; }
int serial_trap_receive(void) { return 0; }
int serial_trap_ready(void) { return 0; }
void serial_traps_reset(void) {}
void serial_trap_eof_callback_set(void (*func)(void)) {}
void serial_trap_attention_callback_set(void (*func)(void)) {}
void serial_trap_truedrive_set(unsigned int flag) {}

int serial_realdevice_enable(void) { return 0; }
void serial_realdevice_disable(void) {}

int serial_iec_lib_directory(unsigned int unit, const char *pattern, BYTE **buf) { return 0; }
int serial_iec_lib_read_sector(unsigned int unit, unsigned int track, unsigned int sector, BYTE *buf) { return 0; }
int serial_iec_lib_write_sector(unsigned int unit, unsigned int track, unsigned int sector, BYTE *buf) { return 0; }

serial_t *serial_device_get(unsigned int unit) { return &serialdevices[unit]; }
unsigned int serial_device_type_get(unsigned int unit) { return 0; }
void serial_device_type_set(unsigned int type, unsigned int unit) {}

void serial_iec_device_set_machine_parameter(long cycles_per_sec) {}
void serial_iec_device_exec(CLOCK clk_value) {}

void serial_iec_bus_init(void) {}

