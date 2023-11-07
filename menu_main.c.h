// menu_main.c.h

#define 	local_count		MAIN_ITEMS
#define		local_cursor	m_main_cursor

//=============================================================================
/* MAIN MENU */

static int	m_main_cursor;

static int MAIN_ITEMS = 4; // Nehahra: Menu Disable


void M_Menu_Main_f(cmd_state_t *cmd)
{
	const char *s;
	s = "gfx/mainmenu";

	if (gamemode == GAME_NEHAHRA) {
		if (FS_FileExists("maps/neh1m4.bsp")) {
			if (FS_FileExists("hearing.dem")) {
				Con_DPrint("Main menu: Nehahra movie and game detected.\n");
				NehGameType = TYPE_BOTH;
			} else {
				Con_DPrint("Nehahra game detected.\n");
				NehGameType = TYPE_GAME;
			}
		} else {
			if (FS_FileExists("hearing.dem")) {
				Con_DPrint("Nehahra movie detected.\n");
				NehGameType = TYPE_DEMO;
			} else {
				Con_DPrint("Nehahra not found.\n");
				NehGameType = TYPE_GAME; // could just complain, but...
			}
		}
		if (NehGameType == TYPE_DEMO)		local_count = 4;
		else if (NehGameType == TYPE_GAME)	local_count = 5;
		else								local_count = 6;
	}
	else local_count = 5;

	// check if the game data is missing and use a different main menu if so
	m_missingdata = !forceqmenu.integer && !Draw_IsPicLoaded(Draw_CachePic_Flags(s, CACHEPICFLAG_FAILONMISSING_256));
	if (m_missingdata)
		local_count = 2;

	key_dest = key_menu;
	menu_state_set_nova (m_main);
	m_entersound = true;
}


static void M_Main_Draw (void)
{
	int		f;
	cachepic_t	*p0;
	char vabuf[1024];

	if (m_missingdata)
	{
		float y;
		const char *s;
		M_Background(640, 480, q_darken_true); //fall back is always to 640x480, this makes it most readable at that.
		y = 480/3-16;
		s = "You have reached this menu due to missing or unlocatable content/data";M_PrintRed ((640-strlen(s)*8)*0.5, (480/3)-16, s);y+=8;
		y+=8;
		s = "You may consider adding";M_Print ((640-strlen(s)*8)*0.5, y, s);y+=8;
		s = "-basedir /path/to/game";M_Print ((640-strlen(s)*8)*0.5, y, s);y+=8;
		s = "to your launch commandline";M_Print ((640-strlen(s)*8)*0.5, y, s);y+=8;
		M_Print (640/2 - 48, 480/2, "Open Console"); //The console usually better shows errors (failures)
		M_Print (640/2 - 48, 480/2 + 8, "Quit");
		M_DrawCharacter(640/2 - 56, 480/2 + (8 * local_cursor), 12+((int)(host.realtime*4)&1));
		return;
	}


	M_Background(320, 200, q_darken_true);
	M_DrawPic (16, 4, "gfx/qplaque", NO_HOTSPOTS_0, NA0, NA0);
	p0 = Draw_CachePic ("gfx/ttl_main");
	M_DrawPic ( (320-Draw_GetPicWidth(p0))/2, 4, "gfx/ttl_main", NO_HOTSPOTS_0, NA0, NA0);
// Nehahra
	if (gamemode == GAME_NEHAHRA) {
		if (NehGameType == TYPE_BOTH) M_DrawPic (72, 32, "gfx/mainmenu", local_count, USE_IMAGE_SIZE_NEG1, /*Q_FAT_ROW_SIZE_*/ 20);
		else if (NehGameType == TYPE_GAME) M_DrawPic (72, 32, "gfx/gamemenu", local_count, USE_IMAGE_SIZE_NEG1, /*Q_FAT_ROW_SIZE_*/ 20);
		else M_DrawPic (72, 32, "gfx/demomenu", local_count, USE_IMAGE_SIZE_NEG1, /*Q_FAT_ROW_SIZE_*/ 20);
	}
	else {
		M_DrawPic (72, 32, "gfx/mainmenu", 5, USE_IMAGE_SIZE_NEG1, /*Q_FAT_ROW_SIZE_*/ 20);
	}

	f = (int)(host.realtime * 10)%6;

	M_DrawPic (54, 32 + local_cursor * 20, va(vabuf, sizeof(vabuf), "gfx/menudot%d", f+1), NO_HOTSPOTS_0, NA0, NA0);
}


static void M_Main_Key(cmd_state_t *cmd, int key, int ascii)
{
	switch (key) {
	case K_ESCAPE: case K_MOUSE2:
		key_dest = key_game;
		menu_state_set_nova (m_none);
		break;
	case K_MOUSE1: if (hotspotx_hover == not_found_neg1) break; else local_cursor = hotspotx_hover; // fall thru

	case K_ENTER:
		m_entersound = true;

		if (m_missingdata) {
			switch (local_cursor) {
			case 0: if (cls.state == ca_connected) {
						menu_state_set_nova (m_none);
					key_dest = key_game;
				} // if
				Con_ToggleConsole (); // Baker: The idea here is if we are not in a game to open the console.
				break;

			case 1: M_Menu_Quit_f(cmd);
				break;
			} // sw
		} // m_missingdata
		else if (gamemode == GAME_NEHAHRA) {
			#include "menu_main_neh.c.h"
		} // GAME_NEHAHRA
		else {
			switch (local_cursor) {
			case 0: M_Menu_SinglePlayer_f(cmd); break;
			case 1: M_Menu_MultiPlayer_f(cmd); break;
			case 2: M_Menu_Options_Nova_f(cmd); break;
			case 3:
				if (1) M_Menu_Maps_f (cmd);
				else M_Menu_Help_f (cmd);
				break;
			case 4: M_Menu_Quit_f(cmd); break;
			} // sw
		} // if
		break;
		
	case K_HOME: 
		local_cursor = 0;
		break;

	case K_END:
		local_cursor = local_count - 1;
		break;

	case K_UPARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		local_cursor --;
		if (local_cursor < 0)
			local_cursor = local_count - 1;
		break;
		
	case K_DOWNARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		local_cursor ++;
		if (local_cursor >= local_count)
			local_cursor = 0;
		break;

	} // sw
}

#undef local_count
#undef local_cursor
