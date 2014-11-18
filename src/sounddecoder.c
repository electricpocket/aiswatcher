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
#ifdef HAVE_ALSA
#include "alsaaudio/alsaaudio.h"
#endif
#ifdef HAVE_PULSEAUDIO
#include "pulseaudio/pulseaudio.h"
#endif
#ifdef WIN32
#include "winmmaudio/winmm_input.h"
#endif

#define MAX_FILENAME_SIZE 512
#define ERROR_MESSAGE_LENGTH 1024
#include "sounddecoder.h"


char errorSoundDecoder[ERROR_MESSAGE_LENGTH];

static struct receiver *rx_a=NULL;
static struct receiver *rx_b=NULL;

static short *buffer=NULL;
static int buffer_l=0;
static int buffer_read=0;
static int channels=0;
static Sound_Channels sound_channels;
static Sound_Driver driver;
static FILE *fp=NULL;

static void readBuffers();

#ifdef HAVE_ALSA
static snd_pcm_t *pcm=NULL;
#endif
#ifdef HAVE_PULSEAUDIO
static pa_simple *pa_dev=NULL;
#endif
#ifdef WIN32
static HWAVEIN winmm_device=0;
static HANDLE winmm_event=0;
#endif

#ifndef WIN32
#ifdef HAVE_ALSA
int initSoundDecoder(const Sound_Channels _channels, const Sound_Driver _driver,const char *file, const char *_alsaDevice) {
#else
int initSoundDecoder(const Sound_Channels _channels, const Sound_Driver _driver, const char *file) {
#endif
#else
int initSoundDecoder(const Sound_Channels _channels, const Sound_Driver _driver, const char *file, unsigned int deviceId) {
#endif
    sound_channels = _channels;
    driver = _driver;
    channels = sound_channels == SOUND_CHANNELS_MONO ? 1 : 2;
    errorSoundDecoder[0]=0;
    char soundFile[MAX_FILENAME_SIZE+1];
    switch (driver) {
    case DRIVER_FILE:
        strncpy(soundFile, file, MAX_FILENAME_SIZE);
        soundFile[MAX_FILENAME_SIZE]=0;
        fp = fopen(soundFile, "rb");
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
#ifdef HAVE_PULSEAUDIO
    case DRIVER_PULSE:
        if((pa_dev = pulseaudio_initialize(channels, NULL)) == NULL){
            pa_dev=NULL;
            strcpy(errorSoundDecoder, "Can't initialize pulse audio");
            return 0;
        } else {
            buffer_l = 1024;
            int extra = buffer_l % 5;
            buffer_l -= extra;
            buffer = (short *) hmalloc(buffer_l * sizeof(short) * channels);
        }
        break;
#endif
#ifdef HAVE_ALSA
    case DRIVER_ALSA:
        if (snd_pcm_open(&pcm, (_alsaDevice != NULL ? _alsaDevice : "default"), SND_PCM_STREAM_CAPTURE, 0) < 0 ) {
            strcpy(errorSoundDecoder, "Can't open default capture device");
            return 0;
        } else {
            if (input_initialize(pcm, &buffer, &buffer_l, channels, errorSoundDecoder) < 0) return 0;
        }
        break;
#endif
#ifdef WIN32
    case DRIVER_WINMM:
        buffer_l = 4096;
        int buffer_len_in_bytes = buffer_l * sizeof(short) * channels;
        buffer = (short *) hmalloc(buffer_len_in_bytes);
        if (!winmm_init(deviceId, &winmm_device, channels, buffer_len_in_bytes, &winmm_event)) {
            strcpy(errorSoundDecoder, "Can't initialize windows audio");
            return 0;
        }
        break;
#endif
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
#ifdef WIN32
    waveInStart(winmm_device);
#endif
    while (!*stop) {
        switch (driver) {
        case DRIVER_FILE:
            buffer_read = fread(buffer, channels * sizeof(short), buffer_l, fp);
            if (buffer_read <= 0) *stop = 1;
            break;

#ifdef HAVE_PULSEAUDIO
        case DRIVER_PULSE:
            buffer_read = pulseaudio_read(pa_dev, buffer, buffer_l, channels);
            break;
#endif
#ifdef HAVE_ALSA
        case DRIVER_ALSA:
            buffer_read = alsa_read(pcm, buffer, buffer_l);
            break;
#endif
#ifdef WIN32
        case DRIVER_WINMM:
            buffer_read=0;
            switch(WaitForSingleObject(winmm_event, 2000)) {
            case WAIT_TIMEOUT:
                buffer_read=0;
                break;
            case WAIT_OBJECT_0:
                buffer_read = winmm_getRecorded(&winmm_device, (char*)buffer);
                break;
            }
            break;
            ResetEvent(winmm_event);
#endif
        }
        readBuffers();
    }
#ifdef WIN32
    waveInStop(winmm_device);
#endif
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
#ifdef HAVE_PULSEAUDIO
    if (pa_dev) {
        pulseaudio_cleanup(pa_dev);
        pa_dev=NULL;
    }
#endif

#ifdef HAVE_ALSA
    if (pcm != NULL) {
        input_cleanup(pcm);
        pcm = NULL;
    }
#endif
#ifdef WIN32
    if (winmm_device) {
        winmm_cleanup(&winmm_device, &winmm_event);
    }
#endif
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
