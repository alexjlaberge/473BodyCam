//*****************************************************************************
//
// basic_wifi_application.c - A command-line driven CC3000 WiFi example.
//
// Copyright (c) 2014-2015 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 2.1.1.71 of the EK-TM4C1294XL Firmware Package.
//
//*****************************************************************************
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>


#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_gpio.h"
#include "inc/hw_ints.h"
#include "driverlib/timer.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/systick.h"
#include <driverlib/sysctl.h>
#include "driverlib/fpu.h"
#include "driverlib/debug.h"
#include "driverlib/ssi.h"
#include "utils/ustdlib.h"
#include "utils/uartstdio.h"
#include "utils/cmdline.h"
#include "driverlib/uart.h"
#include "driverlib/ssi.h"
#include "driverlib/pin_map.h"
#include <driverlib/usb.h>
#include <usblib/usblib.h>
#include <usblib/host/usbhost.h>
#include "ticks.h"
#include "uvc.h"

#include "src/diskio.h"
#include "src/ff.h"

#include "wlan.h"
#include "evnt_handler.h"
#include "nvmem.h"
#include "socket.h"
#include "netapp.h"
#include "spi.h"
#include "hci.h"

#include "dispatcher.h"
#include "spi_version.h"
#include "board.h"
#include "application_version.h"
#include "host_driver_version.h"
#include "security.h"
#include "application_commands.h"

#define WIFI_IMG_START "yo new image"
#define WIFI_IMG_END "kthxbye"

#define WIFI_SSID "dd-wrt"
#define WIFI_PASS "roar2015"

#define WIFI_PKT_TYPE_MJPEG 2
#define WIFI_PKT_TYPE_UNCOMP 4

int i;
volatile int gpsFound;
volatile char GPS[100];
volatile char USB[256];
volatile int GPSSize;

static FATFS fatFS;
static DIR dir;
static FILINFO fileInfo;
static FIL fil;


volatile int disconnect_happened;
volatile int msp430Trigger = 0;
int offset;

FATFS sdVolume;			// FatFs work area needed for each volume
FIL logfile;			// File object needed for each open file
uint16_t fp;			// Used for sizeof

//*****************************************************************************
//
//  Global variables used by this program for event tracking.
//
//*****************************************************************************
volatile uint32_t g_ui32SmartConfigFinished, g_ui32CC3000Connected,
                  g_ui32CC3000DHCP,g_ui32OkToDoShutDown,
                  g_ui32CC3000DHCP_configured;

//*****************************************************************************
//
// Global to see if socket has been connected to (valid for TCP only)
//
//*****************************************************************************
bool g_bSocketConnected = false;

//*****************************************************************************
//
//  Smart Config flag variable. Used to stop Smart Config.
//
//*****************************************************************************
volatile uint8_t g_ui8StopSmartConfig;

//*****************************************************************************
//
// Global socket handle.
//
//*****************************************************************************
volatile uint32_t g_ui32Socket = SENTINEL_EMPTY;

//*****************************************************************************
//
// Global to hold socket type. Rewritten each time 'socketopen' is called.
//
//*****************************************************************************
sockaddr g_tSocketAddr;

//*****************************************************************************
//
// Flag used to signify type of socket, TCP or UDP.
//
//*****************************************************************************
uint32_t g_ui32SocketType = 0;

//*****************************************************************************
//
// Flag used to denote of bind has been called on socket yet
//
//*****************************************************************************
uint32_t g_ui32BindFlag = SENTINEL_EMPTY;

//*****************************************************************************
//
// Simple config prefix
//
//*****************************************************************************
char g_pcCC3000_prefix[] = {'T', 'T', 'T'};

const uint8_t g_pui8smartconfigkey[] = {0x73,0x6d,0x61,0x72,0x74,0x63,0x6f,
                                        0x6e,0x66,0x69,0x67,0x41,0x45,0x53,
                                        0x31,0x36};

#if defined(__CCS__) || defined(ccs)
uint8_t g_pui8CC3000_Rx_Buffer[CC3000_APP_BUFFER_SIZE +
                                            CC3000_RX_BUFFER_OVERHEAD_SIZE];
#endif



char *
sendDriverPatch(unsigned long *Length)
{
    *Length = 0;
    return(NULL);
}

char *
sendBootLoaderPatch(unsigned long *Length)
{
    *Length = 0;
    return(NULL);
}

