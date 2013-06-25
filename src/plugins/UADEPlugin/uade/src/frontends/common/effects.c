/* Effect module for libuade.

   Copyright 2005 (C) Antti S. Lankila <alankila@bel.fi>

   This module is licensed under the GNU LGPL.
*/

#include <uade/effects.h>
#include <uade/uadestate.h>
#include <uade/compilersupport.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <math.h>

/*** old headphone effect ***/
#define UADE_EFFECT_HEADPHONES_DELAY_DIRECT 0.3
#define UADE_EFFECT_HEADPHONES_CROSSMIX_VOL 0.80

/*** new headphone effect ***/

/* delay time defines the width of the head. 0.5 ms gives us 15 cm virtual distance
 * between sound arriving to either ear. */
#define HEADPHONE2_DELAY_K 0.15
/* head shadow frequency cutoff */
#define HEADPHONE2_SHADOW_FREQ 8000.0
/* high shelve keeps frequencies below cutoff intact and attenuates
 * the rest in an uniform way. The effect is to make bass more "mono" than "stereo". */
#define HEADPHONE2_SHELVE_FREQ 100.0
#define HEADPHONE2_SHELVE_LEVEL -2.0

#define DENORMAL_OFFSET 1E-10

static void gain(int gain_amount, int16_t * sm, int frames);
static void pan(int pan_amount, int16_t * sm, int frames);
static void headphones(int16_t * sm, int frames, struct uade_effect_state *es);
static void headphones2(int16_t * sm, int frames, struct uade_effect_state *es);

static void consistencycheck(void)
{
	struct uade_effect_state state;
	size_t s;
	size_t t;

	/* Self-consistency check for a #define in effects.h */
	s = (size_t) (MAXIMUM_SAMPLING_RATE*HEADPHONE2_DELAY_TIME + 1);
	t = sizeof(state.headphone2_ap_l) / sizeof(state.headphone2_ap_l[0]);
	assert(s == t);
}

static inline int sampleclip(int x)
{
	if (unlikely(x > 32767 || x < -32768)) {
		if (x > 32767)
			x = 32767;
		else
			x = -32768;
	}
	return x;
}

/* calculate a high shelve filter */
static void calculate_shelve(double fs, double fc, double g, uade_biquad_t * bq)
{
	float A, omega, sn, cs, beta, b0, b1, b2, a0, a1, a2;

	A = powf(10, g / 40);
	omega = 2 * M_PI * fc / fs;
	omega = tan(omega / 2) * 2;
	sn = sin(omega);
	cs = cos(omega);
	beta = sqrt(A + A);

	b0 = A * ((A + 1) + (A - 1) * cs + beta * sn);
	b1 = -2 * A * ((A - 1) + (A + 1) * cs);
	b2 = A * ((A + 1) + (A - 1) * cs - beta * sn);
	a0 = (A + 1) - (A - 1) * cs + beta * sn;
	a1 = 2 * ((A - 1) - (A + 1) * cs);
	a2 = (A + 1) - (A - 1) * cs - beta * sn;

	bq->b0 = b0 / a0;
	bq->b1 = b1 / a0;
	bq->b2 = b2 / a0;
	bq->a1 = a1 / a0;
	bq->a2 = a2 / a0;
}

/* calculate 1st order lowpass filter */
static void calculate_rc(double fs, double fc, uade_biquad_t * bq)
{
	float omega;

	if (fc >= fs / 2) {
		bq->b0 = 1.0;
		bq->b1 = 0.0;
		bq->b2 = 0.0;
		bq->a1 = 0.0;
		bq->a2 = 0.0;
		return;
	}
	omega = 2 * M_PI * fc / fs;
	omega = tan(omega / 2) * 2;

	bq->b0 = 1 / (1 + 1 / omega);
	bq->b1 = 0;
	bq->b2 = 0;
	bq->a1 = -1 + bq->b0;
	bq->a2 = 0;
}

