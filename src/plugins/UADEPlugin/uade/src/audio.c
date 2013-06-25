 /*
  * UADE's audio state machine emulation
  *
  * Copyright 1995, 1996, 1997 Bernd Schmidt
  * Copyright 1996 Marcus Sundberg
  * Copyright 1996 Manfred Thole
  * Copyright 2005 Heikki Orsila
  * Copyright 2005 Antti S. Lankila
  */

#include <math.h>

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "custom.h"
#include "gensound.h"
#include "sd-sound.h"
#include "events.h"
#include "cia.h"
#include "audio.h"
#include <uade/amigafilter.h>
#include "uadectl.h"
#include <uade/compilersupport.h>

#include "sinctable.h"

#include "text_scope.h"


struct audio_channel_data audio_channel[4];
static void (*sample_handler) (void);
static void (*sample_prehandler) (unsigned long best_evtime);

/* Average time in bus cycles to output a new sample */
static float sample_evtime_interval;
static float next_sample_evtime;

int sound_available;

static int use_text_scope;

static int sound_use_filter = FILTER_MODEL_A500;

/* denormals are very small floating point numbers that force FPUs into slow
   mode. All lowpass filters using floats are suspectible to denormals unless
   a small offset is added to avoid very small floating point numbers. */
#define DENORMAL_OFFSET (1E-10)

static unsigned long last_audio_cycles;

static int audperhack;

static struct filter_state {
    float rc1, rc2, rc3, rc4, rc5;
} sound_filter_state[2];

static float a500e_filter1_a0;
static float a500e_filter2_a0;
static float filter_a0; /* a500 and a1200 use the same */


static inline int clamp_sample(int o)
{
    if (unlikely(o > 32767 || o < -32768)) {
	if (o > 32767) {
	    return 32767;
	} else {
	    return -32768;
	}
    }
    return o;
}


/* Amiga has two separate filtering circuits per channel, a static RC filter
 * on A500 and the LED filter. This code emulates both.
 * 
 * The Amiga filtering circuitry depends on Amiga model. Older Amigas seem
 * to have a 6 dB/oct RC filter with cutoff frequency such that the -6 dB
 * point for filter is reached at 6 kHz, while newer Amigas have no filtering.
 *
 * The LED filter is complicated, and we are modelling it with a pair of
 * RC filters, the other providing a highboost. The LED starts to cut
 * into signal somewhere around 5-6 kHz, and there's some kind of highboost
 * in effect above 12 kHz. Better measurements are required.
 *
 * The current filtering should be accurate to 2 dB with the filter on,
 * and to 1 dB with the filter off.
*/

static int filter(int input, struct filter_state *fs)
{
    float tmp, normal_output, led_output;

    switch (sound_use_filter) {
    case FILTER_MODEL_A500: 
	fs->rc1 = a500e_filter1_a0 * input + (1 - a500e_filter1_a0) * fs->rc1 + DENORMAL_OFFSET;
	fs->rc2 = a500e_filter2_a0 * fs->rc1 + (1-a500e_filter2_a0) * fs->rc2;
	normal_output = fs->rc2;

	fs->rc3 = filter_a0 * normal_output + (1 - filter_a0) * fs->rc3;
	fs->rc4 = filter_a0 * fs->rc3       + (1 - filter_a0) * fs->rc4;
	fs->rc5 = filter_a0 * fs->rc4       + (1 - filter_a0) * fs->rc5;

	led_output = fs->rc5;
        break;

    case FILTER_MODEL_A1200:
        normal_output = input;

        fs->rc2 = filter_a0 * normal_output + (1 - filter_a0) * fs->rc2 + DENORMAL_OFFSET;
        fs->rc3 = filter_a0 * fs->rc2       + (1 - filter_a0) * fs->rc3;
        fs->rc4 = filter_a0 * fs->rc3       + (1 - filter_a0) * fs->rc4;

        led_output = fs->rc4;
        break;

    default:
	fprintf(stderr, "Unknown filter mode\n");
	exit(1);
    }

    return clamp_sample(gui_ledstate ? led_output : normal_output);
}


