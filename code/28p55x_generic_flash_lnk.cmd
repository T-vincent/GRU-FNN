MEMORY
{
   BEGIN            : origin = 0x080000, length = 0x000002  // Update the codestart location as needed

   BOOT_RSVD        : origin = 0x000002, length = 0x000126     /* Part of M0, BOOT rom will use this for stack */
   RAMM0            : origin = 0x000128, length = 0x0002D8
   RAMM1            : origin = 0x000400, length = 0x000400

   RAMLS_CODE : origin = 0x008000, length = 0x002800
   RAMLS5           : origin = 0x00A800, length = 0x000800
   RAMLS6           : origin = 0x00B000, length = 0x000800
   RAMLS7           : origin = 0x00B800, length = 0x000800
   RAMLS8           : origin = 0x014000, length = 0x002000  // When configured as CLA program use the address 0x4000
   RAMLS9           : origin = 0x016000, length = 0x002000  // When configured as CLA program use the address 0x6000

   // RAMLS8_CLA    : origin = 0x004000, length = 0x002000  // Use only if configured as CLA program memory
   // RAMLS9_CLA    : origin = 0x006000, length = 0x002000  // Use only if configured as CLA program memory

   RAMGS0           : origin = 0x00C000, length = 0x002000
   RAMGS1           : origin = 0x00E000, length = 0x002000

   /* Flash Banks (128 sectors each) */
   FLASH_BANK0     : origin = 0x080002, length = 0x1FFFE
   FLASH_BANK1     : origin = 0x0A0000, length = 0x20000


   CLATOCPURAM      : origin = 0x001480,   length = 0x000080
   CPUTOCLARAM      : origin = 0x001500,   length = 0x000080
   CLATODMARAM      : origin = 0x001680,   length = 0x000080
   DMATOCLARAM      : origin = 0x001700,   length = 0x000080

   RESET            : origin = 0x3FFFC0, length = 0x000002
}


SECTIONS
{
   Cla1Prog : LOAD=FLASH_BANK0, RUN=RAMLS7, LOAD_START(Cla1ProgLoadStart), LOAD_SIZE(Cla1ProgLoadSize), RUN_START(Cla1ProgRunStart), ALIGN(4)
   CpuToCla1MsgRAM : > CPUTOCLARAM, TYPE=NOINIT
   Cla1ToCpuMsgRAM : > CLATOCPURAM, TYPE=NOINIT
   .scratchpad : > RAMLS6
   .bss_cla : > RAMLS6
   .const_cla : > RAMLS6
   codestart        : > BEGIN
   .text            : >> FLASH_BANK0 | FLASH_BANK1, ALIGN(8)
   .cinit           : > FLASH_BANK0, ALIGN(8)
   .switch          : > FLASH_BANK0, ALIGN(8)
   .reset           : > RESET, TYPE = DSECT /* not used, */
   .gru_const : LOAD = FLASH_BANK0, RUN = RAMLS8, LOAD_START(GruConstLoadStart), LOAD_SIZE(GruConstLoadSize), RUN_START(GruConstRunStart), ALIGN(8)


   .rodata.tvm : { -lmod.a(.const) } LOAD = FLASH_BANK0, RUN = RAMLS8, LOAD_START(NpuConstLoadStart), LOAD_SIZE(NpuConstLoadSize), RUN_START(NpuConstRunStart), ALIGN(16)
   .bss.noinit.tvm : > RAMGS1, ALIGN(16)
   .dma_payload : > RAMGS1, ALIGN(4)
   .npu_io : > RAMGS1, ALIGN(16)
   .gru_sig_r : LOAD=FLASH_BANK1, RUN=RAMLS9, LOAD_START(SigRLoadStart), LOAD_SIZE(SigRLoadSize), RUN_START(SigRRunStart), ALIGN(8)
   .gru_sig_z : LOAD=FLASH_BANK1, RUN=RAMGS0, LOAD_START(SigZLoadStart), LOAD_SIZE(SigZLoadSize), RUN_START(SigZRunStart), ALIGN(8)
   .stack           : > RAMM1
#if defined(__TI_EABI__)
   .bss             : > RAMLS5
   .bss:output      : > RAMGS1
   .init_array      : > FLASH_BANK0, ALIGN(8)
   .const           : > FLASH_BANK0, ALIGN(8)
   .data            : > RAMLS5  | RAMLS6
   .sysmem          : > RAMGS1
#else
   .pinit           : > FLASH_BANK0, ALIGN(8)
   .ebss            : >> RAMLS5 | RAMLS6
   .econst          : > FLASH_BANK0, ALIGN(8)
   .esysmem         : > RAMLS5
#endif

   ramgs0 : > RAMGS0
   ramgs1 : > RAMGS1


   #if defined(__TI_EABI__)
       .TI.ramfunc : { -lmod.a(.text) f28p55x_npu.obj(.text) } LOAD = FLASH_BANK0,
                        RUN = RAMLS_CODE,
                        LOAD_START(RamfuncsLoadStart),
                        LOAD_SIZE(RamfuncsLoadSize),
                        LOAD_END(RamfuncsLoadEnd),
                        RUN_START(RamfuncsRunStart),
                        RUN_SIZE(RamfuncsRunSize),
                        RUN_END(RamfuncsRunEnd),
                        ALIGN(8)
   #else
       .TI.ramfunc : { -lmod.a(.text) f28p55x_npu.obj(.text) } LOAD = FLASH_BANK0,
                        RUN = RAMLS_CODE,
                        LOAD_START(_RamfuncsLoadStart),
                        LOAD_SIZE(_RamfuncsLoadSize),
                        LOAD_END(_RamfuncsLoadEnd),
                        RUN_START(_RamfuncsRunStart),
                        RUN_SIZE(_RamfuncsRunSize),
                        RUN_END(_RamfuncsRunEnd),
                        ALIGN(8)
   #endif

}

/*
//===========================================================================
// End of file.
//===========================================================================
*/
