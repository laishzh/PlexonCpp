//
//   EventWait.cpp 
//
//   (c) 1999-2012 Plexon Inc. Dallas Texas 75206 
//   www.plexoninc.com
//
//   Console-mode app that reads spike timestamps and NIDAQ samples from the Server 
//   and prints the individual timestamps and samples to the console window.  Uses Win32
//   synchronization objects to wait on the server's polling event, rather than using a call
//   to Sleep().
//
//   Built using Microsoft Visual C++ 8.0.  Must include Plexon.h and link with PlexClient.lib.
//
//   See SampleClients.doc for more information.
//

#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

//** header file containing the Plexon APIs (link with PlexClient.lib, run with PlexClient.dll)
#include "../../include/plexon.h"

//** maximum number of MAP events to be read at one time from the Server
#define MAX_MAP_EVENTS_PER_READ 500000


int main(int argc, char* argv[])
{
  PL_WaveLong*  pServerEventBuffer;     //** buffer in which the Server will return MAP events
  int           NumMAPEvents;           //** number of MAP events returned from the Server
  int           NumNIDAQSamples;        //** number of samples within a NIDAQ sample block
  int           NumSpikeTimestamps;     //** number of spike timestamps in one read from server
  int           SpikeChannel;           //** DSP channel on which a spike timestamp occurred
  char          SpikeUnit;              //** a,b,c or d
  UINT          SpikeTime;              //** spike timestamp
  UINT          SampleTime;             //** timestamp a NIDAQ sample
  int           MAPSampleRate;          //** samples/sec for MAP channels
  int           NIDAQSampleRate;        //** samples/sec for NIDAQ channels
  int           ServerDropped;          //** nonzero if server dropped any data
  int           MMFDropped;             //** nonzero if MMF dropped any data
  int           PollHigh;               //** high 32 bits of polling time
  int           PollLow;                //** low 32 bits of polling time
  int           MAPEventIndex;          //** loop counter
  int           SampleIndex;            //** loop counter
  HANDLE        hServerPollEvent;       //** handle to Win32 synchronization event
  int           Dummy[64];

  //** connect to the server
  PL_InitClientEx3(0, NULL, NULL);

  //** allocate memory in which the server will return MAP events
  pServerEventBuffer = (PL_WaveLong*)malloc(sizeof(PL_WaveLong)*MAX_MAP_EVENTS_PER_READ);
  if (pServerEventBuffer == NULL)
  {
    printf("Couldn't allocate memory, I can't continue!\r\n");
    Sleep(3000); //** pause before console window closes
    return 0;
  }

  //** open the Win32 synchronization event used to synchronize with the server
  hServerPollEvent = OpenEvent(SYNCHRONIZE, FALSE, "PlexonServerEvent");
  if (!hServerPollEvent)
  {
    printf("Couldn't open server poll event, I can't continue!\r\n");
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

  // read any backlog first
  DWORD backlog = 0;
  for (;;)
  {
    NumMAPEvents = MAX_MAP_EVENTS_PER_READ;
    PL_GetLongWaveFormStructuresEx2(&NumMAPEvents, pServerEventBuffer, &ServerDropped, &MMFDropped, &PollHigh, &PollLow);
    backlog += NumMAPEvents;
    if (NumMAPEvents < MAX_MAP_EVENTS_PER_READ)
      break;
  }
  printf("read %u blocks of backlog before main loop\r\n", backlog);

  //** this loop reads from the Server, then waits until the Server event is signaled (indicating that
  //** more data is available), until the user hits Control-C
  //**
  //** note: the time between reads will actually be one second plus the amount of time required to
  //** dump the timestamps to the console

  //while (TRUE)
  for (DWORD count = 0; count < 1000; count++)
  {
    printf("\r\nreading from server\r\n");

    //** this tells the Server the max number of MAP events that can be returned to us in one read
    NumMAPEvents = MAX_MAP_EVENTS_PER_READ;

    //** call the Server to get all the MAP events since the last time we called PL_GetLongWaveFormStructuresEx2
    PL_GetLongWaveFormStructuresEx2(&NumMAPEvents, pServerEventBuffer, 
      &ServerDropped, &MMFDropped, &PollHigh, &PollLow);

    printf("[%u] t = %.3f, %u blocks\n", count, GetTickCount()*0.001f, NumMAPEvents);
    
    //** step through the array of MAP events, displaying only the NIDAQ samples
    //** note: any number of spike timestamps and blocks of NIDAQ samples may occur in any order
    NumSpikeTimestamps = 0;
    for (MAPEventIndex = 0; MAPEventIndex < NumMAPEvents; MAPEventIndex++)
    {
      //** is this MAP event the timestamp of a sorted spike? (unsorted spikes have Unit == 0)
      if (pServerEventBuffer[MAPEventIndex].Type == PL_SingleWFType && 
          pServerEventBuffer[MAPEventIndex].Unit >= 1 && 
          pServerEventBuffer[MAPEventIndex].Unit <= 4)
      {
        SpikeChannel = pServerEventBuffer[MAPEventIndex].Channel; //** DSP channel number 1,2,3...
        SpikeUnit = 'a' + (pServerEventBuffer[MAPEventIndex].Unit-1); //** unit 1,2,3,4 = a,b,c,d
        SpikeTime = pServerEventBuffer[MAPEventIndex].TimeStamp; 
        printf("SPK%d%c t=%f\r\n", SpikeChannel, SpikeUnit, (float)SpikeTime/(float)MAPSampleRate);
        NumSpikeTimestamps++;
      }
      
      //** is this MAP event a block of NIDAQ samples from the first NIDAQ channel?
      else if (pServerEventBuffer[MAPEventIndex].Type == PL_ADDataType &&
          pServerEventBuffer[MAPEventIndex].Channel == 0) 
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
          printf("value=%d t=%f\r\n", 
            pServerEventBuffer[MAPEventIndex].WaveForm[SampleIndex], (float)SampleTime/(float)MAPSampleRate);
        }
      }

    }

    if (!NumSpikeTimestamps)
      printf("no spikes\r\n");

    //** wait on the server event to indicate that another batch of MAP events are ready
    if (::WaitForSingleObject(hServerPollEvent, 10000) == WAIT_TIMEOUT)
    {
      printf("WaitForSingleObject timed out (10 secs)\r\n");
      return 0;
    }
  }

  //** in this sample, we will never get to this point, but this is how we would clean up and disconnect
  //** from the server
  CloseHandle(hServerPollEvent);
  free(pServerEventBuffer);
  PL_CloseClient();

	return 0;
}

