//
//   TimeStampRead.cpp 
//
//   (c) 1999-2012 Plexon Inc. Dallas Texas 75206 
//   www.plexoninc.com
//
//   Simple console-mode app that reads spike timestamps from the Server 
//   and prints the individual timestamps to the console window.  
//
//   Built using Microsoft Visual C++ 8.0.  Must include Plexon.h and link with PlexClient.lib.
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
#define MAX_MAP_EVENTS_PER_READ 500000


int main(int argc, char* argv[])
{
  PL_Event*     pServerEventBuffer;     //** buffer in which the Server will return MAP events
  int           NumMAPEvents;           //** number of MAP events returned from the Server
  int           SpikeChannel;           //** DSP channel on which a spike timestamp occurred
  char          SpikeUnit;              //** a,b,c or d
  UINT          SpikeTime;              //** spike timestamp
  int           MAPSampleRate;          //** samples/sec for MAP channels
  int           MAPEventIndex;          //** loop counter

  //** connect to the server
  PL_InitClientEx3(0, NULL, NULL);

  //** allocate memory in which the server will return MAP events
  pServerEventBuffer = (PL_Event*)malloc(sizeof(PL_Event)*MAX_MAP_EVENTS_PER_READ);
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
  
  //** this loop reads from the Server once per second until the user hits Control-C
  //** note: the time between reads will actually be one second plus the amount of time required to
  //** dump the timestamps to the console 
  while (TRUE)
  {
    printf("reading from server\r\n");

    //** this tells the Server the max number of MAP events that can be returned to us in one read
    NumMAPEvents = MAX_MAP_EVENTS_PER_READ;

    //** call the Server to get all the MAP events since the last time we called PL_GetTimeStampStructures
	  PL_GetTimeStampStructures(&NumMAPEvents, pServerEventBuffer);

    //** step through the array of MAP events, displaying only the spike timestamps
    for (MAPEventIndex = 0; MAPEventIndex < NumMAPEvents; MAPEventIndex++)
    {
      //** is this the timestamp of a sorted spike? (unsorted spikes have Unit == 0)
      if (pServerEventBuffer[MAPEventIndex].Type == PL_SingleWFType && 
          pServerEventBuffer[MAPEventIndex].Unit >= 1 && 
          pServerEventBuffer[MAPEventIndex].Unit <= 4)
      {
        SpikeChannel = pServerEventBuffer[MAPEventIndex].Channel; //** 1-based DSP channel number
        SpikeUnit = 'a' + (pServerEventBuffer[MAPEventIndex].Unit-1); //** map 1,2,3,4 -> a,b,c,d
        SpikeTime = pServerEventBuffer[MAPEventIndex].TimeStamp; 
        printf("SPK%d%c t=%f\r\n", SpikeChannel, SpikeUnit, (float)SpikeTime/(float)MAPSampleRate);
      }
    }

    //** yield to other programs for 200 msec before calling the Server again
    Sleep(200);
  }

  //** in this sample, we will never get to this point, but this is how we would free the 
  //** allocated memory and disconnect from the Server

  free(pServerEventBuffer);
  PL_CloseClient();

	return 0;
}

