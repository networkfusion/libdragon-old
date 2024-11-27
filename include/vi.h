/**
 * @file vi.h
 * @brief Video Interface Subsystem
 * @ingroup display
 * 
 * This module offers a low-level interface to VI programming. Most applications
 * should use display.h, which sits on top of vi.h, and offers a higher level
 * interface, plus automatic memory management of multiple framebuffers,
 * FPS utilities, and much more.
 * 
 * ## Framebuffer resizing and video display size
 * 
 * VI has a powerful resampling engine: the framebuffer is not displayed as-is
 * on TV, but it is actually resampled (scaled, stretched), optionally with
 * bilinear filtering. This is a very powerful hardware feature that is a bit
 * complicated to configure, so an effort was made to expose an intuitive API
 * to programmers.
 *
 * By default, this module configures the VI to resample an arbitrary framebuffer
 * picture into a virtual 640x480 display output with 4:3 aspect ratio (on NTSC
 * and MPAL), or a virtual 640x576 display output though with the same 4:3
 * aspect ratio on PAL. This is done to achieve TV type independence: since
 * both resolution share the same aspect ratio on their respective TV standards
 * (given that dots are square on NTSC but they are not on PAL), it means
 * that the framebuffer will look the same on all TVs; on PAL, you will be
 * trading additional vertical resolution (assuming the framebuffer is big
 * enough to have that detail). 
 * 
 * This also means that games don't have to do anything special to handle NTSC
 * vs PAL (besides maybe accounting for the different refresh rate), which is
 * an important goal.
 * 
 * In reality, TVs didn't have a 480-lines vertical resolution so the actual
 * output depends on whether you request interlaced display or not:
 * 
 *  * In case of non interlaced display, the actual resolution is 640x240, but
 *    since dots will be configured to be twice as big vertically, the aspect
 *    ratio will be 4:3 as-if the image was 640x480 (with duplicated scanlines)
 *  * In case of interlaced display, you do get to display 480 scanlines, by
 *    alternating two slightly-shifted 640x240 pictures.
 * 
 * As an example, if you configure a framebuffer resolution like 512x320, with
 * interlacing turned off, what happens is that the image gets scaled into
 * 640x240, so horizontally some pixels will be duplicated to enlarge the
 * resolution to 640, but vertically some scanlines will be dropped. The
 * output display aspect ratio will still be 4:3, which is not the source aspect
 * ratio of the framebuffer (512 / 320 = 1.6666 = 16:10), so the image will
 * appear squished, unless obviously this was accounted for while drawing to
 * the framebuffer.
 * 
 * While resampling the framebuffer into the display output, the VI can use either
 * bilinear filtering or simple nearest sampling (duplicating or dropping pixels).
 * See #filter_options_t for more information on configuring
 * the VI image filters.
 * 
 * The 640x480 virtual display output can be fully viewed on emulators and on
 * modern screens (via grabbers, converters, etc.). When displaying on old
 * CRTs though, part of the display will be hidden because of the overscan.
 * To account for that, it is possible to reduce the 640x480 display output
 * by adding black borders. For instance, if you specify 12 dots of borders
 * on all the four edges, you will get a 616x456 display output, plus
 * the requested 12 dots of borders on all sides; the actual display output
 * will thus be smaller, and possibly get fully out of overscan. The value
 * #VI_CRT_MARGIN is a good default you can use for overscan compensation on
 * most CRT TVs.
 * 
 * Notice that adding borders also affect the aspect ratio of the display output;
 * for instance, in the above example, the 616x456 display output is not
 * exactly 4:3 anymore, but more like 4.05:3. By carefully calculating borders,
 * thus, it is possible to obtain specific display outputs with custom aspect
 * ratios (eg: 16:9).
 * 
 * To help calculating the borders by taking both potential goals into account
 * (overscan compensation and aspect ratio changes), you can use #vi_calc_borders.
 */
#ifndef __LIBDRAGON_VI_H
#define __LIBDRAGON_VI_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct surface_s surface_t;

/** @brief Number of useful 32-bit registers at the register base */
#define VI_REGISTERS_COUNT      14

/** @brief Base pointer to hardware Video interface registers that control various aspects of VI configuration.
 * Shouldn't be used by itself, use VI_ registers to get/set their values. */
