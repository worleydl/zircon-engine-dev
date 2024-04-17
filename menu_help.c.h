// menu_help.c.h



//=============================================================================
/* HELP MENU */

static int		help_page;
#define	NUM_HELP_PAGES	6


void M_Menu_Help_f(cmd_state_t *cmd)
{
	KeyDest_Set (key_menu); // key_dest = key_menu;
	menu_state_set_nova (m_help);
	m_entersound = true;
	help_page = 0;
}



static void M_Help_Draw (void)
{
	char vabuf[1024];
	M_Background(320, 200, q_darken_true);
	M_DrawPic (0, 0, va(vabuf, sizeof(vabuf), "gfx/help%d", help_page), NO_HOTSPOTS_0, NA0, NA0);
}


static void M_Help_Key(cmd_state_t *cmd, int key, int ascii)
{
	switch (key)
	{
	case K_ESCAPE:	case K_MOUSE2:
		M_Menu_Main_f(cmd);
		break;

	case K_UPARROW:
	case K_RIGHTARROW:
		m_entersound = true;
		if (++help_page >= NUM_HELP_PAGES)
			help_page = 0;
		break;

	case K_DOWNARROW:
	case K_LEFTARROW:
		m_entersound = true;
		if (--help_page < 0)
			help_page = NUM_HELP_PAGES-1;
		break;
	}

}
