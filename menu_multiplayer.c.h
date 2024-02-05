// menu_multiplayer.c.h

#define		local_count		MULTIPLAYER_ITEMS
#define		local_cursor	m_multiplayer_cursor


//=============================================================================
/* MULTIPLAYER MENU */

static int	m_multiplayer_cursor;
#define	MULTIPLAYER_ITEMS	3


char my_ipv4_address[256];
char my_ipv6_address[256];

void M_Menu_MultiPlayer_f(cmd_state_t *cmd)
{
	key_dest = key_menu;
	menu_state_set_nova (m_multiplayer);
	m_entersound = true;

	UDP4_GetHostNameIP (NULL, 0, my_ipv4_address, sizeof(my_ipv4_address));
	UDP6_GetHostNameIP (NULL, 0, my_ipv6_address, sizeof(my_ipv6_address));

}


static void M_MultiPlayer_Draw (void)
{
	int		f;
	cachepic_t	*p0;
	char vabuf[1024];

	M_Background(320, 200, q_darken_true);

	drawidx = 0; drawsel_idx = not_found_neg1;  // PPX_Start - does not have frame cursor
	M_DrawPic (16, 4, "gfx/qplaque", NO_HOTSPOTS_0, NA0, NA0);
	p0 = Draw_CachePic ("gfx/p_multi");
	M_DrawPic ( (320-Draw_GetPicWidth(p0))/2, 4, "gfx/p_multi", NO_HOTSPOTS_0, NA0, NA0);
	M_DrawPic (72, 32, "gfx/mp_menu", 3, USE_IMAGE_SIZE_NEG1, USE_IMAGE_SIZE_NEG1);

	f = (int)(host.realtime * 10)%6;

	M_DrawPic (54, 32 + local_cursor * 20, va(vabuf, sizeof(vabuf), "gfx/menudot%d", f + 1), NO_HOTSPOTS_0, NA0, NA0);
	PPX_DrawSel_End ();
}


static void M_MultiPlayer_Key(cmd_state_t *cmd, int key, int ascii)
{
	switch (key) {
	case K_MOUSE2: // Fall ..
	case K_ESCAPE:
		M_Menu_Main_f(cmd);
		break;

	case K_MOUSE1: 
		if (hotspotx_hover == not_found_neg1) 
			break; 
		
		local_cursor = hotspotx_hover; 
		// fall thru

	case K_ENTER:
		m_entersound = true;
		switch (local_cursor) {
		case 0:
		case 1:
#if 1
			{
				static int		lanConfig_cursor;
				lanConfig_cursor = 1; // M_Menu_LanConfig_f
				Cbuf_AddTextLine (cmd, "stopdemo");
				M_Menu_ServerList_f(cmd);
			}
#else
			M_Menu_LanConfig_f(cmd);
#endif
			break;

		case 2:
			M_Menu_Setup_f(cmd);
			break;
		} // sw
		break;

	case K_UPARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		local_cursor --;
		if (local_cursor < 0) // K_UPARROW wraps around to end
			local_cursor = local_count - 1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		local_cursor ++;
		if (local_cursor >= local_count) // K_DOWNARROW wraps around to start
			local_cursor = 0;
		break;

	case K_HOME:
		local_cursor = 0;
		break;

	case K_END:
		local_cursor = local_count - 1;
		break;


	} // sw
}

#undef local_count
#undef local_cursor
