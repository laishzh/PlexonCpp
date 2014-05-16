//
//   VTRead.cpp 
//
//   (c) 1999-2005 Plexon Inc. Dallas Texas 75206 
//   www.plexoninc.com
//
//   Simple console-mode app that reads timestamps from the Server,
//   decodes VT data, and prints VT packages.  
//
//   Built using Microsoft Visual C++ 8.0.  Must include Plexon.h and link with PlexClient.lib.
//
//   See SampleClients.doc for more information.
//

#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <windows.h>

//** header file containing the Plexon APIs (link with PlexClient.lib, run with PlexClient.dll)
#include "../../include/plexon.h"

#include "vt_interpret.h"

//** maximum number of MAP events to be read at one time from the Server
#define MAX_MAP_EVENTS_PER_READ 500000


int main(int argc, char* argv[])
{
  PL_Event*     pServerEventBuffer;     //** buffer in which the Server will return MAP events
  int           NumMAPEvents;           //** number of MAP events returned from the Server
  int           i;                      //** loop counter

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

  VT_Data data;   // decoded VT data

  VT_Acc  acc;    // accumulator of VT data
  VT_Acc_Init( &acc );

  unsigned __int64 acceptable_delay = (unsigned __int64)( 1e6 / PL_GetTimeStampTick() / 105.0 + 0.5 );

  //** this loop reads from the Server once per second until the user hits Control-C
  while (TRUE)
  { 
    //** this tells the Server the max number of MAP events that can be returned to us in one read
    NumMAPEvents = MAX_MAP_EVENTS_PER_READ;

    //** call the Server to get all the MAP events since the last time we called PL_GetTimeStampStructures
	  PL_GetTimeStampStructures(&NumMAPEvents, pServerEventBuffer);

    //** step through the array of MAP events, processing only the event timestamps
    for (i = 0; i < NumMAPEvents; i++)
    {
      //** is this the timestamp of a strobed event?
      if (pServerEventBuffer[i].Type == PL_ExtEventType &&          //** event timestamp
          pServerEventBuffer[i].Channel == PL_StrobedExtChannel)   //** strobed event only
      {
        // extract VT data and add it to the accumulator
        VT_Data_Init(&data, &pServerEventBuffer[i]);
        bool accepted = VT_Acc_Accept(&acc, &data, acceptable_delay);
        if (!accepted) 
        {
          // => data was rejected
          if (VT_Acc_Mode(&acc) != UNKNOWN) 
          {
            // print valid VT data
            VT_Acc_Print(&acc);
          }
          // reset the accumulator
          VT_Acc_Clear(&acc);
          // add VT data to it
          VT_Acc_Accept(&acc, &data, acceptable_delay);
        }
      }
    }

    //** yield to other programs for 200 msec before calling the Server again
    Sleep(200);
  }

  //** in this sample, we will never get to this point, but this is how we would free the 
  //** allocated memory and disconnect from the Server
  //** additionally we should print the last accumulated state

  if (VT_Acc_Mode(&acc) != UNKNOWN) 
  {
    // print valid VT data
    VT_Acc_Print(&acc);
  }

  free(pServerEventBuffer);
  PL_CloseClient();

	return 0;
}

