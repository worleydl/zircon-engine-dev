// menu.h

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

#ifndef MENU_H
#define MENU_H

#include "qtypes.h"
struct serverlist_entry_s;

enum m_state_e {
	m_none,
	m_main,
	m_demo,
	m_singleplayer,
	m_load,
	m_save,
	m_load2,
	m_save2,
	m_multiplayer,
	m_setup,
	m_options_nova,
	m_options_classic,
	m_video_classic,
	m_keys,
	m_help,
	m_credits,
	m_quit,
	m_lanconfig,
	m_gameoptions,
	m_slist,
	m_options_effects,
	m_options_graphics,
	m_options_colorcontrol,
	m_reset,
	m_modlist,
	m_video_nova,
	m_maps,
	m_zdev,
};

extern enum m_state_e m_state;
extern char m_return_reason[128];
void M_Update_Return_Reason(const char *s);

void Menu_Resets(void); // Baker r1402: Reset menu cursor on gamedir change

/*
// hard-coded menus
//
void M_Init (void);
void M_KeyEvent (int key);
void M_Draw (void);
void M_ToggleMenu (int mode);

//
// menu prog menu
//
void MP_Init (void);
void MP_KeyEvent (int key);
void MP_Draw (void);
void MP_ToggleMenu (int mode);
void MP_Shutdown (void);*/

qbool MP_ConsoleCommand(const char *text);

//
// menu router
//

void MR_Init_Commands (void);
void MR_Init (void);
void MR_Restart (void);
extern void (*MR_KeyEvent) (int key, int ascii, qbool downevent);
extern void (*MR_Draw) (void);
extern void (*MR_ToggleMenu) (int mode);
extern void (*MR_Shutdown) (void);
extern void (*MR_NewMap) (void);
extern int (*MR_GetServerListEntryCategory) (const struct serverlist_entry_s *entry);

typedef struct video_resolution_s
{
	const char *type;
	int width, height;
	int conwidth, conheight;
	double pixelheight; ///< pixel aspect
}
video_resolution_t;
extern video_resolution_t *video_resolutions;
extern int video_resolutions_count;
extern video_resolution_t video_resolutions_hardcoded[];
extern int video_resolutions_hardcoded_count;

// netconn.c @ "Connection accepted" sets menu state
void menu_state_set_nova(int ee); // Baker: allows nostartdemos to exit menu?

#ifdef CONFIG_MENU
extern qbool menu_is_csqc;
#endif

int SList_Tiebreaker_Bias (const char *s);

#ifdef CONFIG_MENU
extern int m_load2_oldload_cursor; // Need to reset dynamic texture in menu.  DYNAMICTEX_Q3_END
extern qbool m_load2_scroll_is_blocked;
#endif

#define DOUBLE_CLICK_0_5 0.5 // Windows double-click time

#endif // ! MENU_H

