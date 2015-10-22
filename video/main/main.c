// Use for the Tiva 123 dev board
#define PART_TM4C123GH6PM

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <inc/hw_ints.h>
#include <driverlib/interrupt.h>
#include <driverlib/sysctl.h>
#include <driverlib/usb.h>

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
int main(void) {
	uint32_t out_addr, in_addr;
	uint32_t host_status;

	//configure_gpio();

	configure_usb();

	out_addr = USBHostAddrGet(0, USB_EP_0, USB_EP_HOST_OUT);
	in_addr = USBHostAddrGet(USB_CONTROLLER, USB_EP_0, USB_EP_HOST_IN);
	out_addr = USBHostAddrGet(0, USB_EP_1, USB_EP_HOST_OUT);
	in_addr = USBHostAddrGet(USB_CONTROLLER, USB_EP_1, USB_EP_HOST_IN);

	//IntTrigger(INT_USB0);

	while (running)
	{
		//host_status = USBEndpointStatus(USB_CONTROLLER, OUR_EP_IN);
	}

	/*
	while (running)
	{
		USBEndpointDataAvail(0, endpoint);
		USBEndpointDataGet(0, endpoint, buf, &buflen);
		USBEndpointDataPut();
		USBEndpointDataSend();

		USBEndpointStatus();
	}
	*/

	return 0;
}
