/* -- macosxCG.c -- */

/*
 * We need to keep this separate from nearly everything else, e.g. rfb.h
 * and the other stuff, otherwise it does not work properly, mouse drags
 * will not work!!
 */

#if (defined(__MACH__) && defined(__APPLE__))

#include <ApplicationServices/ApplicationServices.h>
#include <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h>

void macosxCG_init(void);
void macosxCG_event_loop(void);
char *macosxCG_get_fb_addr(void);

int macosxCG_CGDisplayPixelsWide(void);
int macosxCG_CGDisplayPixelsHigh(void);
int macosxCG_CGDisplayBitsPerPixel(void);
int macosxCG_CGDisplayBitsPerSample(void);
int macosxCG_CGDisplaySamplesPerPixel(void);
int macosxCG_CGDisplayBytesPerRow(void);

void macosxCG_pointer_inject(int mask, int x, int y);
int macosxCG_get_cursor_pos(int *x, int *y);
int macosxCG_get_cursor(void);
void macosxCG_init_key_table(void);
void macosxCG_key_inject(int down, unsigned int keysym);

CGDirectDisplayID displayID = NULL;

extern int collect_macosx_damage(int x_in, int y_in, int w_in, int h_in, int call);

static void macosxCG_callback(CGRectCount n, const CGRect *rects, void *dum) {
	int i, db = 0;
	if (db) fprintf(stderr, "macosx_callback: n=%d\n", (int) n);
	for (i=0; i < n; i++) {
		if (db > 1) fprintf(stderr, "               : %g %g - %g %g\n", rects[i].origin.x, rects[i].origin.y, rects[i].size.width, rects[i].size.height);
		collect_macosx_damage( (int) rects[i].origin.x, (int) rects[i].origin.y,
		    (int) rects[i].size.width, (int) rects[i].size.height, 1);
	}
}

int dragum(void) {
#if 0
        int x =200, y = 150, dy = 10, i;
        CGPoint loc;

	CGDirectDisplayID displayID2 = kCGDirectMainDisplay;
	(void) GetMainDevice();

        for (i=0; i< 50; i++) {
                usleep(1000*100);
                loc.x = x;
                loc.y = y + i*dy;
                CGPostMouseEvent(loc, TRUE, 1, TRUE);
        }
        CGPostMouseEvent(loc, TRUE, 1, FALSE);
        usleep(4*1000*1000);
#endif
	return 0;
}

static int callback_set = 0;
extern int nofb;

void macosxCG_refresh_callback_on(void) {
	if (nofb) {
		return;
	}

	if (! callback_set) {
		if (1) fprintf(stderr, "macosxCG_callback: register\n");
		CGRegisterScreenRefreshCallback(macosxCG_callback, NULL);
	}
	callback_set = 1;
}

void macosxCG_refresh_callback_off(void) {
	if (callback_set) {
		if (1) fprintf(stderr, "macosxCG_callback: unregister\n");
		CGUnregisterScreenRefreshCallback(macosxCG_callback, NULL);
	}
	callback_set = 0;
}

extern int macosx_noscreensaver;
extern void macosxGCS_initpb(void);

void macosxCG_init(void) {
	if (displayID == NULL) {
		fprintf(stderr, "macosxCG_init: initializing display.\n");
		//dragum();

		displayID = kCGDirectMainDisplay;
		(void) GetMainDevice();

		CGSetLocalEventsSuppressionInterval(0.0);
		CGSetLocalEventsFilterDuringSupressionState(
		    kCGEventFilterMaskPermitAllEvents,
		    kCGEventSupressionStateSupressionInterval);
		CGSetLocalEventsFilterDuringSupressionState(
		    kCGEventFilterMaskPermitAllEvents,
		    kCGEventSupressionStateRemoteMouseDrag);

		macosxCGP_init_dimming();
		if (macosx_noscreensaver) {
			macosxCGP_screensaver_timer_on();
		}
		macosxGCS_initpb();
	}
}

void macosxCG_fini(void) {
	macosxCGP_dim_shutdown();
	if (macosx_noscreensaver) {
		macosxCGP_screensaver_timer_off();
	}
	macosxCG_refresh_callback_off();
}

