/* This code demonstrates the use of both switches, labelled S1 (RESET) and S2 (P1.3) on the MSP430 Launchpad.
 * The code implements a software debounce using the WDT and also indicates a long press i.e. switch pressed
 * for more than 1.5 seconds
 * Switch S1 uses the RST/NMI pin as an input
 * Switch S2 uses the P1.3 general purpose I/O pin
 * The P1.0 Red LED1 is used to indicate when S2 (P1.3) is pressed (Active Pressed).
 * The P1.6 Green LED2 is used to indicate when S1 (RESET) is pressed (Active Pressed).
 * These LEDs are turned off when the switches are released.
 * The Watchdog Timer (WDT) is used both to debounce the switches and to time the length of the press. The debounce
 * is achieved by using the delay provided by the WDT to re-enable the Switch interrupts only after the bouncing has
 * finished. The WDT interrupt service routine also increments separate counters (one for each switch) when the
 * switches are pressed. When the count exceeds a certain time the P1.0 LED (for Switch 1) and the P1.6 LED (for
 * Switch 2) are turned on to indicate a long press.
 * In order to run the code using the RESET pin as a switch input you need to cycle the power on the LaunchPad.
 */
 
#include "msp430g2553.h";
#define FLIP_HOLD (0x3300 | WDTHOLD) // flip HOLD while preserving other bits
 
#define S1 0x0001
#define S2 0x0002
#define NORMAL 0
#define SPECIAL 1
#define TIMEHOLD 47

unsigned char PressCountS1 = 0;
unsigned char PressCountS2 = 0;
unsigned char Pressed = 0;
unsigned char PressRelease = 0;
unsigned char NormalMode = 0;
unsigned char State = 0; // 0 normal /1 special
void InitialiseSwitch2(void);
void OperateNormalNormalMode(void);//counter NarmolNormalMode

void main (void)
{
 WDTCTL = WDTPW + WDTHOLD; // Stop WDT
 
P1DIR |= (BIT0|BIT6|BIT4|BIT5|BIT1|BIT2); // Set the LEDs on P1.0, P1.1, P1.2 and P1.6 as outputs
 P1OUT = 0;
 P1OUT |= (BIT0|BIT6); // Turn on P1.0 and P1.6 LEDs to indicate initial state
 
InitialiseSwitch2(); // Initialise Switch 2 which is attached to P1.3
 
// The Watchdog Timer (WDT) will be used to debounce s1 and s2
 WDTCTL = WDTPW | WDTHOLD | WDTNMIES | WDTNMI; //WDT password + Stop WDT + detect RST button falling edge + set RST/NMI pin to NMI
 IFG1 &= ~(WDTIFG | NMIIFG); // Clear the WDT and NMI interrupt flags
 IE1 |= WDTIE | NMIIE; // Enable the WDT and NMI interrupts
 // The CPU is free to do other tasks, or go to sleep
 __bis_SR_register(LPM0_bits | GIE);
}

void OperateNormalNormalMode(void){
    if(NormalMode == 0){
        P1OUT &= ~(BIT5|BIT4|BIT2|BIT1);
        P1OUT |= BIT1;
        NormalMode = 1;
    }
    else if(NormalMode == 1){
        P1OUT &= ~(BIT5|BIT4|BIT2|BIT1);
        P1OUT |= BIT2 ;
        NormalMode = 2;
    }
    else if(NormalMode == 2){
        P1OUT &= ~(BIT5|BIT4|BIT2|BIT1);
        P1OUT |= BIT4 ;
        NormalMode = 3;
    }
    else if(NormalMode == 3){
        P1OUT &= ~(BIT5|BIT4|BIT2|BIT1);
        P1OUT |= BIT5 ;
        NormalMode = 0;
    }
    else{}
    return;
}
 
/* This function configures the button so it will trigger interrupts
 * when pressed. Those interrupts will be handled by PORT1_ISR() */
void InitialiseSwitch2(void)
{
 P1DIR &= ~BIT3; // Set button pin as an input pin
 P1OUT |= BIT3; // Set pull up resistor on for button
 P1REN |= BIT3; // Enable pull up resistor for button to keep pin high until pressed
 P1IES |= BIT3; // Enable Interrupt to trigger on the falling edge (high (unpressed) to low (pressed) transition)
 P1IFG &= ~BIT3; // Clear the interrupt flag for the button
 P1IE |= BIT3; // Enable interrupts on port 1 for the button
}
 
 #pragma vector = NMI_VECTOR