static void check_sound_buffers (void)
{
    intptr_t bytes;

    if (uadecore_reboot)
	return;

    assert(uadecore_read_size > 0);

    bytes = ((intptr_t) sndbufpt) - ((intptr_t) sndbuffer);

    if (uadecore_audio_output) {
	if (bytes == uadecore_read_size) {
	    uadecore_check_sound_buffers(uadecore_read_size);
	    sndbufpt = sndbuffer;
	}
    } else {
	uadecore_audio_skip += bytes;
	/* if sound core doesn't report audio output start in 3 seconds from
	   the reboot, begin audio output anyway */
	if (uadecore_audio_skip >= (sound_bytes_per_second * 3)) {
	    fprintf(stderr, "involuntary audio output start\n");
	    uadecore_audio_output = 1;
	}
	sndbufpt = sndbuffer;
    }
}


static inline void sample_backend(int left, int right)
{
#if AUDIO_DEBUG
    int nr;
    for (nr = 0; nr < 4; nr++) {
	struct audio_channel_data *cdp = audio_channel + nr;
	if (cdp->state != 0 && cdp->datpt != 0 && (dmacon & (1 << nr)) && cdp->datpt >= cdp->datptend) {
	    fprintf(stderr, "Audio output overrun on channel %d: %.8x/%.8x\n", nr, cdp->datpt, cdp->datptend);
	}
    }
#endif

    /* samples are in range -16384 (-128*64*2) and 16256 (127*64*2) */
    left <<= 16 - 14 - 1;
    right <<= 16 - 14 - 1;
    /* [-32768, 32512] */

    if (sound_use_filter) {
	left = filter(left, &sound_filter_state[0]);
	right = filter(right, &sound_filter_state[1]);
    }

    *(sndbufpt++) = left;
    *(sndbufpt++) = right;

    check_sound_buffers();
}


static void sample16s_handler (void)
{
    int datas[4];
    int i;

    for (i = 0; i < 4; i++) {
	datas[i] = audio_channel[i].current_sample * audio_channel[i].vol;
	datas[i] &= audio_channel[i].adk_mask;
    }

    sample_backend(datas[0] + datas[3], datas[1] + datas[2]);
}


/* This interpolator examines sample points when Paula switches the output
 * voltage and computes the average of Paula's output */
static void sample16si_anti_handler (void)
{
    int i;
    int datas[4];

    for (i = 0; i < 4; i += 1) {
        datas[i] = audio_channel[i].sample_accum / audio_channel[i].sample_accum_time;
        audio_channel[i].sample_accum = 0;
	audio_channel[i].sample_accum_time = 0;
    }

    sample_backend(datas[0] + datas[3], datas[1] + datas[2]);
}


/* this interpolator performs BLEP mixing (bleps are shaped like integrated sinc
 * functions) with a type of BLEP that matches the filtering configuration. */
static void sample16si_sinc_handler (void)
{
    int i, n;
    int const *winsinc;
    int datas[4];

    if (sound_use_filter) {
	n = (sound_use_filter == FILTER_MODEL_A500) ? 0 : 2;
        if (gui_ledstate)
            n += 1;
    } else {
	n = 4;
    }
    winsinc = winsinc_integral[n];
    
    for (i = 0; i < 4; i += 1) {
        int j;
        struct audio_channel_data *acd = &audio_channel[i];
        /* The sum rings with harmonic components up to infinity... */
	int sum = acd->output_state << 17;
        /* ...but we cancel them through mixing in BLEPs instead */
        int offsetpos = acd->sinc_queue_head & (SINC_QUEUE_LENGTH - 1);
        for (j = 0; j < SINC_QUEUE_LENGTH; j += 1) {
            int age = acd->sinc_queue_time - acd->sinc_queue[offsetpos].time;
            if (age >= SINC_QUEUE_MAX_AGE)
                break;
            sum -= winsinc[age] * acd->sinc_queue[offsetpos].output;
            offsetpos = (offsetpos + 1) & (SINC_QUEUE_LENGTH - 1);
        }
        datas[i] = sum >> 16;
    }

    *(sndbufpt++) = clamp_sample(datas[0] + datas[3]);
    *(sndbufpt++) = clamp_sample(datas[1] + datas[2]);

    check_sound_buffers();
}


