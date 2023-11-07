// menu.c

/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "quakedef.h"
#include "cdaudio.h"
#include "image.h"
#include "progsvm.h"

#include "mprogdefs.h"

#define TYPE_DEMO 1
#define TYPE_GAME 2
#define TYPE_BOTH 3

static cvar_t forceqmenu = {CF_CLIENT, "forceqmenu", "0", "enables the quake menu instead of the quakec menu.dat (if present)"};
static cvar_t menu_progs = {CF_CLIENT, "menu_progs", "menu.dat", "name of quakec menu.dat file"};
static cvar_t menu_options_colorcontrol_correctionvalue = {CF_CLIENT, "menu_options_colorcontrol_correctionvalue", "0.5", "intensity value that matches up to white/black dither pattern, should be 0.5 for linear color"};

static void M_Init(void);

// Baker: For showfps in Zircon
static int rotc(int count, int val, int dir)
{
	int j = val + dir;
	     if (j >= count)	j = 0;
	else if (j < 0)			j = count - 1;
	return j;
}

static int NehGameType;
void menu_state_set_nova (int ee);
static qbool m_missingdata = false;

int menu_state_reenter = 0; // To renter at maps menu


extern cvar_t gl_picmip;
extern dllhandle_t jpeg_dll;
extern cvar_t host_timescale;
extern cvar_t gl_texture_anisotropy;
extern cvar_t r_textshadow;
extern cvar_t r_hdr_scenebrightness;


static int menuplyr_width, menuplyr_height, menuplyr_top, menuplyr_bottom, menuplyr_load;
static unsigned char *menuplyr_pixels;
static unsigned int *menuplyr_translated;

enum m_state_e m_state;
char m_return_reason[128];

void M_Menu_Main_f(cmd_state_t *cmd);
	void M_Menu_SinglePlayer_f(cmd_state_t *cmd);
		void M_Menu_Load_f(cmd_state_t *cmd);
		void M_Menu_Save_f(cmd_state_t *cmd);
	void M_Menu_MultiPlayer_f(cmd_state_t *cmd);
		void M_Menu_Setup_f(cmd_state_t *cmd);
	void M_Menu_Options_Nova_f(cmd_state_t *cmd);
	void M_Menu_Options_Classic_f(cmd_state_t *cmd);
	void M_Menu_Options_Effects_f(cmd_state_t *cmd);
	void M_Menu_Options_Graphics_f(cmd_state_t *cmd);
	void M_Menu_Options_ColorControl_f(cmd_state_t *cmd);
		void M_Menu_Keys_f(cmd_state_t *cmd);
		void M_Menu_Reset_f(cmd_state_t *cmd);
		void M_Menu_Video_Classic_f(cmd_state_t *cmd);
		void M_Menu_Video_Nova_f(cmd_state_t *cmd);
	void M_Menu_Help_f(cmd_state_t *cmd);
		void M_Menu_Maps_f(cmd_state_t *cmd);
	void M_Menu_Credits_f(cmd_state_t *cmd);
	void M_Menu_Quit_f(cmd_state_t *cmd);
void M_Menu_LanConfig_f(cmd_state_t *cmd);
void M_Menu_GameOptions_f(cmd_state_t *cmd);
void M_Menu_ServerList_f(cmd_state_t *cmd);
void M_Menu_ModList_f(cmd_state_t *cmd);

static void M_Main_Draw (void);
	static void M_SinglePlayer_Draw (void);
		static void M_Load_Draw (void);
		static void M_Save_Draw (void);
	static void M_MultiPlayer_Draw (void);
		static void M_Setup_Draw (void);
	static void M_Options_Classic_Draw (void);
	static void M_Options_Nova_Draw (void);
	static void M_Options_Effects_Draw (void);
	static void M_Options_Graphics_Draw (void);
	static void M_Options_ColorControl_Draw (void);
		static void M_Keys_Draw (void);
		static void M_Reset_Draw (void);
		static void M_Video_Classic_Draw (void);
		static void M_Video_Nova_Draw (void);
		static void M_Maps_Draw (void);
	static void M_Help_Draw (void);
	static void M_Credits_Draw (void);
	static void M_Quit_Draw (void);
static void M_LanConfig_Draw (void);
static void M_GameOptions_Draw (void);
static void M_ServerList_Draw (void);
static void M_ModList_Draw (void);


static void M_Main_Key(cmd_state_t *cmd, int key, int ascii);
	static void M_SinglePlayer_Key(cmd_state_t *cmd, int key, int ascii);
		static void M_Load_Key(cmd_state_t *cmd, int key, int ascii);
		static void M_Save_Key(cmd_state_t *cmd, int key, int ascii);
	static void M_MultiPlayer_Key(cmd_state_t *cmd, int key, int ascii);
		static void M_Setup_Key(cmd_state_t *cmd, int key, int ascii);
	static void M_Options_Classic_Key(cmd_state_t *cmd, int key, int ascii);
	static void M_Options_Nova_Key (cmd_state_t *cmd, int key, int ascii);
	static void M_Options_Effects_Key(cmd_state_t *cmd, int key, int ascii);
	static void M_Options_Graphics_Key(cmd_state_t *cmd, int key, int ascii);
	static void M_Options_ColorControl_Key(cmd_state_t *cmd, int key, int ascii);
		static void M_Keys_Key(cmd_state_t *cmd, int key, int ascii);
		static void M_Reset_Key(cmd_state_t *cmd, int key, int ascii);
		static void M_Video_Classic_Key(cmd_state_t *cmd, int key, int ascii);
		static void M_Video_Nova_Key (cmd_state_t *cmd, int key, int ascii);
		static void M_Maps_Key (cmd_state_t *cmd, int k, int ascii);
	static void M_Help_Key(cmd_state_t *cmd, int key, int ascii);
	static void M_Credits_Key(cmd_state_t *cmd, int key, int ascii);
	static void M_Quit_Key(cmd_state_t *cmd, int key, int ascii);
