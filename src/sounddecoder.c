/*
 *    sounddecoder.cpp
 *
 *    This file is part of AISDecoder.
 *
 *    Copyright (C) 2013
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

#include <string.h>
#include <stdio.h>
#include "config.h"

#include "receiver.h"
#include "hmalloc.h"

#define MAX_FILENAME_SIZE 512
#define ERROR_MESSAGE_LENGTH 1024
#include "sounddecoder.h"

char errorSoundDecoder[ERROR_MESSAGE_LENGTH];
static FILE *fp=NULL;
static struct receiver *rx_a=NULL;
static struct receiver *rx_b=NULL;

static short *buffer=NULL;
static int buffer_l=0;
static int buffer_read=0;
static int channels=0;
static Sound_Channels sound_channels;
static Sound_Driver driver;


static void readBuffers();


int initSoundDecoder( const Sound_Channels _channels, const Sound_Driver _driver, const char *file) {

	sound_channels = _channels;
	driver = _driver;
	channels = sound_channels == SOUND_CHANNELS_MONO ? 1 : 2;
	errorSoundDecoder[0]=0;
	char soundFile[MAX_FILENAME_SIZE+1];
	switch (driver) {
	case DRIVER_FILE:
		strncpy(soundFile, file, MAX_FILENAME_SIZE);
		soundFile[MAX_FILENAME_SIZE]=0;
		fp = fopen(soundFile, "w+b");
		if (fp) {

			buffer_l = 1024;
			int extra = buffer_l % 5;
			buffer_l -= extra;
			buffer = (short *) hmalloc(buffer_l * sizeof(short) * channels);
		} else {
			strcpy(errorSoundDecoder, "Can't open raw file for read");
			return 0;
		}
		break;

	}

	if (sound_channels == SOUND_CHANNELS_MONO) {
		rx_a = init_receiver('A', 1, 0);
	} else {
		rx_a = init_receiver('A', 2, 0);
		rx_b = init_receiver('B', 2, 1);
	}
	return 1;
}

void runSoundDecoder(int *stop) {

	while (!*stop) {
		switch (driver) {
		case DRIVER_FILE:

			buffer_read = fread(buffer, channels * sizeof(short), buffer_l, fp);

			if (buffer_read <= 0)
			{

				*stop = 1;
			}
			break;


		}
		readBuffers();
	}

}

static void readBuffers() {
	if (buffer_read <= 0) return;
	if (rx_a != NULL && sound_channels != SOUND_CHANNELS_RIGHT)
		receiver_run(rx_a, buffer, buffer_read);

	if (rx_b != NULL &&
			(sound_channels == SOUND_CHANNELS_STEREO || sound_channels == SOUND_CHANNELS_RIGHT)
	) receiver_run(rx_b, buffer, buffer_read);
}

void freeSoundDecoder() {

	if (fp != NULL) {
		fclose(fp);
		fp=NULL;
	}

	if (rx_a != NULL) {
		free_receiver(rx_a);
		rx_a=NULL;
	}

	if (rx_b != NULL) {
		free_receiver(rx_b);
		rx_b=NULL;
	}

	if (buffer != NULL) {
		hfree(buffer);
		buffer = NULL;
	}
}