//*****************************************************************************
//
// This function returns a pointer to the driver patch.  Since there is
// no patch (patches are taken from the EEPROM and not from the host)
// it returns NULL.
//
//*****************************************************************************
char *
sendWLFWPatch(unsigned long *Length)
{
    *Length = 0;
    return(NULL);
}

#define USB_CLK_DIV 8
#define USB_CONTROLLER 0

#define OUR_EP_IN USB_EP_0
#define OUR_EP_OUT USB_EP_1
#define OUR_EP_CTRL USB_EP_2
#define OUR_EP_INT USB_EP_3
#define OUR_MAX_PAYLOAD 512
#define OUR_TIMEOUT 10
#define OUR_USB_CTRL_INTS (USB_INTCTRL_CONNECT || \
		USB_INTCTRL_DISCONNECT || \
		USB_INTCTRL_POWER_FAULT || \
		USB_INTCTRL_VBUS_ERR)

volatile int running = 1;

#define MEM_POOL_SIZE 1024
uint8_t MEM_POOL[MEM_POOL_SIZE];

void usb_int(void)
{
	running = 0;

	GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 1);
	//uint32_t ctrl_status = USBIntStatusControl(USB_CONTROLLER);
	//uint32_t endpt_status = USBIntStatusEndpoint(USB_CONTROLLER);
}

void configure_pins()
{
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
}

#define TICKS_PER_SECOND 100
#define MS_PER_SYSTICK (1000 / TICKS_PER_SECOND)

enum uvc_camera_state
{
    CAMERA_CONNECTED,
    CAMERA_INIT,
    CAMERA_UNCONNECTED
};
enum uvc_camera_state CAMERA_STATE;

void USBHCDEvents(void *pvData)
{
    tEventInfo *pEventInfo = (tEventInfo *) pvData;

    switch(pEventInfo->ui32Event)
    {
        case USB_EVENT_CONNECTED:
            GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 255);
            if (USBHCDDevClass(pEventInfo->ui32Instance, 0) == USB_CLASS_VIDEO)
            {
                CAMERA_STATE = CAMERA_INIT;
            }
            break;

        case USB_EVENT_UNKNOWN_CONNECTED:
            CAMERA_STATE = CAMERA_UNCONNECTED;
            GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 255);
            break;

        case USB_EVENT_DISCONNECTED:
            CAMERA_STATE = CAMERA_UNCONNECTED;
            GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1, 0);
            break;

        case USB_EVENT_POWER_FAULT:
            CAMERA_STATE = CAMERA_UNCONNECTED;
            break;

        default:
            break;
    }
}

DECLARE_EVENT_DRIVER(g_sUSBEventDriver, 0, 0, USBHCDEvents);

static tUSBHostClassDriver const * const g_ppHostClassDrivers[] =
{
	&uvc_driver,
    &g_sUSBEventDriver
};

static const uint32_t g_ui32NumHostClassDrivers =
    sizeof(g_ppHostClassDrivers) / sizeof(tUSBHostClassDriver *);

#if defined(ewarm)
#pragma data_alignment=1024
tDMAControlTable g_sDMAControlTable[6];
#elif defined(ccs)
#pragma DATA_ALIGN(g_sDMAControlTable, 1024)
tDMAControlTable g_sDMAControlTable[6];
#else
tDMAControlTable g_sDMAControlTable[6] __attribute__ ((aligned(1024)));
#endif