static void M_LanConfig_Key(cmd_state_t *cmd, int key, int ascii);
static void M_GameOptions_Key(cmd_state_t *cmd, int key, int ascii);
static void M_ServerList_Key(cmd_state_t *cmd, int key, int ascii);
static void M_ModList_Key(cmd_state_t *cmd, int key, int ascii);

static qbool	m_entersound;		///< play after drawing a frame, so caching won't disrupt the sound

video_resolution_t video_resolutions_hardcoded[] =
{
{"Standard 4x3"              ,  320, 240, 320, 240, 1     },
{"Standard 4x3"              ,  400, 300, 400, 300, 1     },
{"Standard 4x3"              ,  512, 384, 512, 384, 1     },
{"Standard 4x3"              ,  640, 480, 640, 480, 1     },
{"Standard 4x3"              ,  800, 600, 640, 480, 1     },
{"Standard 4x3"              , 1024, 768, 640, 480, 1     },
{"Standard 4x3"              , 1152, 864, 640, 480, 1     },
{"Standard 4x3"              , 1280, 960, 640, 480, 1     },
{"Standard 4x3"              , 1400,1050, 640, 480, 1     },
{"Standard 4x3"              , 1600,1200, 640, 480, 1     },
{"Standard 4x3"              , 1792,1344, 640, 480, 1     },
{"Standard 4x3"              , 1856,1392, 640, 480, 1     },
{"Standard 4x3"              , 1920,1440, 640, 480, 1     },
{"Standard 4x3"              , 2048,1536, 640, 480, 1     },
{"Short Pixel (CRT) 5x4"     ,  320, 256, 320, 256, 0.9375},
{"Short Pixel (CRT) 5x4"     ,  640, 512, 640, 512, 0.9375},
{"Short Pixel (CRT) 5x4"     , 1280,1024, 640, 512, 0.9375},
{"Tall Pixel (CRT) 8x5"      ,  320, 200, 320, 200, 1.2   },
{"Tall Pixel (CRT) 8x5"      ,  640, 400, 640, 400, 1.2   },
{"Tall Pixel (CRT) 8x5"      ,  840, 525, 640, 400, 1.2   },
{"Tall Pixel (CRT) 8x5"      ,  960, 600, 640, 400, 1.2   },
{"Tall Pixel (CRT) 8x5"      , 1680,1050, 640, 400, 1.2   },
{"Tall Pixel (CRT) 8x5"      , 1920,1200, 640, 400, 1.2   },
{"Square Pixel (LCD) 5x4"    ,  320, 256, 320, 256, 1     },
{"Square Pixel (LCD) 5x4"    ,  640, 512, 640, 512, 1     },
{"Square Pixel (LCD) 5x4"    , 1280,1024, 640, 512, 1     },
{"WideScreen 5x3"            ,  640, 384, 640, 384, 1     },
{"WideScreen 5x3"            , 1280, 768, 640, 384, 1     },
{"WideScreen 8x5"            ,  320, 200, 320, 200, 1     },
{"WideScreen 8x5"            ,  640, 400, 640, 400, 1     },
{"WideScreen 8x5"            ,  720, 450, 720, 450, 1     },
{"WideScreen 8x5"            ,  840, 525, 640, 400, 1     },
{"WideScreen 8x5"            ,  960, 600, 640, 400, 1     },
{"WideScreen 8x5"            , 1280, 800, 640, 400, 1     },
{"WideScreen 8x5"            , 1440, 900, 720, 450, 1     },
{"WideScreen 8x5"            , 1680,1050, 640, 400, 1     },
{"WideScreen 8x5"            , 1920,1200, 640, 400, 1     },
{"WideScreen 8x5"            , 2560,1600, 640, 400, 1     },
{"WideScreen 8x5"            , 3840,2400, 640, 400, 1     },
{"WideScreen 14x9"           ,  840, 540, 640, 400, 1     },
{"WideScreen 14x9"           , 1680,1080, 640, 400, 1     },
{"WideScreen 16x9"           ,  640, 360, 640, 360, 1     },
{"WideScreen 16x9"           ,  683, 384, 683, 384, 1     },
{"WideScreen 16x9"           ,  960, 540, 640, 360, 1     },
{"WideScreen 16x9"           , 1280, 720, 640, 360, 1     },
{"WideScreen 16x9"           , 1360, 768, 680, 384, 1     },
{"WideScreen 16x9"           , 1366, 768, 683, 384, 1     },
{"WideScreen 16x9"           , 1920,1080, 640, 360, 1     },
{"WideScreen 16x9"           , 2560,1440, 640, 360, 1     },
{"WideScreen 16x9"           , 3840,2160, 640, 360, 1     },
{"NTSC 3x2"                  ,  360, 240, 360, 240, 1.125 },
{"NTSC 3x2"                  ,  720, 480, 720, 480, 1.125 },
{"PAL 14x11"                 ,  360, 283, 360, 283, 0.9545},
{"PAL 14x11"                 ,  720, 566, 720, 566, 0.9545},
{"NES 8x7"                   ,  256, 224, 256, 224, 1.1667},
{"SNES 8x7"                  ,  512, 448, 512, 448, 1.1667},
{NULL, 0, 0, 0, 0, 0}
};
// this is the number of the default mode (640x480) in the list above
int video_resolutions_hardcoded_count = sizeof(video_resolutions_hardcoded) / sizeof(*video_resolutions_hardcoded) - 1;