static void anti_prehandler(unsigned long best_evtime)
{
    int i, j, output;
    struct audio_channel_data *acd;

    /* Handle accumulator antialiasiation */
    for (i = 0; i < 4; i++) {
	acd = &audio_channel[i];
	output = (acd->current_sample * acd->vol) & acd->adk_mask;
	acd->sample_accum += output * best_evtime;
	acd->sample_accum_time += best_evtime;
    }
}


static void sinc_prehandler(unsigned long best_evtime)
{
    int i, j, output;
    struct audio_channel_data *acd;

    for (i = 0; i < 4; i++) {
	acd = &audio_channel[i];
	output = (acd->current_sample * acd->vol) & acd->adk_mask;

        /* if output state changes, record the state change and also
         * write data into sinc queue for mixing in the BLEP */
        if (acd->output_state != output) {
            acd->sinc_queue_head = (acd->sinc_queue_head - 1) & (SINC_QUEUE_LENGTH - 1);
            acd->sinc_queue[acd->sinc_queue_head].time = acd->sinc_queue_time;
            acd->sinc_queue[acd->sinc_queue_head].output = output - acd->output_state;
            acd->output_state = output;
        }
        
        acd->sinc_queue_time += best_evtime;
    }
}


static void audio_handler (int nr)
{
    struct audio_channel_data *cdp = audio_channel + nr;

    switch (cdp->state) {
     case 0:
	fprintf(stderr, "Bug in sound code\n");
	break;

     case 1:
	/* We come here at the first hsync after DMA was turned on. */
	cdp->evtime = maxhpos;

	cdp->state = 5;
	INTREQ(0x8000 | (0x80 << nr));
	if (cdp->wlen != 1)
	    cdp->wlen = (cdp->wlen - 1) & 0xFFFF;
	cdp->nextdat = chipmem_bank.wget(cdp->pt);

	cdp->nextdatpt = cdp->pt;
	cdp->nextdatptend = cdp->ptend;

	/* BUG in UAE. Only hsync handler should increase DMA pointer
	   cdp->pt += 2;
	*/
	break;

     case 5:
	/* We come here at the second hsync after DMA was turned on. */
	cdp->evtime = cdp->per;
	cdp->dat = cdp->nextdat;

	cdp->datpt = cdp->nextdatpt;
	cdp->datptend = cdp->nextdatptend;

	cdp->current_sample = (uae_s8)(cdp->dat >> 8);

	cdp->state = 2;
	{
	    int audav = adkcon & (1 << nr);
	    int audap = adkcon & (16 << nr);
	    int napnav = (!audav && !audap) || audav;
	    if (napnav)
		cdp->data_written = 2;
	}
	break;

     case 2:
	/* We come here when a 2->3 transition occurs */
	cdp->current_sample = (uae_s8)(cdp->dat & 0xFF);
	cdp->evtime = cdp->per;

	cdp->state = 3;

	/* Period attachment? */
	if (adkcon & (0x10 << nr)) {
	    if (cdp->intreq2 && cdp->dmaen) {
		INTREQ(0x8000 | (0x80 << nr));
	    }
	    cdp->intreq2 = 0;

	    cdp->dat = cdp->nextdat;

	    cdp->datpt = cdp->nextdatpt;
	    cdp->datptend = cdp->nextdatptend;

	    if (cdp->dmaen)
		cdp->data_written = 2;
	    if (nr < 3) {
		if (cdp->dat == 0)
		    (cdp+1)->per = 65535;
		else
		    (cdp+1)->per = cdp->dat;
	    }
	}
	break;

     case 3:
	/* We come here when a 3->2 transition occurs */
	cdp->evtime = cdp->per;

	if ((INTREQR() & (0x80 << nr)) && !cdp->dmaen) {
	    cdp->state = 0;
	    cdp->current_sample = 0;
	    break;
	} else {
	    int audav = adkcon & (1 << nr);
	    int audap = adkcon & (16 << nr);
	    int napnav = (!audav && !audap) || audav;
	    cdp->state = 2;

	    if ((cdp->intreq2 && cdp->dmaen && napnav)
		|| (napnav && !cdp->dmaen)) {
	      INTREQ(0x8000 | (0x80 << nr));
	    }
	    cdp->intreq2 = 0;

	    cdp->dat = cdp->nextdat;

	    cdp->datpt = cdp->nextdatpt;
	    cdp->datptend = cdp->nextdatptend;

	    cdp->current_sample = (uae_s8)(cdp->dat >> 8);

	    if (cdp->dmaen && napnav)
		cdp->data_written = 2;

	    /* Volume attachment? */
	    if (audav) {
		if (nr < 3) {
		    (cdp+1)->vol = cdp->dat;
		}
	    }
	}
	break;

     default:
	cdp->state = 0;
	break;
    }
}


