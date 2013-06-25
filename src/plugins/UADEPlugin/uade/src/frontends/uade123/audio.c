#include "audio.h"
#include "uade123.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <ao/ao.h>

static ao_sample_format format;

static ao_device *libao_device = NULL;

static ao_option *options = NULL;

void audio_close(void)
{
	if (libao_device == NULL)
		return;

	/* Work-around libao/alsa, sleep 10ms to drain audio stream. */
	if (uade_output_file_name[0] == 0)
		usleep(10000);

	ao_close(libao_device);
}

static void process_config_options(const struct uade_state *us, char **opts)
{
	char *s;
	char *key;
	char *value;
	char **opt;

	if (buffertime > 0) {
		char val[32];
		/* buffer_time is given in milliseconds, so convert to microseconds */
		snprintf(val, sizeof val, "%d", 1000 * buffertime);
		ao_append_option(&options, "buffer_time", val);
	}

	format.bits = UADE_BYTES_PER_SAMPLE * 8;
	format.channels = UADE_CHANNELS;
	format.rate = uade_get_sampling_rate(us);
	format.byte_format = AO_FMT_NATIVE;

	opt = opts;
	while (opt != NULL && *opt != NULL) {
		s = strdup(*opt);
		if (s == NULL)
			uade_die("Can not allocate memory for ao option\n");

		key = s;
		value = strchr(key, ':');
		if (value == NULL)
			uade_die("uade: Invalid ao option: %s\n", s);
		*value = 0;
		value++;
		ao_append_option(&options, key, value);

		free(s);
		opt++;
	}
}

int audio_init(const struct uade_state *us, char **opts)
{
	int driver;

	if (uade_no_audio_output)
		return 1;

	process_config_options(us, opts);

	ao_initialize();

	if (uade_output_file_name[0]) {
		driver = ao_driver_id(uade_output_file_format[0] ? uade_output_file_format : "wav");
		if (driver < 0) {
			fprintf(stderr, "Invalid libao driver\n");
			return 0;
		}
		libao_device = ao_open_file(driver, uade_output_file_name, 1, &format, NULL);
	} else {
		driver = ao_default_driver_id();
		libao_device = ao_open_live(driver, &format, options);
	}

	if (libao_device == NULL)
		fprintf(stderr, "Can not open ao device: %d\n", errno);

	return libao_device != NULL;
}


int audio_play(char *samples, int bytes)
{
	if (libao_device == NULL)
		return bytes;

	/* ao_play returns 0 on failure */
	return ao_play(libao_device, samples, bytes);
}