static qbool menu_video_resolutions_forfullscreen;

void M_Update_Return_Reason(const char *s)
{
	strlcpy(m_return_reason, s, sizeof(m_return_reason));
	if (s)
		Con_DPrintf ("%s\n", s);
}

#define StartingGame	(m_multiplayer_cursor == 1)
#define JoiningGame		(m_multiplayer_cursor == 0)

// Nehahra
#define NumberOfNehahraDemos 34
typedef struct nehahrademonames_s
{
	const char *name;
	const char *desc;
} nehahrademonames_t;

static nehahrademonames_t NehahraDemos[NumberOfNehahraDemos] =
{
	{"intro", "Prologue"},
	{"genf", "The Beginning"},
	{"genlab", "A Doomed Project"},
	{"nehcre", "The New Recruits"},
	{"maxneh", "Breakthrough"},
	{"maxchar", "Renewal and Duty"},
	{"crisis", "Worlds Collide"},
	{"postcris", "Darkening Skies"},
	{"hearing", "The Hearing"},
	{"getjack", "On a Mexican Radio"},
	{"prelude", "Honor and Justice"},
	{"abase", "A Message Sent"},
	{"effect", "The Other Side"},
	{"uhoh", "Missing in Action"},
	{"prepare", "The Response"},
	{"vision", "Farsighted Eyes"},
	{"maxturns", "Enter the Immortal"},
	{"backlot", "Separate Ways"},
	{"maxside", "The Ancient Runes"},
	{"counter", "The New Initiative"},
	{"warprep", "Ghosts to the World"},
	{"counter1", "A Fate Worse Than Death"},
	{"counter2", "Friendly Fire"},
	{"counter3", "Minor Setback"},
	{"madmax", "Scores to Settle"},
	{"quake", "One Man"},
	{"cthmm", "Shattered Masks"},
	{"shades", "Deal with the Dead"},
	{"gophil", "An Unlikely Hero"},
	{"cstrike", "War in Hell"},
	{"shubset", "The Conspiracy"},
	{"shubdie", "Even Death May Die"},
	{"newranks", "An Empty Throne"},
	{"seal", "The Seal is Broken"}
};

static float menu_x, menu_y, menu_width, menu_height;

static void M_Background(int width, int height, int shall_darken)
{
	menu_width = bound(1.0f, (float)width, vid_conwidth.value);
	menu_height = bound(1.0f, (float)height, vid_conheight.value);
	menu_x = (vid_conwidth.integer - menu_width) * 0.5;
	menu_y = (vid_conheight.integer - menu_height) * 0.5;
	//DrawQ_Fill(menu_x, menu_y, menu_width, menu_height, 0, 0, 0, 0.5, 0);

	if (shall_darken) {
		DrawQ_Fill(0, 0, vid_conwidth.integer, vid_conheight.integer, 0, 0, 0, 0.5, 0);
	}
}

/*
================
M_DrawCharacter

Draws one solid graphics character
================
*/
static void M_DrawCharacter (float cx, float cy, int num)
{
	char temp[2];
	temp[0] = num;
	temp[1] = 0;
	DrawQ_String(menu_x + cx, menu_y + cy, temp, 1, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_MENU);
}

static void M_PrintColored(float cx, float cy, const char *str)
{
	DrawQ_String(menu_x + cx, menu_y + cy, str, 0, 8, 8, 1, 1, 1, 1, 0, NULL, false, FONT_MENU);
}

static void M_Print(float cx, float cy, const char *str)
{
	DrawQ_String(menu_x + cx, menu_y + cy, str, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_MENU);
}

// Handful of users, like server browser column header
//  Ping Players  Name
static void M_PrintBronzey(float cx, float cy, const char *str)
{
	DrawQ_String(menu_x + cx, menu_y + cy, str, 0, 8, 8, /*rgb*/ 1.03, 0.5, 0.36, 1, 0, NULL, true, FONT_MENU);
}

static void M_PrintRed(float cx, float cy, const char *str)
{
	DrawQ_String(menu_x + cx, menu_y + cy, str, 0, 8, 8, 1, 0, 0, 1, 0, NULL, true, FONT_MENU);
}

