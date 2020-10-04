
.globl _my_entry
_my_entry:
	b main

.globl return_to_user
return_to_user:
	# Set the user stack pointer to be the bottom of the frame
	movw r0, #0x1f98
	movt r0, #0x2000
	msr PSP, r0

	# Drop privileges from thread mode
	movs r0, #1
	msr CONTROL, r0

	# Set EXC_RETURN mode to return into thread mode
	movw r1, #0xffed
	movt r1, #0xffff
	mov lr, r1
	bx lr

.globl handle_svc
handle_svc:
	# Check if we're coming from the kernel for the first time
	movw r0, #0xfff9
	movt r0, #0xffff
	cmp lr, r0
	beq return_to_user
	# otherwise just bounce back up
	bx lr

.globl nmi
nmi:
	b loop4ever

.globl hard_fault
hard_fault:
	b loop4ever


.globl mm_fault
mm_fault:
	b loop4ever

.globl bus_fault
bus_fault:
	b blink_blue

.globl usage_fault
usage_fault:
	b loop4ever

.globl secure_fault
secure_fault:
	b loop4ever

.globl loop4ever
loop4ever:
	b loop4ever
