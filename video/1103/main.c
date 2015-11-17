// 20151103Jiqing







#include <stdint.h>
#include <stdbool.h>

#include "driverlib/gpio.h"

#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/ssi.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"

#include "inc/hw_ssi.h"


#include "fatfs/src/ff.h"
#include "third_party/fatfs/src/ff.c"
#include "fatfs/src/diskio.h"



#define D1 GPIO_PIN_1
#define D2 GPIO_PIN_0



#define PATH_BUF_SIZE   80		//size of the buffers that hold the path, or temporary data from the SD card
#define CMD_BUF_SIZE    64











#define NUM_SSI_DATA 8
const unsigned char ulDataTx[NUM_SSI_DATA];


unsigned short g_pusTxBuffer[16];




static char g_pcCwdBuf[PATH_BUF_SIZE] = "/";
static char g_pcTmpBuf[PATH_BUF_SIZE];
static char g_pcCmdBuf[CMD_BUF_SIZE];		//commend line buffer




// The following are data structures used by FatFs.
static FATFS g_sFatFs;
static DIR g_sDirObject;
static FILINFO g_sFileInfo;
static FIL g_sFileObject;


typedef struct
{
    FRESULT iFResult;
    char *pcResultStr;
}
tFResultString;

#define FRESULT_ENTRY(f)        { (f), (#f) }





unsigned char
Reverse(unsigned char ucNumber) {
	unsigned short ucIndex;
	unsigned short ucReversedNumber = 0;
	for(ucIndex=0; ucIndex<8; ucIndex++) {
		ucReversedNumber = ucReversedNumber << 1;
		ucReversedNumber |= ((1 << ucIndex) & ucNumber) >> ucIndex;
	}
	return ucReversedNumber;
}



//uint32_t g_ui32SysClock;




//#########################################################################




//*****************************************************************************
//
// This function implements the "cat" command.  It reads the contents of a file
// and prints it to the console.  This should only be used on text files.  If
// it is used on a binary file, then a bunch of garbage is likely to printed on
// the console.
//
//*****************************************************************************
int
Cmd_cat(int argc, char *argv[])
{
    FRESULT iFResult;
    uint32_t ui32BytesRead;

    //
    // First, check to make sure that the current path (CWD), plus the file
    // name, plus a separator and trailing null, will all fit in the temporary
    // buffer that will be used to hold the file name.  The file name must be
    // fully specified, with path, to FatFs.
    //
    if(strlen(g_pcCwdBuf) + strlen(argv[1]) + 1 + 1 > sizeof(g_pcTmpBuf))
    {
        UARTprintf("Resulting path name is too long\n");
        return(0);
    }

    //
    // Copy the current path to the temporary buffer so it can be manipulated.
    //
    strcpy(g_pcTmpBuf, g_pcCwdBuf);

    //
    // If not already at the root level, then append a separator.
    //
    if(strcmp("/", g_pcCwdBuf))
    {
        strcat(g_pcTmpBuf, "/");
    }

    //
    // Now finally, append the file name to result in a fully specified file.
    //
    strcat(g_pcTmpBuf, argv[1]);

    //
    // Open the file for reading.
    //
    iFResult = f_open(&g_sFileObject, g_pcTmpBuf, FA_READ);

    //
    // If there was some problem opening the file, then return an error.
    //
    if(iFResult != FR_OK)
    {
        return((int)iFResult);
    }

    //
    // Enter a loop to repeatedly read data from the file and display it, until
    // the end of the file is reached.
    //
    do
    {
        //
        // Read a block of data from the file.  Read as much as can fit in the
        // temporary buffer, including a space for the trailing null.
        //
        iFResult = f_read(&g_sFileObject, g_pcTmpBuf, sizeof(g_pcTmpBuf) - 1,
                          (UINT *)&ui32BytesRead);

        //
        // If there was an error reading, then print a newline and return the
        // error to the user.
        //
        if(iFResult != FR_OK)
        {
            UARTprintf("\n");
            return((int)iFResult);
        }

        //
        // Null terminate the last block that was read to make it a null
        // terminated string that can be used with printf.
        //
        g_pcTmpBuf[ui32BytesRead] = 0;

        //
        // Print the last chunk of the file that was received.
        //
        UARTprintf("%s", g_pcTmpBuf);

        //
        // Wait for the UART transmit buffer to empty.
        //
        UARTFlushTx(false);

    //
    // Continue reading until less than the full number of bytes are
    // read.  That means the end of the buffer was reached.
    //
    }
    while(ui32BytesRead == sizeof(g_pcTmpBuf) - 1);

    //
    // Return success.
    //
    return(0);
}



