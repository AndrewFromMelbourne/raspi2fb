//-------------------------------------------------------------------------
//
// The MIT License (MIT)
//
// Copyright (c) 2015 Andrew Duncan
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//-------------------------------------------------------------------------

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "syslogUtilities.h"

//-------------------------------------------------------------------------

void
messageLog(
    bool isDaemon,
    const char *name,
    int priority,
    const char *format,
    ...)
{
    va_list args;
    va_start(args, format);

    if (isDaemon)
    {
        vsyslog(LOG_MAKEPRI(LOG_USER, priority), format, args);
    }
    else
    {
        fprintf(stderr, "%s[%d]:", name, getpid());

        switch (priority)
        {
        case LOG_DEBUG:

            fprintf(stderr, "debug");
            break;

        case LOG_INFO:

            fprintf(stderr, "info");
            break;

        case LOG_NOTICE:

            fprintf(stderr, "notice");
            break;

        case LOG_WARNING:

            fprintf(stderr, "warning");
            break;

        case LOG_ERR:

            fprintf(stderr, "error");
            break;

        default:

            fprintf(stderr, "unknown(%d)", priority);
            break;
        }

        fprintf(stderr, ":");

        vfprintf(stderr, format, args);

        fprintf(stderr, "\n");
    }

    va_end(args);
}

//-------------------------------------------------------------------------

void
perrorLog(
    bool isDaemon,
    const char *name,
    const char *s)
{
    messageLog(isDaemon,
               name,
               LOG_ERR,
               "%s -  %s",
               s,
               strerror(errno));
}

//-------------------------------------------------------------------------

void
exitAndRemovePidFile(
    int status,
    struct pidfh *pfh)
{
    if (pfh)
    {
        pidfile_remove(pfh);
    }

    exit(status);
}
