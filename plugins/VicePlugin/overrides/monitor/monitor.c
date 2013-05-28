#include <stdlib.h>
#include "monitor.h"

unsigned monitor_mask[NUM_MEMSPACES];
struct break_list_s *breakpoints[NUM_MEMSPACES];
struct break_list_s *watchpoints_load[NUM_MEMSPACES];
struct break_list_s *watchpoints_store[NUM_MEMSPACES];
static int set_keep_monitor_open(int val, void *param) { return 0; }
int monitor_resources_init(void) { return 0; }
void monitor_init(monitor_interface_t *maincpu_interface,
                         monitor_interface_t *drive_interface_init[],
                         struct monitor_cpu_type_s **asmarray) {}
void monitor_shutdown(void) {}
int monitor_cmdline_options_init(void) { return 0; }
void monitor_startup(MEMSPACE mem) {}
void monitor_startup_trap(void) {}
void monitor_abort(void) {}
int monitor_force_import(MEMSPACE mem) { return 0; }
void monitor_check_icount(WORD a) {}
void monitor_check_icount_interrupt(void) {}
void monitor_check_watchpoints(unsigned int lastpc, unsigned int pc) {}

void monitor_cpu_type_set(const char *cpu_type) {}

void monitor_watch_push_load_addr(WORD addr, MEMSPACE mem) {}
void monitor_watch_push_store_addr(WORD addr, MEMSPACE mem) {}

monitor_interface_t *monitor_interface_new(void) { return NULL; }
void monitor_interface_destroy(monitor_interface_t *monitor_interface) {}

int monitor_diskspace_dnr(int mem) { return 0; }
int monitor_diskspace_mem(int dnr) { return 0; }

int mon_out(const char *format, ...) { return 0; }

/* Prototypes */
int monitor_breakpoint_check_checkpoint(MEMSPACE mem, WORD addr, struct break_list_s *list) { return 0; }
int monitor_check_breakpoints(MEMSPACE mem, WORD addr) { return 0; }

const char *mon_disassemble_to_string(MEMSPACE mem, unsigned int addr, unsigned int x, unsigned int p1, unsigned int p2, unsigned int p3, int hex_mode, const char *cpu_type) { return NULL; }

/** Register interface.  */
struct mon_reg_list_s *mon_register_list_get(int mem) { return NULL; }
void mon_ioreg_add_list(struct mem_ioreg_list_s **list, const char *name, int start, int end, void *dump) {}

/* Assembler initialization.  */
void asm6502_init(struct monitor_cpu_type_s *monitor_cpu_type) {}
void asm6502dtv_init(struct monitor_cpu_type_s *monitor_cpu_type) {}
void asmz80_init(struct monitor_cpu_type_s *monitor_cpu_type) {}

monitor_cartridge_commands_t mon_cart_cmd;

/* CPU history/memmap prototypes */
void monitor_cpuhistory_store(unsigned int addr, unsigned int op, unsigned int p1, unsigned int p2, BYTE reg_a, BYTE reg_x, BYTE reg_y, BYTE reg_sp, unsigned int reg_st) {}
void monitor_memmap_store(unsigned int addr, unsigned int type) {}

/* HACK to enable fetch/load separation */
BYTE memmap_state;
