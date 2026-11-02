#include <stdio.h>
#include <string.h>
#include "apsis_controls.h"
#include "apsis_harmony.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL:%d: %s\n", __LINE__, #x); return 1; } } while (0)
static uint32_t bit(uint8_t b) { return 1u << b; }
static int32_t absi(int32_t v) { return v < 0 ? -v : v; }

static uint8_t nav(uint8_t button, uint8_t current)
{
    ApsisFm1InputFrame in; memset(&in, 0, sizeof in);
    in.buttons_pressed = bit(button); in.buttons_held = bit(button);
    return apsis_controls_surface_from_fm1(&in, current);
}

static int test_p9a(void)
{
    ApsisFm1InputFrame in;
    static const struct { uint8_t button, surface; } direct[] = {
        {APSIS_FM1_BTN_HOME, APSIS_SURFACE_MOTHER},{APSIS_FM1_BTN_ENV, APSIS_SURFACE_STRING},
        {APSIS_FM1_BTN_LFO, APSIS_SURFACE_DRIFT},{APSIS_FM1_BTN_SEQ, APSIS_SURFACE_CLOCK},
        {APSIS_FM1_BTN_ARP, APSIS_SURFACE_SPIRAL},{APSIS_FM1_BTN_FX, APSIS_SURFACE_BLOOM},
        {APSIS_FM1_BTN_GLO, APSIS_SURFACE_GRAVITY},{APSIS_FM1_BTN_EDIT, APSIS_SURFACE_CHAIN},
        {APSIS_FM1_BTN_SAVE, APSIS_SURFACE_KEEP}
    };
    uint32_t i;
    CHECK(APSIS_FM1_BUTTON_COUNT==14u && APSIS_FM1_ENCODER_COUNT==7u && APSIS_FM1_KEY_COUNT==27u && APSIS_SURFACE_COUNT==10u);
    for(i=0;i<sizeof direct/sizeof direct[0];++i) CHECK(nav(direct[i].button,APSIS_SURFACE_MOTHER)==direct[i].surface);
    memset(&in,0,sizeof in); in.buttons_pressed=bit(APSIS_FM1_BTN_FX); in.buttons_held=bit(APSIS_FM1_BTN_SEL)|bit(APSIS_FM1_BTN_FX);
    CHECK(apsis_controls_surface_from_fm1(&in,APSIS_SURFACE_BLOOM)==APSIS_SURFACE_SPACE);
    in.buttons_pressed|=bit(APSIS_FM1_BTN_SEL); CHECK(apsis_controls_surface_from_fm1(&in,APSIS_SURFACE_BLOOM)==APSIS_SURFACE_SPACE);
    memset(&in,0,sizeof in); in.buttons_pressed=bit(APSIS_FM1_BTN_SEL); in.buttons_held=bit(APSIS_FM1_BTN_SEL)|bit(APSIS_FM1_BTN_FX);
    CHECK(apsis_controls_surface_from_fm1(&in,APSIS_SURFACE_BLOOM)==APSIS_SURFACE_BLOOM);
    memset(&in,0,sizeof in); in.buttons_pressed=bit(APSIS_FM1_BTN_HOME)|bit(APSIS_FM1_BTN_FX); in.buttons_held=bit(APSIS_FM1_BTN_HOME)|bit(APSIS_FM1_BTN_SEL)|bit(APSIS_FM1_BTN_FX);
    CHECK(apsis_controls_surface_from_fm1(&in,APSIS_SURFACE_SPACE)==APSIS_SURFACE_MOTHER);
    memset(&in,0,sizeof in); in.buttons_pressed=bit(APSIS_FM1_BTN_PLAY); in.buttons_held=bit(APSIS_FM1_BTN_PLAY);
    CHECK(apsis_controls_global_action(&in)==APSIS_GLOBAL_ACTION_TRANSPORT_TOGGLE); in.buttons_held|=bit(APSIS_FM1_BTN_SEL); CHECK(apsis_controls_global_action(&in)==APSIS_GLOBAL_ACTION_NONE);
    return 0;
}

