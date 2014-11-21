#ifndef __MAIN_H
#define __MAIN_H


// ============================= Include files ==========================

#ifndef _WIN32
    #include "stdio.h"
    #include <string.h>
    #include <stdlib.h>
    #include <pthread.h>
    #include <stdint.h>
    #include <errno.h>
    #include <unistd.h>
    #include <math.h>
    #include <sys/time.h>
    #include <sys/timeb.h>
    #include <signal.h>
    #include <fcntl.h>
    #include <ctype.h>
    #include <sys/stat.h>
    #include <sys/ioctl.h>
    #include "rtl-sdr.h"
#else
    #include "winstubs.h" //Put everything Windows specific in here
    #include "rtl-sdr.h"
    #include "anet.h"
#endif

#define MODES_DEFAULT_PPM          52
#define MODES_DEFAULT_RATE         48000
#define MODES_DEFAULT_FREQ         161975000 //2nd ais frequency 162025000
#define MODES_DEFAULT_WIDTH        1000
#define MODES_DEFAULT_HEIGHT       700
#define MODES_ASYNC_BUF_NUMBER     16
#define MODES_ASYNC_BUF_SIZE       (16*16384)                 // 256k
#define MODES_ASYNC_BUF_SAMPLES    (MODES_ASYNC_BUF_SIZE / 2) // Each sample is 2 bytes
#define MODES_AUTO_GAIN            -100                       // Use automatic gain
#define MODES_MAX_GAIN             999999                     // Use max available gain
#define MODES_MSG_SQUELCH_LEVEL    0x02FF                     // Average signal strength limit
#define MODES_MSG_ENCODER_ERRS     3                          // Maximum number of encoding errors

// Program global state
struct {                             // Internal state


    // RTLSDR
    int           dev_index;
    int           gain;
    int           enable_agc;
    rtlsdr_dev_t *dev;
    int           freq;
    int           ppm_error;


#ifdef _WIN32
    WSADATA        wsaData;          // Windows socket initialisation
#endif

    // Configuration
    char *filename;                  // Input form file, --ifile option

    int   debug;                     // Debugging mode

    // User details
    double fUserLat;                // Users receiver/antenna lat/lon needed for initial surface location
    double fUserLon;                // Users receiver/antenna lat/lon needed for initial surface location
    int    bUserFlags;              // Flags relating to the user details




} Modes;

extern FILE *fp;

void run_rtl_fm( );

void modesInitConfig();
int verbose_device_search(char *s);
void modesInitRTLSDR(void);
#endif
