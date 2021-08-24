
// #include <sys/types.h>
// #include <string.h>
// #include <sys/ioctl.h>
// #include <sys/types.h>
// #include <ctype.h>
// #include <time.h>
// #include <fcntl.h>
// #include <errno.h>
// #include <termios.h>
// #include <unistd.h>
// #include <math.h>
// #include <stdlib.h>
// #include <pthread.h>
// #include "./pico.h"
// #include "./serial.h"
// #include "./narrative.h"
// #include "./readUART.h"
// #include "../inc/config.h"

// #include "libpicohrdl-1.0/HRDL.h"

// #define Sleep(a) usleep(1000*a)
// #define scanf_s scanf
// #define fscanf_s fscanf
// #define memcpy_s(a,b,c,d) memcpy(a,c,d)

// typedef enum enBOOL{FALSE,TRUE} BOOL;

// struct structChannelSettings 
// {
// 	int16_t enabled;
// 	HRDL_RANGE range;
// 	int16_t singleEnded;
// } g_channelSettings[HRDL_MAX_ANALOG_CHANNELS + 1];

// int32_t		g_times[BUFFER_SIZE];
// int32_t		g_values[BUFFER_SIZE];

// int32_t		g_scaleTo_mv;
// int16_t		g_device;
// int16_t		g_doSet;
// int16_t		g_maxNoOfChannels;

// double inputRangeDivider [] = {1, 2, 4, 8, 16, 32, 64}; // Used for different voltage scales


// int32_t _kbhit()
// {
//         struct termios oldt, newt;
//         int32_t bytesWaiting;
//         tcgetattr(STDIN_FILENO, &oldt);
//         newt = oldt;
//         newt.c_lflag &= ~( ICANON | ECHO );
//         tcsetattr(STDIN_FILENO, TCSANOW, &newt);
//         setbuf(stdin, NULL);
//         ioctl(STDIN_FILENO, FIONREAD, &bytesWaiting);

//         tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
//         return bytesWaiting;
// }


// /****************************************************************************
// *
// * AdcTo_mv
// *
// * If the user selects scaling to millivolts,
// * Convert an ADC count into millivolts
// *
// ****************************************************************************/
// float AdcToMv (HRDL_INPUTS channel, int32_t raw)
// { 
// 	int32_t maxAdc = 0;
// 	int32_t minAdc = 0;

// 	if (channel < HRDL_ANALOG_IN_CHANNEL_1 || channel > HRDL_MAX_ANALOG_CHANNELS)
// 	{
// 		return 0;
// 	}

// 	if (raw == -1)
// 	{
// 		return -1;
// 	}

// 	HRDLGetMinMaxAdcCounts(g_device, &minAdc, &maxAdc, channel);

// 	// To convert from adc to V you need to use the following equation
// 	//            maxV - minV
// 	//   raw =  ---------------
// 	//          maxAdc - minAdc
// 	//
// 	// if we assume that V and adc counts are bipolar and symmetrical about 0, the 
// 	// equation reduces to the following:
// 	//            maxV
// 	//   raw =  --------
// 	//           maxAdc 
// 	//
// 	//
// 	// Note the use of the maxAdc count for the HRDL in the equation below:  
// 	//
// 	// maxAdc is always 1 adc count short of the advertised full voltage scale
// 	// minAdc is always exactly the advertised minimum voltage scale.
// 	//
// 	// It is this way to ensure that we have an adc value that
// 	// equates to exactly zero volts.
// 	//
// 	// maxAdc     == maxV
// 	// 0 adc      == 0 V
// 	// minAdc     == minV
// 	// 

// 	if (g_scaleTo_mv)
// 	{
// 		return (float)  ((double) raw * (2500.0 / pow(2.0, (double) g_channelSettings[channel].range)) / (double)(maxAdc));
// 	}
// 	else
// 	{
// 		return (float) raw;
// 	}

// }


