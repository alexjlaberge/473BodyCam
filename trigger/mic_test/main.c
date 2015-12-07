#include <msp430.h> 

#define SW_TRIG BIT0 //p1.0
#define MIC_TRIG BIT3 //p1.3
#define RFID_TRIG BIT5 //p1.5
#define TIVA_WAKE BIT0 //p2.0

//1 second = 1 tick
#define RFID_timeout 300 // 5 minutes
#define MIC_timeout 10 // 10 seconds

volatile unsigned int SW_ON = 0;
volatile unsigned int MIC_ON = 0;
volatile unsigned int RFID_ON = 0;

volatile unsigned int RFID_time = 0;
volatile unsigned int MIC_time = 0;

void config()
{
	// Configure Timer Interrupt
	WDTCTL = WDTPW + WDTHOLD; 							// Stop watchdog timer
	TACCTL0 |= CCIE; // enable CCRO interrupt
	//TACCR0 = 32768-1;
	TACCR0 = 5300;
	TACTL |= TASSEL_1 + MC_1; // ACLK, upmode

	BCSCTL3 |= LFXT1S_2;		// Basic Clock System Control.

	// Configure GPIO
	P2DIR |= (TIVA_WAKE);
	P2OUT &= ~(TIVA_WAKE);

	P2DIR |= (BIT2);
	P2OUT &= ~(BIT2);

	// Configure GPIO Interrupt
	P1IE |= (SW_TRIG + MIC_TRIG + RFID_TRIG);						// Port 1 interrupt enabled for SW_TRIGitch
	P1IES &= ~(SW_TRIG + MIC_TRIG + ~RFID_TRIG); 						// Interrupt edge set Bit0 = low to high, Bit1 = high to low
	P1IFG &= ~(SW_TRIG + MIC_TRIG + RFID_TRIG); 						// Clear interrupt flag
	__enable_interrupt(); // enable all interrupts
}


void update()
{
	if(SW_ON || MIC_ON || RFID_ON)
		P2OUT |= TIVA_WAKE;
	else
		P2OUT &= ~TIVA_WAKE;
}

int main(void)
{
	config();

	while(1)
	{
		_bis_SR_register(LPM3_bits + GIE); //Enter Low Power Mode 3 with interrupts
	}

}
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A (void)
{
	P2OUT ^= BIT2;
	if (RFID_ON){
		if(RFID_time == RFID_timeout) {
			RFID_time = 0;
			RFID_ON = 0;
			update();
		}
		else {
			RFID_time++;
		}
	}
	if (MIC_ON){
		if(MIC_time >= MIC_timeout) {
			MIC_time = 0; // reset count
			MIC_ON = 0; // turn flag off
			update();
		}
		else {
			MIC_time++;
		}
	}
	else {
		MIC_time = 0;
		update();
	}
}

// Port 1 interrupt service routine
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
{
	// Switch Triggered
	if((P1IFG & SW_TRIG) == SW_TRIG)
	{
		P1IES ^= SW_TRIG; 						// Toggle interrupt edge for SW_TRIGitch
		P1IFG &= ~SW_TRIG;						// Clear interrupt flag
		SW_ON = 0;
		if((P1IN & SW_TRIG) == SW_TRIG) {
			SW_ON = 1;
		}
	}
	// Mic Triggered
	if ((P1IFG & MIC_TRIG) == MIC_TRIG)
	{

		MIC_ON = 1;

		P1IFG &= ~MIC_TRIG;
	}
	// RFID Triggered
	if ((P1IFG & RFID_TRIG) == RFID_TRIG)
	{
		P1IES ^= RFID_TRIG;
		P1IFG &= ~RFID_TRIG;
		if((P1IN & RFID_TRIG) == RFID_TRIG) {
			RFID_ON = 0;
		}
		else
			RFID_ON = 1;
		P1IFG &= ~RFID_TRIG;
	}
	update();
}
