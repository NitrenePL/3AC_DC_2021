
MEMORY
{
PAGE 0 :
   /* BEGIN is used for the "boot to Flash" bootloader mode   */

   BEGIN           	: origin = 0x080000, length = 0x000002
   RAMM0           	: origin = 0x0000F6, length = 0x00030A

   RAMLS_CODE        : origin = 0x008000, length = 0x002800
   RESET           	: origin = 0x3FFFC0, length = 0x000002

   /*
    * Flash application region.
    * BEGIN keeps the boot entry at 0x080000, and the last 0x10 words are
    * reserved per the "Memory: Prefetching Beyond Valid Memory" advisory.
    */
   FLASH_APP        : origin = 0x080002, length = 0x01FFEE

PAGE 1 :

   BOOT_RSVD        : origin = 0x000002, length = 0x0000F1     /* Part of M0, BOOT rom will use this for stack */
   RAMM1            : origin = 0x000400, length = 0x0003F8     /* on-chip RAM block M1 */
//   RAMM1_RSVD      : origin = 0x0007F8, length = 0x000008     /* Reserve and do not use for code as per the errata advisory "Memory: Prefetching Beyond Valid Memory" */

   RAMLS_DATA  : origin = 0x00A800, length = 0x001800

   /* Last 0x8 words reserved per the "Memory: Prefetching Beyond Valid Memory" advisory. */
   RAMGS_DATA  : origin = 0x00C000, length = 0x007FF8
}


SECTIONS
{
   codestart        : > BEGIN,     PAGE = 0, ALIGN(4)
   .text            : > FLASH_APP,  PAGE = 0, ALIGN(4)
   .cinit           : > FLASH_APP,  PAGE = 0, ALIGN(4)
   .switch          : > FLASH_APP,  PAGE = 0, ALIGN(4)
   .reset           : > RESET,     PAGE = 0, TYPE = DSECT /* not used, */

   .stack           : > RAMM1,     PAGE = 1

#if defined(__TI_EABI__)
   .init_array      : > FLASH_APP,    PAGE = 0, ALIGN(4)
   .bss             : > RAMLS_DATA,   PAGE = 1
   .bss:output      : > RAMLS_DATA,   PAGE = 1
   .bss:cio         : > RAMLS_DATA,   PAGE = 1
   .data            : > RAMLS_DATA,   PAGE = 1
   .sysmem          : > RAMLS_DATA,   PAGE = 1
   /* Initalized sections go in Flash */
   .const           : > FLASH_APP,    PAGE = 0, ALIGN(4)
#else
   .pinit           : > FLASH_APP,    PAGE = 0, ALIGN(4)
   .ebss            : > RAMLS_DATA,   PAGE = 1
   .esysmem         : > RAMLS_DATA,   PAGE = 1
   .cio             : > RAMLS_DATA,   PAGE = 1
   .econst          : > FLASH_APP,    PAGE = 0, ALIGN(4)
#endif

   ramgs0           : > RAMGS_DATA, PAGE = 1
   ramgs1           : > RAMGS_DATA, PAGE = 1

 
#if defined(__TI_EABI__) 
      GROUP{
      .TI.ramfunc
      dclfuncs
      }               LOAD = FLASH_APP,
                      RUN = RAMLS_CODE,
                      LOAD_START(RamfuncsLoadStart),
                      LOAD_SIZE(RamfuncsLoadSize),
                      LOAD_END(RamfuncsLoadEnd),
                      RUN_START(RamfuncsRunStart),
                      RUN_SIZE(RamfuncsRunSize),
                      RUN_END(RamfuncsRunEnd),
                      PAGE = 0, ALIGN(4)
#else					  
   GROUP{
      .TI.ramfunc
      dclfuncs
      }               LOAD = FLASH_APP,
                      RUN = RAMLS_CODE,
                      LOAD_START(_RamfuncsLoadStart),
                      LOAD_SIZE(_RamfuncsLoadSize),
                      LOAD_END(_RamfuncsLoadEnd),
                      RUN_START(_RamfuncsRunStart),
                      RUN_SIZE(_RamfuncsRunSize),
                      RUN_END(_RamfuncsRunEnd),
                      PAGE = 0, ALIGN(4)

#endif

   controlVariables : > RAMLS_DATA, PAGE = 1
   logVariables     : > RAMGS_DATA, PAGE = 1

}

/*
//===========================================================================
// End of file.
//===========================================================================
*/