__interrupt void nmi_isr(void)
{
 if (IFG1 & NMIIFG) // Check if NMI interrupt was caused by nRST/NMI pin
 {
 IFG1 &= ~NMIIFG; // clear NMI interrupt flag
 if (WDTCTL & WDTNMIES) // falling edge detected
 {
 P1OUT |= BIT6; // Turn on P1.0 red LED to indicate switch 1 is pressed
 Pressed |= S1; // Set Switch 2 Pressed flag
 PressCountS1 = 0; // Reset Switch 2 long press count
 WDTCTL = WDT_MDLY_32 | WDTNMI; // WDT 32ms delay + set RST/NMI pin to NMI
 // Note: WDT_MDLY_32 = WDTPW | WDTTMSEL | WDTCNTCL // WDT password + Interval mode + clear count
 // Note: this will also set the NMI interrupt to trigger on the rising edge
 
}
 else // rising edge detected
 {
 P1OUT & ~(BIT6+BIT0); // Turn off P1.6 and P1.0 LEDs
 Pressed &= ~S1; // Reset Switch 1 Pressed flag
 PressRelease |= S1; // Set Press and Released flag
 WDTCTL = WDT_MDLY_32 | WDTNMIES | WDTNMI; // WDT 32ms delay + falling edge + set RST/NMI pin to NMI
 }
 } // Note that NMIIE is now cleared; the wdt_isr will set NMIIE 32ms later
 else {/* add code here to handle other kinds of NMI, if any */}
}
 
 #pragma vector=PORT1_VECTOR
__interrupt void PORT1_ISR(void)
{
 if (P1IFG & BIT3)
 {
 P1IE &= ~BIT3; // Disable Button interrupt to avoid bounces
 P1IFG &= ~BIT3; // Clear the interrupt flag for the button
 if (P1IES & BIT3)
 { // Falling edge detected
 P1OUT |= BIT0; // Turn on P1.0 red LED to indicate switch 2 is pressed
 Pressed |= S2; // Set Switch 2 Pressed flag
 PressCountS2 = 0; // Reset Switch 2 long press count
 }
 else
 { // Rising edge detected
 P1OUT &= ~(BIT0+BIT6); // Turn off P1.0 and P1.6 LEDs
 Pressed &= ~S2; // Reset Switch 2 Pressed flag
 PressRelease |= S2; // Set Press and Released flag
 ///////////////////////////////
 OperateNormalNormalMode();
 /*if(++NormalMode == 4)
    NormalMode = 0;*/
 //////////////////////////////
 }
 

 
 P1IES ^= BIT3; // Toggle edge detect
 IFG1 &= ~WDTIFG; // Clear the interrupt flag for the WDT
 WDTCTL = WDT_MDLY_32 | (WDTCTL & 0x007F); // Restart the WDT with the same NMI status as set by the NMI interrupt
 }
 else {/* add code here to handle other PORT1 interrupts, if any */}
}
 
// WDT is used to debounce s1 and s2 by delaying the re-enable of the NMIIE and P1IE interrupts
// and to time the length of the press
#pragma vector = WDT_VECTOR
__interrupt void wdt_isr(void)
{
 if (Pressed & S1) // Check if switch 1 is pressed
 {
    if (++PressCountS1 == TIMEHOLD ) // Long press duration 47*32ms = 1.5s
    {
        P1OUT |= BIT0; // Turn on the P1.1 LED to indicate long press
    }
 }
if (Pressed & S2) // Check if switch 2 is pressed
 {
     if (++PressCountS2 == TIMEHOLD ) // Long press duration 47*32ms = 1.5s
    {
        P1OUT |= BIT6; // Turn on the P1.2 LED to indicate long press
    }
 }
 
 IFG1 &= ~NMIIFG; // Clear the NMI interrupt flag (in case it has been set by bouncing)
 P1IFG &= ~BIT3; // Clear the button interrupt flag (in case it has been set by bouncing)
 IE1 |= NMIIE; // Re-enable the NMI interrupt to detect the next edge
 P1IE |= BIT3; // Re-enable interrupt for the button on P1.3
}