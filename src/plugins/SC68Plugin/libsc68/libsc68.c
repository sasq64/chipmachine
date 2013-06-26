/*
 *                sc68 - version Copyright (C) 2001-2009
 *    Ben(jamin) Gerard <benjihan -4t- users.sourceforge -d0t- net>
 *
 * This  program is  free  software: you  can  redistribute it  and/or
 * modify  it under the  terms of  the GNU  General Public  License as
 * published by the Free Software  Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
 * MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
 * General Public License for more details.
 *
 * You should have  received a copy of the  GNU General Public License
 * along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 */

/* $Id: libsc68.c 106 2009-05-11 04:58:26Z benjihan $ */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "sc68.h"

#ifndef PACKAGE_STRING
# define PACKAGE_STRING "libsc68 n/a"
#endif

#ifndef PACKAGE_VERNUM
# define PACKAGE_VERNUM 0
#endif

const char * sc68_versionstr(void)
{
  return PACKAGE_STRING;
}

int sc68_version(void)
{
  return PACKAGE_VERNUM;
}