static inline float evaluate_biquad(float input, uade_biquad_t * bq)
{
	float output = DENORMAL_OFFSET;

	output += input * bq->b0 + bq->x[0] * bq->b1 + bq->x[1] * bq->b2;
	output -= bq->y[0] * bq->a1 + bq->y[1] * bq->a2;

	bq->x[1] = bq->x[0];
	bq->x[0] = input;

	bq->y[1] = bq->y[0];
	bq->y[0] = output;

	return output;
}

void uade_effect_disable_all(struct uade_state *state)
{
	struct uade_effect_state *es = &state->effectstate;
	es->enabled = 0;
}

void uade_effect_disable(struct uade_state *state, uade_effect_t effect)
{
	struct uade_effect_state *es = &state->effectstate;
	es->enabled &= ~(1 << effect);
}

void uade_effect_enable(struct uade_state *state, uade_effect_t effect)
{
	struct uade_effect_state *es = &state->effectstate;
	es->enabled |= 1 << effect;
}

/* Returns 1 if effect is enabled, and zero otherwise. */
int uade_effect_is_enabled(const struct uade_state *state, uade_effect_t effect)
{
	const struct uade_effect_state *es = &state->effectstate;
	return (es->enabled & (1 << effect)) != 0;
}

void uade_effect_run(struct uade_state *state, int16_t *samples, int frames)
{
	struct uade_effect_state *es = &state->effectstate;
	if (es->enabled & (1 << UADE_EFFECT_ALLOW)) {
		if (es->enabled & (1 << UADE_EFFECT_PAN))
			pan(es->pan, samples, frames);
		if (es->enabled & (1 << UADE_EFFECT_HEADPHONES))
			headphones(samples, frames, es);
		if (es->enabled & (1 << UADE_EFFECT_HEADPHONES2) && es->rate)
			headphones2(samples, frames, es);
		if (es->enabled & (1 << UADE_EFFECT_GAIN))
			gain(es->gain, samples, frames);
	}
}

void uade_effect_toggle(struct uade_state *state, uade_effect_t effect)
{
	struct uade_effect_state *es = &state->effectstate;
	es->enabled ^= 1 << effect;
}

void uade_effect_set_defaults(struct uade_state *state)
{
	struct uade_effect_state *es = &state->effectstate;
	consistencycheck();

	memset(es, 0, sizeof *es);

	uade_effect_disable_all(state);
	uade_effect_enable(state, UADE_EFFECT_ALLOW);
	uade_effect_gain_set_amount(state, 1.0);
	uade_effect_pan_set_amount(state, 0.7);
}

/* Rate of 0 means undefined. Effects that depend on sample rate must
   self-check against this because they can not implemented properly */
void uade_effect_set_sample_rate(struct uade_state *state, int rate)
{
	struct uade_effect_state *es = &state->effectstate;
	assert(rate >= 0);
	es->rate = rate;

	if (rate == 0)
		return;

	calculate_shelve(rate, HEADPHONE2_SHELVE_FREQ, HEADPHONE2_SHELVE_LEVEL,
			 &es->headphone2_shelve_l);
	calculate_shelve(rate, HEADPHONE2_SHELVE_FREQ, HEADPHONE2_SHELVE_LEVEL,
			 &es->headphone2_shelve_r);
	calculate_rc(rate, HEADPHONE2_SHADOW_FREQ, &es->headphone2_rc_l);
	calculate_rc(rate, HEADPHONE2_SHADOW_FREQ, &es->headphone2_rc_r);
	es->headphone2_delay_length = HEADPHONE2_DELAY_TIME * rate + 0.5;
	if (es->headphone2_delay_length > HEADPHONE2_DELAY_MAX_LENGTH) {
		fprintf(stderr,	"effects.c: truncating headphone delay line due to samplerate exceeding 96 kHz.\n");
		es->headphone2_delay_length = HEADPHONE2_DELAY_MAX_LENGTH;
	}
}

void uade_effect_gain_set_amount(struct uade_state *state, float amount)
{
	struct uade_effect_state *es = &state->effectstate;
	assert(amount >= 0.0 && amount <= 128.0);
	es->gain = amount * 256.0;
}