#define VI_REGISTERS      ((volatile uint32_t*)0xA4400000)
/** @brief VI Index register of controlling general display filters/bitdepth configuration */
#define VI_CTRL           (&VI_REGISTERS[0])
/** @brief VI Index register of RDRAM base address of the video output Frame Buffer. This can be changed as needed to implement double or triple buffering. */
#define VI_ORIGIN         (&VI_REGISTERS[1])
/** @brief VI Index register of width in pixels of the frame buffer. */
#define VI_WIDTH          (&VI_REGISTERS[2])
/** @brief VI Index register of vertical interrupt. */
#define VI_V_INTR         (&VI_REGISTERS[3])
/** @brief VI Index register of the current half line, sampled once per line. */
#define VI_V_CURRENT      (&VI_REGISTERS[4])
/** @brief VI Index register of sync/burst values */
#define VI_BURST          (&VI_REGISTERS[5])
/** @brief VI Index register of total visible and non-visible lines. 
 * This should match either NTSC (non-interlaced: 0x20D, interlaced: 0x20C) or PAL (non-interlaced: 0x271, interlaced: 0x270) */
#define VI_V_SYNC         (&VI_REGISTERS[6])
/** @brief VI Index register of total width of a line */
#define VI_H_SYNC         (&VI_REGISTERS[7])
/** @brief VI Index register of an alternate scanline length for one scanline during vsync. */
#define VI_H_SYNC_LEAP    (&VI_REGISTERS[8])
/** @brief VI Index register of start/end of the active video image, in screen pixels */
#define VI_H_VIDEO        (&VI_REGISTERS[9])
/** @brief VI Index register of start/end of the active video image, in screen half-lines. */
#define VI_V_VIDEO        (&VI_REGISTERS[10])
/** @brief VI Index register of start/end of the color burst enable, in half-lines. */
#define VI_V_BURST        (&VI_REGISTERS[11])
/** @brief VI Index register of horizontal subpixel offset and 1/horizontal scale up factor. */
#define VI_X_SCALE        (&VI_REGISTERS[12])
/** @brief VI Index register of vertical subpixel offset and 1/vertical scale up factor. */
#define VI_Y_SCALE        (&VI_REGISTERS[13])

/** @brief VI register by index (0-13)*/
#define VI_TO_REGISTER(index) (((index) >= 0 && (index) <= VI_REGISTERS_COUNT)? &VI_REGISTERS[index] : NULL)

/** @brief VI index from register */
#define VI_TO_INDEX(reg) ((reg) - VI_REGISTERS)

/**
 * @brief Video Interface borders structure
 *
 * This structure defines how thick (in dots) should the borders around
 * a framebuffer be.
 * 
 * The dots refer to the VI virtual display output (640x480, on both NTSC, PAL,
 * and M-PAL), and thus reduce the actual display output, and even potentially
 * modify the aspect ratio. The framebuffer will be scaled to fit under them.
 * 
 * For example, when displaying on CRT TVs, one can add borders around a
 * framebuffer so that the whole image can be seen on the screen. 
 * 
 * If no borders are applied, the output will use the entire virtual dsplay
 * output (640x480) for showing a framebuffer. This is useful for emulators,
 * upscalers, and LCD TVs.
 * 
 * Notice that borders can also be *negative*: this obtains the effect of
 * actually enlarging the output, growing from 640x480. Doing so will very
 * likely create problems with most TV grabbers and upscalers, but it might
 * work correctly on most CRTs (though the added pixels will surely be
 * part of the overscan so not really visible). Horizontally, the maximum display
 * output will probably be ~700-ish on CRTs, after which the sync will be lost.
 * Vertically, any negative number will likely create immediate syncing problems,
 */
typedef struct vi_borders_s {
    int16_t left, right, up, down;
} vi_borders_t;


/** @brief Initialize the VI module */
void vi_init(void);

/** 
 * @name Low-level register access 
 * 
 * These functions allow to read/write VI hardware registers directly, in a safe
 * way. Consider that most timing-related registers can cause a hard-crash of VI
 * if they are changed during the active display area, so it is generally good
 * practice to only change them during vblank. Thus, we provide a way to delay
 * writing registers until next vblank.
 * 
 * @{
 */

/**
 * @brief Read the current state of a register, including pending changes.
 * 
 * This function is similar to directly sampling the hardware register,
 * but also takes into account any pending write that might have issued
 * before but was still not applied. This helps achieving consistency during
 * multi-register writes.
 * 
 * @param reg               Register to read
 * @return uint32_t         Current value of the register (or pending value if any)
 */
uint32_t vi_read(volatile uint32_t *reg);

/**
 * @brief Partially write a VI register at next vblank
 * 
 * This is similar to #vi_write, but only write some bits of the register.
 * The specified \p wmask specifies which bits will actually be changed
 * using the \p value, while the other bits will be left untouched.
 *  
 * @param reg               Register to write
 * @param wmask             Mask of bits to write
 * @param value             Value to write
 * 
 * @see #vi_write
 * @see #vi_write_begin
 */
void vi_write_masked(volatile uint32_t *reg, uint32_t wmask, uint32_t value);

