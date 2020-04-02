#include "address_map_arm.h"
#include "interrupt_ID.h"
#include "defines.h"


/*Constant defines here*/
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define LOWEST_Y 202
#define OUT_SCREEN -50
#define ENDING 254

extern bool mario_move_forward;
extern bool mario_move_backward;
extern bool mario_jump;
extern bool mario_fall;
extern bool isGameOver;
extern bool isWin;

extern int MARIO_JUMP_HIGHT;
//which map 
extern int map_num;

extern int lives;
extern int score;
extern int time_left;

//Mario's position
extern int mario_x;
extern int mario_y;

//up to three bad mushrooms
extern bool isBadMushroom[3];
extern int badMushroom_x[3];
extern int badMushroom_y[3];
extern bool isBadMushroomMovingRight[3];

//up to three moneys
extern bool isMoney[3];
extern int money_x[3];
extern int money_y[3];

//Good mushroom
extern bool isGoodMushroom;
extern int goodMushroom_x;
extern int goodMushroom_y;

//turtle
extern bool isTurtle;
extern bool isTurtleMovingRight;
extern int turtle_x ;
extern int turtle_y;

volatile char byte1 = 0;
volatile char byte2 = 0;
volatile char byte3 = 0; // PS/2 variables

void pushbutton_ISR (void);
void config_interrupt (int, int);
void config_KEYs (void);
void disable_A9_interrupts (void);
void set_A9_IRQ_stack (void);
void config_GIC (void);
void config_KEYs (void);
void config_PS2();
void config_interval_timer(void);
void enable_A9_interrupts (void);
void PS2_ISR( void );
void interval_timer_ISR();
void reset();

// Define the IRQ exception handler
void __attribute__ ((interrupt)) __cs3_isr_irq (void)
{
	// Read the ICCIAR from the CPU interface in the GIC
	int address;
	int interrupt_ID;
	
	address = MPCORE_GIC_CPUIF + ICCIAR;
	interrupt_ID = *(int *)address;
   
	if (interrupt_ID == KEYS_IRQ)		// check if interrupt is from the KEYs
		pushbutton_ISR ();
	else if (interrupt_ID == PS2_IRQ)				// check if interrupt is from the PS/2
		PS2_ISR ();
	else if (interrupt_ID ==
             INTERVAL_TIMER_IRQ) // check if interrupt is from the Altera timer
        interval_timer_ISR();
	else
		while (1);							// if unexpected, then stay here

	// Write to the End of Interrupt Register (ICCEOIR)
	address = MPCORE_GIC_CPUIF + ICCEOIR;
	*(int *)address = interrupt_ID;

	return;
} 

// Define the remaining exception handlers
void __attribute__ ((interrupt)) __cs3_reset (void)
{
    while(1);
}

void __attribute__ ((interrupt)) __cs3_isr_undef (void)
{
    while(1);
}

void __attribute__ ((interrupt)) __cs3_isr_swi (void)
{
    while(1);
}

void __attribute__ ((interrupt)) __cs3_isr_pabort (void)
{
    while(1);
}

void __attribute__ ((interrupt)) __cs3_isr_dabort (void)
{
    while(1);
}

void __attribute__ ((interrupt)) __cs3_isr_fiq (void)
{
    while(1);
}

/* 
 * Turn off interrupts in the ARM processor
*/
void disable_A9_interrupts(void)
{
	int status = 0b11010011;
	asm("msr cpsr, %[ps]" : : [ps]"r"(status));
}

/* 
 * Initialize the banked stack pointer register for IRQ mode
*/
void set_A9_IRQ_stack(void)
{
	int stack, mode;
	stack = A9_ONCHIP_END - 7;		// top of A9 onchip memory, aligned to 8 bytes
	/* change processor to IRQ mode with interrupts disabled */
	mode = INT_DISABLE | IRQ_MODE;
	asm("msr cpsr, %[ps]" : : [ps] "r" (mode));
	/* set banked stack pointer */
	asm("mov sp, %[ps]" : : [ps] "r" (stack));

	/* go back to SVC mode before executing subroutine return! */
	mode = INT_DISABLE | SVC_MODE;
	asm("msr cpsr, %[ps]" : : [ps] "r" (mode));
}