static void M_ItemPrint(float cx, float cy, const char *str, int unghosted)
{
	if (unghosted)
		DrawQ_String(menu_x + cx, menu_y + cy, str, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_MENU);
	else
		DrawQ_String(menu_x + cx, menu_y + cy, str, 0, 8, 8, 0.4, 0.4, 0.4, 1, 0, NULL, true, FONT_MENU);
}

static void M_ItemPrint2(float cx, float cy, const char *str, int unghosted)
{
	if (unghosted)
		DrawQ_String(menu_x + cx, menu_y + cy, str, 0, 8, 8, 1, 1, 1, 1, 0, NULL, /*ignore color codes*/ false, FONT_MENU);
	else
		DrawQ_String(menu_x + cx, menu_y + cy, str, 0, 8, 8, 0.4, 0.4, 0.4, 1, 0, NULL, /*ignore color codes*/ false, FONT_MENU);
}

#define NO_HOTSPOTS_0		0	// Draw an item that is not interactable like a title.
#define USE_IMAGE_SIZE_NEG1	-1
#define USE_IMAGE_SIZE_NEG1	-1
#define NA0					0	// Not applicable (for non-hotspots) like uninteractive text

typedef enum {
	hotspottype_none_0 = 0,
	hotspottype_inert,			// It's a hotspot.  It's entirely inert.  Used to fill to keep indexes static.
	hotspottype_toggle,			// More than 2 choices.		Responds to left and right.
	hotspottype_slider,			// Slider.					Responds to left and right.
	hotspottype_button,			// Execute.					Example: Single Player, Reset Defaults
	hotspottype_button_line,	// Execute.					Namemaker.  Same as button but little extra trim of what is highlighted for clarity.
	hotspottype_listitem,		// A list item				Demos and levels and serverlist.
	hotspottype_text,			// Text						// ?
	hotspottype_textbutton,		// Text button.  Enter fires.
	hotspottype_screen,			// Help uses this for next.  Remember, it is not acceptable for a K_MOUSE1 with no hotspot to do something.
	hotspottype_vscroll,		//
	hotspottype_hscroll,		//
} hotspottype_e;


typedef struct {
		float	left, top, width, height;
} crectf_s;

typedef struct {
	crectf_s 		r;			// Rect collision .left .top .width .height
	int				idx;		// Our number (cursor).

// Scrolling lists use trueidx for the index of an item drawn.
// For instance, if there are 400 maps
// A draw might start at map list index 200.
// So for collision identification we need to use
// the trueidx to find the proper array index
// for a list item.
	int				trueidx;	// Set to not_found_neg1 (-1) for non-list elements.			
	hotspottype_e	hotspottype;
} hotspotx_s;

hotspotx_s hotspotxs[128];
int hotspotx_count;
int hotspotx_hover;
hotspotx_s *hotspotx_hover_hs;

// These are set each menu draw frame
static int drawidx;		// Set to 0 at start of draw.  This is the draw index -- the hotspot slot filled
static int drawcur_y;	// Some of the menus reset to this to zero and use this as the draw cursor for y positioning.
static int drawsel_idx; // Set to not_found_neg1 at start of draw.

// drawsel_idx is sort of a universal "this item index is selected".

int Hotspots_Hit (void)
{
	int n; for (n = 0; n < hotspotx_count; n ++) {
		hotspotx_s *h = &hotspotxs[n];
		int is_hit = RECT_HIT (h->r, in_windowmouse_x, in_windowmouse_y);
		if (is_hit) {
			return n;
		} // hit
	} // for
	return not_found_neg1;
}

int Hotspots_DidHit_Slider (void)
{
	hotspotx_hover = Hotspots_Hit();
	if (hotspotx_hover != not_found_neg1) {
		hotspotx_hover_hs = &hotspotxs[hotspotx_hover];
		if (hotspotx_hover_hs->hotspottype == hotspottype_slider)
			return true;
	}
	return false;
}

int Hotspots_DidHit (void)
{
	hotspotx_hover = Hotspots_Hit();
	if (hotspotx_hover != not_found_neg1) {
		return true;
	}
	return false;
}

int Hotspots_GetIdx (int hidx)
{
	if (in_range_beyond (0,  hidx, hotspotx_count )) {
		hotspotx_s *h = &hotspotxs[hidx];
		if (h->trueidx != not_found_neg1)
			return h->trueidx;

		return hidx;
	}
	return not_found_neg1;
}


hotspotx_s *Hotspots_Add (float left, float top, float width, float height, int count_, hotspottype_e hotspottype)
{
	// convert to screen pixels
	float xmag = vid.width / vid_conwidth.value; // 1300 / 240
	float ymag = vid.height / vid_conheight.value;
	left = left * xmag;
	top = top * ymag;
	width = width * xmag;
	height = height * ymag;


	hotspotx_s *h = NULL;
	int count	= count_ ? count_ : 1;
	int nheight = count_ ? height / count : height;

	int n; for (n = 0; n < count; n ++) {
		int idx = hotspotx_count;
		h = &hotspotxs[idx];
		h->idx			= idx;
		h->trueidx 		= not_found_neg1;
		h->hotspottype	= hotspottype;
		RECT_SET (h->r, left, top, width, nheight);
		// Con_PrintLinef ("Hotspot %d = %d, %d %d x %d", hotspot_menu_item[idx].idx, hotspot_menu_item[idx].rect.left, hotspot_menu_item[idx].rect.top, hotspot_menu_item[idx].rect.width, hotspot_menu_item[idx].rect.height);
		top += nheight;
		hotspotx_count ++;
	} // for


	return h;
}