//*****************************************************************************
//
// This function handles asynchronous events that come from CC3000.
//
//*****************************************************************************
void
CC3000_UsynchCallback(long lEventType, char *pcData, unsigned char ucLength)
{
    netapp_pingreport_args_t *psPingData;

    //
    // Handle completion of simple config callback
    //
    if(lEventType == HCI_EVNT_WLAN_ASYNC_SIMPLE_CONFIG_DONE)
    {

        UARTprintf("\r    Received asynchronous simple config done event "
                         "from CC3000\n>",0x08);

        g_ui32SmartConfigFinished = 1;
        g_ui8StopSmartConfig = 1;
    }

    //
    // Handle unsolicited connect callback.
    //
    if(lEventType == HCI_EVNT_WLAN_UNSOL_CONNECT)
    {
        //
        // Set global variable to indicate connection.
        //
        g_ui32CC3000Connected = 1;
    }

    //
    // Handle unsolicited disconnect callback. Turn LED Green -> Red.
    //
    if(lEventType == HCI_EVNT_WLAN_UNSOL_DISCONNECT)
    {
        UARTprintf("\r    Received Unsolicited Disconnect from CC3000\n>");

        disconnect_happened = 1;
        g_ui32CC3000Connected = 0;
        g_ui32CC3000DHCP = 0;
        g_ui32CC3000DHCP_configured = 0;

        //
        // Turn off the LED3.
        //
        turnLedOff(LED3);

        //
        // Turn back on the LED1.
        //
        turnLedOn(LED1);
    }

    //
    // Handle DHCP connection callback.
    //
    if(lEventType == HCI_EVNT_WLAN_UNSOL_DHCP)
    {
        //
        // Notes:
        // 1) IP config parameters are received swapped
        // 2) IP config parameters are valid only if status is OK,
        //      i.e. g_ui32CC3000DHCP becomes 1
        //

        //
        // Only if status is OK, the flag is set to 1 and the addresses are
        // valid
        //
        if( *(pcData + NETAPP_IPCONFIG_MAC_OFFSET) == 0)
        {
            UARTprintf("\r    DHCP Connected. IP: %d.%d.%d.%d\n",
                       pcData[3], pcData[2], pcData[1], pcData[0]);

            //
            // DHCP success, set global accordingly.
            //
            g_ui32CC3000DHCP = 1;

            //
            // Turn on the LED3.
            //
            turnLedOn(LED3);
        }
        else
        {
            //
            // DHCP failed, set global accordingly.
            //
            g_ui32CC3000DHCP = 0;
        }
    }
    //
    // Ping event handler
    //
    if(lEventType == HCI_EVNT_WLAN_ASYNC_PING_REPORT)
    {
        //
        // Ping data received, print to screen
        //
        psPingData = (netapp_pingreport_args_t *)pcData;


        //
        // Test for ping failure
        //
        if(psPingData->min_round_time == -1)
        {
            UARTprintf("\r    Ping Failed. Please check address and try "
                       "again.\n>");
        }
        else
        {
            UARTprintf("\r    Ping Results:\n"
                       "    sent: %d, received: %d, min time: %dms,"
                       " max time: %dms, avg time: %dms\n>",
                       psPingData->packets_sent, psPingData->packets_received,
                       psPingData->min_round_time, psPingData->max_round_time,
                       psPingData->avg_round_time);
        }
    }

    //
    // Handle shutdown callback.
    //
    if(lEventType == HCI_EVENT_CC3000_CAN_SHUT_DOWN)
    {
        //
        // Set global variable to indicate the device can be shutdown.
        //
        g_ui32OkToDoShutDown = 1;
    }
}

//*****************************************************************************
//
// This function initializes a CC3000 device and triggers it to start
// operation
//
//*****************************************************************************
int
initWiFiAndSysUART(void)
{
    //
    // Initialize the system clock and board pinout.
    //
    pio_init();

    //
    // Initialize SPI
    //
    init_spi(1000000,g_ui32SysClock);

    //
    // Enable processor interrupts.
    //
    MAP_IntMasterEnable();

    //
    // Initialize the CC3000 firmware.
    //
    wlan_init(CC3000_UsynchCallback, sendWLFWPatch, sendDriverPatch,
              sendBootLoaderPatch, ReadWlanInterruptPin,
              WlanInterruptEnable, WlanInterruptDisable, WriteWlanPin);

    //
    // Start the WiFI stack on the CC3000.
    //4
    DispatcherUARTConfigure(g_ui32SysClock);
   // UARTprintf("EB\n\r");

    wlan_start(0);

    //
    // Turn on the LED1 to indicate that we are active and initiated
    // WLAN successfully.
    //
    turnLedOn(LED1);

    //
    // Mask out all non-required events from CC3000
    //
    wlan_set_event_mask(HCI_EVNT_WLAN_KEEPALIVE | HCI_EVNT_WLAN_UNSOL_INIT);

    //DispatcherUARTConfigure(g_ui32SysClock);

    //ROM_SysCtlDelay(1000000);

    //
    // Print version string.
    //
    //UARTprintf("\n\n\rCC3000 Basic Wifi Application: driver version "
                //"%d.%d.%d.%d \n\r",PLATFORM_VERSION, APPLICATION_VERSION,
                //SPI_VERSION_NUMBER, DRIVER_VERSION_NUMBER );
    //UARTprintf("Type 'help' for a list of commands\n");

    //
    // Set flag to stop smart config if running.
    //
    g_ui8StopSmartConfig = 0;

    //
    // Configure SysTick to occur X times per second, to use as a time
    // reference. Enable SysTick to generate interrupts.
    //
    InitSysTick();

    return(0);
}

