/**
 * @ingroup  file68_lib
 * @file     sc68/audio68.h
 * @author   Benjamin Gerard
 * @date     1998-09-03
 * @brief    audio backend header.
 *
 */

/* $Id: audio68.h 75 2009-02-04 19:12:14Z benjihan $ */

/* Copyright (C) 1998-2009 Benjamin Gerard */

#ifndef _FILE68_AUDIO68_H_
#define _FILE68_AUDIO68_H_

#include "file68_api.h"

/** @defgroup  file68_audio  Audio output interface.
 *  @ingroup   file68_lib
 *
 *    Provides function for controling audio output backend.
 *
 *  @{
 */

FILE68_API
/** Set/Get audio output default sampling rate.
 *
 *    Set the default sampling rate for audio output. This default
 *    rate is used by the audio output backend if no sampling rate is
 *    specified.
 *
 *  @param   hz  New default sampling rate [8000..96000]; 0:Get current.
 *  @return  Either new or current sampling rate. The value may not be
 *           the exact requested value; it depends on the granularity of
 *           the audio backend.
 */
unsigned int audio68_sampling_rate(const unsigned int hz);

/**
 *  @}
 */

#endif /* #ifndef _FILE68_AUDIO68_H_ */
