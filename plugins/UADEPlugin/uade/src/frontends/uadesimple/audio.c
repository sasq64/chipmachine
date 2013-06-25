#include "audio.h"

#include <uade/uade.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ao/ao.h>

static ao_device *libao_device = NULL;

void audio_close(void)
{
	if (libao_device) {
		/* Work-around libao/alsa, sleep 10ms to drain audio stream. */
		usleep(10000);

		ao_close(libao_device);
	}
}


int audio_init(int frequency)
{
	int driver;
	ao_sample_format format = {.bits = UADE_BYTES_PER_SAMPLE * 8,
				   .channels = UADE_CHANNELS,
				   .rate = frequency,
				   .byte_format = AO_FMT_NATIVE,
				  };

	ao_initialize();

	driver = ao_default_driver_id();
	libao_device = ao_open_live(driver, &format, NULL);
	if (libao_device == NULL)
		fprintf(stderr, "Error opening device: errno %d\n", errno);
	return libao_device != NULL;
}


int audio_play(char *samples, int bytes)
{
	/* ao_play returns 0 on failure */
	return ao_play(libao_device, samples, bytes);
}
