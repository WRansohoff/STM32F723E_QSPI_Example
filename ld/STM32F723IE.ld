/* Label for the program's entry point */
ENTRY(reset_handler)

/* End of RAM/Start of stack */
/* (176KB in SRAM1) TODO: Use SRAM2 for stack? */
_estack = 0x2003C000;

/* Set minimum size for stack and dynamic memory. */
/* (The linker will generate an error if there is
 * less than this much RAM leftover.) */
/* (1KB) */
_Min_Leftover_RAM = 0x400;

MEMORY
{
    FLASH   ( rx )    : ORIGIN = 0x08000000, LENGTH = 512K
    RAM2    ( rxw )   : ORIGIN = 0x2003C000, LENGTH = 16K
    RAM     ( rxw )   : ORIGIN = 0x20010000, LENGTH = 176K
    DTCMRAM ( rxw )   : ORIGIN = 0x20000000, LENGTH = 64K
    ITCMRAM ( rxw )   : ORIGIN = 0x00000000, LENGTH = 16K
}

INCLUDE "sections.ld"
