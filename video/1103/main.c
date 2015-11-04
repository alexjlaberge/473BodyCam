// 10251103Jiqing



#include "inc/hw_ints.h"



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

#include "inc/hw_ssi.h"


#define D1 GPIO_PIN_1
#define D2 GPIO_PIN_0


#define NUM_SSI_DATA 8


const unsigned char ulDataTx[NUM_SSI_DATA];
unsigned short g_pusTxBuffer[16];


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



uint32_t g_ui32SysClock;

/*
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
*/

int main(void) {



	unsigned long ulindex;
	unsigned long ulData;


	//50 MHz
	SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);


	SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0); SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

	GPIOPinConfigure(GPIO_PA2_SSI0CLK);
	GPIOPinConfigure(GPIO_PA3_SSI0FSS);
	GPIOPinConfigure(GPIO_PA5_SSI0XDAT1);
	GPIOPinTypeSSI(GPIO_PORTA_BASE,GPIO_PIN_5|GPIO_PIN_3|GPIO_PIN_2);

	SSIConfigSetExpClk(SSI0_BASE,SysCtlClockGet(),SSI_FRF_MOTO_MODE_0,SSI_MODE_MASTER,10000,16);
	SSIEnable(SSI0_BASE);

	//configure_pins();


	while(1) {
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