// Anything that can scroll must come here instead
// Only maps list and server calls this
void Hotspots_Add2 (float left, float top, float width, float height, int count_, hotspottype_e hotspottype, int trueidx)
{
	hotspotx_s *h = Hotspots_Add (left, top, width, height, count_, hotspottype);
	h->trueidx = trueidx;
}

// Called at the start of most M_ Draw frames (almost).
// Why does M_Main not call it?
#define PPX_Start(realcursor, frameselcursor) drawidx = 0; drawsel_idx = not_found_neg1; frameselcursor = realcursor

// Call at end of every M_ Draw frame (almost)
// Why does M_Main not call it, for instance?
// It highlights the current item.
static void PPX_DrawSel_End (void)
{
	if (drawsel_idx!= not_found_neg1) { // PPX SEL
		crectf_s r	= hotspotxs[drawsel_idx].r;
		float xmag	= vid.width / vid_conwidth.value; // 1300 / 240
		float ymag	= vid.height / vid_conheight.value;

		float left		= r.left	/ xmag;
		float top		= r.top		/ ymag;
		float width		= r.width	/ xmag;
		float height	= r.height	/ ymag;

		float redx = 0.5 + 0.2 * sin(host.realtime * M_PI);
		DrawQ_Fill(left, top, width, height, /*rgb*/ redx, 0, 0, /*a*/ 0.5, DRAWFLAG_ADDITIVE);
	} // if
}

//
// SYNC
//
#pragma message ("Baker: Are we hardened enough against p0 == NULL and doing p0->width?")

static void M_DrawPic(float cx, float cy, const char *picname, int count, int _colsize, int _rowsize)
{
	cachepic_t *pico = Draw_CachePic (picname);
	DrawQ_Pic(menu_x + cx, menu_y + cy, pico, 0, 0, 1, 1, 1, 1, 0);
	if (count) {
		int width  = (_colsize == USE_IMAGE_SIZE_NEG1) ? Draw_GetPicWidth(pico) : _colsize * count;
		int height = (_rowsize == USE_IMAGE_SIZE_NEG1) ? Draw_GetPicHeight(pico) : _rowsize * count;
		Hotspots_Add (menu_x + cx, menu_y + cy, width, height, count, hotspottype_button);
	}
}

static void M_DrawTextBox(float x, float y, float width, float height)
{
	int n;
	float cx, cy;

	// draw left side
	cx = x;
	cy = y;
	M_DrawPic (cx, cy, "gfx/box_tl", NO_HOTSPOTS_0, NA0, NA0);
	for (n = 0; n < height; n++)
	{
		cy += 8;
		M_DrawPic (cx, cy, "gfx/box_ml", NO_HOTSPOTS_0, NA0, NA0);
	}
	M_DrawPic (cx, cy+8, "gfx/box_bl", NO_HOTSPOTS_0, NA0, NA0);

	// draw middle
	cx += 8;
	while (width > 0)
	{
		cy = y;
		M_DrawPic (cx, cy, "gfx/box_tm", NO_HOTSPOTS_0, NA0, NA0);
		for (n = 0; n < height; n++)
		{
			cy += 8;
			if (n >= 1)
				M_DrawPic (cx, cy, "gfx/box_mm2", NO_HOTSPOTS_0, NA0, NA0);
			else
				M_DrawPic (cx, cy, "gfx/box_mm", NO_HOTSPOTS_0, NA0, NA0);
		}
		M_DrawPic (cx, cy+8, "gfx/box_bm", NO_HOTSPOTS_0, NA0, NA0);
		width -= 2;
		cx += 16;
	}

	// draw right side
	cy = y;
	M_DrawPic (cx, cy, "gfx/box_tr", NO_HOTSPOTS_0, NA0, NA0);
	for (n = 0; n < height; n++)
	{
		cy += 8;
		M_DrawPic (cx, cy, "gfx/box_mr", NO_HOTSPOTS_0, NA0, NA0);
	}
	M_DrawPic (cx, cy+8, "gfx/box_br", NO_HOTSPOTS_0, NA0, NA0);
}

//=============================================================================

//int m_save_demonum;

/*
================
M_ToggleMenu
================
*/
static void M_ToggleMenu(int mode)
{
	m_entersound = true;

	if ((key_dest != key_menu && key_dest != key_menu_grabbed) || m_state != m_main)
	{
		if (mode == 0) {
			return; // the menu is off, and we want it off
		} else {

#if 1
	#pragma message ("Baker: Until the key release issue can never-ever happen, make opening the menu do it")
			Key_ReleaseAll();
#endif
			if (menu_state_reenter) {
				menu_state_reenter = 0;
				M_Menu_Maps_f (cmd_local);
			} else {
				M_Menu_Main_f (cmd_local);
			}
		} // mode
	} // keydest
	else
	{
		if (mode == 1)
			return; // the menu is on, and we want it on
		key_dest = key_game;
		menu_state_set_nova (m_none);
	}
}


