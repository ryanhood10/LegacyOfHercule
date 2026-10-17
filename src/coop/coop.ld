/* New code goes into the second 8 MiB of a padded 16 MiB ROM; globals into the top of EWRAM, which
   tools/patch_rom.py keeps out of the game's heap (Arena_Init size patch). */
ENTRY(idle_update_hook)
EXTERN(walk_update diag_update input_poll_hook hud_draw_hook hud_update_hook hud_destroy_hook g_p2_remote g_p1_remote
       dash_update_hook ctrl_8277_hook ctrl_85ad_hook entity_list_update_hook camera_follow_hook
       exp_award_hook)

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

    .coop (NOLOAD) : {
        KEEP(*(.coop.0))
        KEEP(*(.coop.0p1))
        . = ALIGN(4);
        KEEP(*(.coop.1))
    } > ewram

    .bss (NOLOAD) : {
        *(.bss*)
        *(COMMON)
    } > ewram

    /DISCARD/ : { *(.ARM.exidx*) *(.ARM.attributes) *(.comment) *(.data*) }
}