static int test_clock_gravity(void)
{
    ApsisClockState c; ApsisFm1InputFrame in; uint8_t degree=99u; memset(&c,0,sizeof c);
    CHECK(apsis_controls_clock_layer_step(APSIS_CLOCK_CONTROL_MAIN,-1)==APSIS_CLOCK_CONTROL_GROUP);
    c.n=5;c.main_mask=0x1Fu;CHECK(apsis_controls_clock_apply_keys(&c,APSIS_CLOCK_CONTROL_MAIN,(1u<<1)|(1u<<4)|(1u<<5))==2u);
    memset(&c,0,sizeof c);c.n=8;c.bounds[0]=6;c.bounds[1]=2;c.bounds[2]=4;
    CHECK(apsis_controls_clock_apply_keys(&c,APSIS_CLOCK_CONTROL_GROUP,1u<<4)==1u);CHECK(c.bounds[0]==2u&&c.bounds[1]==6u&&c.bounds[2]==0u);
    CHECK(apsis_controls_clock_apply_keys(&c,APSIS_CLOCK_CONTROL_GROUP,1u<<5)==1u);CHECK(apsis_controls_clock_apply_keys(&c,APSIS_CLOCK_CONTROL_GROUP,1u<<3)==0u);
    memset(&in,0,sizeof in);in.notes_pressed=(1u<<5)|(1u<<2);CHECK(apsis_controls_gravity_degree_from_fm1(&in,&degree)==1u&&degree==2u);
    in.buttons_pressed=bit(APSIS_FM1_BTN_OCT_UP);CHECK(apsis_controls_gravity_ring_from_fm1(&in,2u)==0u);
    in.buttons_pressed=bit(APSIS_FM1_BTN_OCT_DOWN)|bit(APSIS_FM1_BTN_OCT_UP);CHECK(apsis_controls_gravity_ring_from_fm1(&in,1u)==1u);
    return 0;
}

static void string_state(ApsisState *s)
{
    memset(s,0,sizeof *s);
    s->string.surface_mode=APSIS_STRING_DUAL;
    s->string.span_q15=APSIS_Q15_ONE;
    s->string.quantize=0u;
    s->string.harmony=APSIS_HARMONY_SCALE;
    s->string.resonance_q15=APSIS_Q15_MILLI(660);
    s->string.brightness_q15=APSIS_Q15_MILLI(540);
    s->string.glide_q15=APSIS_Q15_MILLI(55);
    s->gravity.scale=APSIS_SCALE_DORIAN;
    s->gravity.voicing=APSIS_GRAVITY_COLOUR;
    s->gravity.voices=4u;
}

static int test_string_source_keyboard(void)
{
    ApsisState s; ApsisFm1InputFrame in; uint8_t key=99u; uint16_t split; int32_t mag;
    string_state(&s); memset(&in,0,sizeof in);
    in.buttons_held=bit(APSIS_FM1_BTN_SEL);CHECK(apsis_controls_string_side_from_fm1(&s.string,&in)==APSIS_SIDE_LEFT);
    s.string.surface_mode=APSIS_STRING_CLASSIC;CHECK(apsis_controls_string_side_from_fm1(&s.string,&in)==APSIS_SIDE_RIGHT);s.string.surface_mode=APSIS_STRING_DUAL;
    in.notes_pressed=(1u<<19)|(1u<<4);CHECK(apsis_controls_string_key_from_fm1(&in,&key)==1u&&key==4u);

    memset(&in,0,sizeof in);in.buttons_pressed=bit(APSIS_FM1_BTN_OCT_UP);CHECK(apsis_controls_string_bank_from_fm1(&in,0u)==1u);CHECK(apsis_controls_string_bank_from_fm1(&in,1u)==1u);
    in.buttons_pressed=bit(APSIS_FM1_BTN_OCT_DOWN);CHECK(apsis_controls_string_bank_from_fm1(&in,1u)==0u);
    in.buttons_pressed=bit(APSIS_FM1_BTN_OCT_DOWN)|bit(APSIS_FM1_BTN_OCT_UP);CHECK(apsis_controls_string_bank_from_fm1(&in,1u)==1u);

    split=apsis_controls_string_split_for_key(&s,APSIS_SIDE_RIGHT,0u,0u);CHECK(split==APSIS_STRING_SPLIT_MID_Q15);
    split=apsis_controls_string_split_for_key(&s,APSIS_SIDE_RIGHT,8u,0u);mag=apsis_string_magnitude_milli(split,s.string.span_q15);CHECK(absi(mag-16000)<20);
    split=apsis_controls_string_split_for_key(&s,APSIS_SIDE_RIGHT,9u,0u);mag=apsis_string_magnitude_milli(split,s.string.span_q15);CHECK(absi(mag-12000)<20);
    split=apsis_controls_string_split_for_key(&s,APSIS_SIDE_LEFT,8u,0u);CHECK(split<APSIS_STRING_SPLIT_MID_Q15);mag=apsis_string_magnitude_milli(split,s.string.span_q15);CHECK(absi(mag-16000)<20);

    s.string.quantize=1u;s.string.harmony=APSIS_HARMONY_CHROMATIC;
    split=apsis_controls_string_split_for_key(&s,APSIS_SIDE_RIGHT,8u,0u);mag=apsis_string_magnitude_milli(split,s.string.span_q15);CHECK(absi(mag-12000)<20);
    s.string.harmony=APSIS_HARMONY_SCALE;
    split=apsis_controls_string_split_for_key(&s,APSIS_SIDE_RIGHT,8u,0u);mag=apsis_string_magnitude_milli(split,s.string.span_q15);CHECK(absi(mag-14000)<20);
    split=apsis_controls_string_split_for_key(&s,APSIS_SIDE_RIGHT,9u,0u);mag=apsis_string_magnitude_milli(split,s.string.span_q15);CHECK(absi(mag-12000)<20);

    CHECK(apsis_controls_string_split_nudge(APSIS_SIDE_RIGHT,APSIS_STRING_SPLIT_MID_Q15,1)>APSIS_STRING_SPLIT_MID_Q15);
    CHECK(apsis_controls_string_split_nudge(APSIS_SIDE_LEFT,APSIS_STRING_SPLIT_MID_Q15,1)<APSIS_STRING_SPLIT_MID_Q15);
    CHECK(apsis_controls_string_split_nudge(APSIS_SIDE_RIGHT,APSIS_STRING_SPLIT_MAX_Q15,1000)==APSIS_STRING_SPLIT_MAX_Q15);
    CHECK(apsis_controls_string_mode_step(APSIS_STRING_DUAL,1)==APSIS_STRING_CLASSIC);
    return 0;
}

