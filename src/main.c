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
#include "main.h"
#include <unistd.h>
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
		"Usage: " PROJECT_NAME " -h hostname -p port [-t protocol]  [-f /path/file.raw] [-l] [-d] [-D] [-G] [-C] [-F]\n\n"\
		"-h\tDestination host or IP address\n"\
		"-p\tDestination port\n"\
		"-t\tProtocol 0=UDP 1=TCP (UDP default)\n"\
		"-f\tFull path to 48kHz raw audio file (default /tmp/aisdata)\n"\
		"-l\tLog sound levels to console (stderr)\n"\
		"-d\tLog NMEA sentences to console (stderr)\n"\
		"-D <index>   Select RTL device (default: 0)\n"\
		"-G db  Set gain (default: max gain. Use -10 for auto-gain)\n"\
		"-C  Enable the Automatic Gain Control (default: off)\n"\
		"-F hz  Set frequency (default: 161.97 Mhz)\n"\
		"-H\tDisplay this help\n"

#define MAX_BUFFER_LENGTH 2048
static char buffer[MAX_BUFFER_LENGTH];
static unsigned int buffer_count=0;

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
	Sound_Channels channels = SOUND_CHANNELS_MONO;
	char *host, *port, *file_name=NULL;
	const char *params=CMD_PARAMS;
	int file=0;
	int hfnd=0, pfnd=0, afnd=0;


#ifdef COMBINED_RTL_SDR
	modesInitConfig();
#endif

#ifdef WIN32
	unsigned int deviceId=WAVE_MAPPER;
#endif
#ifdef HAVE_ALSA
	char *alsaDevice=NULL;
