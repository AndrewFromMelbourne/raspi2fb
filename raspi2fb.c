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

#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <bsd/libutil.h>

#include <linux/fb.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#include "bcm_host.h"
#pragma GCC diagnostic pop

#include "syslogUtilities.h"

//-------------------------------------------------------------------------

#define DEFAULT_DEVICE "/dev/fb1"
#define DEFAULT_DISPLAY_NUMBER 0
#define DEFAULT_FPS 10

//-------------------------------------------------------------------------

volatile bool run = true;

//-------------------------------------------------------------------------

void
printUsage(
    FILE *fp,
    const char *name)
{
    fprintf(fp, "\n");
    fprintf(fp, "Usage: %s <options>\n", name);
    fprintf(fp, "\n");
    fprintf(fp, "    --daemon - start in the background as a daemon\n");
    fprintf(fp, "    --device <device> - framebuffer device");
    fprintf(fp, " (default %s)\n", DEFAULT_DEVICE);
    fprintf(fp, "    --display <number> - Raspberry Pi display number");
    fprintf(fp, " (default %d)\n", DEFAULT_DISPLAY_NUMBER);
    fprintf(fp, "    --fps <fps> - set desired frames per second");
    fprintf(fp, " (default %d frames per second)\n", DEFAULT_FPS);
    fprintf(fp, "    --copyrect - copy only a rectangle the same size as the dest framebuffer\n");
    fprintf(fp, "    --rectx <x> - copy rectangle from source fb at <x> in copyrect mode");
    fprintf(fp, " (default 0)\n");
    fprintf(fp, "    --recty <y> - copy rectangle from source fb at <y> in copyrect mode");
    fprintf(fp, " (default 0)\n");
    fprintf(fp, "    --pidfile <pidfile> - create and lock PID file");
    fprintf(fp, " (if being run as a daemon)\n");
    fprintf(fp, "    --once - copy only one time, then exit\n");
    fprintf(fp, "    --help - print usage and exit\n");
    fprintf(fp, "\n");
}

//-------------------------------------------------------------------------

static void
signalHandler(
    int signalNumber)
{
    switch (signalNumber)
    {
    case SIGINT:
    case SIGTERM:

        run = false;
        break;
    };
}

//-------------------------------------------------------------------------

