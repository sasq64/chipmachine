/**
 * @ingroup  file68_lib
 * @file     sc68/url68.h
 * @author   Benjamin Gerard
 * @date     2003-10-28
 * @brief    URL manipulation header.
 *
 */

/* $Id: url68.h 102 2009-03-14 17:21:58Z benjihan $ */

/* Copyright (C) 1998-2009 Benjamin Gerard */

#ifndef _FILE68_URL68_H_
#define _FILE68_URL68_H_

#ifndef FILE68_API
# include "file68_api.h"
#endif
#include "istream68.h"

/** @defgroup  file68_url  URL manipulation
 *  @ingroup   file68_lib
 *
 *     Provides functions for sc68 URL manipulation.
 *
 *  @{
 */

FILE68_API
/** Get protocol from URL string.
 *
 *  @param  protocol  buffer to store URL
 *  @param  size      protocol buffer size
 *  @param  url       UR string
 *
 *  @return error code
 *  @retval 0     success
 *  @retval -1    failure
 */
int url68_get_protocol(char * protocol, int size, const char *url);

FILE68_API
/** Test if a protocol is local.
 *
 *  @note Currently the url68_local_protocol() function tests if
 *        protocol is local and seekable.
 *
 *  @param  protocol  protocol to test
 *
 *  @return  1  protocol is local (0,"","FILE","LOCAL","NULL")
 *  @return  0  protocol may be remote
 */
int url68_local_protocol(const char * protocol);

FILE68_API
/** Create a stream for an URL.
 *
 *  Here is a list of protocols currently supported:
 *  - file:// local file
 *  - local:// alias for file://
 *  - stdin:// standard input
 *  - stdout:// standard output
 *  - stderr:// standard error
 *  - null:// null (zero) file
 *  - protocol supported by curl (HTTP, HTTPS, FTP, GOPHER,
 *       DICT,  TELNET,  LDAP  or FILE)
 *
 *  @param  url      URL or file
 *  @param  mode     open mode (1:read, 2:write).
 *
 *  @return stream
 *  @retval 0 error
 */
istream68_t * url68_stream_create(const char * url, int mode);

/**
 *  @}
 */

#endif /* #ifndef _FILE68_URL68_H_ */
