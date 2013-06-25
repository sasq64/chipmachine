#ifndef _UADE2_EFFECTS_H_
#define _UADE2_EFFECTS_H_

#include <uade/uade.h>
#include <stdint.h>

typedef struct {
	float b0;
	float b1;
	float b2;
	float a1;
	float a2;
	float x[2];
	float y[2];
} uade_biquad_t;

#define UADE_EFFECT_HEADPHONES_DELAY_LENGTH 22

#define HEADPHONE2_DELAY_TIME 0.49e-3
#define MAXIMUM_SAMPLING_RATE 96000

/*
 * HEADPHONE2_DELAY_MAX_LENGTH ==
 * (int) (MAXIMUM_SAMPLING_RATE*HEADPHONE2_DELAY_TIME + 1).
 * Note: (int) (0.49e-3 * 96000 + 1) == 48. The value is asserted in code.
 * The assert fails if the next #define is not updated properly for new values.
 */
#define HEADPHONE2_DELAY_MAX_LENGTH 48

struct uade_effect_state {
	uade_effect_t enabled;
	int gain;
	int pan;
	int rate;

	/* Headphone variables */
	float headphones_ap_l[UADE_EFFECT_HEADPHONES_DELAY_LENGTH];
	float headphones_ap_r[UADE_EFFECT_HEADPHONES_DELAY_LENGTH];
	float headphones_rc_l[4];
	float headphones_rc_r[4];

	float headphone2_ap_l[HEADPHONE2_DELAY_MAX_LENGTH];
	float headphone2_ap_r[HEADPHONE2_DELAY_MAX_LENGTH];
	int headphone2_delay_length;
	uade_biquad_t headphone2_shelve_l;
	uade_biquad_t headphone2_shelve_r;
	uade_biquad_t headphone2_rc_l;
	uade_biquad_t headphone2_rc_r;
};

void uade_effect_set_defaults(struct uade_state *state);
void uade_effect_set_sample_rate(struct uade_state *state, int rate);

/* reset state at start of song */
void uade_effect_reset_internals(struct uade_state *state);

/* process n frames of sample buffer */
void uade_effect_run(struct uade_state *s, int16_t *sample, int frames);

#endif