static int demo_cursor;
static void M_Demo_Draw (void)
{
	int i;

	M_Background(320, 200, q_darken_true);

	for (i = 0;i < NumberOfNehahraDemos;i++)
		M_Print(16, 16 + 8*i, NehahraDemos[i].desc);

	// line cursor
	M_DrawCharacter (8, 16 + demo_cursor*8, 12+((int)(host.realtime*4)&1));
}


static void M_Menu_Demos_f(cmd_state_t *cmd)
{
	key_dest = key_menu;
	menu_state_set_nova (m_demo);
	m_entersound = true;
}


static void M_Demo_Key (cmd_state_t *cmd, int k, int ascii)
{
	char vabuf[1024];
	switch (k)
	{
	case K_ESCAPE:	case K_MOUSE2:
		M_Menu_Main_f (cmd);
		break;

	case K_ENTER:
		S_LocalSound ("sound/misc/menu2.wav");
		menu_state_set_nova (m_none);
		key_dest = key_game;
		Cbuf_AddTextLine (cmd, va(vabuf, sizeof(vabuf), "playdemo %s", NehahraDemos[demo_cursor].name));
		return;

	case K_HOME:
		demo_cursor = 0;
		break;

	case K_END:
		demo_cursor = NumberOfNehahraDemos - 1;
		break;

	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		demo_cursor--;
		if (demo_cursor < 0)
			demo_cursor = NumberOfNehahraDemos-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		demo_cursor++;
		if (demo_cursor >= NumberOfNehahraDemos)
			demo_cursor = 0;
		break;
	}
}



//=============================================================================
/* OPTIONS MENU */

#define	SLIDER_RANGE	10

static void M_DrawSlider (int x, int y, float num, float rangemin, float rangemax)
{
	char text[16];
	int i;
	float range;
	range = bound(0, (num - rangemin) / (rangemax - rangemin), 1);
	M_DrawCharacter (x-8, y, 128);
	for (i = 0;i < SLIDER_RANGE;i++)
		M_DrawCharacter (x + i*8, y, 129);
	M_DrawCharacter (x+i*8, y, 130);
	M_DrawCharacter (x + (SLIDER_RANGE-1)*8 * range, y, 131);
	if (fabs((int)num - num) < 0.01)
		dpsnprintf(text, sizeof(text), "%d", (int)num);
	else
		dpsnprintf(text, sizeof(text), "%.3f", num);
	M_Print(x + (SLIDER_RANGE+2) * 8, y, text);
}

static void M_DrawCheckbox (int x, int y, int on)
{
	if (on)
		M_Print(x, y, "on");
	else
		M_Print(x, y, "off");
}

// Baker: So yes no dialog can be called by
// 2 different menus and still return to the right one.
// Yes we have "new" options menu and the classic one
int m_resetdef_prevstate;

void M_Menu_Reset_f(cmd_state_t *cmd)
{
	key_dest = key_menu;
	m_resetdef_prevstate = m_state;
	menu_state_set_nova (m_reset);
	m_entersound = true;
}

// Reset to defaults confirmation dialog.
static void M_Reset_Key(cmd_state_t *cmd, int key, int ascii)
{
	char vabuf[1024];
	switch (key) {
	case K_MOUSE1:
	case 'Y':
	case 'y': {
		Cbuf_AddTextLine (cmd, "cvar_resettodefaults_all;exec default.cfg");

		// Baker 1024: Prevent stale values of vid_width or vid_height after a reset
		Cbuf_AddTextLine (cmd, va(vabuf, sizeof(vabuf), "vid_width %d", vid_width.integer));
		Cbuf_AddTextLine (cmd, va(vabuf, sizeof(vabuf), "vid_height %d", vid_height.integer));
		Cbuf_AddTextLine (cmd, va(vabuf, sizeof(vabuf), "vid_fullscreen %d", vid_fullscreen.integer));

		// no break here since we also exit the menu
			  }
	case K_ESCAPE:	case K_MOUSE2:
	case 'n':
	case 'N':

		if (m_resetdef_prevstate == m_options_nova) {
			menu_state_set_nova (m_options_nova);
		} else {
			menu_state_set_nova (m_options_classic);
		}
		m_entersound = true;
		break;

	default:
		break;
	}
}

static void M_Reset_Draw (void)
{
	int lines = 2, linelength = 20;
	M_Background(linelength * 8 + 16, lines * 8 + 16, q_darken_true);
	M_DrawTextBox(0, 0, linelength, lines);
	M_Print(8 + 4 * (linelength - 19),  8, "Reset all settings?");
	M_Print(8 + 4 * (linelength - 11), 16, "Press y / n");
}

// Baker: Transfusion
//=============================================================================
/* CREDITS MENU */

void M_Menu_Credits_f(cmd_state_t *cmd)
{
	key_dest = key_menu;
	menu_state_set_nova (m_credits);
	m_entersound = true;
}