/* 
 * Turn on interrupts in the ARM processor
*/
void enable_A9_interrupts(void)
{
	int status = SVC_MODE | INT_ENABLE;
	asm("msr cpsr, %[ps]" : : [ps]"r"(status));
}

/* 
 * Configure the Generic Interrupt Controller (GIC)
*/
void config_GIC(void)
{
	int address;

	/* configure the FPGA interval timer and KEYs interrupts */
    *((int *)0xFFFED848) = 0x00000101;
    *((int *)0xFFFED108) = 0x00000300;

  	config_interrupt (KEYS_IRQ, CPU0); 	// configure the FPGA KEYs interrupt
	config_interrupt (PS2_IRQ, CPU0);  
    
  	// Set Interrupt Priority Mask Register (ICCPMR). Enable interrupts of all priorities 
	address = MPCORE_GIC_CPUIF + ICCPMR;
  	*(int *) address = 0xFFFF;       

  	// Set CPU Interface Control Register (ICCICR). Enable signaling of interrupts
	address = MPCORE_GIC_CPUIF + ICCICR;
  	*(int *) address = 1;       

	// Configure the Distributor Control Register (ICDDCR) to send pending interrupts to CPUs 
	address = MPCORE_GIC_DIST + ICDDCR;
  	*(int *) address = 1;          
}

/* 
 * Configure registers in the GIC for an individual interrupt ID
 * We configure only the Interrupt Set Enable Registers (ICDISERn) and Interrupt 
 * Processor Target Registers (ICDIPTRn). The default (reset) values are used for 
 * other registers in the GIC
*/
void config_interrupt (int N, int CPU_target)
{
	int reg_offset, index, value, address;
    
	/* Configure the Interrupt Set-Enable Registers (ICDISERn). 
	 * reg_offset = (integer_div(N / 32) * 4
	 * value = 1 << (N mod 32) */
	reg_offset = (N >> 3) & 0xFFFFFFFC; 
	index = N & 0x1F;
	value = 0x1 << index;
	address = MPCORE_GIC_DIST + ICDISER + reg_offset;
	/* Now that we know the register address and value, set the appropriate bit */
   *(int *)address |= value;

	/* Configure the Interrupt Processor Targets Register (ICDIPTRn)
	 * reg_offset = integer_div(N / 4) * 4
	 * index = N mod 4 */
	reg_offset = (N & 0xFFFFFFFC);
	index = N & 0x3;
	address = MPCORE_GIC_DIST + ICDIPTR + reg_offset + index;
	/* Now that we know the register address and value, write to (only) the appropriate byte */
	*(char *)address = (char) CPU_target;
}

/* setup the KEY interrupts in the FPGA */
void config_KEYs()
{
	volatile int * KEY_ptr = (int *) KEY_BASE;	// pushbutton KEY base address

	*(KEY_ptr + 2) = 0xF; 	// enable interrupts for the two KEYs
}

/* setup the PS/2 interrupts */
void config_PS2() {
    volatile int * PS2_ptr = (int *)PS2_BASE; // PS/2 port address

    *(PS2_ptr) = 0xFF; /* reset */
    *(PS2_ptr + 1) =
        0x1; /* write to the PS/2 Control register to enable interrupts */
}

/* setup the interval timer interrupts in the FPGA */
void config_interval_timer()
{
    volatile int * interval_timer_ptr =
        (int *)TIMER_BASE; // interal timer base address

    /* set the interval timer period for scrolling the HEX displays */
    int counter                 = 1E8; // 1/(100 MHz) x 10 ^ 8 = 1sec
    *(interval_timer_ptr + 0x2) = (counter & 0xFFFF);
    *(interval_timer_ptr + 0x3) = (counter >> 16) & 0xFFFF;

    /* start interval timer, enable its interrupts */
    *(interval_timer_ptr + 1) = 0x7; // STOP = 0, START = 1, CONT = 1, ITO = 1
}


