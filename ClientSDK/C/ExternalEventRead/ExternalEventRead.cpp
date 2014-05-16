//
//   ExternalEventRead.cpp 
//
//   (c) 1999-2012 Plexon Inc. Dallas Texas 75206 
//   www.plexoninc.com
//
//   Simple console-mode app that reads external events from the Server 
//   and prints the individual timestamps and strobed word values to the console window.  
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
  int           MAPSampleRate;          //** samples/sec for MAP channels
  int           MAPEventIndex;          //** loop counter
  int           EventChannel;           //** channel on which an external event occurred
  WORD          EventExtra;             //** additional external event info
  UINT          EventTime;              //** event timestamp

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

    //** step through the array of MAP events, displaying only the external events
    for (MAPEventIndex = 0; MAPEventIndex < NumMAPEvents; MAPEventIndex++)
    {
      //** is this the timestamp of an external event?
      if (pServerEventBuffer[MAPEventIndex].Type == PL_ExtEventType)
      {
        EventTime = pServerEventBuffer[MAPEventIndex].TimeStamp; 
        EventChannel = pServerEventBuffer[MAPEventIndex].Channel; 
        EventExtra = pServerEventBuffer[MAPEventIndex].Unit; // interpretation depends on external event type
        
        switch (EventChannel)
        {
          case PL_StrobedExtChannel:
            // For strobed events, EventExtra contains the actual strobed word value
            // NOTE: the high (16th) bit is reserved to indicate whether the DI board from which this event
            // was input was on the first DSP section (high bit = 0) or on some other DSP section (high bit = 1).
            // This allows up to two strobed DI boards to be installed in a MAP while still being able to 
            // distinguish their strobed data words, but one of the two must be installed on the first DSP.
            // To extract the low 15 bits which constitute the original strobed word value, the high bit must
            // be set to zero.  Also, the "set high bit" option in the MAP Server must be checked or this bit
            // will always be 0, regardless of which DSP section the DI was on -- this is acceptable if only 
            // a single strobed DI board is installed.
            if (EventExtra & 0x8000) // test high bit
            {
              // print with an asterisk to indicate that the high bit was 1
              printf("* Strobed external event t=%f strobed_word=%u\r\n", (float)EventTime/(float)MAPSampleRate, 
                     EventExtra & 0x7FFF); // mask: low 15 bits
            }
            else
            {
              printf("Strobed external event t=%f strobed_word=%u\r\n", (float)EventTime/(float)MAPSampleRate, 
                     EventExtra);
            }
            break;
            
          case PL_StartExtChannel:
            printf("Start-recording event t=%f\r\n", (float)EventTime/(float)MAPSampleRate);
            break;
            
          case PL_StopExtChannel:
            printf("Stop-recording event t=%f\r\n", (float)EventTime/(float)MAPSampleRate);
            break;
            
          case PL_Pause:
            printf("Pause-recording event t=%f\r\n", (float)EventTime/(float)MAPSampleRate);
            break;
            
          case PL_Resume:
            printf("Resume-recording event t=%f\r\n", (float)EventTime/(float)MAPSampleRate);
            break;
            
          default:
            // all single-bit / unstrobed events
            // in this case, EventExtra indicates the DSP section on which the DI board was located
            printf("Single-bit (unstrobed) external event DSPSection=%d EventChan=%d t=%f\r\n", EventExtra, EventChannel, (float)EventTime/(float)MAPSampleRate);
            break;
        }
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

