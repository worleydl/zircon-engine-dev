// menu_single_player.c.h

#define 	local_count		SINGLEPLAYER_ITEMS
#define		local_cursor	m_singleplayer_cursor

//=============================================================================
/* SINGLE PLAYER MENU */

static int	m_singleplayer_cursor;
#define	SINGLEPLAYER_ITEMS	3


void M_Menu_SinglePlayer_f(cmd_state_t *cmd)
{
	key_dest = key_menu;
	menu_state_set_nova (m_singleplayer);
	m_entersound = true;
}


static void M_SinglePlayer_Draw (void)
{
	cachepic_t	*p;
	char vabuf[1024];

	M_Background(320, 200, q_darken_true);

	M_DrawPic (16, 4, "gfx/qplaque", NO_HOTSPOTS_0, NA0, NA0);
	p = Draw_CachePic ("gfx/ttl_sgl");

	int		f = (int)(host.realtime * 10)%6;

	M_DrawPic ( (320-Draw_GetPicWidth(p))/2, 4, "gfx/ttl_sgl", NO_HOTSPOTS_0, NA0, NA0);
	M_DrawPic (72, 32, "gfx/sp_menu", 3, USE_IMAGE_SIZE_NEG1, USE_IMAGE_SIZE_NEG1);

	M_DrawPic (54, 32 + local_cursor * 20, va(vabuf, sizeof(vabuf), "gfx/menudot%d", f+1), NO_HOTSPOTS_0, NA0, NA0);
}


static void M_SinglePlayer_Key (cmd_state_t *cmd, int key, int ascii)
{
	switch (key) {
	case K_MOUSE2: // fall
	case K_ESCAPE:
		M_Menu_Main_f(cmd);
		break;

	case K_MOUSE1: if (hotspotx_hover == not_found_neg1) break; else local_cursor = hotspotx_hover; // fall thru
	case K_ENTER:
		m_entersound = true;

		switch (local_cursor) {
		case 0:
			key_dest = key_game;
			if (sv.active)
				Cbuf_AddTextLine (cmd, "disconnect");
			Cbuf_AddTextLine (cmd, "maxplayers 1");
			Cbuf_AddTextLine (cmd, "deathmatch 0");
			Cbuf_AddTextLine (cmd, "coop 0");
			Cbuf_AddTextLine (cmd, "startmap_sp");
			break;

		case 1:
			M_Menu_Load_f(cmd);
			break;

		case 2:
//			if (!sv.active || (cl.islocalgame && cl.intermission) ) {
//				M_Menu_NoSave_f (cmd);	
//			} else {
				M_Menu_Save_f(cmd);
//			}
			break;
		} // sw


	case K_HOME: 
		local_cursor = 0;
		break;

	case K_END:
		local_cursor = local_count - 1;
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
	} // sw
}

#undef local_count
#undef local_cursor