static void M_Credits_Draw (void)
{
	M_Background(640, 480, q_darken_true);
	M_DrawPic (0, 0, "gfx/creditsmiddle", NO_HOTSPOTS_0, NA0, NA0);
	M_Print (640/2 - 14/2*8, 236, "Coming soon...");
	M_DrawPic (0, 0, "gfx/creditstop", NO_HOTSPOTS_0, NA0, NA0);
	M_DrawPic (0, 433, "gfx/creditsbottom", NO_HOTSPOTS_0, NA0, NA0);
}


static void M_Credits_Key(cmd_state_t *cmd, int key, int ascii)
{
		M_Menu_Main_f(cmd);
}



//=============================================================================
/* Menu Subsystem */

static void M_KeyEvent(int key, int ascii, qbool downevent);
static void M_Draw(void);
void M_ToggleMenu(int mode);
static void M_Shutdown(void);

WARP_X_ (M_Draw)

void M_KeyEvent (int key, int ascii, qbool downevent)
{
	cmd_state_t *cmd = cmd_local;
	if (!downevent)
		return;

	switch (m_state){ 
	case m_none: return;
	case m_main: M_Main_Key(cmd, key, ascii); return;
	case m_demo: M_Demo_Key(cmd, key, ascii); return;
	case m_singleplayer: M_SinglePlayer_Key(cmd, key, ascii); return;
	case m_load: M_Load_Key(cmd, key, ascii); return;
	case m_save: M_Save_Key(cmd, key, ascii); return;
	case m_multiplayer: M_MultiPlayer_Key(cmd, key, ascii); return;
	case m_setup: M_Setup_Key(cmd, key, ascii); return;
	case m_options_nova:			M_Options_Nova_Key (cmd, key, ascii);				return;
	case m_options_classic:			M_Options_Classic_Key (cmd, key, ascii);					return;
	case m_options_effects: M_Options_Effects_Key(cmd, key, ascii); return;
	case m_options_graphics: M_Options_Graphics_Key(cmd, key, ascii); return;
	case m_options_colorcontrol: M_Options_ColorControl_Key(cmd, key, ascii); return;

	case m_keys: M_Keys_Key(cmd, key, ascii); return;
	case m_reset: M_Reset_Key(cmd, key, ascii); return;
	case m_video_classic:					M_Video_Classic_Key (cmd, key, ascii);					return;
	case m_video_nova:				M_Video_Nova_Key (cmd, key, ascii);				return;
	case m_help: M_Help_Key(cmd, key, ascii); return;
	case m_credits: M_Credits_Key(cmd, key, ascii); return;
	case m_quit: M_Quit_Key(cmd, key, ascii); return;
	case m_lanconfig: M_LanConfig_Key(cmd, key, ascii); return;
	case m_gameoptions: M_GameOptions_Key(cmd, key, ascii); return;
	case m_slist: M_ServerList_Key(cmd, key, ascii); return;
	case m_modlist:	M_ModList_Key(cmd, key, ascii); return;
	case m_maps:					M_Maps_Key (cmd, key, ascii);					return;
	} // sw

}

WARP_X_ (M_Options_Draw M_Options_Nova_Draw)

void M_Draw (void)
{
	if (key_dest != key_menu && 
			key_dest != key_menu_grabbed) {
		// Does this happen? During demo play at startup only.
		menu_state_set_nova (m_none); 
	}
	
	if (m_state == m_none)
		return;

	// Hover is 1 frame behind, drawn first
	if (hotspotx_count && hotspotx_hover != not_found_neg1) {
		crectf_s r = hotspotxs[hotspotx_hover].r;
		float xmag		= vid.width / vid_conwidth.value; // 1300 / 240
		float ymag		= vid.height / vid_conheight.value;

		float left		= r.left / xmag;
		float top		= r.top / ymag;
		float width		= r.width / xmag;
		float height	= r.height / ymag;

		if (isin4 (m_state, m_maps, m_modlist, m_keys, m_slist)) {
			DrawQ_Fill(left, top, width, height, /*bronzey*/ 0.5, 0.25, 0.1, 0.75, DRAWFLAG_NORMAL);
		} else {
			DrawQ_Fill(left, top, width, height, /*bronzey*/ 0.5, 0.25, 0.1, 0.37, DRAWFLAG_NORMAL);
		}
	} // if

	hotspotx_count = 0;

	switch (m_state) {
	case m_none: break;
	case m_main: M_Main_Draw (); break;
	case m_demo: M_Demo_Draw (); break;
	case m_singleplayer: M_SinglePlayer_Draw (); break;
	case m_load: M_Load_Draw (); break;
	case m_save: M_Save_Draw (); break;
	case m_multiplayer: M_MultiPlayer_Draw (); break;
	case m_setup: M_Setup_Draw (); break;
	case m_options_nova:			M_Options_Nova_Draw (); 		break;
	case m_options_classic:			M_Options_Classic_Draw ();		break;
	case m_options_effects: M_Options_Effects_Draw (); break;
	case m_options_graphics: M_Options_Graphics_Draw (); break;
	case m_options_colorcontrol: M_Options_ColorControl_Draw (); break;
	case m_keys: M_Keys_Draw (); break;
	case m_reset: M_Reset_Draw (); break;
	case m_video_classic:			M_Video_Classic_Draw ();				break;
	case m_video_nova:				M_Video_Nova_Draw ();			break;
	case m_help: M_Help_Draw (); break;
	case m_credits: M_Credits_Draw (); break;
	case m_quit: M_Quit_Draw (); break;
	case m_lanconfig: M_LanConfig_Draw (); break;
	case m_gameoptions: M_GameOptions_Draw (); break;
	case m_slist: M_ServerList_Draw (); break;
	case m_modlist: M_ModList_Draw (); break;
	case m_maps:					M_Maps_Draw ();					break;
	}

	if (m_entersound) {
		S_LocalSound ("sound/misc/menu2.wav");
		m_entersound = false;
	}

	hotspotx_hover = Hotspots_Hit ();
	S_ExtraUpdate ();
}

