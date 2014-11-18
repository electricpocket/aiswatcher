#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>

//#ifdef EXTENSIBLE
#define WAVE_BUFFERS 4

#ifdef __cplusplus
extern "C" {
#endif

int winmm_init(unsigned int devId,
               LPHWAVEIN device,
               int channels,
               int bytesCount,
               HANDLE *eventHandler);

int winmm_getRecorded(LPHWAVEIN device, char *out);
void winmm_cleanup(LPHWAVEIN device, HANDLE *eventHandler);

#ifdef __cplusplus
}
#endif
