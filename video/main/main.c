// This appears to be provided by CCS
// #define PART_TM4C1290NCPDT

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
// #include <drivers/pinout.h>

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

#define MEM_POOL_SIZE 128
uint8_t MEM_POOL[MEM_POOL_SIZE];

void usb_int(void)
{
	running = 0;

	//uint32_t ctrl_status = USBIntStatusControl(USB_CONTROLLER);
	uint32_t endpt_status = USBIntStatusEndpoint(USB_CONTROLLER);
}

void configure_usb(void)
{
	// TODO enable peripheral clock
	// TODO enable GPIO clock
	SysCtlClockSet(SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_24MHZ);
	SysCtlUSBPLLEnable();

	USBClockEnable(USB_CONTROLLER, USB_CLK_DIV, USB_CLOCK_INTERNAL);
	USBHostMode(USB_CONTROLLER);
	USBHostPwrConfig(USB_CONTROLLER,
			USB_HOST_PWRFLT_LOW |
			USB_HOST_PWRFLT_EP_TRI |
			USB_HOST_PWREN_MAN_HIGH);
	USBHostPwrFaultDisable(USB_CONTROLLER);

	IntMasterEnable();
	USBIntRegister(USB_CONTROLLER, usb_int);

	USBIntEnableControl(USB_CONTROLLER, OUR_USB_CTRL_INTS);
	USBIntEnableEndpoint(USB_CONTROLLER, USB_INTEP_ALL);

	USBHighSpeed(USB_CONTROLLER, false);
/*
	USBHostEndpointConfig(
			USB_CONTROLLER,
			OUR_EP_IN,
			OUR_MAX_PAYLOAD,
			OUR_TIMEOUT,
			0, // TODO I do not know what target_endpoint is supposed to be
			USB_EP_HOST_IN | USB_EP_MODE_ISOC);

	USBHostEndpointConfig(
			USB_CONTROLLER,
			OUR_EP_OUT,
			OUR_MAX_PAYLOAD,
			OUR_TIMEOUT,
			0, // TODO same as above
			USB_EP_HOST_OUT | USB_EP_MODE_BULK);
*/
	USBHostPwrEnable(USB_CONTROLLER);
}

/*
void configure_gpio(void)
{
	GPIODirModeSet(PA2, 2, GPIO_DIR_MODE_OUT);
}
*/

/*
 * main.c
 */
//int main(void) {
//	uint32_t out_addr, in_addr;
//	uint32_t host_status;
//
//	//configure_gpio();
//
//	configure_usb();
//
//	out_addr = USBHostAddrGet(0, USB_EP_0, USB_EP_HOST_OUT);
//	in_addr = USBHostAddrGet(USB_CONTROLLER, USB_EP_0, USB_EP_HOST_IN);
//	out_addr = USBHostAddrGet(0, USB_EP_1, USB_EP_HOST_OUT);
//	in_addr = USBHostAddrGet(USB_CONTROLLER, USB_EP_1, USB_EP_HOST_IN);
//
//	//IntTrigger(INT_USB0);
//
//	while (running)
//	{
//		//host_status = USBEndpointStatus(USB_CONTROLLER, OUR_EP_IN);
//	}
//
//	/*
//	while (running)
//	{
//		USBEndpointDataAvail(0, endpoint);
//		USBEndpointDataGet(0, endpoint, buf, &buflen);
//		USBEndpointDataPut();
//		USBEndpointDataSend();
//
//		USBEndpointStatus();
//	}
//	*/
//
//	return 0;
//}