//*****************************************************************************
//
// Takes in a string of the form YYY.YYY.YYY.YYY (where YYY is a string
// representation of a decimal between0 and 255), converts each pair to a
// decimal. Used to parse user input from the command line.
//
// Returns 0 on success, -1 on fail.
//
//*****************************************************************************
int
DotDecimalDecoder(char *pcString, uint8_t *pui8Val1, uint8_t *pui8Val2,
                  uint8_t *pui8Val3, uint8_t *pui8Val4)
{
    uint32_t ui32Block1,ui32Block2,ui32Block3,ui32Block4;
    char *pcEndData, *pcStartData;

    //
    // Extract 1st octet of address.
    //
    pcStartData = pcString;
    pcEndData = ustrstr(pcStartData,".");
    ui32Block1 = ustrtoul(pcStartData, 0,10);

    //
    // Extract 2nd octet of address.
    //
    pcStartData = pcEndData +1;
    pcEndData = ustrstr(pcStartData,".");
    ui32Block2 = ustrtoul(pcStartData, 0,10);

    //
    // Extract 3rd octet of address.
    //
    pcStartData = pcEndData +1;
    pcEndData = ustrstr(pcStartData,".");
    ui32Block3 = ustrtoul(pcStartData, 0,10);

    //
    // Extract 4th octet of address.
    //
    pcStartData = pcEndData +1;
    pcEndData = ustrstr(pcStartData,".");
    ui32Block4 = ustrtoul(pcStartData, 0,10);

    //
    // Validate data. Valid values are between 0->255.
    //
    if((ui32Block1 > 255) || (ui32Block2 > 255) || (ui32Block3 > 255) ||
       (ui32Block4 > 255))
    {
        //
        // Exit with failure if any values are > 255
        //
        return(COMMAND_FAIL);
    }

    //
    // Assign address values to variables passed in
    //
    *pui8Val1 = (uint8_t)ui32Block1;
    *pui8Val2 = (uint8_t)ui32Block2;
    *pui8Val3 = (uint8_t)ui32Block3;
    *pui8Val4 = (uint8_t)ui32Block4;

    return(0);
}




int sd;
//*****************************************************************************
//
// Send Data to a destination port at a given IP Address.
// Arguments:
// [1] IP Address in the form xxx.XXX.yyy.YYY
// [2] Port as an integer 0->65536
// [3] Data to send as a string, <255bytes, no spaces
//
//*****************************************************************************

/**
 * | offset | size | field  |
 * |      0 |    2 | length |
 * |      2 |    1 | ID     |
 * |      3 |    1 | type   |
 * |      4 |    4 | offset |
 */

#define WIFI_HEADER_SIZE 8
#define WIFI_PACKET_SIZE (256 - WIFI_HEADER_SIZE)

volatile uint8_t pkt_id = 0;

int WIFI_sendUSBData(const uint8_t * buf, size_t size)
{
	int a;
	size_t i;
	uint8_t pkt[WIFI_PACKET_SIZE + WIFI_HEADER_SIZE];

	a = socket(AF_INET, SOCK_DGRAM, 0);

	*((uint16_t *) pkt + 0) = (uint16_t) WIFI_PACKET_SIZE;
	pkt[2] = pkt_id;
	if (uvc_is_uncomp())
	{
		pkt[3] = 4;
	}
	else if (uvc_is_mjpeg())
	{
		pkt[3] = 2;
	}
	else
	{
		pkt[3] = 0;
	}

	for (i = 0; i < (size / WIFI_PACKET_SIZE) - 1; i++)
	{
		uint32_t offset = (uint32_t) (i * WIFI_PACKET_SIZE);

		pkt[4] = (uint8_t) ((offset & 0x000000FF) >> 0);
		pkt[5] = (uint8_t) ((offset & 0x0000FF00) >> 8);
		pkt[6] = (uint8_t) ((offset & 0x00FF0000) >> 16);
		pkt[7] = (uint8_t) ((offset & 0xFF000000) >> 24);

		memcpy(&(pkt[WIFI_HEADER_SIZE]),
			&(buf[offset]),
			WIFI_PACKET_SIZE);

		sendto(a,
			pkt,
			WIFI_PACKET_SIZE + WIFI_HEADER_SIZE,
			0,
			&g_tSocketAddr,
			sizeof(sockaddr));
	}

    closesocket(a);

    return(0);
}

