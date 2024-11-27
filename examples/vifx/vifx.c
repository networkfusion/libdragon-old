#include <libdragon.h>

#define USE_DISPLAY     0
#define MAX(a, b)       ((a) > (b) ? (a) : (b))
#define MIN(a, b)       ((a) < (b) ? (a) : (b))

int main(void)
{
    debug_init_isviewer();
    debug_init_usblog();
    joypad_init();

    #if USE_DISPLAY
    display_init(RESOLUTION_320x240, DEPTH_32_BPP, 1, GAMMA_NONE, FILTERS_DISABLED);
    #else
    vi_init();
    #endif
    dfs_init(DFS_DEFAULT_LOCATION);
    rdpq_init();
    rdpq_debug_start();

    sprite_t *bkg = sprite_load("rom:/philips.rgba32.sprite");
    #if USE_DISPLAY
    surface_t* fb2 = display_get();
    surface_t fb = *fb2;
    #else
    surface_t fb = surface_alloc(FMT_RGBA32, 320, 240);
    vi_show(&fb);
    #endif

    rdpq_attach(&fb, NULL);
    rdpq_set_mode_standard();
    rdpq_sprite_blit(bkg, 0, 0, &(rdpq_blitparms_t){
        .scale_x = 0.5f, .scale_y = 0.5f,
    });
    rdpq_detach_wait();

    #if USE_DISPLAY
    display_show(fb2);
    #endif

    bool borders = false;
    bool rescale = true;

    while (1) {

        joypad_poll();
        joypad_buttons_t btn = joypad_get_buttons_pressed(JOYPAD_PORT_1);
        joypad_buttons_t down = joypad_get_buttons_held(JOYPAD_PORT_1);

        if (btn.a) { borders = !borders; rescale = true; }

        vi_write_begin();            
        if (rescale) {
            vi_set_borders(vi_calc_borders2(4.0f / 3.0f, borders ? VI_CRT_MARGIN : 0));
            vi_set_scroll(191, 107);
            vi_set_xscale(fb.width);
            vi_set_yscale(fb.height);
            rescale = false;
        }

        if (down.c_up)    vi_scroll(0, -1);
        if (down.c_down)  vi_scroll(0, 1);
        if (down.c_left)  vi_scroll(-1, 0);
        if (down.c_right) vi_scroll(1, 0);
        vi_write_end();

        vi_wait_vblank();
    }
}