#endif
	int opt;
	//default to read from fifo
	file = 1;
	afnd = 1;
	protocol=0;
	channels = SOUND_CHANNELS_MONO;
	file_name="/tmp/aisdata";
	while ((opt = getopt(argc, argv, params)) != -1) {
		switch (opt) {
		//rtl-sdr options
		case 'D':
#ifdef COMBINED_RTL_SDR
			Modes.dev_index = verbose_device_search(optarg);
#endif
			break;
		case 'G':
			Modes.gain = (int) (atof(optarg)*10); // Gain is in tens of DBs
			break;
		case 'C':
			Modes.enable_agc++;
			break;
		case 'F':
			Modes.freq = (int) strtoll(optarg,NULL,10);
			break;
		case 'P':
			Modes.ppm_error = atoi(optarg);
			break;
			//aisdecoder options
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
		case 'l':
			show_levels = 1;
			break;
		case 'f':
			file_name = optarg;
			break;
		case 'd':
			debug_nmea = 1;
			break;
		case 'H':
			fprintf(stderr, HELP_MSG);
			return EXIT_SUCCESS;
			break;
		default:
			fprintf(stderr, "%c\n",(char)opt);
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

	Modes.filename=file_name;


	if (!initSocket(host, port)) {
		return EXIT_FAILURE;
	}



	//aisdecoder startup
	if (show_levels) on_sound_level_changed=sound_level_changed;
	on_nmea_sentence_received=nmea_sentence_received;
	Sound_Driver driver = DRIVER_FILE;

	int OK=0;

	int status = mkfifo (file_name,  0666); //create the fifo for communicating between rts-sdr and the decoder
	if (status < 0 && errno != EEXIST) {
		fprintf(stderr, "Can't create fifo\n");
		return EXIT_FAILURE;
	}
	// rtl-sdr Initialization

	pid_t pID = fork();

	if (pID < 0)            // failed to fork
	{
		fprintf(stderr, "failed to fork\n");
		exit(1);
		// Throw exception
	}



	//We need to bring in the rtl_fm code here to interact with rtl_sdr to demod and give
	//write audio sample out to our fifo file
	// fprintf(stderr, "Giving rtl_fm time to start\n");
	if (pID == 0)                // child
	{
		// Code only executed by child process
		run_rtl_fm();
	}
	else if (pID)                // child
	{

		fprintf(stderr, "Starting sound decoder first\n");

		OK=initSoundDecoder(channels, driver, file_name);


		int stop=0;
		fprintf(stderr, "running sound decoder %d  \n",OK);
		if (OK) {
			runSoundDecoder(&stop);
		} else {
			fprintf(stderr, "%s\n", errorSoundDecoder);
		}
		freeSoundDecoder();
		freeaddrinfo(addr);
	}
#ifdef WIN32
	WSACleanup();
#endif
	return 0;
}

void run_rtl_fm( )
{

	char *my_args[8];

	my_args[0] = "rtl_fm";
	/*
	         my_args[1] = "-h";
	         my_args[2] = NULL;
	 */
	my_args[1] = "-f 161975000";
	my_args[2] = "-g 40";
	my_args[3] = "-p 95";
	my_args[4] = "-s 48k";
	my_args[5] = "-r 48k";
	my_args[6] = Modes.filename;

	my_args[7] = NULL;


	fprintf(stderr, "Spawning rtl_fm\n");

	int succ = execv( "/usr/local/bin/rtl_fm", my_args);
	if (succ) fprintf(stderr, "Spawning rtl_fm failed %d\n",succ);



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

#ifdef COMBINED_RTL_SDR

void modesInitConfig(void) {
	// Default everything to zero/NULL
	memset(&Modes, 0, sizeof(Modes));

	// Now initialise things that should not be 0/NULL to their defaults
	Modes.gain                    = MODES_MAX_GAIN;
	Modes.freq                    = MODES_DEFAULT_FREQ;
	Modes.ppm_error               = MODES_DEFAULT_PPM;
	Modes.dev_index                 = -1; //not given

}

void modesInitRTLSDR(void) {
	int j;
	int device_count;
	char vendor[256], product[256], serial[256];

	device_count = rtlsdr_get_device_count();
	if (!device_count) {
		fprintf(stderr, "No supported RTLSDR devices found.\n");
		exit(1);
	}

	fprintf(stderr, "Found %d device(s):\n", device_count);
	for (j = 0; j < device_count; j++) {
		rtlsdr_get_device_usb_strings(j, vendor, product, serial);
		fprintf(stderr, "%d: %s, %s, SN: %s %s\n", j, vendor, product, serial,
				(j == Modes.dev_index) ? "(currently selected)" : "");
	}

	if (rtlsdr_open(&Modes.dev, Modes.dev_index) < 0) {
		fprintf(stderr, "Error opening the RTLSDR device: %s\n",
				strerror(errno));
		exit(1);
	}

	// Set gain, frequency, sample rate, and reset the device
	rtlsdr_set_tuner_gain_mode(Modes.dev,
			(Modes.gain == MODES_AUTO_GAIN) ? 0 : 1);
	if (Modes.gain != MODES_AUTO_GAIN) {
		if (Modes.gain == MODES_MAX_GAIN) {
			// Find the maximum gain available
			int numgains;
			int gains[100];

			numgains = rtlsdr_get_tuner_gains(Modes.dev, gains);
			Modes.gain = gains[numgains-1];
			fprintf(stderr, "Max available gain is: %.2f\n", Modes.gain/10.0);
		}
		rtlsdr_set_tuner_gain(Modes.dev, Modes.gain);
		fprintf(stderr, "Setting gain to: %.2f\n", Modes.gain/10.0);
	} else {
		fprintf(stderr, "Using automatic gain control.\n");
	}
	rtlsdr_set_freq_correction(Modes.dev, Modes.ppm_error);
	if (Modes.enable_agc) rtlsdr_set_agc_mode(Modes.dev, 1);
	rtlsdr_set_center_freq(Modes.dev, Modes.freq);
	rtlsdr_set_sample_rate(Modes.dev, MODES_DEFAULT_RATE);
	rtlsdr_reset_buffer(Modes.dev);
	fprintf(stderr, "Gain reported by device: %.2f\n",
			rtlsdr_get_tuner_gain(Modes.dev)/10.0);
}

int verbose_device_search(char *s)
{
	int i, device_count, device, offset;
	char *s2;
	char vendor[256], product[256], serial[256];
	device_count = rtlsdr_get_device_count();
	if (!device_count) {
		fprintf(stderr, "No supported devices found.\n");
		return -1;
	}
	fprintf(stderr, "Found %d device(s):\n", device_count);
	for (i = 0; i < device_count; i++) {
		rtlsdr_get_device_usb_strings(i, vendor, product, serial);
		fprintf(stderr, "  %d:  %s, %s, SN: %s\n", i, vendor, product, serial);
	}
	fprintf(stderr, "\n");
	/* does string look like raw id number */
	device = (int)strtol(s, &s2, 0);
	if (s2[0] == '\0' && device >= 0 && device < device_count) {
		fprintf(stderr, "Using device %d: %s\n",
				device, rtlsdr_get_device_name((uint32_t)device));
		return device;
	}
	/* does string exact match a serial */
	for (i = 0; i < device_count; i++) {
		rtlsdr_get_device_usb_strings(i, vendor, product, serial);
		if (strcmp(s, serial) != 0) {
			continue;}
		device = i;
		fprintf(stderr, "Using device %d: %s\n",
				device, rtlsdr_get_device_name((uint32_t)device));
		return device;
	}
	/* does string prefix match a serial */
	for (i = 0; i < device_count; i++) {
		rtlsdr_get_device_usb_strings(i, vendor, product, serial);
		if (strncmp(s, serial, strlen(s)) != 0) {
			continue;}
		device = i;
		fprintf(stderr, "Using device %d: %s\n",
				device, rtlsdr_get_device_name((uint32_t)device));
		return device;
	}
	/* does string suffix match a serial */
	for (i = 0; i < device_count; i++) {
		rtlsdr_get_device_usb_strings(i, vendor, product, serial);
		offset = strlen(serial) - strlen(s);
		if (offset < 0) {
			continue;}
		if (strncmp(s, serial+offset, strlen(s)) != 0) {
			continue;}
		device = i;
		fprintf(stderr, "Using device %d: %s\n",
				device, rtlsdr_get_device_name((uint32_t)device));
		return device;
	}
	fprintf(stderr, "No matching devices found.\n");
	return -1;
}
#endif