static int test_string_source_params(void)
{
    ApsisState s; ApsisFm1InputFrame in; ApsisControlParamEdit e[4]; uint8_t n;
    uint16_t brightness_native;
    string_state(&s); memset(&in,0,sizeof in);
    in.encoder_steps[APSIS_FM1_ENC_K1]=1;in.encoder_steps[APSIS_FM1_ENC_K2]=-2;in.encoder_steps[APSIS_FM1_ENC_K3]=2;in.encoder_steps[APSIS_FM1_ENC_K4]=1;
    n=apsis_controls_string_param_edits(&s.string,&in,e);CHECK(n==4u);
    CHECK(e[0].param==APSIS_CONTROL_PARAM_STRING_RESONANCE&&e[0].value==APSIS_Q15_MILLI(670));
    brightness_native=(uint16_t)(APSIS_Q15_MILLI(540)-2u*APSIS_Q15_MILLI(10));
    CHECK(e[1].param==APSIS_CONTROL_PARAM_STRING_BRIGHTNESS&&e[1].value==brightness_native);
    CHECK(absi(e[1].value-(int32_t)APSIS_Q15_MILLI(520))<=1);
    CHECK(e[2].param==APSIS_CONTROL_PARAM_STRING_GLIDE&&e[2].value==APSIS_Q15_MILLI(63));
    CHECK(e[3].param==APSIS_CONTROL_PARAM_STRING_SPAN&&e[3].value==APSIS_Q15_MILLI(1280));
    s.string.resonance_q15=APSIS_Q15_MILLI(880);s.string.brightness_q15=APSIS_Q15_MILLI(200);s.string.glide_q15=APSIS_Q15_MILLI(380);s.string.span_q15=APSIS_Q15_MILLI(1560);
    memset(&in,0,sizeof in);in.encoder_steps[APSIS_FM1_ENC_K1]=100;in.encoder_steps[APSIS_FM1_ENC_K2]=-100;in.encoder_steps[APSIS_FM1_ENC_K3]=100;in.encoder_steps[APSIS_FM1_ENC_K4]=100;
    n=apsis_controls_string_param_edits(&s.string,&in,e);CHECK(n==4u);CHECK(e[0].value==APSIS_Q15_MILLI(880));CHECK(e[1].value==APSIS_Q15_MILLI(200));CHECK(e[2].value==APSIS_Q15_MILLI(380));CHECK(e[3].value==APSIS_Q15_MILLI(1560));
    return 0;
}