extern int dpy_x, dpy_y, bpp, wdpy_x, wdpy_y;
extern int client_count, nofb;
extern void do_new_fb(int);
extern int macosx_wait_for_switch, macosx_resize;

void macosxCG_event_loop(void) {
	OSStatus rc;
	int nbpp;

	macosxGCS_poll_pb();
	if (nofb) {
		return;
	}

	rc = RunCurrentEventLoop(kEventDurationSecond/30);

	if (client_count) {
		macosxCG_refresh_callback_on();
	} else {
		macosxCG_refresh_callback_off();
	}

	nbpp = macosxCG_CGDisplayBitsPerPixel();

		
	if (nbpp > 0 && nbpp != bpp) {
		if (macosx_resize) {
			do_new_fb(1);
		}
	} else if (wdpy_x != (int) CGDisplayPixelsWide(displayID)) {
	    if (wdpy_y != (int) CGDisplayPixelsHigh(displayID)) {
		if (macosx_wait_for_switch) {
			int cnt = 0;
			while (1) {
				if(CGDisplayPixelsWide(displayID) > 0) {
					if(CGDisplayPixelsHigh(displayID) > 0) {
						usleep(500*1000);
						break;
					}
				}
				if ((cnt++ % 120) == 0) {
					fprintf(stderr, "waiting for user to "
					    "switch back..\n");
				}
				sleep(1);
			}
			if (wdpy_x == (int) CGDisplayPixelsWide(displayID)) {
				if (wdpy_y == (int) CGDisplayPixelsHigh(displayID)) {
					fprintf(stderr, "we're back...\n");
					return;
				}
			}
		}
		if (macosx_resize) {
			do_new_fb(1);
		}
	    }
	}
}

char *macosxCG_get_fb_addr(void) {
	macosxCG_init();
	return (char *) CGDisplayBaseAddress(displayID);
}

int macosxCG_CGDisplayPixelsWide(void) {
	return (int) CGDisplayPixelsWide(displayID);
}
int macosxCG_CGDisplayPixelsHigh(void) {
	return (int) CGDisplayPixelsHigh(displayID);
}
int macosxCG_CGDisplayBitsPerPixel(void) {
	return (int) CGDisplayBitsPerPixel(displayID);
}
int macosxCG_CGDisplayBitsPerSample(void) {
	return (int) CGDisplayBitsPerSample(displayID);
}
int macosxCG_CGDisplaySamplesPerPixel(void) {
	return (int) CGDisplaySamplesPerPixel(displayID);
}
int macosxCG_CGDisplayBytesPerRow(void) {
	return (int) CGDisplayBytesPerRow(displayID);;
}

typedef int CGSConnectionRef;
static CGSConnectionRef conn = 0;
extern CGError CGSNewConnection(void*, CGSConnectionRef*);
extern CGError CGSReleaseConnection(CGSConnectionRef);
extern CGError CGSGetGlobalCursorDataSize(CGSConnectionRef, int*);
extern CGError CGSGetGlobalCursorData(CGSConnectionRef, unsigned char*,
    int*, int*, CGRect*, CGPoint*, int*, int*, int*);
extern CGError CGSGetCurrentCursorLocation(CGSConnectionRef, CGPoint*);
extern int CGSCurrentCursorSeed(void);
extern int CGSHardwareCursorActive(); 

static unsigned int last_local_button_mask = 0;
static unsigned int last_local_mod_mask = 0;
static int last_local_x = 0;
static int last_local_y = 0;

extern unsigned int display_button_mask;
extern unsigned int display_mod_mask;
extern int got_local_pointer_input;
extern time_t last_local_input;

static CGPoint current_cursor_pos(void) {
	CGPoint pos;
	pos.x = 0;
	pos.y = 0;
	if (! conn) {
		if (CGSNewConnection(NULL, &conn) != kCGErrorSuccess) {
			fprintf(stderr, "CGSNewConnection error\n");
		}
	}
	if (CGSGetCurrentCursorLocation(conn, &pos) != kCGErrorSuccess) {
		fprintf(stderr, "CGSGetCurrentCursorLocation error\n");
	}

	display_button_mask = GetCurrentButtonState();
#if 0
/* not used yet */
	display_mod_mask = GetCurrentKeyModifiers();
#endif

	if (last_local_button_mask != display_button_mask) {
		got_local_pointer_input++;
		last_local_input = time(NULL);
	} else if (pos.x != last_local_x || pos.y != last_local_y) {
		got_local_pointer_input++;
		last_local_input = time(NULL);
	}
	last_local_button_mask = display_button_mask;
	last_local_mod_mask = display_mod_mask;
	last_local_x = pos.x;
	last_local_y = pos.y;

	return pos;
}

