/* New code goes into the second 8 MiB of a padded 16 MiB ROM; globals into the top of EWRAM, which
   tools/patch_rom.py keeps out of the game's heap (Arena_Init size patch). */
ENTRY(idle_update_hook)
EXTERN(walk_update diag_update input_poll_hook hud_draw_hook hud_update_hook hud_destroy_hook g_p2_remote g_p1_remote
       dash_update_hook ctrl_8277_hook ctrl_85ad_hook entity_list_update_hook camera_follow_hook
       exp_award_hook enemy_spawn_hook damage_calc_hook ai_speed8_create_hook ai_speed4_create_hook
       bossbar_create_hook camera_shake_hook vm_coop_call coop2 sprite_emit_hook player_msg_hook p1_take_damage_hook hercule_awaken
       g_party_block party_init_hook party_add_exp_hook flag_test_hook flag_set_hook flag_clear_hook flag_cond_hook
       vm_counter_get_hook vm_counter_dec_hook vm_counter_sub_hook item_use_hook vm_seen_hook
       save_game_hook load_slot_hook write_slot_hook save_menu_load_hook save_menu_any_hook save_menu_saved_hook
       save_menu_location_hook save_menu_count_hook save_menu_member_in_party_hook save_menu_member_selectable_hook
       save_menu_active_pos_hook save_menu_member_sprite_hook save_menu_set_active_hook save_menu_erase_hook save_menu_start_hook
       switch_view switch_create_hook switch_next_hook switch_prev_hook switch_select_hook switch_arrows_hook switch_update_hook
       hercule_punch_ready hercule_punch_use hercule_kick_ready hercule_kick_use hercule_megaton_ready hercule_megaton_use hercule_taunt_ready hercule_taunt_use ssj_ready ssj_use sprite_index display_rows display_row_count ssj_record
       ui_strings portrait_module status_module status_title_hook status_pages_hook status_draw_hook
       vm_push_name_hook vm_push_caps_hook kiblast_spawn_hook krillin_rush_ready krillin_rush_use krillin_shock_ready
       krillin_shock_use krillin_disc_ready krillin_disc_use kr_disc_icon_0 kr_disc_icon_1 kr_state krillin_record)

MEMORY
{
    rom   (rx)  : ORIGIN = 0x08800000, LENGTH = 0x00100000
    ewram (rw)  : ORIGIN = 0x0203FF00, LENGTH = 0x00000100
    vtram (rw)  : ORIGIN = 0x0203FE00, LENGTH = 0x00000100   /* below the pinned block, above the heap */
    ewram2 (rw) : ORIGIN = 0x0203FB00, LENGTH = 0x00000300   /* co-op difficulty state (enemies.c) */
    partyram (rw) : ORIGIN = 0x0203F000, LENGTH = 0x00000B00 /* relocated party block + save-menu slot store (party.c);
                                                                 the heap cap in tools/patch_rom.py ends here */
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

    .coopvt (NOLOAD) : {
        KEEP(*(.coop.vt))
    } > vtram

    .coop2 (NOLOAD) : {
        KEEP(*(.coop2))
    } > ewram2

    .party (NOLOAD) : {
        KEEP(*(.party))
        KEEP(*(.party.kr))   /* krillin.c state, after the pinned party block */
    } > partyram

    .bss (NOLOAD) : {
        *(.bss*)
        *(COMMON)
    } > ewram

    /DISCARD/ : { *(.ARM.exidx*) *(.ARM.attributes) *(.comment) *(.data*) }
}
