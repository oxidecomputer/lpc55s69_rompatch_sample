MEMORY
{
  FLASH : ORIGIN = 0x00000000, LENGTH = 630K

  /* for use with standard link.x */
  RAM : ORIGIN = 0x20000000, LENGTH = 256K

  /* would be used with proper link.x */
  /* needs changes to r0 (initialization code) */
  /* SRAM0 : ORIGIN = 0x20000000, LENGTH = 64K */
  /* SRAM1 : ORIGIN = 0x20010000, LENGTH = 64K */
  /* SRAM2 : ORIGIN = 0x20020000, LENGTH = 64K */
  /* SRAM3 : ORIGIN = 0x20030000, LENGTH = 64K */

  /* CASPER SRAM regions */
  /* SRAMX0: ORIGIN = 0x1400_0000, LENGTH = 4K /1* to 0x1400_0FFF *1/ */
  /* SRAMX1: ORIGIN = 0x1400_4000, LENGTH = 4K /1* to 0x1400_4FFF *1/ */

  /* USB1 SRAM regin */
  /* USB1_SRAM : ORIGIN = 0x40100000, LENGTH = 16K */

  /* To define our own USB RAM section in one regular */
  /* RAM, probably easiest to shorten length of RAM */
  /* above, and use this freed RAM section */

}


ENTRY(_my_entry);

SECTIONS
{
  PROVIDE(_stack_start = ORIGIN(RAM) + LENGTH(RAM));

  PROVIDE(_stext = ORIGIN(FLASH));

  /* ### .text */
  .text _stext :
  {
	LONG(_stack_start);
	LONG(_my_entry);
	LONG(nmi|1);
	LONG(hard_fault|1);
	LONG(mm_fault|1);
	LONG(bus_fault|1);
	LONG(usage_fault|1);
	LONG(secure_fault|1);
	LONG(loop4ever|1);
	LONG(loop4ever|1);
	LONG(loop4ever|1);
	LONG(handle_svc|1);
	LONG(loop4ever|1);
	LONG(loop4ever|1);
	LONG(loop4ever|1);
	LONG(loop4ever|1);
	. = . + 0x1000;
    *(.text .text.*);
    . = ALIGN(4);
    __etext = .;
  } > FLASH =0xdededede

  /* ### .rodata */
  .rodata __etext : ALIGN(4)
  {
    *(.rodata .rodata.*);

    /* 4-byte align the end (VMA) of this section.
       This is required by LLD to ensure the LMA of the following .data
       section will have the correct alignment. */
    . = ALIGN(4);
    __erodata = .;
  } > FLASH

  /* ## Sections in RAM */
  /* ### .data */
  .data : AT(__erodata) ALIGN(4)
  {
    . = ALIGN(4);
    __sdata = .;
    *(.data .data.*);
    . = ALIGN(4); /* 4-byte align the end (VMA) of this section */
    __edata = .;
  } > RAM

  /* LMA of .data */
  __sidata = LOADADDR(.data);

  /* ### .bss */
  .bss : ALIGN(4)
  {
    . = ALIGN(4);
    __bss_start__ =  .;
    __sbss = .;
    *(.bss .bss.*);
    . = ALIGN(4); /* 4-byte align the end (VMA) of this section */
    __ebss = .;
    __bss_end__ =  .;
  } > RAM

  /* ### .uninit */
  .uninit (NOLOAD) : ALIGN(4)
  {
    . = ALIGN(4);
    *(.uninit .uninit.*);
    . = ALIGN(4);
  } > RAM

  /* Place the heap right after `.uninit` */
  . = ALIGN(4);
  __sheap = .;

  /* ## .got */
  /* Dynamic relocations are unsupported. This section is only used to detect relocatable code in
     the input files and raise an error if relocatable code is found */
  .got (NOLOAD) :
  {
    KEEP(*(.got .got.*));
  }

  /* ## Discarded sections */
  /DISCARD/ :
  {
    /* Unused exception related info that only wastes space */
    *(.ARM.exidx);
    *(.ARM.exidx.*);
    *(.ARM.extab.*);
  }
}
