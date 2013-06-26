/**
 * @ingroup   io68_ym_devel
 * @file      io68/ym_blep.h
 * @author    Antti Lankila
 *
 */

/* $Id: ym_blep.h 102 2009-03-14 17:21:58Z benjihan $ */

/* Copyright (C) 1998-2009 Antti Lankila */

/** Setup function for ym blep synthesis engine.
 *
 *    The ym_blep_setup() function sets ym blep synthesis engine for
 *    this ym emulator instance.
 *
 *  @parm    ym  ym emulator instance to setup
 *  @retval   0  on success
 *  @retval  -1  on failure
 */
int ym_blep_setup(ym_t * const ym);

enum {
  MAX_BLEPS = 256
};

typedef struct {
  s32 count;
  u32 event;
  u16 flip_flop;
  u16 tonemix;
  u16 noisemix;
  u16 envmask;
  u16 volmask;
} ym_blep_tone_t;

typedef struct {
  u16 stamp;
  s16 level;
} ym_blep_blep_state_t;

typedef struct {
  /* sampling parameters */
  u32 cycles_per_sample; /* 8 bit fixed point ym clocks */
  u32 cycles_to_next_sample;

  /* subsystem states */
  ym_blep_tone_t tonegen[3];
  u32 noise_event;
  s32 noise_count;
  u32 noise_state;
  u16 noise_output;

  u32 env_event;
  s32 env_count;
  u8 env_state;
  u16 env_output;

  /* blep stuff */
  s16 global_output_level;
  u32 blep_idx;
  u16 time;
  s32 hp;

  ym_blep_blep_state_t blepstate[MAX_BLEPS];
} ym_blep_t;