int
WIFI_sendGPSData()
{
    char * pui8Data;
    uint32_t ui32DataLength;

    pui8Data = GPS;

    ui32DataLength = GPSSize;


        sendto(g_ui32Socket, pui8Data, ui32DataLength, 0,
                            &g_tSocketAddr,sizeof(sockaddr));

    return(0);
}

void
KIntHandler(void)
{
	uint32_t ui32Status;
	if(msp430Trigger)
		msp430Trigger = 0;
	else
		msp430Trigger = 1;
	ui32Status = ROM_GPIOIntStatus(GPIO_PORTK_BASE, true);
	ROM_GPIOIntClear(GPIO_PORTK_BASE, ui32Status);
}

void
UARTIntHandler(void)
{
    uint32_t ui32Status;
    char curr;
    //char GPS[100];
    int commacount;
    //
    // Get the interrrupt status.
    //
    ui32Status = ROM_UARTIntStatus(UART6_BASE, true);

    //
    // Clear the asserted interrupts.
    //
    ROM_UARTIntClear(UART6_BASE, ui32Status);

    //
    // Loop while there are characters in the receive FIFO.
    //
    i = 0;
    curr = 0;
    commacount = 0;
    //GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_5, 0x20 );
    //GPS = (char*)malloc(256);
    while(ROM_UARTCharsAvail(UART6_BASE))
    {
    	//GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_5, 0x20 );
        //
        // Read the next character from the UART and write it back to the UART.
        //
        //ROM_UARTCharPutNonBlocking(UART0_BASE,
                                   //ROM_UARTCharGetNonBlocking(UART0_BASE));
    	if(ROM_UARTCharGet(UART6_BASE) == '$')
    	{
    		//GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_5, 0x20 );
    		if(ROM_UARTCharGet(UART6_BASE) == 'G')
    			if(ROM_UARTCharGet(UART6_BASE) == 'P')
    				if(ROM_UARTCharGet(UART6_BASE) == 'G')
    					if(ROM_UARTCharGet(UART6_BASE) == 'G')
    						if(ROM_UARTCharGet(UART6_BASE) == 'A')
    						{
    							if(ROM_UARTCharGet(UART6_BASE) ==',')
    							{
    								GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_5, 0x20 );
									while(i < 100){
										curr =ROM_UARTCharGet(UART6_BASE);
										GPS[i] =curr;
										i = i+1;
										//GPS++;-+
										if(curr== ',')
											commacount = commacount + 1;
										if(curr== '$' || commacount == 5 )
										{
											GPSSize = i;
											break;
										}
										//curr =ROM_UARTCharGetNonBlocking(UART6_BASE);
									}
									i = 0;
									commacount = 0;
									break;
    							}
    							//curr =ROM_UARTCharGetNonBlocking(UART6_BASE)

    						}
    	}



        //
        // Blink the LED to show a character transfer is occuring.
        //
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0);

        //
        // Delay for 1 millisecond.  Each SysCtlDelay is about 3 clocks.
        //
        SysCtlDelay(g_ui32SysClock / (1000 * 3));

        //
        // Turn off the LED
        //
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 0);
    }
    ROM_IntDisable(INT_UART6);
    ROM_UARTIntDisable(UART6_BASE, UART_INT_RX | UART_INT_RT);
    i = 0;
    gpsFound = 1;

}

