/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999-2001 Michael Klein <michael.klein@puffin.lb.shuttle.de>
 */

/* $Id: opencbm.h,v 1.14 2003/09/28 15:48:12 michael Exp $ */

#ifndef OPENCBM_H
#define OPENCBM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

#ifdef _WIN32
  /* we have windows */

#include <windows.h>

# if defined DLL
#  define EXTERN __declspec(dllexport)
# else
#  define EXTERN __declspec(dllimport)
# endif

#define CBMAPIDECL __cdecl
  /* this is already defined here! #define WINAPI */
# define CBM_FILE HANDLE
#else
  /* we have linux */
# define EXTERN extern
# define CBMAPIDECL
# define WINAPI
# define CBM_FILE int
#ifdef MACOSX_SUPPORT
#endif
#endif

#define IEC_DATA   0x01
#define IEC_CLOCK  0x02
#define IEC_ATN    0x04

#define IEC_RESET  0x08

enum cbm_device_type_e 
{
    cbm_dt_unknown = -1,
    cbm_dt_cbm1541,
    cbm_dt_cbm1570,
    cbm_dt_cbm1571,
    cbm_dt_cbm1581
};

/* FIXME: port isn't used yet */
EXTERN int CBMAPIDECL cbm_driver_open(CBM_FILE *f, int port);
EXTERN void CBMAPIDECL cbm_driver_close(CBM_FILE f);

/* FIXME: port isn't used yet */
EXTERN const char * CBMAPIDECL cbm_get_driver_name(int port);

EXTERN int CBMAPIDECL cbm_listen(CBM_FILE f, unsigned char dev, unsigned char secadr);
EXTERN int CBMAPIDECL cbm_talk(CBM_FILE f, unsigned char dev, unsigned char secadr);

EXTERN int CBMAPIDECL cbm_open(CBM_FILE f, unsigned char dev, unsigned char secadr, const void *fname, size_t len);
EXTERN int CBMAPIDECL cbm_close(CBM_FILE f, unsigned char dev, unsigned char secadr);

EXTERN int CBMAPIDECL cbm_raw_read(CBM_FILE f, void *buf, size_t size);
EXTERN int CBMAPIDECL cbm_raw_write(CBM_FILE f, const void *buf, size_t size);

EXTERN int CBMAPIDECL cbm_unlisten(CBM_FILE f);
EXTERN int CBMAPIDECL cbm_untalk(CBM_FILE f);

EXTERN int CBMAPIDECL cbm_get_eoi(CBM_FILE f);

EXTERN int CBMAPIDECL cbm_reset(CBM_FILE f);

EXTERN unsigned char CBMAPIDECL cbm_pp_read(CBM_FILE f);
EXTERN void CBMAPIDECL cbm_pp_write(CBM_FILE f, unsigned char c);

EXTERN int CBMAPIDECL cbm_iec_poll(CBM_FILE f);
EXTERN int CBMAPIDECL cbm_iec_get(CBM_FILE f, int line);
EXTERN void CBMAPIDECL cbm_iec_set(CBM_FILE f, int line);
EXTERN void CBMAPIDECL cbm_iec_release(CBM_FILE f, int line);
EXTERN int CBMAPIDECL cbm_iec_wait(CBM_FILE f, int line, int state);

EXTERN int CBMAPIDECL cbm_upload(CBM_FILE f, unsigned char dev, int adr, const void *prog, size_t size);

EXTERN int CBMAPIDECL cbm_device_status(CBM_FILE f, unsigned char dev, void *buf, size_t bufsize);
EXTERN int CBMAPIDECL cbm_exec_command(CBM_FILE f, unsigned char dev, const void *cmd, size_t len);

EXTERN int CBMAPIDECL cbm_identify(CBM_FILE f, int drv,
                              enum cbm_device_type_e *t,
                              const char **type_str);


EXTERN int CBMAPIDECL cbm_petscii2ascii_c(int c);
EXTERN int CBMAPIDECL cbm_ascii2petscii_c(int c);
EXTERN char * CBMAPIDECL cbm_petscii2ascii(char *str);
EXTERN char * CBMAPIDECL cbm_ascii2petscii(char *str);

#ifdef __cplusplus
}
#endif

#endif /* OPENCBM_H */
