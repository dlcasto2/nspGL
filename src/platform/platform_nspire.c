/* TI-Nspire (Ndless) platform layer */
#ifdef _TINSPIRE
#include "platform.h"
#include <libndls.h>
#include <keys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int64_t nspire_time_ms(void);
static uint16_t *fb;
static char start_dir[256];
static char arg_file[256];
static touchpad_report_t tp_last;
static int tp_had;

static scr_type_t display_type;
static uint8_t *gray_fb;

/* malloc_usable_size is referenced by QuickJS default allocator */
size_t malloc_usable_size(void *p) { (void)p; return 0; }

int plat_init(int argc, char **argv)
{
    if (argc > 0 && argv[0]) {
        snprintf(start_dir, sizeof start_dir, "%s", argv[0]);
        char *s = strrchr(start_dir, '/');
        if (s) s[1] = 0; else strcpy(start_dir, "/documents/");
    } else strcpy(start_dir, "/documents/");
    if (argc > 1 && argv[1]) snprintf(arg_file, sizeof arg_file, "%s", argv[1]);
    cfg_register_fileext("html", "nspgl");
    cfg_register_fileext("htm", "nspgl");
    fb = calloc(SCR_W * SCR_H, sizeof(*fb));
    if (!fb) return -1;

    /* lcd_type() reports SCR_320x240_4 on the OG monochrome Clickpad
     * and SCR_320x240_565 on the color CX. Keep the renderer's internal
     * framebuffer RGB565 and convert only when presenting to a classic LCD. */
    display_type = lcd_type();
    if (display_type != SCR_320x240_4 && display_type != SCR_320x240_565)
        display_type = is_classic ? SCR_320x240_4 : SCR_320x240_565;
    if (display_type == SCR_320x240_4) {
        gray_fb = calloc(SCR_W * SCR_H / 2, sizeof(*gray_fb));
        if (!gray_fb) {
            free(fb);
            fb = NULL;
            return -1;
        }
    }

    if (!lcd_init(display_type)) {
        free(gray_fb);
        gray_fb = NULL;
        free(fb);
        fb = NULL;
        return -1;
    }

    /* Diagnostic build: do not touch undocumented timer registers.
     * Use the Ndless RTC at one-second resolution until startup is stable. */
    show_msgbox("nspGL diagnostic", "Platform initialization passed. Press OK to continue.");
    return 0;
}

void plat_quit(void)
{
    lcd_init(SCR_TYPE_INVALID);
    free(gray_fb);
    gray_fb = NULL;
    free(fb);
    fb = NULL;
}

uint16_t *plat_fb(void) { return fb; }

/* Convert RGB565 to 4-bit luminance for the OG Clickpad LCD. The first
 * pixel occupies the high nibble, matching Ndless's classic LCD format. */
static uint8_t rgb565_to_gray4(uint16_t color)
{
    unsigned r = (color >> 11) & 31u;
    unsigned g = (color >> 5) & 63u;
    unsigned b = color & 31u;
    r = (r << 3) | (r >> 2);
    g = (g << 2) | (g >> 4);
    b = (b << 3) | (b >> 2);
    unsigned luminance = (77u * r + 150u * g + 29u * b + 128u) >> 8;
    return (uint8_t)((luminance * 15u + 127u) / 255u);
}

void plat_present(void)
{
    if (display_type == SCR_320x240_4) {
        for (unsigned i = 0; i < SCR_W * SCR_H; i += 2) {
            uint8_t hi = rgb565_to_gray4(fb[i]);
            uint8_t lo = rgb565_to_gray4(fb[i + 1]);
            gray_fb[i >> 1] = (uint8_t)((hi << 4) | lo);
        }
        lcd_blit(gray_fb, SCR_320x240_4);
    } else {
        lcd_blit(fb, display_type);
    }
}

uint32_t plat_ms(void)
{
    return (*(volatile uint32_t *)0x90090000) * 1000u;
}
/* Wall clock uses the same stable Ndless RTC source during diagnostics. */
int64_t nspire_time_ms(void)
{
    return (int64_t)(*(volatile uint32_t *)0x90090000) * 1000;
}
void plat_wait_ms(int ms)
{
    if (ms > 0) msleep((unsigned)ms);
}

