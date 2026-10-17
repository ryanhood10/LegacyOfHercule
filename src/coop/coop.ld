/* New code goes into the second 8 MiB of a padded 16 MiB ROM; globals into the top of EWRAM. */
ENTRY(idle_update_hook)

MEMORY
{
    rom   (rx)  : ORIGIN = 0x08800000, LENGTH = 0x00100000
    ewram (rw)  : ORIGIN = 0x0203FF00, LENGTH = 0x00000100
}

SECTIONS
{
    .text : {
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
    } > rom

    .bss (NOLOAD) : {
        *(.bss*)
        *(COMMON)
    } > ewram

    /DISCARD/ : { *(.ARM.exidx*) *(.ARM.attributes) *(.comment) *(.data*) }
}