static void M_NewMap(void)
{
}

static int M_GetServerListEntryCategory(const serverlist_entry_t *entry)
{
	return 0;
}

#include "menu_main.c.h"
#include "menu_main_zirc.c.h"
	#include "menu_single_player.c.h"
		#include "menu_saveload.c.h"

	#include "menu_multiplayer.c.h"
		#include "menu_lan.c.h"
		#include "menu_setup.c.h"	// Player look?
		#include "menu_slist.c.h"

	#include "menu_options_nova.c.h"
		#include "menu_keys.c.h"
			#include "menu_video_classic.c.h"	
		#include "menu_video_nova.c.h"
		#include "menu_options_classic.c.h"
			
			#include "menu_color.c.h"
			#include "menu_effects.c.h"
			#include "menu_lighting.c.h"
			#include "menu_modlist.c.h"
			#include "menu_game.c.h"
	#include "menu_help.c.h"
		#include "menu_maps.c.h"
	#include "menu_quit.c.h"
#include "menu_qc.c.h"

qbool menu_is_csqc; // Is menu csqc in effect, if so .. defer to it.

// Baker r1402: Reset menu cursor on gamedir change
void Menu_Resets(void)
{
	m_main_cursor =
		options_cursor =
		options_cursorx =
		m_singleplayer_cursor =
		load_cursor =
		m_multiplayer_cursor =
		mapscursor					=
		options_colorcontrol_cursor =
		options_effects_cursor =
		options_graphics_cursor =
		keys_cursor =
		slist_cursor =
		video_cursor =
		video2_cursor = 0;
		setup_cursor = 0;
	lanConfig_cursor = -1;

}


// Sets menu state
void menu_state_set_nova(int ee)
{
	m_state = (enum m_state_e)ee;

	hotspotx_hover = not_found_neg1;
	hotspotx_count = 0;

	drawsel_idx = not_found_neg1;
}

void M_Shutdown(void)
{
	// reset key_dest
	key_dest = key_game;
}

static void M_Init(void)
{
	menuplyr_load = true;
	menuplyr_pixels = NULL;

	Cmd_AddCommand(CF_CLIENT, "menu_main", M_Menu_Main_f, "open the main menu");
	Cmd_AddCommand(CF_CLIENT, "menu_singleplayer", M_Menu_SinglePlayer_f, "open the singleplayer menu");
	Cmd_AddCommand(CF_CLIENT, "menu_load", M_Menu_Load_f, "open the loadgame menu");
	Cmd_AddCommand(CF_CLIENT, "menu_save", M_Menu_Save_f, "open the savegame menu");
	Cmd_AddCommand(CF_CLIENT, "menu_multiplayer", M_Menu_MultiPlayer_f, "open the multiplayer menu");
	Cmd_AddCommand(CF_CLIENT, "menu_setup", M_Menu_Setup_f, "open the player setup menu");
	Cmd_AddCommand (CF_CLIENT, "menu_options", M_Menu_Options_Nova_f, "open the options menu");
	Cmd_AddCommand(CF_CLIENT, "menu_options_effects", M_Menu_Options_Effects_f, "open the effects options menu");
	Cmd_AddCommand(CF_CLIENT, "menu_options_graphics", M_Menu_Options_Graphics_f, "open the graphics options menu");
	Cmd_AddCommand(CF_CLIENT, "menu_options_colorcontrol", M_Menu_Options_ColorControl_f, "open the color control menu");
	Cmd_AddCommand(CF_CLIENT, "menu_keys", M_Menu_Keys_f, "open the key binding menu");
	Cmd_AddCommand(CF_CLIENT, "menu_video", M_Menu_Video_Classic_f, "open the video options menu");
	Cmd_AddCommand (CF_CLIENT, "menu_video2", M_Menu_Video_Nova_f, "open the video options menu");
	Cmd_AddCommand(CF_CLIENT, "menu_reset", M_Menu_Reset_f, "open the reset to defaults menu");
	Cmd_AddCommand(CF_CLIENT, "menu_mods", M_Menu_ModList_f, "open the mods browser menu");
	Cmd_AddCommand (CF_CLIENT, "menu_maps", M_Menu_Maps_f, "open the maps browser menu");
	Cmd_AddCommand(CF_CLIENT, "help", M_Menu_Help_f, "open the help menu");
	Cmd_AddCommand(CF_CLIENT, "menu_quit", M_Menu_Quit_f, "open the quit menu");

	Cmd_AddCommand(CF_CLIENT, "menu_credits", M_Menu_Credits_f, "open the credits menu");
}