void audio_reset (void)
{
    memset (audio_channel, 0, sizeof audio_channel);
    audio_channel[0].per = 65535;
    audio_channel[1].per = 65535;
    audio_channel[2].per = 65535;
    audio_channel[3].per = 65535;

    last_audio_cycles = 0;
    next_sample_evtime = sample_evtime_interval;

    audperhack = 0;

    memset(sound_filter_state, 0, sizeof sound_filter_state);

    audio_set_resampler(NULL);

    use_text_scope = 0;
}


/* This computes the 1st order low-pass filter term b0.
 * The a1 term is 1.0 - b0. The center frequency marks the -3 dB point. */
static float rc_calculate_a0(int sample_rate, int cutoff_freq)
{
    float omega;

    /* The BLT correction formula below blows up if the cutoff is above nyquist. */
    if (cutoff_freq >= sample_rate / 2)
        return 1.0;

    omega = 2 * M_PI * cutoff_freq / sample_rate;
    /* Compensate for the bilinear transformation. This allows us to specify the
     * stop frequency more exactly, but the filter becomes less steep further
     * from stopband. */
    omega = tan(omega / 2) * 2;
    return 1 / (1 + 1/omega);
}


void audio_set_filter(int filter_type, int filter_force)
{
  /* If filter_type is zero, filtering is disabled, but if it's
     non-zero, it contains the filter type (a500 or a1200) */
  if (filter_type < 0 || filter_type >= FILTER_MODEL_UPPER_BOUND) {
    fprintf(stderr, "Invalid filter number: %d\n", filter_type);
    exit(1);
  }
  sound_use_filter = filter_type;

  if (filter_force & 2) {
    gui_ledstate_forced = filter_force & 3;
    gui_ledstate = gui_ledstate_forced & 1;
  } else {
    gui_ledstate_forced = 0;
    gui_ledstate = (~ciaapra & 2) >> 1;
  }
}


void audio_set_rate(int rate)
{
    sample_evtime_interval = ((float) SOUNDTICKS) / rate;

    /* Although these numbers are in Hz, these values should not be taken to
     * be the true filter cutoff values of Amiga 500 and Amiga 1200.
     * This is because these filters are composites. The true values are
     * 5 kHz (or 4.5 kHz possibly on some models) for A500 fixed lowpass filter
     * and 1.7 kHz 12 db/oct Butterworth for the LED filter.
     */
    a500e_filter1_a0 = rc_calculate_a0(rate, 6200);
    a500e_filter2_a0 = rc_calculate_a0(rate, 20000);
    filter_a0 = rc_calculate_a0(rate, 7000);
}


void audio_set_resampler(char *name)
{
    sample_handler = sample16si_anti_handler;
    sample_prehandler = anti_prehandler;

    if (name == NULL || strcasecmp(name, "default") == 0)
	return;

    if (strcasecmp(name, "sinc") == 0) {
	sample_handler = sample16si_sinc_handler;
	sample_prehandler = sinc_prehandler;
    } else if (strcasecmp(name, "none") == 0) {
	sample_handler = sample16s_handler;
	sample_prehandler = NULL;
    } else {
	fprintf(stderr, "\nUnknown resampling method: %s. Using the default.\n", name);
    }
}


void audio_use_text_scope(void)
{
    use_text_scope = 1;
}