// /****************************************************************************
// *
// * CollectStreaming
// *	This function demonstrates how to use streaming.
// *
// * In this mode, you can collect data continuously.
// *
// * This example writes data to disk...
// * don't leave it running too long or it will fill your disk up!
// *
// * Each call to HRDLGetValues returns the readings since the last call
// *
// ****************************************************************************/

// void CollectStreaming (void *state)
// {
//   struct applicationState *stateptr = (void *)state;
// 	int32_t		i;
// 	int32_t		blockNo;
// 	FILE *		fp;
// 	int32_t		nValues;
// 	int16_t		channel;
// 	int16_t		numberOfActiveChannels;
// 	int8_t		strError[80];
// 	int16_t		status = 1;


// 	for (i = HRDL_ANALOG_IN_CHANNEL_1; i <= g_maxNoOfChannels; i++)
// 	{
// 		status = HRDLSetAnalogInChannel(stateptr->picoDevice,
// 			                         (int16_t)i,
// 										g_channelSettings[i].enabled, 
// 										(int16_t) g_channelSettings[i].range,
// 										g_channelSettings[i].singleEnded);

// 		if (status == 0)
// 		{
// 			HRDLGetUnitInfo(stateptr->picoDevice, strError, (int16_t) 80, HRDL_SETTINGS);
// 			printf("Error occurred: %s\n\n", strError);
// 			return;
// 		}
// 	}

// 	//
// 	// Collect data at 1 second intervals, with maximum resolution
// 	//
// 	HRDLSetInterval(stateptr->picoDevice, 1000, HRDL_60MS);

// 	fprintf(stateptr->pfp, "Starting data collection...\n");

// 	//
// 	// Start it collecting,
// 	status = HRDLRun(stateptr->picoDevice, BUFFER_SIZE, (int16_t) HRDL_BM_STREAM);

// 	if (status == 0)
// 	{
// 		HRDLGetUnitInfo(stateptr->picoDevice, strError, (int16_t) 80, HRDL_SETTINGS);
// 		printf("Error occurred: %s\n\n", strError);
// 		return;
// 	}

// 	while (!HRDLReady(stateptr->picoDevice))
// 	{
// 		Sleep (1000);
// 	}

// 	//
// 	// From here on, we can get data whenever we want...
// 	//
	

// 	HRDLGetNumberOfEnabledChannels(stateptr->picoDevice, &numberOfActiveChannels);
// 	numberOfActiveChannels = numberOfActiveChannels + (int16_t)(g_channelSettings[HRDL_DIGITAL_CHANNELS].enabled);
  
// 	while (!_kbhit())
// 	{ 
// 		nValues = HRDLGetValues(stateptr->picoDevice, g_values, NULL, BUFFER_SIZE/numberOfActiveChannels);
// 		// printf ("%d values\n", nValues);

// 		for (i = 0; i < nValues * numberOfActiveChannels;)
// 		{
// 			for (channel = HRDL_DIGITAL_CHANNELS; channel <= HRDL_MAX_ANALOG_CHANNELS; channel++)
// 			{
// 				//
// 				// Print the channel headers to file
// 				//
// 				if (nValues && channel == HRDL_DIGITAL_CHANNELS && g_channelSettings[channel].enabled)
// 				{
// 					fprintf (stateptr->pfp, "Digital IO (1 2 3 4):,");
// 				}
// 				else if (nValues && g_channelSettings[channel].enabled)
// 				{
// 					fprintf (stateptr->pfp, "Channel %d:,", channel);
// 				}

// 				//
// 				// Print to file the new readings
// 				//

