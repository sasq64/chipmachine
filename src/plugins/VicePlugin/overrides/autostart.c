#include "autostart.h"
#include "snapshot.h"

int autostart_ignore_reset = 0;

int autostart_resources_init(void)
{
    return 0;
}

void autostart_resources_shutdown(void)
{
}

int autostart_cmdline_options_init(void)
{
    return 0;
}

void autostart_reinit(CLOCK _min_cycles, int _handle_drive_true_emulation,
                      int _blnsw, int _pnt, int _pntr, int _lnmx)
{
}

int autostart_init(CLOCK min_cycles, int handle_drive_true_emulation,
                   int blnsw, int pnt, int pntr, int lnmx)
{
    return 0;
}

void autostart_disable(void)
{
}

void autostart_advance(void)
{
}

/* Autostart snapshot file `file_name'.  */
int autostart_snapshot(const char *file_name, const char *program_name)
{
  return 0;
}

/* Autostart tape image `file_name'.  */
int autostart_tape(const char *file_name, const char *program_name,
                   unsigned int program_number, unsigned int runmode)
{
    return 0;
}

/* Autostart disk image `file_name'.  */
int autostart_disk(const char *file_name, const char *program_name,
                   unsigned int program_number, unsigned int runmode)
{
    return 0;
}

/* Autostart PRG file `file_name'.  The PRG file can either be a raw CBM file
   or a P00 file */
int autostart_prg(const char *file_name, unsigned int runmode)
{
    return 0;
}

/* ------------------------------------------------------------------------- */

/* Autostart `file_name', trying to auto-detect its type.  */
int autostart_autodetect(const char *file_name, const char *program_name,
                         unsigned int program_number, unsigned int runmode)
{
    return 0;
}

/* Autostart the image attached to device `num'.  */
int autostart_device(int num)
{
    return 0;
}

int autostart_in_progress(void)
{
    return 0;
}

/* Disable autostart on reset.  */
void autostart_reset(void)
{
}

void autostart_shutdown(void)
{
}

int autostart_autodetect_opt_prgname(const char *file_prog_name, 
                                     unsigned int alt_prg_number,
                                     unsigned int runmode) {
    return 0;
}
