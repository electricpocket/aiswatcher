/*
 *    main.cpp  --  AIS Decoder
 *
 *    Copyright (C) 2014 Pocket Mariner Ltd (support@pocketmariner.com ) and
 *      Astra Paging Ltd / AISHub (info@aishub.net)
 *
 *    AISDecoder is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    AISDecoder uses parts of GNUAIS project (http://gnuais.sourceforge.net/)
 *
 */

#ifndef WIN32
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include "config.h"
#include "sounddecoder.h"
#include "callbacks.h"
#ifdef HAVE_ALSA
#include "alsaaudio/alsaaudio.h"
#endif
#define HELP_AUDIO_DRIVERS "file"
#ifndef WIN32
#ifdef HAVE_ALSA
#define HELP_AUDIO_DEVICE "-D\tALSA input device\n\tUse '-D list' to see all available system devices.\n"
#else
#define HELP_AUDIO_DEVICE ""
#endif
#ifdef HAVE_PULSEAUDIO
#ifdef HAVE_ALSA
#define HELP_AUDIO_DRIVERS "pulse,alsa,file"
#else
#define HELP_AUDIO_DRIVERS "pulse,file"
#endif
#else
#ifdef HAVE_ALSA
#define HELP_AUDIO_DRIVERS "alsa,file"
#endif
#endif
#else
#define HELP_AUDIO_DRIVERS "winmm,file"
#define HELP_AUDIO_DEVICE "-D\tNumeric value for input device (default WAVE_MAPPER)\n\tUse '-D list' to see all available system devices.\n"
#endif

#define HELP_MSG \
"Usage: " PROJECT_NAME " -h hostname -t protocol -p port -a " HELP_AUDIO_DRIVERS " [-f /path/file.raw] [-l]\n\n"\
"-h\tDestination host or IP address\n"\
"-p\tDestination TCP port\n"\
"-t\tProtocol 0=UDP 1=TCP (UDP default)\n"\
"-a\tAudio driver [" HELP_AUDIO_DRIVERS "]\n"  HELP_AUDIO_DEVICE\
"-c\tSound channels [stereo,mono,left,right] (default stereo)\n"\
"-f\tFull path to 48kHz raw audio file\n"\
"-l\tLog sound levels to console (stderr)\n"\
"-d\tLog NMEA sentences to console (stderr)\n"\
"-H\tDisplay this help\n"

#define MAX_BUFFER_LENGTH 2048
static char buffer[MAX_BUFFER_LENGTH];
static unsigned int buffer_count=0;