int
main(
    int argc,
    char *argv[])
{
    const char *program = basename(argv[0]);

    int fps = DEFAULT_FPS;
    suseconds_t frameDuration =  1000000 / fps;
    bool isDaemon = false;
    bool copyRect = false;
    bool once = false;
    uint16_t copyRectX = 0;
    uint16_t copyRectY = 0;
    uint32_t displayNumber = DEFAULT_DISPLAY_NUMBER;
    const char *pidfile = NULL;
    const char *device = DEFAULT_DEVICE;

    //---------------------------------------------------------------------

    static const char *sopts = "df:hn:p:D:";
    static struct option lopts[] = 
    {
        { "daemon", no_argument, NULL, 'd' },
        { "fps", required_argument, NULL, 'f' },
        { "help", no_argument, NULL, 'h' },
        { "display", required_argument, NULL, 'n' },
        { "pidfile", required_argument, NULL, 'p' },
        { "device", required_argument, NULL, 'D' },
        { "copyrect", no_argument, NULL, 'r' },
        { "rectx", required_argument, NULL, 'x' },
        { "recty", required_argument, NULL, 'y' },
        { "once", no_argument, NULL, 'o' },
        { NULL, no_argument, NULL, 0 }
    };

    int opt = 0;

    while ((opt = getopt_long(argc, argv, sopts, lopts, NULL)) != -1)
    {
        switch (opt)
        {
        case 'x':
            copyRectX = atoi(optarg);
            break;

        case 'y':
            copyRectY = atoi(optarg);
            break;

        case 'r':
            copyRect = true;
            break;

        case 'd':

            isDaemon = true;
            break;

        case 'f':

            fps = atoi(optarg);

            if (fps > 0)
            {
                frameDuration = 1000000 / fps;
            }
            else
            {
                fps = 1000000 / frameDuration;
            }

            break;

        case 'h':

            printUsage(stdout, program);
            exit(EXIT_SUCCESS);

            break;

        case 'n':

            displayNumber = atoi(optarg);

            break;

        case 'p':

            pidfile = optarg;

            break;

        case 'o':

            once = true;
            break;

        case 'D':

            device = optarg;

            break;

        default:

            printUsage(stderr, program);
            exit(EXIT_FAILURE);

            break;
        }
    }

    //---------------------------------------------------------------------

    struct pidfh *pfh = NULL;

    if (isDaemon)
    {
        if (pidfile != NULL)
        {
            pid_t otherpid;
            pfh = pidfile_open(pidfile, 0600, &otherpid);

            if (pfh == NULL)
            {
                fprintf(stderr,
                        "%s is already running %jd\n",
                        program,
                        (intmax_t)otherpid);
                exit(EXIT_FAILURE);
            }
        }

        if (daemon(0, 0) == -1)
        {
            fprintf(stderr, "Cannot daemonize\n");

            exitAndRemovePidFile(EXIT_FAILURE, pfh);
        }

        if (pfh)
        {
            pidfile_write(pfh);
        }

        openlog(program, LOG_PID, LOG_USER);
    }

    //---------------------------------------------------------------------

    if (signal(SIGINT, signalHandler) == SIG_ERR)
    {
        perrorLog(isDaemon, program, "installing SIGINT signal handler");

        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }

    //---------------------------------------------------------------------

    if (signal(SIGTERM, signalHandler) == SIG_ERR)
    {
        perrorLog(isDaemon, program, "installing SIGTERM signal handler");

        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }

    //---------------------------------------------------------------------

    bcm_host_init();

    DISPMANX_DISPLAY_HANDLE_T display
        = vc_dispmanx_display_open(displayNumber);

    if (display == 0)
    {
        messageLog(isDaemon, program, LOG_ERR, "cannot open display");
        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }

    DISPMANX_MODEINFO_T info;

    if (vc_dispmanx_display_get_info(display, &info) != 0)
    {
        messageLog(isDaemon,
                   program,
                   LOG_ERR,
                   "cannot get display dimensions");
        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }

    //---------------------------------------------------------------------

    int fbfd = open(device, O_RDWR);

    if (fbfd == -1)
    {
        perrorLog(isDaemon, program, "cannot open framebuffer device");

        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }

    struct fb_fix_screeninfo finfo;

    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1)
    {
        perrorLog(isDaemon,
                  program,
                  "cannot get framebuffer fixed information");

        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }

    struct fb_var_screeninfo vinfo;

    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1)
    {
        perrorLog(isDaemon,
                  program,
                  "cannot get framebuffer variable information");

        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }

    //---------------------------------------------------------------------

    if ((vinfo.xres * 2) != finfo.line_length)
    {
        perrorLog(isDaemon,
                  program,
                  "assumption failed ... framebuffer lines are padded");

        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }

    if ((vinfo.xres % 16) != 0)
    {
        perrorLog(isDaemon,
                  program,
                  "framebuffer width must be a multiple of 16");

        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }

    if (vinfo.bits_per_pixel != 16)
    {
        perrorLog(isDaemon,
                  program,
                  "framebuffer is not 16 bits per pixel");

        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }


    if (copyRectX > (info.width - vinfo.xres))
    {
        char s[80];
        snprintf(s, 80, "rectx must be between 0 and %d for the configured framebuffers", info.width - vinfo.xres);
        perrorLog(isDaemon,
                  program,
                  s);

        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }

    if (copyRectY > (info.height - vinfo.yres))
    {
        char s[80];
        snprintf(s, 80, "recty must be between 0 and %d for the configured framebuffers", info.height - vinfo.yres);
        perrorLog(isDaemon,
                  program,
                  s);

        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }

    //---------------------------------------------------------------------

    uint16_t *fbp = mmap(0,
                         finfo.smem_len,
                         PROT_READ | PROT_WRITE,
                         MAP_SHARED,
                         fbfd,
                         0);

    if (fbp == MAP_FAILED)
    {
        perrorLog(isDaemon, program, "cannot map framebuffer into memory");

        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }

    memset(fbp, 0, finfo.smem_len);

    //---------------------------------------------------------------------

    uint32_t image_ptr;

    DISPMANX_RESOURCE_HANDLE_T resourceHandle;
    VC_RECT_T rect;

    if (copyRect) {
        resourceHandle = vc_dispmanx_resource_create(VC_IMAGE_RGB565,
                                                     info.width,
                                                     info.height,
                                                     &image_ptr);
        vc_dispmanx_rect_set(&rect, 0, 0, info.width, info.height);
    } else {
        resourceHandle = vc_dispmanx_resource_create(VC_IMAGE_RGB565,
                                                     vinfo.xres,
                                                     vinfo.yres,
                                                     &image_ptr);
        vc_dispmanx_rect_set(&rect, 0, 0, vinfo.xres, vinfo.yres);
    }

    //---------------------------------------------------------------------

    uint32_t len = copyRect ? (info.width * info.height * 2) : finfo.smem_len;

    uint16_t *backCopyP = malloc(len);
    uint16_t *frontCopyP = malloc(len);

    uint32_t line_len = copyRect ? (info.width * 2) : finfo.line_length;

    if ((backCopyP == NULL) || (frontCopyP == NULL))
    {
        perrorLog(isDaemon, program, "cannot allocate offscreen buffers");

        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }

    memset(backCopyP, 0, copyRect ? (line_len * info.height) : finfo.line_length * vinfo.yres);

    //---------------------------------------------------------------------

    if (copyRect)
    {
        messageLog(isDaemon,
                   program,
                   LOG_INFO,
                   "raspi2fb copyrect mode, copying window from source fb[%dx%d @ %d,%d] to dest fb[%dx%d]",
                   info.width,
                   info.height,
                   copyRectX,
                   copyRectY,
                   vinfo.xres,
                   vinfo.yres);
    }
    else
    {
        messageLog(isDaemon,
                   program,
                   LOG_INFO,
                   "raspi2fb normal scaling mode, copying from source fb[%dx%d] to dest fb [%dx%d]",
                   info.width,
                   info.height,
                   vinfo.xres,
                   vinfo.yres);
    }

    //---------------------------------------------------------------------

    struct timeval start_time;
    struct timeval end_time;
    struct timeval elapsed_time;

    //---------------------------------------------------------------------

    // pixels = count of destination framebuffer pixels
    uint32_t pixels = vinfo.xres * vinfo.yres;

    while (run)
    {
        gettimeofday(&start_time, NULL);

        //-----------------------------------------------------------------

        vc_dispmanx_snapshot(display, resourceHandle, 0);

        vc_dispmanx_resource_read_data(resourceHandle,
                                       &rect,
                                       frontCopyP,
                                       line_len);

        if (copyRect)
        {
            // rectangle copying mode - eliminated double buffering, not sure why it is done in 'normal' mode
            for (uint16_t pixel_y = 0 ; pixel_y < vinfo.yres ; pixel_y++)
            {
                uint16_t* rowIter = frontCopyP + ((pixel_y + copyRectY) * info.width) + copyRectX;
                uint16_t* fbIter = fbp + (pixel_y * vinfo.xres);

                for (uint16_t pixel_x = 0 ; pixel_x < vinfo.xres ; pixel_x++)
                {
                    *(fbIter++) = *(rowIter++);
                }
            }
        }
        else
        {
            // normal scaled copy mode
            uint16_t *fbIter = fbp;
            uint16_t *frontCopyIter = frontCopyP;
            uint16_t *backCopyIter = backCopyP;

            uint32_t pixel;
            for (pixel = 0 ; pixel < pixels ; pixel++)
            {
                if (*frontCopyIter != *backCopyIter)
                {
                    *fbIter = *frontCopyIter;
                }

                ++frontCopyIter;
                ++backCopyIter;
                ++fbIter;
            }

            uint16_t *tmp = backCopyP;
            backCopyP = frontCopyP;
            frontCopyP = tmp;
        }

        //-----------------------------------------------------------------

        if (once)
        {
            messageLog(isDaemon,
                       program,
                       LOG_INFO,
                       "ran once, exiting now");
            break;
        }

        gettimeofday(&end_time, NULL);
        timersub(&end_time, &start_time, &elapsed_time);

        if (elapsed_time.tv_sec == 0)
        {
            if (elapsed_time.tv_usec < frameDuration)
            {
                usleep(frameDuration -  elapsed_time.tv_usec);
            }
        }
    }

    //---------------------------------------------------------------------

    free(frontCopyP);
    free(backCopyP);

    memset(fbp, 0, finfo.smem_len);

    munmap(fbp, finfo.smem_len);
    close(fbfd);

    //---------------------------------------------------------------------

    vc_dispmanx_resource_delete(resourceHandle);
    vc_dispmanx_display_close(display);

    //---------------------------------------------------------------------

    messageLog(isDaemon, program, LOG_INFO, "exiting");

    if (isDaemon)
    {
        closelog();
    }

    if (pfh)
    {
        pidfile_remove(pfh);
    }

    //---------------------------------------------------------------------

    return 0 ;
}