// 				if (channel == HRDL_DIGITAL_CHANNELS && g_channelSettings[channel].enabled)
// 				{
// 					fprintf (stateptr->pfp, "%d %d %d %d,", 0x01 & (g_values [i]),
// 													0x01 & (g_values [i] >> 0x1),
// 													0x01 & (g_values [i] >> 0x2),
// 													0x01 & (g_values [i] >> 0x3));
// 					i++;
//           fflush(stateptr->pfp);
// 				}
// 				else if (g_channelSettings[channel].enabled)
// 				{
// 					fprintf (stateptr->pfp, "%f,", AdcToMv ((HRDL_INPUTS) channel, g_values [i]));
// 					i++;
// 				}
// 			}

// 			if (nValues && g_channelSettings[channel].enabled)
// 			{
//   				fprintf (stateptr->pfp, "\n");
// 			}
// 		}

// 		if ((blockNo++  % 20) == 0)
// 		{
// 			// printf ("Press any key to stop\n");

// 			if (nValues)
// 			{
// 				fprintf (stateptr->pfp, "\n");
// 			}
		
// 			//
// 			// Wait 2 seconds before asking again
// 			//

// 			Sleep (2000);
// 		}

// 	}

// 	fclose (stateptr->pfp);
// 	HRDLStop(stateptr->picoDevice);
// }


// /****************************************************************************
// *
// * SetAnalogChannels
// *  This function demonstrates how to detect available input range and set it.
// *  We will first check to see if a channel is available, then check what ranges
// *  are available and then check to see if differential mode is supported for thet
// *  channel.
// *
// ****************************************************************************/
// void SetAnalogChannels(void *state)
// { 
//   struct applicationState *stateptr = (void *)state;
// 	int32_t channel;
// 	int16_t range;
// 	int16_t available;
// 	int16_t status = 1;


// 	for (channel = 1; channel < 6; channel++) {
// 	  g_channelSettings[channel].enabled = (int16_t)1;
//     HRDLSetAnalogInChannel(stateptr->picoDevice, (int16_t)channel, (int16_t) 0, (int16_t) HRDL_2500_MV, (int16_t) 1);
//   }

// 	//
// 	// Disable the channel if the user does not require it

// 	if (!g_channelSettings[channel].enabled)
// 	{
// 		printf("Channel %d disabled\n\n", channel);
// 		HRDLSetAnalogInChannel(stateptr->picoDevice, (int16_t)channel, (int16_t) 0, (int16_t) HRDL_1250_MV, (int16_t) 1);
// 		return;
// 	}          

// }



// int16_t OpenDevice(int16_t picoDevice) {
//   picoDevice = HRDLOpenUnit(); 
// 	return picoDevice;
// }


// void *picoStream (void *state) {

//   struct applicationState *stateptr = (void *)state; 

//   struct tm *timenow;
//   time_t now = time(NULL);
//   timenow = gmtime(&now);
//   strftime(stateptr->filename, sizeof(stateptr->filename), "data/%Y%m%d_%H%M%S_PICO_DATA.csv", timenow);

//   stateptr->pfp = fopen(stateptr->filename, "w+");

// 	int32_t		ok = 0;
// 	int8_t 		line [80];  
// 	int16_t		lineNo;

// 	int8_t		ch;

// 	int8_t description[7][25] = { "Driver Version    :", 
// 									"USB Version       :", 
// 									"Hardware Version  :", 
// 									"Variant Info      :", 
// 									"Batch and Serial  :", 
// 									"Calibration Date  :", 
// 									"Kernel Driver Ver.:"};

// 	g_doSet = FALSE;
//   OpenDevice(stateptr->picoDevice);
//   HRDLGetUnitInfo(stateptr->picoDevice, line, sizeof (line), HRDL_ERROR);
// 	memset(g_channelSettings, 0, sizeof(g_channelSettings));
//   //
//   // Get all the information related to the device
//   //
//   g_maxNoOfChannels = 16;
//   g_scaleTo_mv = FALSE;

//   HRDLSetMains(stateptr->picoDevice, 1);
//   SetAnalogChannels((void *)stateptr);
//   CollectStreaming((void *)stateptr);

//   HRDLCloseUnit(stateptr->picoDevice);
// }   