int macosxCG_get_cursor_pos(int *x, int *y) {
	CGPoint pos = current_cursor_pos();
	*x = pos.x;
	*y = pos.y;
	return 1;
}

extern int get_cursor_serial(int);
extern int store_cursor(int serial, unsigned long *data, int w, int h, int cbpp, int xhot, int yhot);

int macosxCG_get_cursor(void) {
	int last_idx = (int) get_cursor_serial(1);
	int which = 1;
	CGError err;
	int datasize, masksize, row_bytes, cdepth, comps, bpcomp;
	CGRect rect;
	CGPoint hot;
	unsigned char *data;
	int res, cursor_seed;
	static int last_cursor_seed = -1;
	static time_t last_fetch = 0;
	time_t now = time(NULL);

	if (last_idx) {
		which = last_idx;
	}

	if (! conn) {
		if (CGSNewConnection(NULL, &conn) != kCGErrorSuccess) {
			fprintf(stderr, "CGSNewConnection error\n");
			return which;
		}
	}

	cursor_seed = CGSCurrentCursorSeed();
	if (last_idx && cursor_seed == last_cursor_seed) {
		if (now < last_fetch + 2) {
			return which;
		}
	}
	last_cursor_seed = cursor_seed;
	last_fetch = now;

	if (CGSGetGlobalCursorDataSize(conn, &datasize) != kCGErrorSuccess) {
		fprintf(stderr, "CGSGetGlobalCursorDataSize error\n");
		return which;
	}

	data = (unsigned char*) malloc(datasize);

	err = CGSGetGlobalCursorData(conn, data, &datasize, &row_bytes,
	    &rect, &hot, &cdepth, &comps, &bpcomp);
	if (err != kCGErrorSuccess) {
		fprintf(stderr, "CGSGetGlobalCursorData error\n");
		return which;
	}

	if (cdepth == 24) {
		cdepth = 32;
	}

	which = store_cursor(cursor_seed, (unsigned long*) data,
	    (int) rect.size.width, (int) rect.size.height, cdepth, (int) hot.x, (int) hot.y);

	free(data);
	return(which);
}

extern int macosx_mouse_wheel_speed;
extern int macosx_swap23;
extern int off_x, coff_x, off_y, coff_y;

void macosxCG_pointer_inject(int mask, int x, int y) {
	int swap23 = macosx_swap23, rc;
	int s1 = 0, s2 = 1, s3 = 2, s4 = 3, s5 = 4;
	CGPoint loc;
	int wheel_distance = macosx_mouse_wheel_speed;
	static int cnt = 0;

	if (swap23) {
		s2 = 2;
		s3 = 1;
	}

	loc.x = x + off_x + coff_x;
	loc.y = y + off_y + coff_y;

	if ((cnt++ % 10) == 0) {
		macosxCGP_undim();
	}

	if ((mask & (1 << s4))) {
		CGPostScrollWheelEvent(1,  wheel_distance);
	}
	if ((mask & (1 << s5))) {
		CGPostScrollWheelEvent(1, -wheel_distance);
	}
	
	CGPostMouseEvent(loc, TRUE, 3,
	    (mask & (1 << s1)) ? TRUE : FALSE,
	    (mask & (1 << s2)) ? TRUE : FALSE,
	    (mask & (1 << s3)) ? TRUE : FALSE
	);
}

#define keyTableSize 0xFFFF

#include <rfb/keysym.h>