int
CmdLineProcess(char *pcCmdLine)
{
    char *pcChar;
    uint_fast8_t ui8Argc;
    bool bFindArg = true;
    tCmdLineEntry *psCmdEntry;

    //
    // Initialize the argument counter, and point to the beginning of the
    // command line string.
    //
    ui8Argc = 0;
    pcChar = pcCmdLine;

    //
    // Advance through the command line until a zero character is found.
    //
    while(*pcChar)
    {
        //
        // If there is a space, then replace it with a zero, and set the flag
        // to search for the next argument.
        //
        if(*pcChar == ' ')
        {
            *pcChar = 0;
            bFindArg = true;
        }

        //
        // Otherwise it is not a space, so it must be a character that is part
        // of an argument.
        //
        else
        {
            //
            // If bFindArg is set, then that means we are looking for the start
            // of the next argument.
            //
            if(bFindArg)
            {
                //
                // As long as the maximum number of arguments has not been
                // reached, then save the pointer to the start of this new arg
                // in the argv array, and increment the count of args, argc.
                //
                if(ui8Argc < CMDLINE_MAX_ARGS)
                {
                    g_ppcArgv[ui8Argc] = pcChar;
                    ui8Argc++;
                    bFindArg = false;
                }

                //
                // The maximum number of arguments has been reached so return
                // the error.
                //
                else
                {
                    return(CMDLINE_TOO_MANY_ARGS);
                }
            }
        }

        //
        // Advance to the next character in the command line.
        //
        pcChar++;
    }

    //
    // If one or more arguments was found, then process the command.
    //
    if(ui8Argc)
    {
        //
        // Start at the beginning of the command table, to look for a matching
        // command.
        //
        psCmdEntry = &g_psCmdTable[0];

        //
        // Search through the command table until a null command string is
        // found, which marks the end of the table.
        //
        while(psCmdEntry->pcCmd)
        {
            //
            // If this command entry command string matches argv[0], then call
            // the function for this command, passing the command line
            // arguments.
            //
            if(!strcmp(g_ppcArgv[0], psCmdEntry->pcCmd))
            {
                return(psCmdEntry->pfnCmd(ui8Argc, g_ppcArgv));
            }

            //
            // Not found, so advance to the next entry.
            //
            psCmdEntry++;
        }
    }

    //
    // Fall through to here means that no matching command was found, so return
    // an error.
    //
    return(CMDLINE_BAD_CMD);
}



//#########################################################################
















void configure_pins()
{
	/*
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ);

    HWREG(GPIO_PORTD_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTD_BASE + GPIO_O_CR) = 0xff;
    GPIOPinConfigure(GPIO_PD6_USB0EPEN);
    GPIOPinTypeUSBAnalog(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    GPIOPinTypeUSBDigital(GPIO_PORTD_BASE, GPIO_PIN_6);
    GPIOPinTypeUSBAnalog(GPIO_PORTL_BASE, GPIO_PIN_6 | GPIO_PIN_7);
    GPIOPinTypeGPIOInput(GPIO_PORTQ_BASE, GPIO_PIN_4);

    // LEDs on Eval Board
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    GPIOPadConfigSet(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1,
    		GPIO_STRENGTH_12MA, GPIO_PIN_TYPE_STD);
    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1, 0);
    */
	GPIOPinConfigure(GPIO_PA2_SSI0CLK);
	GPIOPinConfigure(GPIO_PA3_SSI0FSS);
	GPIOPinConfigure(GPIO_PA4_SSI0XDAT0);    	//Transmit pin
	GPIOPinConfigure(GPIO_PA5_SSI0XDAT1);		//Receive pin
	GPIOPinTypeSSI(GPIO_PORTA_BASE,GPIO_PIN_5|GPIO_PIN_4|GPIO_PIN_3|GPIO_PIN_2);

	//SSI_FRF_MOTO_MODE_0     0x00000000  // Moto fmt, polarity 0, phase 0
	//master
	//bit rate 10000
	//data width 16
	SSIConfigSetExpClk(SSI0_BASE,SysCtlClockGet(),SSI_FRF_MOTO_MODE_0,SSI_MODE_MASTER,10000,16);
	SSIEnable(SSI0_BASE);


}


int main(void) {



	unsigned long ulindex;
	unsigned long ulData;



    int nStatus;
    FRESULT iFResult;

    iFResult = f_mount(0, &g_sFatFs);
    if(iFResult != FR_OK)
    {
        UARTprintf("f_mount error: %s\n", StringFromFResult(iFResult));
        return(1);
    }

    PopulateFileListBox(true);






	//50 MHz
	SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);


	SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);



	configure_pins();


	while(1) {
		// Get a line of text from the user.

		UARTgets(g_pcCmdBuf, sizeof(g_pcCmdBuf));

		//
		// Pass the line from the user to the command processor.  It will be
		// parsed and valid commands executed.
		//
		nStatus = CmdLineProcess(g_pcCmdBuf);



		for(ulindex = 0; ulindex < NUM_SSI_DATA; ulindex++) {
			ulData = (Reverse(ulDataTx[ulindex]) << 8) + (1 << ulindex);
			SSIDataPut(SSI0_BASE, ulData);
			while(SSIBusy(SSI0_BASE)) {

			}
		}

	}




	/*while(1) {
		GPIOPinWrite(GPIO_PORTN_BASE, D2, 255);

	}*/
}