/**
 * @brief Write a VI register at next vblank
 * 
 * The write will be pending until the vblank, but #vi_read will immediately
 * start returning the new value.
 * 
 * Notice that if the function is called while the VI is currently in vblank
 * period, the batched changes will be applied immediately.
 * 
 * If you want to atomically change multiple registers, use #vi_write_begin
 * and #vi_write_end to group the changes.
 * 
 * @param reg               Register to write
 * @param value             Value to write
 * 
 * @see #vi_write_begin
 * @see #vi_write_masked
 */
inline void vi_write(volatile uint32_t *reg, uint32_t value)
{
    vi_write_masked(reg, 0xFFFFFFFF, value);
}

/**
 * @brief Begin a batch of register writes
 * 
 * This function starts a batch of register writes, so that they will be
 * applied atomically at the next vblank. This is useful when you want to
 * change multiple registers at once, to avoid display artifacts.
 * 
 * @see #vi_write_end
 */
void vi_write_begin(void);

/**
 * @brief End a batch of register writes
 * 
 * This function ends a batch of register writes started with #vi_write_begin.
 * It doesn't block until vblank, so when the function returns, the registers
 * are still pending to be written (though #vi_read will return the new values).
 * 
 * Notice that if the function is called while the VI is currently in vblank
 * period, the batched changes will be applied immediately.
 * 
 * @see #vi_write_begin
 */
void vi_write_end(void);

/**
 * @brief Wait for the beginning of next vblank period
 * 
 * This function waits for the beginning of the next vblank period, and
 * returns immediately after that.
 * 
 * If the VI is not active, this function will return immediately.
 */
void vi_wait_vblank(void);

/** @} */

/**
 * @name Functions to query VI state
 * 
 * These functions allow to query the current state of VI, by also providing
 * simple processing of raw register values.
 * 
 * @{
 */

/**
 * @brief Get the current refresh rate of the video output in Hz
 * 
 * The refresh rate is normally 50 for PAL and 60 for NTSC, but this function
 * returns the hardware-accurate number which is close to those but not quite
 * exact. Moreover, this will also account for advanced VI configurations
 * affecting the refresh rate, like PAL60.
 * 
 * @return float        Refresh rate in Hz (frames per second)
 */
float vi_get_refresh_rate(void);

/**
 * @brief Get the current configured borders
 * 
 * @return vi_borders_t 
 */
vi_borders_t vi_get_borders(void);

/**
 * @brief Return the active display width configured in VI
 * 
 * This is the amount of horizontal dots that are actually displayed by
 * VI. It will normally be 640, minus the configured borders if any.
 * 
 * @return int      Number of active horizontal screen dots
 */
int vi_get_display_width(void);

/**
 * @brief Return the active display height configured in VI
 * 
 * This is the amount of vertical dots that are actually displayed by
 * VI. It will normally be 480 (NTSC/MPAL) or 576 (PAL), minus the
 * configured borders if any.
 * 
 * @return int      Number of active vertical screen dots
 */
int vi_get_display_height(void);

/**
 * @brief Get the current offsets for the active display area
 * 
 * This function returns the current scroll offset for the active display
 * area, which is the top-left corner of the active display area, expressed
 * in VI dots.
 * 
 * @param curx      Pointer to store the horizontal scroll offset
 * @param cury      Pointer to store the vertical scroll offset
 * 
 * @see #vi_set_scroll
 * @see #vi_scroll
 */
void vi_get_scroll(int *curx, int *cury);

/**
 * @brief Get the bounds for the scrolling offset
 * 
 * This function returns the minimum and maximum values for the scrolling
 * offset, which are the values that the scroll offset can take without
 * causing display artifacts or even outright crashing the VI.
 * 
 * @param minx      Pointer to store the minimum horizontal scroll offset
 * @param maxx      Pointer to store the maximum horizontal scroll offset
 * @param miny      Pointer to store the minimum vertical scroll offset
 * @param maxy      Pointer to store the maximum vertical scroll offset
 */
void vi_get_scroll_bounds(int *minx, int *maxx, int *miny, int *maxy);


/** @} */

/**
 * @name Functions to configure VI
 * 
 * These functions allow to configure VI in various ways, like setting the
 * framebuffer, the borders, the scaling factors, and more. The functions 
 * provide a "middle-level" interface to VI, which is easier to use than
 * just writing raw registers.
 * 
 * @{
 */

/**
 * @brief Configure VI to display the specified framebuffer
 * 
 * @note You can use #vi_show as a shortcut to do everything required to
 *       display a framebuffer.
 * 
 * This is the most basic function to display a buffer on the screen.
 * It just updates the buffer pointer in RDRAM and its properties
 * (width and bitdepth).
 * 
 * Notice that VI doesn't know the actual width and height of the framebuffer:
 * it will always display contents within the active display area. Instead,
 * you should call #vi_set_xscale and #vi_set_yscale to configure the scaling
 * factors, so that the framebuffer is fits the display area.
 * 
 * @param buffer        Pointer to the framebuffer to display
 * @param pixel_stride  Distance between two consecutive lines in the framebuffer,
 *                      measured in pixels.
 * @param bpp           Bit depth of the framebuffer. Only allowed values are 16 and 32.
 * 
 * @see #vi_show
 * @see #vi_set_xscale
 * @see #vi_set_yscale
 */