//Iterrupt Service Routine
void pushbutton_ISR( void )
{
	volatile int * KEY_ptr = (int *) KEY_BASE;
	volatile int * LED_ptr = (int *) LED_BASE;
	int press, LED_bits;

	press = *(KEY_ptr + 3);					// read the pushbutton interrupt register
	*(KEY_ptr + 3) = press;					// Clear the interrupt
    
	if (press & 0x1){							// KEY0
        mario_move_forward = true;
        LED_bits = 0b1;
    }else if (press & 0x2){					// KEY1
        mario_move_backward = true;
        LED_bits = 0b10;
    }else if (press & 0x4){					// KEY2
		mario_jump = true;
        LED_bits = 0b100;
    }else if (press & 0x8){					// KEY3
        if (isWin || isGameOver){
			reset();
		}
        LED_bits = 0b1000;
    }

	*LED_ptr = LED_bits;
	return;
}

void PS2_ISR( void )
{
  	volatile int * PS2_ptr = (int *) 0xFF200100;		// PS/2 port address
	int PS2_data, RAVAIL;

	PS2_data = *(PS2_ptr);									// read the Data register in the PS/2 port
	RAVAIL = (PS2_data & 0xFFFF0000) >> 16;			// extract the RAVAIL field
	if (RAVAIL > 0)
	{
		/* always save the last three bytes received */
		byte1 = byte2;
		byte2 = byte3;
		byte3 = PS2_data & 0xFF;
		if ( (byte2 == (char) 0xE0) && (byte3 == (char) 0x6B) ) //left arrow
			mario_move_backward = true;
		else if ( (byte2 == (char) 0xE0) && (byte3 == (char) 0x74) ) //right arrow
			mario_move_forward = true;
		else if (byte3 == (char) 0x29) //space bar
			mario_jump = true;
		else if ( (byte2 == (char) 0xE0) && (byte3 == (char) 0x5A) && (isWin || isGameOver)){
			reset();
		}
	}
	return;
}

void reset(){
	//which map 
	map_num = 1;

	lives = 1;

	score = 0;

	time_left = 200;

	//Mario's position
	mario_x = 10;
	mario_y = LOWEST_Y - 25;

	//up to three bad mushrooms
	isBadMushroom[0] = true;
	isBadMushroom[1] = true;
	isBadMushroom[2] = false;
	badMushroom_x[0] = 179;
	badMushroom_x[1] = 152 - 19;
	badMushroom_x[2] = OUT_SCREEN;
	badMushroom_y[0] = LOWEST_Y - 19;
	badMushroom_y[1] = 131 - 19;
	badMushroom_y[2] = OUT_SCREEN;
	isBadMushroomMovingRight[0] = false;
	isBadMushroomMovingRight[1] = false;
	isBadMushroomMovingRight[2] = false;

	//up to three moneys
	isMoney[0] = false;
	isMoney[1] = false;
	isMoney[2] = false;
	money_x[0] = 67;
	money_x[1] = OUT_SCREEN;
	money_x[2] = OUT_SCREEN;
	money_y[0] = 112;
	money_y[1] = OUT_SCREEN;
	money_y[2] = OUT_SCREEN;

	//Good mushroom
	isGoodMushroom = false;
	goodMushroom_x = 118;
	goodMushroom_y = 112;

	//turtle
	isTurtle = false;
	isTurtleMovingRight = true;
	turtle_x = 43;
	turtle_y = 93 - 28;

	byte1 = 0;
	byte2 = 0;
	byte3 = 0; // PS/2 variables

	isWin = false;
	isGameOver = false;
}

void interval_timer_ISR()
{
    volatile int * interval_timer_ptr = (int *)TIMER_BASE;

    *(interval_timer_ptr) = 0; // Clear the interrupt

	if (!isWin && !isGameOver){
		time_left--;
		if (time_left == 0){
			isGameOver = true;
		}
	}
    return;
}