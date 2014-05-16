//
//   NIDAQRead.cpp 
//
//   (c) 1999-2006 Plexon Inc. Dallas Texas 75206 
//   www.plexoninc.com
//
//   Simple console-mode app that reads NIDAQ samples from the Server 
//   and prints the individual samples to the console window.  Note: only the
//   first NIDAQ channel is used, and the NIDAQ sampling rate should be low
//   (100 Hz or less) to minimize the amount of text dumped to the screen.  
//
//   Built using Microsoft Visual C++ 7.1.  Must include Plexon.h and link with PlexClient.lib.
//
//   See SampleClients.rtf for more information.
//

#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

//** header file containing the Plexon APIs (link with PlexClient.lib, run with PlexClient.dll)
#include "../../include/plexon.h"

//** maximum number of MAP events to be read at one time from the Server
#define MAX_MAP_EVENTS_PER_READ 8000


int main(int argc, char* argv[])
{
  PL_WaveLong*  pServerEventBuffer;     //** buffer in which the Server will return MAP events
  int           NumMAPEvents;           //** number of MAP events returned from the Server
  int           NumNIDAQSamples;        //** number of samples within a NIDAQ sample block 
  UINT          SampleTime;             //** timestamp a NIDAQ sample
  int           MAPSampleRate;          //** samples/sec for MAP channels
  int           NIDAQSampleRate;        //** samples/sec for NIDAQ channels
  int           ServerDropped;          //** nonzero if server dropped any data
  int           MMFDropped;             //** nonzero if MMF dropped any data
  int           PollHigh;               //** high 32 bits of polling time
  int           PollLow;                //** low 32 bits of polling time
  int           MAPEventIndex;          //** loop counter
  int           SampleIndex;            //** loop counter
  int           Dummy[64];

  //** connect to the server
  PL_InitClientEx3(0, NULL, NULL);

  //** check to make sure SortClient is running
  if (!PL_IsSortClientRunning())
  {
    printf("The SortClient is not running, I can't continue!\r\n");
    Sleep(3000); //** pause before console window closes
    return 0; 
  }

  //** allocate memory in which the server will return MAP events
  pServerEventBuffer = (PL_WaveLong*)malloc(sizeof(PL_WaveLong)*MAX_MAP_EVENTS_PER_READ);
  if (pServerEventBuffer == NULL)
  {
    printf("Couldn't allocate memory, I can't continue!\r\n");
    Sleep(3000); //** pause before console window closes
    return 0;
  }

  //** get the MAP sampling rate (spike timestamps)
  switch (PL_GetTimeStampTick()) //** returns timestamp resolution in microseconds
  {
    case 25: //** 25 usec = 40 kHz, default
      MAPSampleRate = 40000;
      break;
    case 40: //** 40 usec = 25 kHz
      MAPSampleRate = 25000;
      break;
    case 50: //** 50 usec = 20 kHz
      MAPSampleRate = 20000;
      break;
    default:
      printf("Unsupported MAP sampling time, I can't continue!\r\n");
      Sleep(3000); //** pause before console window closes
      return 0;
  }
  
  //** get the NIDAQ sampling rate
  PL_GetSlowInfo(&NIDAQSampleRate, Dummy, Dummy); //** last two params are unused here

  //** this loop reads from the Server once per second until the user hits Control-C
  //** note: the time between reads will actually be one second plus the amount of time required to
  //** dump the timestamps to the console 
  while (TRUE)
  {
    printf("reading from server\r\n");

    //** this tells the Server the max number of MAP events that can be returned to us in one read
    NumMAPEvents = MAX_MAP_EVENTS_PER_READ;

    //** call the Server to get all the MAP events since the last time we called PL_GetLongWaveFormStructuresEx2
    PL_GetLongWaveFormStructuresEx2(&NumMAPEvents, pServerEventBuffer, 
      &ServerDropped, &MMFDropped, &PollHigh, &PollLow);

    //** step through the array of MAP events, displaying only the NIDAQ samples
    for (MAPEventIndex = 0; MAPEventIndex < NumMAPEvents; MAPEventIndex++)
    {
      //** is this a block of NIDAQ samples from the first NIDAQ channel?
      if (pServerEventBuffer[MAPEventIndex].Type == PL_ADDataType &&
          pServerEventBuffer[MAPEventIndex].Channel == 0) //** channel 0 (ACH0) only to reduce amount of output
      {
        NumNIDAQSamples = pServerEventBuffer[MAPEventIndex].NumberOfDataWords;
        SampleTime = pServerEventBuffer[MAPEventIndex].TimeStamp;
        printf("%d samples:\r\n", NumNIDAQSamples);
        for (SampleIndex = 0; SampleIndex < NumNIDAQSamples; SampleIndex++)
        {
          //** note: the ratio MAPSampleRate/NIDAQSampleRate is always an integer
          if (SampleIndex > 0)
            SampleTime += MAPSampleRate/NIDAQSampleRate; 
          //** this printf generates the majority of the (voluminous) screen output
          //** note: samples are signed 12 bit, i.e. range of +/- 2047
          printf("value=%d t=%f\r\n", 
            pServerEventBuffer[MAPEventIndex].WaveForm[SampleIndex], (float)SampleTime/(float)MAPSampleRate);
        }
      }
    }

    //** yield to other programs for 1000 msec before calling the Server again
    Sleep(1000);
  }

  //** in this sample, we will never get to this point, but this is how we would free the 
  //** allocated memory and disconnect from the Server
  free(pServerEventBuffer);
  PL_CloseClient();

	return 0;
}