#ifdef WIN32
WSADATA wsaData;
void printInDevices() {
    unsigned int count = waveInGetNumDevs();
    WAVEINCAPS caps;
    unsigned int i = 0;
    for (i = 0; i < count; i++) {
        if (waveInGetDevCaps(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR) {
            fprintf(stderr, "%u: %s\r\n", i, caps.szPname);
        } else {
            fprintf(stderr, "Can't read devices\r\n");
            exit(EXIT_FAILURE);
        }
    }
}
#endif

#ifdef HAVE_ALSA
void printInDevices() {
    char **hints;
    /* Enumerate sound devices */
    int err = snd_device_name_hint(0, "pcm", (void***)&hints);
    if (err != 0) {
        fprintf(stderr, "Can't read data\n");
        exit(EXIT_FAILURE);
    }

    char** n = hints;
    while (*n != NULL) {
        char *name = snd_device_name_get_hint(*n, "NAME");
        char *desc = snd_device_name_get_hint(*n, "DESC");
        char *type = snd_device_name_get_hint(*n, "IOID");
        if (name != NULL && desc != NULL) {
            if (type == NULL || !strcmp(type, "Input")) {
                printf("%s\n%s: <%s>\n\n", desc, type == NULL ? "Input/Output" : "Input",name);
            }
        }
        if (name != NULL) free(name);
        if (name != NULL) free(desc);
        if (name != NULL) free(type);
        n++;
    }

    //Free hint buffer too
    snd_device_name_free_hint((void**)hints);
    snd_config_update_free_global();
}

#endif

static int sock;
static struct addrinfo* addr=NULL;
static struct sockaddr_in serv_addr;
static int protocol=0; //1 = TCP, 0 for UDP

static int initSocket(const char *host, const char *portname);
static int show_levels=0;
static int debug_nmea=0;

void sound_level_changed(float level, int channel, unsigned char high) {
    if (high != 0)
        fprintf(stderr, "Level on ch %d too high: %.0f %%\n", channel, level);
    else
        fprintf(stderr, "Level on ch %d: %.0f %%\n", channel, level);
}

void nmea_sentence_received(const char *sentence,
                          unsigned int length,
                          unsigned char sentences,
                          unsigned char sentencenum) {
    if (sentences == 1) {
        if (protocol ==0)
        {
        	if (sendto(sock, sentence, length, 0, addr->ai_addr, addr->ai_addrlen) == -1) abort();
        }
        else if (protocol==1)
        {
        	if (write(sock, sentence, length) < 0 ) abort();
        }

        if (debug_nmea) fprintf(stderr, "%s", sentence);
    } else {
        if (buffer_count + length < MAX_BUFFER_LENGTH) {
            memcpy(&buffer[buffer_count], sentence, length);
            buffer_count += length;
        } else {
            buffer_count=0;
        }

        if (sentences == sentencenum && buffer_count > 0) {
            if (sendto(sock, buffer, buffer_count, 0, addr->ai_addr, addr->ai_addrlen) == -1) abort();
            if (debug_nmea) fprintf(stderr, "%s", buffer);
            buffer_count=0;
        };
    }
}

#define CMD_PARAMS_COMMON "h:p:a:lHf:dc:"
#ifndef WIN32
#ifdef HAVE_ALSA
#define CMD_PARAMS CMD_PARAMS_COMMON "D:"
#else
#define CMD_PARAMS CMD_PARAMS_COMMON
#endif
#else
#define CMD_PARAMS CMD_PARAMS_COMMON "D:"
#endif

int main(int argc, char *argv[]) {
    Sound_Channels channels = SOUND_CHANNELS_STEREO;
    char *host, *port, *file_name=NULL;
    const char *params=CMD_PARAMS;
    int alsa=0, pulse=0, file=0, winmm=0;
    int hfnd=0, pfnd=0, afnd=0;

#ifdef WIN32
    unsigned int deviceId=WAVE_MAPPER;
#endif
#ifdef HAVE_ALSA
    char *alsaDevice=NULL;
#endif
    int opt;
    while ((opt = getopt(argc, argv, params)) != -1) {
        switch (opt) {
        case 'h':
            host = optarg;
            hfnd = 1;
            break;
        case 'p':
            port = optarg;
            pfnd = 1;
            break;
      case 't':
           protocol = atoi(optarg);
           break;
        case 'a':
        #ifdef HAVE_PULSEAUDIO
            pulse = strcmp(optarg, "pulse") == 0;
        #endif
        #ifdef HAVE_ALSA
            alsa = strcmp(optarg, "alsa") == 0;
        #endif
        #ifdef WIN32
            winmm = strcmp(optarg, "winmm") == 0;
        #endif
            file = strcmp(optarg, "file") == 0;
            afnd = 1;
            break;
        case 'c':
            if (!strcmp(optarg, "mono")) channels = SOUND_CHANNELS_MONO;
            else if (!strcmp(optarg, "left")) channels = SOUND_CHANNELS_LEFT;
            else if (!strcmp(optarg, "right")) channels = SOUND_CHANNELS_RIGHT;
            break;
        case 'l':
            show_levels = 1;
            break;
        case 'f':
            file_name = optarg;
            break;
        case 'd':
            debug_nmea = 1;
            break;
#ifdef WIN32
        case 'D':
            if (!strcmp(optarg, "list")) {
                printInDevices();
                return EXIT_SUCCESS;
            } else {
                deviceId = atoi(optarg);
            }
            break;
#endif
#ifdef HAVE_ALSA
        case 'D':
            if (!strcmp(optarg, "list")) {
                printInDevices();
                return EXIT_SUCCESS;
            } else {
                alsaDevice = optarg;
            }
            break;
#endif
        case 'H':
        default:
            fprintf(stderr, HELP_MSG);
            return EXIT_SUCCESS;
            break;
        }
    }

    if (argc < 2) {
        fprintf(stderr, HELP_MSG);
        return EXIT_FAILURE;
    }

    if (!hfnd) {
        fprintf(stderr, "Host is not set\n");
        return EXIT_FAILURE;
    }

    if (!pfnd) {
        fprintf(stderr, "Port is not set\n");
        return EXIT_FAILURE;
    }

    if (!afnd) {
        fprintf(stderr, "Audio driver is not set\n");
        return EXIT_FAILURE;
    }

    if (!alsa && !pulse && !winmm && !file) {
        fprintf(stderr, "Invalid audio driver\n");
        return EXIT_FAILURE;
    }

    if (!initSocket(host, port)) {
        return EXIT_FAILURE;
    }
    if (show_levels) on_sound_level_changed=sound_level_changed;
    on_nmea_sentence_received=nmea_sentence_received;
    Sound_Driver driver = DRIVER_FILE;
#ifdef HAVE_ALSA
    if (alsa) driver = DRIVER_ALSA;
#endif
#ifdef HAVE_PULSEAUDIO
    if (pulse) driver = DRIVER_PULSE;
#endif
    int OK=0;
#ifdef WIN32
    if (!file) driver = DRIVER_WINMM;
    OK=initSoundDecoder(channels, driver, file_name, deviceId);
#else
#ifdef HAVE_ALSA
    OK=initSoundDecoder(channels, driver, file_name, alsaDevice);
#else
    int status = mkfifo (file_name,  0666); //create the fifo for communicating between rts-sdr and the decoder
    if (status < 0 && errno != EEXIST) {
               fprintf(stderr, "Can't create fifo\n");
               return EXIT_FAILURE;
           }
    OK=initSoundDecoder(channels, driver, file_name);
#endif
#endif
    int stop=0;
    if (OK) {
        runSoundDecoder(&stop);
    } else {
        fprintf(stderr, "%s\n", errorSoundDecoder);
    }
    freeSoundDecoder();
    freeaddrinfo(addr);
#ifdef WIN32
    WSACleanup();
#endif
    return 0;
}

int initSocket(const char *host, const char *portname) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));


    hints.ai_family=AF_UNSPEC;
    hints.ai_socktype=SOCK_DGRAM;
    hints.ai_protocol=IPPROTO_UDP;
    if (protocol==1)
    {
        hints.ai_family=AF_INET;
        hints.ai_socktype=SOCK_STREAM;
        hints.ai_protocol=0; //let the server decide
    }

#ifndef WIN32
    hints.ai_flags=AI_ADDRCONFIG;
#else

    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 0;
    }
#endif
    int err=getaddrinfo(host, portname, &hints, &addr);
    if (err!=0) {
        fprintf(stderr, "Failed to resolve remote socket address!\n");
#ifdef WIN32
        WSACleanup();
#endif
        return 0;
    }

    sock=socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    if (sock==-1) {
        fprintf(stderr, "%s",strerror(errno));


#ifdef WIN32
        WSACleanup();
#endif
        return 0;
    }
    if (protocol ==1 )
    {
        struct hostent *server;
        server = gethostbyname(host);
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr,
              (char *)&serv_addr.sin_addr.s_addr,
              server->h_length);
        int portno=atoi(portname);
        serv_addr.sin_port = htons(portno);

        if (connect(sock,&serv_addr,sizeof(serv_addr)) < 0)
        {
            fprintf(stderr, "Failed to connect to remote socket address!\n");
            return 0;
        }
    }
    return 1;
}