static int USKeyCodes[] = {
    /* The alphabet */
    XK_A,                  0,      /* A */
    XK_B,                 11,      /* B */
    XK_C,                  8,      /* C */
    XK_D,                  2,      /* D */
    XK_E,                 14,      /* E */
    XK_F,                  3,      /* F */
    XK_G,                  5,      /* G */
    XK_H,                  4,      /* H */
    XK_I,                 34,      /* I */
    XK_J,                 38,      /* J */
    XK_K,                 40,      /* K */
    XK_L,                 37,      /* L */
    XK_M,                 46,      /* M */
    XK_N,                 45,      /* N */
    XK_O,                 31,      /* O */
    XK_P,                 35,      /* P */
    XK_Q,                 12,      /* Q */
    XK_R,                 15,      /* R */
    XK_S,                  1,      /* S */
    XK_T,                 17,      /* T */
    XK_U,                 32,      /* U */
    XK_V,                  9,      /* V */
    XK_W,                 13,      /* W */
    XK_X,                  7,      /* X */
    XK_Y,                 16,      /* Y */
    XK_Z,                  6,      /* Z */
    XK_a,                  0,      /* a */
    XK_b,                 11,      /* b */
    XK_c,                  8,      /* c */
    XK_d,                  2,      /* d */
    XK_e,                 14,      /* e */
    XK_f,                  3,      /* f */
    XK_g,                  5,      /* g */
    XK_h,                  4,      /* h */
    XK_i,                 34,      /* i */
    XK_j,                 38,      /* j */
    XK_k,                 40,      /* k */
    XK_l,                 37,      /* l */
    XK_m,                 46,      /* m */
    XK_n,                 45,      /* n */
    XK_o,                 31,      /* o */
    XK_p,                 35,      /* p */
    XK_q,                 12,      /* q */
    XK_r,                 15,      /* r */
    XK_s,                  1,      /* s */
    XK_t,                 17,      /* t */
    XK_u,                 32,      /* u */
    XK_v,                  9,      /* v */
    XK_w,                 13,      /* w */
    XK_x,                  7,      /* x */
    XK_y,                 16,      /* y */
    XK_z,                  6,      /* z */

    /* Numbers */
    XK_0,                 29,      /* 0 */
    XK_1,                 18,      /* 1 */
    XK_2,                 19,      /* 2 */
    XK_3,                 20,      /* 3 */
    XK_4,                 21,      /* 4 */
    XK_5,                 23,      /* 5 */
    XK_6,                 22,      /* 6 */
    XK_7,                 26,      /* 7 */
    XK_8,                 28,      /* 8 */
    XK_9,                 25,      /* 9 */

    /* Symbols */
    XK_exclam,            18,      /* ! */
    XK_at,                19,      /* @ */
    XK_numbersign,        20,      /* # */
    XK_dollar,            21,      /* $ */
    XK_percent,           23,      /* % */
    XK_asciicircum,       22,      /* ^ */
    XK_ampersand,         26,      /* & */
    XK_asterisk,          28,      /* * */
    XK_parenleft,         25,      /* ( */
    XK_parenright,        29,      /* ) */
    XK_minus,             27,      /* - */
    XK_underscore,        27,      /* _ */
    XK_equal,             24,      /* = */
    XK_plus,              24,      /* + */
    XK_grave,             50,      /* ` */  /* XXX ? */
    XK_asciitilde,        50,      /* ~ */
    XK_bracketleft,       33,      /* [ */
    XK_braceleft,         33,      /* { */
    XK_bracketright,      30,      /* ] */
    XK_braceright,        30,      /* } */
    XK_semicolon,         41,      /* ; */
    XK_colon,             41,      /* : */
    XK_apostrophe,        39,      /* ' */
    XK_quotedbl,          39,      /* " */
    XK_comma,             43,      /* , */
    XK_less,              43,      /* < */
    XK_period,            47,      /* . */
    XK_greater,           47,      /* > */
    XK_slash,             44,      /* / */
    XK_question,          44,      /* ? */
    XK_backslash,         42,      /* \ */
    XK_bar,               42,      /* | */
    // OS X Sends this (END OF MEDIUM) for Shift-Tab (with US Keyboard)
    0x0019,               48,      /* Tab */
    XK_space,             49,      /* Space */
};