static int test_spiral_banks(void)
{
    ApsisFm1InputFrame in; uint8_t point=99u;
    memset(&in,0,sizeof in);
    in.buttons_pressed=bit(APSIS_FM1_BTN_OCT_UP);CHECK(apsis_controls_spiral_bank_from_fm1(&in,0u)==1u);CHECK(apsis_controls_spiral_bank_from_fm1(&in,1u)==1u);
    in.buttons_pressed=bit(APSIS_FM1_BTN_OCT_DOWN);CHECK(apsis_controls_spiral_bank_from_fm1(&in,1u)==0u);
    in.buttons_pressed=bit(APSIS_FM1_BTN_OCT_DOWN)|bit(APSIS_FM1_BTN_OCT_UP);CHECK(apsis_controls_spiral_bank_from_fm1(&in,1u)==1u);

    memset(&in,0,sizeof in);in.notes_pressed=(1u<<20)|(1u<<2);
    CHECK(apsis_controls_spiral_point_from_fm1(&in,0u,&point)==1u&&point==2u);
    CHECK(apsis_controls_spiral_point_from_fm1(&in,1u,&point)==1u&&point==14u);
    in.notes_pressed=1u<<23;CHECK(apsis_controls_spiral_point_from_fm1(&in,1u,&point)==1u&&point==35u);
    in.notes_pressed=(1u<<24)|(1u<<26);CHECK(apsis_controls_spiral_point_from_fm1(&in,1u,&point)==0u);
    return 0;
}

static int test_spiral_route_edit(void)
{
    ApsisSpiralState s; uint8_t cursor=0u,i;
    memset(&s,0,sizeof s);
    CHECK(apsis_controls_spiral_append(&s,4u,&cursor)==1u&&s.route_count==1u&&s.route[0]==4u&&cursor==0u);
    CHECK(apsis_controls_spiral_append(&s,4u,&cursor)==1u&&s.route_count==2u&&s.route[0]==4u&&s.route[1]==4u&&cursor==1u);
    CHECK(apsis_controls_spiral_insert(&s,7u,&cursor)==1u&&s.route_count==3u&&s.route[0]==4u&&s.route[1]==7u&&s.route[2]==4u&&cursor==1u);
    CHECK(apsis_controls_spiral_move_selected(&s,cursor,3)==1u&&s.route[1]==10u&&s.route[0]==4u&&s.route[2]==4u);
    CHECK(apsis_controls_spiral_move_selected(&s,cursor,100)==1u&&s.route[1]==35u);
    CHECK(apsis_controls_spiral_move_selected(&s,cursor,1)==0u&&s.route[1]==35u);
    CHECK(apsis_controls_spiral_cursor_step(2u,3u,1)==0u);CHECK(apsis_controls_spiral_cursor_step(0u,3u,-1)==2u);
    CHECK(apsis_controls_spiral_delete_selected(&s,&cursor)==1u&&s.route_count==2u&&s.route[0]==4u&&s.route[1]==4u&&cursor==1u);

    memset(&s,0,sizeof s);cursor=0u;
    for(i=0u;i<APSIS_SPIRAL_ROUTE_MAX;++i)CHECK(apsis_controls_spiral_append(&s,i,&cursor)==1u);
    CHECK(s.route_count==APSIS_SPIRAL_ROUTE_MAX&&s.route[0]==0u&&s.route[29]==29u);
    cursor=10u;CHECK(apsis_controls_spiral_insert(&s,35u,&cursor)==0u&&s.route[10]==10u&&s.route_count==APSIS_SPIRAL_ROUTE_MAX);
    CHECK(apsis_controls_spiral_append(&s,35u,&cursor)==1u&&s.route_count==APSIS_SPIRAL_ROUTE_MAX&&s.route[0]==1u&&s.route[28]==29u&&s.route[29]==35u&&cursor==29u);
    return 0;
}

int main(void)
{
    CHECK(test_p9a()==0);CHECK(test_clock_gravity()==0);CHECK(test_string_source_keyboard()==0);CHECK(test_string_source_params()==0);CHECK(test_spiral_banks()==0);CHECK(test_spiral_route_edit()==0);
    puts("PASS: APSIS P9A/P9B/P9C/P9D source-locked FM-1 interaction contract");return 0;
}