void uade_effect_pan_set_amount(struct uade_state *state, float amount)
{
	struct uade_effect_state *es = &state->effectstate;
	assert(amount >= 0.0 && amount <= 2.0);
	es->pan = amount * 256.0 / 2.0;
}

static void gain(int gain_amount, int16_t *sm, int frames)
{
	int i;
	for (i = 0; i < 2 * frames; i += 1)
		sm[i] = sampleclip((sm[i] * gain_amount) >> 8);
}

/* Panning effect. Turns stereo into mono in a specific degree */
static void pan(int pan_amount, int16_t *sm, int frames)
{
	int i, l, r, m;
	for (i = 0; i < frames; i += 1) {
		l = sm[0];
		r = sm[1];
		m = (r - l) * pan_amount;
		sm[0] = ((l << 8) + m) >> 8;
		sm[1] = ((r << 8) - m) >> 8;
		sm += 2;
	}
}

/* All-pass delay. Its purpose is to confuse the phase of the sound a bit
 * and also provide some delay to locate the source outside the head. This
 * seems to work better than a pure delay line. */
static float headphones_allpass_delay(float in, float *state)
{
	int i;
	float tmp, output;

	tmp = in - UADE_EFFECT_HEADPHONES_DELAY_DIRECT * state[0];
	output = state[0] + UADE_EFFECT_HEADPHONES_DELAY_DIRECT * tmp;

	/* FIXME: use modulo and index */
	for (i = 1; i < UADE_EFFECT_HEADPHONES_DELAY_LENGTH; i += 1)
		state[i - 1] = state[i];
	state[UADE_EFFECT_HEADPHONES_DELAY_LENGTH - 1] = tmp;

	return output;
}

static float headphones_lpf(float in, float *state)
{
	float out = in * 0.53;
	out += 0.47 * state[0];
	state[0] = out;

	return out;
}

/* A real implementation would simply perform FIR with recorded HRTF data. */
static void headphones(int16_t *sm, int frames, struct uade_effect_state *es)
{
	int i;
	float ld, rd;
	int l_final;
	int r_final;

	for (i = 0; i < frames; i += 1) {
		ld = headphones_allpass_delay(sm[0], es->headphones_ap_l);
		rd = headphones_allpass_delay(sm[1], es->headphones_ap_r);
		ld = headphones_lpf(ld, es->headphones_rc_l);
		rd = headphones_lpf(rd, es->headphones_rc_r);

		l_final =
		    (sm[0] + rd * UADE_EFFECT_HEADPHONES_CROSSMIX_VOL) / 2;
		r_final =
		    (sm[1] + ld * UADE_EFFECT_HEADPHONES_CROSSMIX_VOL) / 2;
		sm[0] = sampleclip(l_final);
		sm[1] = sampleclip(r_final);

		sm += 2;
	}
}

static float headphone2_allpass_delay(float in, float *state,
				      struct uade_effect_state *es)
{
	int i;
	float tmp, output;

	tmp = in - HEADPHONE2_DELAY_K * state[0];
	output = state[0] + HEADPHONE2_DELAY_K * tmp;

	/* FIXME: use modulo and index */
	for (i = 1; i < es->headphone2_delay_length; i += 1)
		state[i - 1] = state[i];
	state[es->headphone2_delay_length - 1] = tmp;

	return output;
}

static void headphones2(int16_t *sm, int frames, struct uade_effect_state *es)
{
	int i;

	for (i = 0; i < frames; i += 1) {
		float ld, rd;

		ld = headphone2_allpass_delay(sm[0], es->headphone2_ap_l, es);
		rd = headphone2_allpass_delay(sm[1], es->headphone2_ap_r, es);
		ld = evaluate_biquad(ld, &es->headphone2_rc_l);
		rd = evaluate_biquad(rd, &es->headphone2_rc_r);
		ld = evaluate_biquad(ld, &es->headphone2_shelve_l);
		rd = evaluate_biquad(rd, &es->headphone2_shelve_r);

		sm[0] = sampleclip((sm[0] + rd) / 2);
		sm[1] = sampleclip((sm[1] + ld) / 2);
		sm += 2;
	}
}