void plat_keys(uint8_t *d)
{
    memset(d, 0, K_COUNT);
    d[K_UP] = isKeyPressed(KEY_NSPIRE_UP) || isKeyPressed(KEY_NSPIRE_LEFTUP) || isKeyPressed(KEY_NSPIRE_UPRIGHT);
    d[K_DOWN] = isKeyPressed(KEY_NSPIRE_DOWN) || isKeyPressed(KEY_NSPIRE_RIGHTDOWN) || isKeyPressed(KEY_NSPIRE_DOWNLEFT);
    d[K_LEFT] = isKeyPressed(KEY_NSPIRE_LEFT) || isKeyPressed(KEY_NSPIRE_LEFTUP) || isKeyPressed(KEY_NSPIRE_DOWNLEFT);
    d[K_RIGHT] = isKeyPressed(KEY_NSPIRE_RIGHT) || isKeyPressed(KEY_NSPIRE_UPRIGHT) || isKeyPressed(KEY_NSPIRE_RIGHTDOWN);
    d[K_ENTER] = isKeyPressed(KEY_NSPIRE_ENTER);
    d[K_RET] = isKeyPressed(KEY_NSPIRE_RET);
    d[K_ESC] = isKeyPressed(KEY_NSPIRE_ESC);
    d[K_TAB] = isKeyPressed(KEY_NSPIRE_TAB);
    d[K_MENU] = isKeyPressed(KEY_NSPIRE_MENU);
    d[K_DEL] = isKeyPressed(KEY_NSPIRE_DEL);
    d[K_SPACE] = isKeyPressed(KEY_NSPIRE_SPACE);
    d[K_SHIFT] = isKeyPressed(KEY_NSPIRE_SHIFT);
    d[K_CTRL] = isKeyPressed(KEY_NSPIRE_CTRL);
    d[K_DOC] = isKeyPressed(KEY_NSPIRE_DOC);
    d[K_CLICK] = isKeyPressed(KEY_NSPIRE_CLICK);
    d[K_PLUS] = isKeyPressed(KEY_NSPIRE_PLUS);
    d[K_MINUS] = isKeyPressed(KEY_NSPIRE_MINUS);
    d[K_MUL] = isKeyPressed(KEY_NSPIRE_MULTIPLY);
    d[K_DIV] = isKeyPressed(KEY_NSPIRE_DIVIDE);
    d[K_COMMA] = isKeyPressed(KEY_NSPIRE_COMMA);
    d[K_PERIOD] = isKeyPressed(KEY_NSPIRE_PERIOD);
    d[K_HOME] = isKeyPressed(KEY_NSPIRE_HOME);
    d[K_EQU] = isKeyPressed(KEY_NSPIRE_EQU);
    d[K_LP] = isKeyPressed(KEY_NSPIRE_LP);
    d[K_RP] = isKeyPressed(KEY_NSPIRE_RP);
    d[K_VAR] = isKeyPressed(KEY_NSPIRE_VAR);
    d[K_CAT] = isKeyPressed(KEY_NSPIRE_CAT);
    d[K_SCRATCH] = isKeyPressed(KEY_NSPIRE_SCRATCHPAD);
    d[K_EE] = isKeyPressed(KEY_NSPIRE_EE);
    d[K_NEG] = isKeyPressed(KEY_NSPIRE_NEGATIVE);
    d[K_TRIG] = isKeyPressed(KEY_NSPIRE_TRIG);
    d[K_POW] = isKeyPressed(KEY_NSPIRE_EXP);
    d[K_SQU] = isKeyPressed(KEY_NSPIRE_SQU);
    static const t_key *letters[26];
    static int init;
    if (!init) {
        const t_key *l[26] = { &KEY_NSPIRE_A, &KEY_NSPIRE_B, &KEY_NSPIRE_C, &KEY_NSPIRE_D, &KEY_NSPIRE_E, &KEY_NSPIRE_F,
            &KEY_NSPIRE_G, &KEY_NSPIRE_H, &KEY_NSPIRE_I, &KEY_NSPIRE_J, &KEY_NSPIRE_K, &KEY_NSPIRE_L, &KEY_NSPIRE_M,
            &KEY_NSPIRE_N, &KEY_NSPIRE_O, &KEY_NSPIRE_P, &KEY_NSPIRE_Q, &KEY_NSPIRE_R, &KEY_NSPIRE_S, &KEY_NSPIRE_T,
            &KEY_NSPIRE_U, &KEY_NSPIRE_V, &KEY_NSPIRE_W, &KEY_NSPIRE_X, &KEY_NSPIRE_Y, &KEY_NSPIRE_Z };
        memcpy(letters, l, sizeof l);
        init = 1;
    }
    for (int i = 0; i < 26; i++) d[K_A + i] = isKeyPressed(*letters[i]);
    const t_key *digits[10] = { &KEY_NSPIRE_0, &KEY_NSPIRE_1, &KEY_NSPIRE_2, &KEY_NSPIRE_3, &KEY_NSPIRE_4,
        &KEY_NSPIRE_5, &KEY_NSPIRE_6, &KEY_NSPIRE_7, &KEY_NSPIRE_8, &KEY_NSPIRE_9 };
    for (int i = 0; i < 10; i++) d[K_0 + i] = isKeyPressed(*digits[i]);
}

int plat_touch(int *dx, int *dy)
{
    *dx = *dy = 0;
    if (!is_touchpad) return 0;
    touchpad_report_t r;
    if (touchpad_scan(&r)) return 0;
    if (r.contact && tp_had && !r.pressed) {
        *dx = ((int)r.x - (int)tp_last.x) / 6;
        *dy = -((int)r.y - (int)tp_last.y) / 6;
    }
    if (r.contact && (*dx || *dy || !tp_had)) tp_last = r;
    tp_had = r.contact;
    return r.contact;
}

int plat_on_pressed(void) { return on_key_pressed(); }
const char *plat_start_dir(void) { return start_dir; }
const char *plat_arg_file(void) { return arg_file[0] ? arg_file : NULL; }
int plat_is_host(void) { return 0; }
void plat_log(const char *s) { (void)s; }

/* Diagnostic build: use the Ndless-provided process stack.
 * The custom ARM stack trampoline is disabled until startup is verified. */
void plat_run(void (*fn)(void))
{
    fn();
}
#endif