void initGPS()
{
	int clock;
	clock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 120000000);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART6);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP);
	ROM_IntMasterEnable();
	GPIOPinConfigure(GPIO_PP0_U6RX);
	GPIOPinConfigure(GPIO_PP1_U6TX);
	ROM_GPIOPinTypeUART(GPIO_PORTP_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	ROM_UARTConfigSetExpClk(UART6_BASE, clock, 4800, (UART_CONFIG_WLEN_8 |UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
	ROM_IntEnable(INT_UART6);
	ROM_UARTIntEnable(UART6_BASE, UART_INT_RX);
}

initWiFiEndpoint()
{
	uint32_t ui32Port = 8888;
	uint8_t ui8IPBlock1,ui8IPBlock2,ui8IPBlock3,ui8IPBlock4;

	//
	// Extract IP Address.
	//
	DotDecimalDecoder("192.168.10.255",&ui8IPBlock1,&ui8IPBlock2,&ui8IPBlock3, &ui8IPBlock4);

	//
	// The family is always AF_INET on CC3000.
	//
	g_tSocketAddr.sa_family = AF_INET;

	//
	// The destination port.
	//
	ui32Port = 8888;
	g_tSocketAddr.sa_data[0] = (ui32Port & 0xFF00) >> 8;
	g_tSocketAddr.sa_data[1] = (ui32Port & 0x00FF) >> 0;

	//
	// The destination IP address.
	//
	g_tSocketAddr.sa_data[2] = ui8IPBlock1;
	g_tSocketAddr.sa_data[3] = ui8IPBlock2;
	g_tSocketAddr.sa_data[4] = ui8IPBlock3;
	g_tSocketAddr.sa_data[5] = ui8IPBlock4;
}

void initTrigger()
{
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
	ROM_GPIODirModeSet(GPIO_PORTK_BASE, GPIO_PIN_4, GPIO_DIR_MODE_IN);
	MAP_GPIOPadConfigSet(GPIO_PORTK_BASE, GPIO_PIN_4,
						 GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD);
	GPIOIntTypeSet(GPIO_PORTK_BASE, GPIO_PIN_4,
			GPIO_BOTH_EDGES);
	ROM_GPIOIntEnable(GPIO_PORTK_BASE, GPIO_INT_PIN_4);
	ROM_GPIOIntEnable(GPIO_PORTK_BASE, GPIO_INT_PIN_4);
	ROM_IntEnable(INT_GPIOK);
	ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOK);
}

#define USB_BUF_SIZE (128 * 1024)
#define WIFI_PKT_HEADER_SIZE 8
volatile uint8_t usb_buf[USB_BUF_SIZE];
volatile size_t usb_buf_len = 0;
volatile size_t usb_buf_i = 0;
volatile uint8_t pkt_type = 0;
volatile uint32_t pkt_offset = 0;
void uvc_start_cb(void)
{
	if (usb_buf_len != 0)
	{
		return;
	}

	if (uvc_is_mjpeg())
	{
		pkt_type = WIFI_PKT_TYPE_MJPEG;
	}
	else if (uvc_is_uncomp())
	{
		pkt_type = WIFI_PKT_TYPE_UNCOMP;
	}
	if(pkt_id == 255)
	{
		pkt_id = 1;
	}
	else
	{
		pkt_id++;
	}
	//usb_starting = 1;
	pkt_offset = 0;
}

void uvc_end_cb(void);

void uvc_data_cb(const uint8_t *buf, size_t len)
{
	//*((uint16_t *) (usb_buf + 0)) = (uint16_t) len;
	//*((uint8_t*) usb_buf + 2) = pkt_id;
	//*((uint8_t*) usb_buf + 3) = pkt_type;
	//*((uint32_t *) (usb_buf + 4)) = pkt_offset;
	//pkt_offset += (uint32_t) len;

	size_t i;

	if (usb_buf_len != 0)
	{
		return;
	}

	for (i = 0; i < len && usb_buf_i < USB_BUF_SIZE; i++)
	{
		usb_buf[usb_buf_i] = buf[i];
		usb_buf_i++;
	}

	if (usb_buf_i == USB_BUF_SIZE)
	{
		uvc_end_cb();
	}
}

void uvc_end_cb(void)
{
	usb_buf_len = usb_buf_i;
	usb_buf_i = 0;
}

int
main(void) {


	volatile FRESULT iFResult;

	g_ui32CC3000DHCP = 0;
    g_ui32Socket = SENTINEL_EMPTY;
    FPULazyStackingEnable();

    //Initialize MSP430 Trigger (PK4)
	initTrigger();

    //Initialize Sleep
	SysCtlLDOSleepSet(SYSCTL_LDO_1_15V);
	SysCtlSleepPowerSet(SYSCTL_SRAM_STANDBY);

    //Initialize USB
	uint32_t ui32SysClock;
	CAMERA_STATE = CAMERA_UNCONNECTED;
	g_ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
		                               SYSCTL_OSC_MAIN |
		                               SYSCTL_USE_PLL |
		                               SYSCTL_CFG_VCO_480), 120000000);
	configure_pins();

	SysTickPeriodSet(g_ui32SysClock / TICKS_PER_SECOND);
	SysTickEnable();
	SysTickIntEnable();

	IntMasterEnable();


	SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
	uDMAEnable();
	uDMAControlBaseSet(g_sDMAControlTable);

	USBStackModeSet(0, eUSBModeHost, 0);
	USBHCDRegisterDrivers(0, g_ppHostClassDrivers, g_ui32NumHostClassDrivers);
	USBHCDPowerConfigInit(0, USBHCD_VBUS_AUTO_HIGH | USBHCD_VBUS_FILTER);
	USBHCDInit(0, MEM_POOL, MEM_POOL_SIZE);
	uvc_init(10000000, uvc_start_cb, uvc_data_cb, uvc_end_cb);

	/*Initialize GPS stuff
    initGPS();
	ROM_IntEnable(INT_UART6);
	ROM_UARTIntEnable(UART6_BASE, UART_INT_RX);
	*/

    //Initialize WiFi and corresponding debug UART
    initWiFiAndSysUART();
    initWiFiEndpoint();


	// SD EXAMPLE
    iFResult = f_mount(0, &fatFS);
    iFResult = f_opendir(&dir, "");
    uint32_t count_num = 8*1024;
    iFResult = f_open(&fil, "bbb.txt", FA_CREATE_ALWAYS|FA_WRITE);
    //SysCtlDelay(SysCtlClockGet()/3);
    //iFResult = f_write(&fil, "Jake Sucks!", 11, &count_num);
    //iFResult = f_close(&fil);
    //f_mount(0, NULL);

    wlan_connect(WLAN_SEC_WPA2, WIFI_SSID, ustrlen(WIFI_SSID), NULL, WIFI_PASS, ustrlen(WIFI_PASS));
    while(!g_ui32CC3000DHCP)
    {
    	if(disconnect_happened)
    	{
    		wlan_connect(WLAN_SEC_WPA2, WIFI_SSID, ustrlen(WIFI_SSID), NULL, WIFI_PASS, ustrlen(WIFI_PASS));
    		disconnect_happened = 0;
    	}

    }
    /*int a;
    a = socket(AF_INET, SOCK_DGRAM, 0);
    while(1)
    {
    	sendto(a,
    				"HelloHELLOHelloHELLOHelloHELLOHelloHELLOHelloHELLOHelloHELLOHelloHELLOHelloHELLOHelloHELLOHelloHELLOHelloHELLOHelloHELLOHelloHELLOHelloHELLOHelloHELLOHelloHELLOHelloHELLOHelloHELLOHelloHELLOHelloHELLO",
    				200,
    				0,
    				&g_tSocketAddr,
    				sizeof(sockaddr));

    	//WIFI_sendUSBData("Hello", 5);
    }*/
    while(1)
    {
    	//Sleep part of loop
    	while(!msp430Trigger)
    	{
    		CAMERA_STATE = CAMERA_UNCONNECTED;
    		ROM_SysCtlSleep();
    	}
    	//Awake part of loop
    	while(msp430Trigger)
    	{
    		USBHCDMain();

			switch (CAMERA_STATE)
			{
				case CAMERA_INIT:
					uvc_probe_set_cur();
					uvc_set_iface();
					CAMERA_STATE = CAMERA_CONNECTED;
					break;

				case CAMERA_CONNECTED:
					uvc_main();
					break;

				case CAMERA_UNCONNECTED:
					break;

				default:
					break;
			}

			if (usb_buf_len > 0)
			{
				WIFI_sendUSBData(usb_buf, usb_buf_len);
				iFResult = f_open(&fil, "bbb.txt", FA_OPEN_ALWAYS|FA_WRITE);
				iFResult = f_lseek(&fil, f_size(&fil));
				iFResult = f_write(&fil, usb_buf, usb_buf_len, &count_num);
				iFResult = f_close(&fil);
				usb_buf_len = 0;
			}
    	}

    }
}
