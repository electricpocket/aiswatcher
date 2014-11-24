#ifndef __MAIN_H
#define __MAIN_H


// ============================= Include files ==========================


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

//rtl_fm -f 161975000 -g 40 -p 95 -s 48k -r 48k /tmp/aisdata
    // RTLSDR
    int           dev_index;
    int           gain;
    int           enable_agc;
    int           freq;
    int           ppm_error;

    // Configuration
    char *filename;                  // Input form file, --ifile option

} Modes;


void run_rtl_fm(char **my_args);
int openTcpSocket(const char *host, const char *portname);
void modesInitConfig();
int verbose_device_search(char *s);
void modesInitRTLSDR(void);
int openSerialOut();
#endif
