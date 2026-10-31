#include "apsis_controls.h"

static uint32_t apsis_button_bit(uint8_t button)
{
    return 1u << button;
}

uint8_t apsis_controls_surface_from_fm1(const ApsisFm1InputFrame *input,
                                        uint8_t current_surface)
{
    uint32_t p, h;
    uint8_t current = current_surface < APSIS_SURFACE_COUNT ?
                      current_surface : APSIS_SURFACE_MOTHER;

    if (!input) return current;
    p = input->buttons_pressed;
    h = input->buttons_held;

    /* HOME is intentionally absolute even if another modifier/button edge is
     * present in the same consumed input frame. */
    if (p & apsis_button_bit(APSIS_FM1_BTN_HOME))
        return APSIS_SURFACE_MOTHER;

    /* SPACE is a secondary shared-room surface, not a sixth source. The FX
     * press edge is interpreted contextually from SEL's held state. */
    if ((h & apsis_button_bit(APSIS_FM1_BTN_SEL)) &&
        (p & apsis_button_bit(APSIS_FM1_BTN_FX)))
        return APSIS_SURFACE_SPACE;

    if (p & apsis_button_bit(APSIS_FM1_BTN_ENV))
        return APSIS_SURFACE_STRING;
    if (p & apsis_button_bit(APSIS_FM1_BTN_LFO))
        return APSIS_SURFACE_DRIFT;
    if (p & apsis_button_bit(APSIS_FM1_BTN_SEQ))
        return APSIS_SURFACE_CLOCK;
    if (p & apsis_button_bit(APSIS_FM1_BTN_ARP))
        return APSIS_SURFACE_SPIRAL;
    if (p & apsis_button_bit(APSIS_FM1_BTN_FX))
        return APSIS_SURFACE_BLOOM;
    if (p & apsis_button_bit(APSIS_FM1_BTN_GLO))
        return APSIS_SURFACE_GRAVITY;
    if (p & apsis_button_bit(APSIS_FM1_BTN_EDIT))
        return APSIS_SURFACE_CHAIN;
    if (p & apsis_button_bit(APSIS_FM1_BTN_SAVE))
        return APSIS_SURFACE_KEEP;

    return current;
}

ApsisGlobalAction apsis_controls_global_action(const ApsisFm1InputFrame *input)
{
    uint32_t play, select;
    if (!input) return APSIS_GLOBAL_ACTION_NONE;
    play = apsis_button_bit(APSIS_FM1_BTN_PLAY);
    select = apsis_button_bit(APSIS_FM1_BTN_SEL);
    if ((input->buttons_pressed & play) && !(input->buttons_held & select))
        return APSIS_GLOBAL_ACTION_TRANSPORT_TOGGLE;
    return APSIS_GLOBAL_ACTION_NONE;
}