void configure_pins()
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOM);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ);

    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // PB0-1/PD6/PL6-7 are used for USB.
    // PQ4 can be used as a power fault detect on this board but it is not
    // the hardware peripheral power fault input pin.
    //
    HWREG(GPIO_PORTD_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTD_BASE + GPIO_O_CR) = 0xff;
    GPIOPinConfigure(GPIO_PD6_USB0EPEN);
    GPIOPinTypeUSBAnalog(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    GPIOPinTypeUSBDigital(GPIO_PORTD_BASE, GPIO_PIN_6);
    GPIOPinTypeUSBAnalog(GPIO_PORTL_BASE, GPIO_PIN_6 | GPIO_PIN_7);
    GPIOPinTypeGPIOInput(GPIO_PORTQ_BASE, GPIO_PIN_4);

    //
    // This app does not want Ethernet LED function so configure as
    // standard outputs for LED driving.
    //
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4);

    //
    // Default the LEDs to OFF.
    //
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4, 0);
    MAP_GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4,
                         GPIO_STRENGTH_12MA, GPIO_PIN_TYPE_STD);

    //
    // PJ0 and J1 are used for user buttons
    //
    GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    GPIOPinWrite(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1, 0);

    //
    // PN0 and PN1 are used for USER LEDs.
    //
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    MAP_GPIOPadConfigSet(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1,
                             GPIO_STRENGTH_12MA, GPIO_PIN_TYPE_STD);

    //
    // Default the LEDs to OFF.
    //
    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1, 0);
}

#define TICKS_PER_SECOND 100

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
            if(USBHCDDevClass(pEventInfo->ui32Instance, 0) == USB_CLASS_VIDEO)
            {
                CAMERA_STATE = CAMERA_INIT;
            }
            break;

        case USB_EVENT_UNKNOWN_CONNECTED:
            CAMERA_STATE = CAMERA_UNCONNECTED;
            break;

        case USB_EVENT_DISCONNECTED:
            CAMERA_STATE = CAMERA_UNCONNECTED;
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
    &g_sUSBHIDClassDriver,
    &g_sUSBEventDriver
};

static const uint32_t g_ui32NumHostClassDrivers =
    sizeof(g_ppHostClassDrivers) / sizeof(tUSBHostClassDriver *);

tDMAControlTable g_sDMAControlTable[6] __attribute__ ((aligned(1024)));

int main(void)
{
    uint32_t ui32SysClock;
    CAMERA_STATE = CAMERA_UNCONNECTED;

    ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                           SYSCTL_OSC_MAIN |
                                           SYSCTL_USE_PLL |
                                           SYSCTL_CFG_VCO_480), 120000000);

    //
    // Enable the pins and peripherals used by this example.
    //
    // TODO PinoutSet(0,1);

    SysTickPeriodSet(ui32SysClock / TICKS_PER_SECOND);
    SysTickEnable();
    SysTickIntEnable();

    SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
    uDMAEnable();
    uDMAControlBaseSet(g_sDMAControlTable);

    USBStackModeSet(0, eUSBModeHost, 0);

    USBHCDRegisterDrivers(0, g_ppHostClassDrivers, g_ui32NumHostClassDrivers);

    //
    // Open an instance of the keyboard driver.  The keyboard does not need
    // to be present at this time, this just save a place for it and allows
    // the applications to be notified when a keyboard is present.
    //
    //g_psKeyboardInstance = USBHKeyboardOpen(KeyboardCallback, g_pui8Buffer,
    //                                        KEYBOARD_MEMORY_SIZE);

    USBHCDPowerConfigInit(0, USBHCD_VBUS_AUTO_HIGH | USBHCD_VBUS_FILTER);
    USBHCDInit(0, MEM_POOL, MEM_POOL_SIZE);

    while (1)
    {
        switch (CAMERA_STATE)
        {
            case CAMERA_INIT:
                // USBHKeyboardInit(g_psKeyboardInstance);
                //CAMERA_STATE = STATE_KEYBOARD_CONNECTED;
                break;

            case CAMERA_CONNECTED:
                break;

            case CAMERA_UNCONNECTED:
                break;

            default:
                break;
        }

        if (CAMERA_STATE == CAMERA_INIT)
        {
        	break;
        }
    }

    while (1)
    {

    }
}