void vi_set_origin(void *buffer, int pixel_stride, int bpp);

/**
 * @brief Configure the horizontal scale factor to display the framebuffer.
 * 
 * This function calculates and configures the horizontal scale factor
 * needed to display the specified framebuffer width on the screen, so
 * that it fits exactly the active display area.
 *  
 * @param fb_width      Width of the framebuffer in pixels
 */
void vi_set_xscale(float fb_width);

/**
 * @brief Configure the vertical scale factor to display the framebuffer.
 * 
 * This function calculates and configures the vertical scale factor
 * needed to display the specified framebuffer width on the screen, so
 * that it fits exactly the active display area.
 *  
 * @param fb_height      Height of the framebuffer in pixels
 */
void vi_set_yscale(float fb_height);

/**
 * @brief Enable or disable the interlaced mode
 * 
 * @param interlaced     If true, the VI will display the framebuffer in
 *                       interlaced mode.
 */
void vi_set_interlaced(bool interlaced);

/**
 * @brief Calculate correct VI borders for a target aspect ratio.
 * 
 * This function calculates the appropriate VI borders to obtain the specified
 * aspect ratio, and optionally adding a margin to make the picture CRT-safe.
 * 
 * The margin is expressed as a percentage relative to the virtual VI display
 * output (640x480). A good default for this margin for most CRTs is
 * #VI_CRT_MARGIN (5%).
 * 
 * For instance, to create a 16:9 resolution, you can do:
 * 
 * \code{.c}
 *      vi_borders_t borders = vi_calc_borders(16./9, false, 0, 0);
 * \endcode
 * 
 * @param aspect_ratio      Target aspect ratio
 * @param overscan_margin   Margin to add to compensate for TV overscan. Use 0
 *                          to use full picture (eg: for emulators), and something
 *                          like #VI_CRT_MARGIN to get a good CRT default.
 * @return vi_borders_t The requested border settings
 */
vi_borders_t vi_calc_borders2(float aspect_ratio, float overscan_margin);

/**
 * @brief Apply the specified borders
 * 
 * Configures the VI to the specified border size.
 * 
 * Notice that applying the border does *NOT* rescale the framebuffer picture:
 * the left and up border will translate it, while the right and down border
 * will just crop it.
 * 
 * If you want the top and left border to also crop the picture, you must 
 * call #vi_set_xoffset and #vi_set_yoffset passing the
 * 
 * @param b 
 * @param rescale 
 */
void vi_set_borders(vi_borders_t b);

/**
 * @brief Set the specified scroll offset for the active display area
 * 
 * This function sets the scroll offset for the active display area, which
 * basically corresponds to the top-left corner of the active display area,
 * expressed in VI dots.
 * 
 * Notice that not all positions are valid for the scroll offset, so the
 * function will clamp it to make sure to keep the display stable (and
 * avoid crashing the VI, which is not very tolerant to invalid values).
 * 
 * To query the bounds for the scrolling offset, use #vi_get_scroll_bounds.
 * 
 * @param x             Horizontal scroll offset
 * @param y             Vertical scroll offset
 * 
 * @see #vi_get_scroll_bounds
 * @see #vi_get_scroll
 * @see #vi_scroll
 */
void vi_set_scroll(int x, int y);

/**
 * @brief Scroll the active display area by the specified amount
 * 
 * This function is similar to #vi_set_scroll, but scrolls the display area
 * by the specified amount, instead of setting the absolute position.
 * 
 * The scrolling is clamped to the valid range.
 * 
 * @param deltax        Horizontal scroll amount
 * @param deltay        Vertical scroll amount
 * 
 * @see #vi_set_scroll
 * @see #vi_get_scroll
 * @see #vi_get_scroll_bounds
 */
void vi_scroll(int deltax, int deltay);

/**
 * @brief Show the specified surface (at next vblank)
 * 
 * This function is a shortcut to do the following:
 * 
 *  * Call #vi_set_origin to specify the new buffer, stride and bpp
 *  * Call #vi_set_xscale to set the horizontal scale factor
 *  * Call #vi_set_yscale to set the vertical scale factor
 * 
 * If the specified surface is NULL, the VI will be turned off altogether,
 * and will stop issuing any video signal.
 * 
 * @param surf      Surface to show
 */
void vi_show(surface_t *fb);


#ifdef __cplusplus
}
#endif

#endif