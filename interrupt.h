#include "address_map_arm.h"
#include "interrupt_ID.h"
#include "defines.h"

void pushbutton_ISR (void);
void config_interrupt (int, int);
void config_KEYs (void);
void disable_A9_interrupts (void);
void set_A9_IRQ_stack (void);
void config_GIC (void);
void config_KEYs (void);
void enable_A9_interrupts (void);


extern bool mario_move_forward;
extern bool mario_move_backward;
extern bool mario_jump;

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
  	config_interrupt (KEYS_IRQ, CPU0); 	// configure the FPGA KEYs interrupt
    
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
    }else if (press & 0x4){
        LED_bits = 0b100;
    }else if (press & 0x8){
        mario_jump = true;
        LED_bits = 0b1000;
    }

	*LED_ptr = LED_bits;
	return;
}