static int SpecialKeyCodes[] = {
    /* "Special" keys */
    XK_Return,            36,      /* Return */
    XK_Delete,           117,      /* Delete */
    XK_Tab,               48,      /* Tab */
    XK_Escape,            53,      /* Esc */
    XK_Caps_Lock,         57,      /* Caps Lock */
    XK_Num_Lock,          71,      /* Num Lock */
    XK_Scroll_Lock,      107,      /* Scroll Lock */
    XK_Pause,            113,      /* Pause */
    XK_BackSpace,         51,      /* Backspace */
    XK_Insert,           114,      /* Insert */

    /* Cursor movement */
    XK_Up,               126,      /* Cursor Up */
    XK_Down,             125,      /* Cursor Down */
    XK_Left,             123,      /* Cursor Left */
    XK_Right,            124,      /* Cursor Right */
    XK_Page_Up,          116,      /* Page Up */
    XK_Page_Down,        121,      /* Page Down */
    XK_Home,             115,      /* Home */
    XK_End,              119,      /* End */

    /* Numeric keypad */
    XK_KP_0,              82,      /* KP 0 */
    XK_KP_1,              83,      /* KP 1 */
    XK_KP_2,              84,      /* KP 2 */
    XK_KP_3,              85,      /* KP 3 */
    XK_KP_4,              86,      /* KP 4 */
    XK_KP_5,              87,      /* KP 5 */
    XK_KP_6,              88,      /* KP 6 */
    XK_KP_7,              89,      /* KP 7 */
    XK_KP_8,              91,      /* KP 8 */
    XK_KP_9,              92,      /* KP 9 */
    XK_KP_Enter,          76,      /* KP Enter */
    XK_KP_Decimal,        65,      /* KP . */
    XK_KP_Add,            69,      /* KP + */
    XK_KP_Subtract,       78,      /* KP - */
    XK_KP_Multiply,       67,      /* KP * */
    XK_KP_Divide,         75,      /* KP / */

    /* Function keys */
    XK_F1,               122,      /* F1 */
    XK_F2,               120,      /* F2 */
    XK_F3,                99,      /* F3 */
    XK_F4,               118,      /* F4 */
    XK_F5,                96,      /* F5 */
    XK_F6,                97,      /* F6 */
    XK_F7,                98,      /* F7 */
    XK_F8,               100,      /* F8 */
    XK_F9,               101,      /* F9 */
    XK_F10,              109,      /* F10 */
    XK_F11,              103,      /* F11 */
    XK_F12,              111,      /* F12 */

    /* Modifier keys */
    XK_Alt_L,             55,      /* Alt Left (-> Command) */
    XK_Alt_R,             55,      /* Alt Right (-> Command) */
    XK_Shift_L,           56,      /* Shift Left */
    XK_Shift_R,           56,      /* Shift Right */
    XK_Meta_L,            58,      /* Option Left (-> Option) */
    XK_Meta_R,            58,      /* Option Right (-> Option) */
    XK_Super_L,           58,      /* Option Left (-> Option) */
    XK_Super_R,           58,      /* Option Right (-> Option) */
    XK_Control_L,         59,      /* Ctrl Left */
    XK_Control_R,         59,      /* Ctrl Right */
};

CGKeyCode keyTable[keyTableSize];
unsigned char keyTableMods[keyTableSize];

void macosxCG_init_key_table(void) {
	static int init = 0;
	int i;
	if (init) {
		return;
	}
	init = 1;

	for (i=0; i < keyTableSize; i++) {
		keyTable[i] = 0xFFFF;
		keyTableMods[i] = 0;
	}
	for (i=0; i< (sizeof(USKeyCodes) / sizeof(int)); i += 2) {
		int j = USKeyCodes[i];
		keyTable[(unsigned short) j] = (CGKeyCode) USKeyCodes[i+1];
	}
	for (i=0; i< (sizeof(SpecialKeyCodes) / sizeof(int)); i += 2) {
		int j = SpecialKeyCodes[i];
		keyTable[(unsigned short) j] = (CGKeyCode) SpecialKeyCodes[i+1];
	}
}

void macosxCG_key_inject(int down, unsigned int keysym) {
	static int control = 0, alt = 0;
	int pressModsForKeys = FALSE;

	CGKeyCode keyCode = keyTable[(unsigned short)keysym];
	CGCharCode keyChar = 0;
	UInt32 modsForKey = keyTableMods[keysym] << 8;

	init_key_table();

	if (keysym < 0xFF) {
		keyChar = (CGCharCode) keysym;
	}
	if (keyCode == 0xFFFF) {
		return;
	}
	macosxCGP_undim();
	CGPostKeyboardEvent(keyChar, keyCode, down);
}

#endif	/* __APPLE__ */


