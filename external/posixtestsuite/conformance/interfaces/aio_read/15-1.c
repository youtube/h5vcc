/*
 * Copyright (c) 2004, Bull SA. All rights reserved.
 * Created by:  Laurent.Vivier@bull.net
 * This file is licensed under the GPL license.  For the full content
 * of this license, see the COPYING file at the top level of this 
 * source tree.
 */

/*
 * assertion:
 *
 * aio_read() shall fail with:
 * [EOVERFLOW] if aio_nbytes is greater than 0, aio_offset is before EOF
 * and is at or beyond the offset maximum associated with aio_fildes.
 *
 * method:
 *
 *	UNTESTED
 *
 */

#define _XOPEN_SOURCE 600
#include <unistd.h>

#include "posixtest.h"

int main()
{
#if _POSIX_ASYNCHRONOUS_IO != 200112L
	exit(PTS_UNSUPPORTED);
#endif

	return PTS_UNTESTED;
}
