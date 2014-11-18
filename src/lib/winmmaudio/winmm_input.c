#include "winmm_input.h"
#include <ksmedia.h>
#include <stdio.h>

#ifdef EXTENSIBLE
#define _KSDATAFORMAT_SUBTYPE_PCM            (GUID) {0x00000001,0x0000,0x0010,{0x80,0x00,0x00,0xaa,0x00,0x38,0x9b,0x71}}
#endif

static WAVEHDR wave_chunk[WAVE_BUFFERS];
static LPWAVEHDR current;
static unsigned int buffer_size=0;

static void CALLBACK waveInProc(				// wave in I/O completion procedure
    HWAVEIN hwi, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2
) {
    switch (uMsg) {
    case WIM_DATA:
        current = (LPWAVEHDR)dwParam1;
        SetEvent((HANDLE)dwInstance);
        break;
    }
}

int winmm_init(unsigned int devId,
               LPHWAVEIN device,
               int channels,
               int bytesCount,
               HANDLE *eventHandler
) {

    *eventHandler=CreateEvent(NULL, FALSE, FALSE, NULL);
    if (*eventHandler == NULL) return 0;

#ifndef EXTENSIBLE
    WAVEFORMATEX Format;
    Format.cbSize = 0;
    Format.wFormatTag = WAVE_FORMAT_PCM;
    Format.nChannels = channels;
    Format.wBitsPerSample = 16;
    Format.nSamplesPerSec = 48000L;
    Format.nBlockAlign = Format.nChannels * Format.wBitsPerSample / 8;
    Format.nAvgBytesPerSec = Format.nSamplesPerSec * Format.nBlockAlign;
    LPWAVEFORMATEX pFmt=&Format;
#else
    WAVEFORMATEXTENSIBLE fmt;
    fmt.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
    fmt.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    fmt.Format.nChannels = channels;
    fmt.Format.wBitsPerSample = 16;
    fmt.Format.nSamplesPerSec = 48000L;
    fmt.Format.nBlockAlign = fmt.Format.nChannels * fmt.Format.wBitsPerSample / 8;
    fmt.Format.nAvgBytesPerSec = fmt.Format.nSamplesPerSec * fmt.Format.nBlockAlign;
    fmt.SubFormat=_KSDATAFORMAT_SUBTYPE_PCM;
    fmt.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    fmt.Samples.wReserved=0;
    fmt.Samples.wValidBitsPerSample=16;
    LPWAVEFORMATEX pFmt=&fmt.Format;
#endif

    MMRESULT err = waveInOpen(device, devId, pFmt, (DWORD)waveInProc, (DWORD)*eventHandler, CALLBACK_FUNCTION);
    if (err == MMSYSERR_NOERROR) {
        buffer_size=sizeof(WAVEHDR);
        size_t i;
        for (i=0; i<WAVE_BUFFERS; i++) {
            ZeroMemory(&wave_chunk[i], buffer_size);
            wave_chunk[i].lpData = malloc(bytesCount);
            wave_chunk[i].dwBufferLength = bytesCount;
            wave_chunk[i].dwUser = pFmt->nBlockAlign;
            ZeroMemory(wave_chunk[i].lpData, wave_chunk[i].dwBufferLength);
            if (waveInPrepareHeader(*device, &wave_chunk[i], buffer_size) == MMSYSERR_NOERROR) {
                waveInAddBuffer(*device, &wave_chunk[i], buffer_size);
            } else return 0;
        }
        return 1;
    }
    return 0;
}

int winmm_getRecorded(LPHWAVEIN device, char *out) {
    int number=current->dwBytesRecorded/current->dwUser;
    if (number > 0) {
        memcpy(out, current->lpData, current->dwBytesRecorded);
    }
    waveInUnprepareHeader(*device, current, buffer_size);
    current->dwBytesRecorded=0;
    current->dwFlags=0;
    waveInPrepareHeader(*device, current, buffer_size);
    waveInAddBuffer(*device, current, buffer_size);
    current=NULL;
    return number;
}

void winmm_cleanup(LPHWAVEIN device, HANDLE *eventHandler) {
    if (device != NULL) {
        waveInStop(*device);
        size_t i;
        for (i=0; i<WAVE_BUFFERS; i++) {
            waveInUnprepareHeader(*device, &wave_chunk[i], wave_chunk[i].dwBufferLength);
        }
        waveInClose(*device);
        *device=0;
        if (*eventHandler != NULL) {
            CloseHandle(*eventHandler);
            eventHandler=NULL;
        }
    }
}
