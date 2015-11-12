// This appears to be provided by CCS, but just in case...
#ifndef PART_TM4C1294NCPDT
#define PART_TM4C1294NCPDT
#endif /* PART_TM4C1294NCPDT */

#include "ticks.h"
#include "uvc.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <inc/hw_gpio.h>
#include <inc/hw_ints.h>
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>

#include <driverlib/gpio.h>
#include <driverlib/interrupt.h>
#include <driverlib/pin_map.h>
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <driverlib/sysctl.h>
#include <driverlib/systick.h>
#include <driverlib/udma.h>
#include <driverlib/usb.h>

#include <usblib/usblib.h>

#include <usblib/host/usbhost.h>

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

int main(void)
{
    uint32_t ui32SysClock;
    CAMERA_STATE = CAMERA_UNCONNECTED;
	struct uvc_iad iad;
	tConfigDescriptor conf;

    ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                           SYSCTL_OSC_MAIN |
                                           SYSCTL_USE_PLL |
                                           SYSCTL_CFG_VCO_480), 120000000);

    configure_pins();

    SysTickPeriodSet(ui32SysClock / TICKS_PER_SECOND);
    SysTickEnable();
    SysTickIntEnable();

    SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
    uDMAEnable();
    uDMAControlBaseSet(g_sDMAControlTable);

    USBStackModeSet(0, eUSBModeHost, 0);

    USBHCDRegisterDrivers(0, g_ppHostClassDrivers, g_ui32NumHostClassDrivers);

    USBHCDPowerConfigInit(0, USBHCD_VBUS_AUTO_HIGH | USBHCD_VBUS_FILTER);
    USBHCDInit(0, MEM_POOL, MEM_POOL_SIZE);

    while (1)
    {
        USBHCDMain();

        switch (CAMERA_STATE)
        {
            case CAMERA_INIT:
            	conf = uvc_get_config();
				uvc_probe_set_cur();
				uvc_set_iface();
            	CAMERA_STATE = CAMERA_CONNECTED;
                break;

            case CAMERA_CONNECTED:
                break;

            case CAMERA_UNCONNECTED:
                break;

            default:
                break;
        }

        if (CAMERA_STATE == CAMERA_CONNECTED &&
        		conf.bLength != 9)
        {
        	break;
        }
    }
}
