#include <stdlib.h>
#include "attach.h"

int file_system_resources_init(void)
{
    return 0;
}

int file_system_cmdline_options_init(void)
{
    return 0;
}

void file_system_init(void)
{
}

void file_system_shutdown(void)
{
}

struct vdrive_s *file_system_get_vdrive(unsigned int unit)
{
    return NULL;
}

const char *file_system_get_disk_name(unsigned int unit)
{
    return NULL;
}

int file_system_bam_get_disk_id(unsigned int unit, BYTE *id)
{
    return 0;
}

int file_system_bam_set_disk_id(unsigned int unit, BYTE *id)
{
    return 0;
}

int file_system_attach_disk(unsigned int unit, const char *filename)
{
    return 0;
}

void file_system_detach_disk(int unit)
{
}

void file_system_detach_disk_shutdown(void)
{
}

void file_system_event_playback(unsigned int unit, const char *filename)
{
}
