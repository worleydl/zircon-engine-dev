// menu_maps.c.h

#define		local_count		m_maplist_count
#define		local_cursor	mapscursor
#define 	visiblerows 	m_mlist_visiblerows
#define 	startrow 		m_mlist_startrow
#define 	endrow 			m_mlist_endrow


int mapscursor;
int m_mlist_visiblerows;
int m_mlist_startrow;
int m_mlist_endrow;



void M_Menu_Maps_f(cmd_state_t *cmd)
{
	key_dest = key_menu;
	menu_state_set_nova (m_maps);
	m_entersound = true;

	GetMapList("", NULL, 0, /*is_menu_fill*/ true, /*autocompl*/ false, /*suppress*/ false);

	startrow = not_found_neg1, endrow = not_found_neg1;

	// Baker: We want the map menu very re-entrant
	// and do not wish the cursor to get messed with
	// someone might have picked the wrong map
	// press ESC, we want the current map right there.
	#pragma message ("Baker: What if someone deletes maps and then reenters?  Or a map downloads and then the index is no longer the same?")
	//local_cursor = 0;
	if (local_cursor >= local_count)
		local_cursor = local_count - 1;
	if (local_cursor < 0)
		local_cursor = 0;
}

static void M_Maps_Draw (void)
{
	int n;
	cachepic_t *p0;
	const char *s_available = "Maps";

	drawidx = 0; drawsel_idx = not_found_neg1;  // PPX_Start - does not have frame cursor

	M_Background(640, vid_conheight.integer, q_darken_true);

	M_Print (48 + 32, 32, s_available);

	// scroll the list as the cursor moves

	drawcur_y = 8 * 6;
	visiblerows = (int)((menu_height - (2 * 8) - drawcur_y) / 8) - 8;
	
	// Baker: Do it this way because a short list may have more visible rows than the list count
	// so using bound doesn't work. 
	startrow = local_cursor - (visiblerows / 2);
	if (startrow > local_count - visiblerows)	
		startrow = local_count - visiblerows;
	if (startrow < 0)	
		startrow = 0; // visiblerows can exceed local_count
	
	endrow = Smallest(startrow + visiblerows, local_count);

	p0 = Draw_CachePic ("gfx/p_option");
	M_DrawPic((640 - Draw_GetPicWidth(p0)) / 2, 4, "gfx/p_option", NO_HOTSPOTS_0, NA0, NA0);

	if (endrow > startrow) {
		for (n = startrow; n < endrow; n++) {
			if (!in_range_beyond (0, n, local_count))
				continue;

			if (n == local_cursor) 
				drawsel_idx = (n - startrow) /*relative*/;

			maplist_s *mx = &m_maplist[n];

			Hotspots_Add2 (menu_x + (8 * 9), menu_y + drawcur_y, (55 * 8) /*360*/, 8, 1, hotspottype_button, n);
			M_ItemPrint ((8 +  0) * 10, drawcur_y, (const char *)mx->s_name_trunc_16_a, true);
			M_ItemPrint ((8 + 18) * 10, drawcur_y, (const char *)mx->s_bsp_code, true);
			M_ItemPrint2 ((8 + 21) * 10, drawcur_y, (const char *)mx->s_map_title_trunc_28_a, true);
			
			drawcur_y +=8;
		} // for

		// Print current map
		if (1) {
			const char *s = "";
			if (cls.state == ca_connected && cls.signon == SIGNONS_4) // MAPS MENU
				s = cl.worldbasename;
			int slen = (int)strlen(s);
			M_Print        (640 - (11   * 8) - 12, 40, "current map");
			M_PrintBronzey (640 - (slen * 8) - 12, 48, s);
		}
	} // endrow > startrow
	else
	{
		M_Print(80, drawcur_y, "No Maps found");
	}

	PPX_DrawSel_End ();
}

#pragma message ("Check out what happens if no servers founds or no maps found and end up pgup hit")
static void M_Maps_Key(cmd_state_t *cmd, int k, int ascii)
{
	switch (k) {
	case K_MOUSE2: // fall thru
	case K_ESCAPE:
		M_Menu_Main_f(cmd);
		break;

	case K_MOUSE1:
		if (hotspotx_hover == not_found_neg1) 
			break;

		local_cursor = hotspotx_hover + startrow; // fall thru

	case K_ENTER:
		S_LocalSound ("sound/misc/menu2.wav");
		if (local_count) {
			char vabuf[1024];
			maplist_s *mx = &m_maplist[local_cursor];
			va (vabuf, sizeof(vabuf), "map %s", mx->s_name_after_maps_folder_a);
			menu_state_reenter = 1;
			Cbuf_AddTextLine (cmd, vabuf );

			// Baker r0072: Add maps menu map to command history for recall.
			Key_History_Push_String (vabuf);

			key_dest = key_game;
			menu_state_set_nova (m_none);
		}

		break;

	case K_SPACE:
		//ModList_RebuildList();
		break;

	case K_HOME:
		if (local_count)
			local_cursor = 0;
		break;

	case K_END:
		if (local_count)
			local_cursor = local_count - 1;
		break;

	case K_PGUP:
		local_cursor -= visiblerows / 2;
		if (local_cursor < 0) 
			local_cursor = 0;
		break;

	case K_MWHEELUP:
		local_cursor -= visiblerows / 4;
		if (local_cursor < 0) 
			local_cursor = 0;
		break;

	case K_PGDN:
		local_cursor += visiblerows / 2;
		if (local_cursor >= local_count) 
			local_cursor = local_count - 1;
		break;

	case K_MWHEELDOWN:
		local_cursor += visiblerows / 4;
		if (local_cursor >= local_count) 
			local_cursor = local_count - 1;
		break;

	case K_UPARROW:
		//S_LocalSound ("sound/misc/menu1.wav");
		local_cursor--;
		if (local_cursor < 0)
			local_cursor = 0;
		break;

	case K_DOWNARROW:
		//S_LocalSound ("sound/misc/menu1.wav");
		local_cursor ++;
		if (local_cursor >= local_count)
			local_cursor = local_count - 1 ;
		break;

	case K_LEFTARROW:
		break;

	case K_RIGHTARROW:
		break;

	default:
		int lcase_ascii;
		lcase_ascii = tolower(ascii);
		if (in_range ('a', lcase_ascii, 'z')) {
			// Baker: This is a wraparound seek
			// Find the next item starting with 'a'
			// or whatever key someone pressed
			int startx = local_cursor;
			char sprefix[2] = { (char)lcase_ascii, 0 };

			for (int iters = 0; iters < local_count; iters ++) {
				startx ++;
				if (startx >= local_count) {
					startx = 0;
				}

				maplist_s *mx = &m_maplist[startx];

				if (String_Does_Start_With_Caseless ((char *)mx->s_name_after_maps_folder_a, sprefix)) {
					local_cursor = startx;
					break;
				} // if
			} // iters


		} // if a-z
		break;
	} // sw

}

#undef	local_count	
#undef	local_cursor
#undef 	visiblerows
#undef 	startrow
#undef 	endrow
