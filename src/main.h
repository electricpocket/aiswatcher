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
#define MODES_DEFAULT_RATE         2000000
#define MODES_DEFAULT_FREQ         1090000000
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
    int   phase_enhance;             // Enable phase enhancement if true
    int   nfix_crc;                  // Number of crc bit error(s) to correct
    int   check_crc;                 // Only display messages with good CRC
    int   raw;                       // Raw output format
    int   beast;                     // Beast binary format output
    int   mode_ac;                   // Enable decoding of SSR Modes A & C
    int   debug;                     // Debugging mode
    int   net;                       // Enable networking
    int   net_only;                  // Enable just networking
    int   net_heartbeat_count;       // TCP heartbeat counter
    int   net_heartbeat_rate;        // TCP heartbeat rate
    int   net_output_sbs_port;       // SBS output TCP port
    int   net_output_raw_size;       // Minimum Size of the output raw data
    int   net_output_raw_rate;       // Rate (in 64mS increments) of output raw data
    int   net_output_raw_rate_count; // Rate (in 64mS increments) of output raw data
    int   net_output_raw_port;       // Raw output TCP port
    int   net_input_raw_port;        // Raw input TCP port
    int   net_output_beast_port;     // Beast output TCP port
    int   net_input_beast_port;      // Beast input TCP port
    char  *net_bind_address;         // Bind address
    int   net_http_port;             // HTTP port
    int   net_sndbuf_size;           // TCP output buffer size (64Kb * 2^n)
    int   quiet;                     // Suppress stdout
    int   interactive;               // Interactive mode
    int   interactive_rows;          // Interactive mode: max number of rows
    int   interactive_display_ttl;   // Interactive mode: TTL display
    int   interactive_delete_ttl;    // Interactive mode: TTL before deletion
    int   stats;                     // Print stats at exit in --ifile mode
    int   onlyaddr;                  // Print only ICAO addresses
    int   metric;                    // Use metric units
    int   mlat;                      // Use Beast ascii format for raw data output, i.e. @...; iso *...;
    int   interactive_rtl1090;       // flight table in interactive mode is formatted like RTL1090

    // User details
    double fUserLat;                // Users receiver/antenna lat/lon needed for initial surface location
    double fUserLon;                // Users receiver/antenna lat/lon needed for initial surface location
    int    bUserFlags;              // Flags relating to the user details




} Modes;