/* update_audio() emulates actions of audio state machine since it was last
   time called. One can assume it is called at least once per horizontal
   line and possibly more often. */
void update_audio (void)
{
    /* Number of cycles that has passed since last call to update_audio() */
    unsigned long n_cycles = cycles - last_audio_cycles;

    while (n_cycles > 0) {
	unsigned long best_evtime = n_cycles + 1;
	int i;
	unsigned long rounded;
	float f;

	for (i = 0; i < 4; i++) {
	    if (audio_channel[i].state != 0 && best_evtime > audio_channel[i].evtime)
		best_evtime = audio_channel[i].evtime;
	}

	/* next_sample_evtime >= 0 so floor() behaves as expected */
	rounded = floorf(next_sample_evtime);
	if ((next_sample_evtime - rounded) >= 0.5)
	    rounded++;

	if (best_evtime > rounded)
	    best_evtime = rounded;

	if (best_evtime > n_cycles)
	    best_evtime = n_cycles;
	
	/* Decrease time-to-wait counters */
	next_sample_evtime -= best_evtime;

	/* sample_prehandler makes it possible to compute effects with
	   accuracy of one bus cycle. sample_handler is only called when
	   a sample is outputted. */
	if (sample_prehandler != NULL)
	    sample_prehandler(best_evtime);

	for (i = 0; i < 4; i++)
	    audio_channel[i].evtime -= best_evtime;

	n_cycles -= best_evtime;

	/* Test if new sample needs to be outputted */
	if (rounded == best_evtime) {
	    /* Before the following addition, next_sample_evtime is in range
	       [-0.5, 0.5) */
	    next_sample_evtime += sample_evtime_interval;
	    (*sample_handler) ();
	}

	/* Call audio state machines if needed */
	for (i = 0; i < 4; i++) {
	    if (audio_channel[i].evtime == 0 && audio_channel[i].state != 0)
		audio_handler(i);
	}
    }

    last_audio_cycles = cycles - n_cycles;
}


void AUDxDAT (int nr, uae_u16 v)
{
    struct audio_channel_data *cdp = audio_channel + nr;

    TEXT_SCOPE(cycles, nr, PET_DAT, v);

    update_audio ();

    cdp->dat = v;
    cdp->datpt = 0;

    if (cdp->state == 0 && !(INTREQR() & (0x80 << nr))) {
	cdp->state = 2;
	INTREQ(0x8000 | (0x80 << nr));
	/* data_written = 2 ???? */
	cdp->evtime = cdp->per;
    }
}


void AUDxLCH (int nr, uae_u16 v)
{
    TEXT_SCOPE(cycles, nr, PET_LCH, v);

    update_audio ();

    audio_channel[nr].lc = (audio_channel[nr].lc & 0xffff) | ((uae_u32)v << 16);
}


void AUDxLCL (int nr, uae_u16 v)
{
    TEXT_SCOPE(cycles, nr, PET_LCL, v);

    update_audio ();

    audio_channel[nr].lc = (audio_channel[nr].lc & ~0xffff) | (v & 0xFFFE);
}


void AUDxPER (int nr, uae_u16 v)
{
    TEXT_SCOPE(cycles, nr, PET_PER, v);

    update_audio ();

    if (v == 0)
	v = 65535;
    else if (v < 16) {
	/* With the risk of breaking super-cool players,
	   we limit the value to 16 to save cpu time on not so powerful
	   machines. robocop customs use low values for example. */
	if (!audperhack) {
	    audperhack = 1;
	    uadecore_send_debug("Eagleplayer inserted %d into aud%dper.", v, nr);
	}
	v = 16;
    }
    audio_channel[nr].per = v;
}


void AUDxLEN (int nr, uae_u16 v)
{
    TEXT_SCOPE(cycles, nr, PET_LEN, v);

    update_audio ();

    audio_channel[nr].len = v;
}


void AUDxVOL (int nr, uae_u16 v)
{
    int v2 = v & 64 ? 63 : v & 63;

    TEXT_SCOPE(cycles, nr, PET_VOL, v);

    update_audio ();

    audio_channel[nr].vol = v2;
}
