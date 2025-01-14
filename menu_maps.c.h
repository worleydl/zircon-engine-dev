// menu_maps.c.h

#define		local_count					m_maplist_count
#define		local_cursor				m_maplist_cursor
#define 	visiblerows 				m_maplist_visiblerows
#define 	startrow 					m_maplist_startrow
#define 	endrow 						m_maplist_endrow
#define		local_click_time			m_maplist_click_time
#define		local_scroll_is_blocked		m_maplist_scroll_is_blocked

float	m_maplist_click_time;
int		m_maplist_scroll_is_blocked;
int		m_maplist_cursor;
int		m_maplist_visiblerows;
int		m_maplist_startrow;
int		m_maplist_endrow;

void M_Menu_Maps_f(cmd_state_t *cmd)
{
	KeyDest_Set (key_menu);
	menu_state_set_nova (m_maps);
	m_entersound = true;

	GetMapList("", NULL, 0, /*is_menu_fill*/ true, /*autocompl*/ false, /*suppress*/ false);

	startrow = not_found_neg1, endrow = not_found_neg1;

#if 0 // RE-ENTRANT SO DON'T SET CURSOR
	local_cursor = 0;
#endif

	if (local_cursor >= local_count)
		local_cursor = local_count - 1;
	if (local_cursor < 0)
		local_cursor = 0;

	menu_state_reenter = 0; local_scroll_is_blocked = false;
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
	if (local_scroll_is_blocked == false) {
		startrow = local_cursor - (visiblerows / 2);

		if (startrow > local_count - visiblerows)	
			startrow = local_count - visiblerows;
		if (startrow < 0)	
			startrow = 0; // visiblerows can exceed local_count
	}

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

			Hotspots_Add2	(menu_x + (8 * 9), menu_y + drawcur_y, (55 * 8) /*360*/, 8, 1, hotspottype_button, n);
			M_ItemPrint		((8 +  0) * 10, drawcur_y, (const char *)mx->s_name_trunc_16_a, true);
			M_ItemPrint		((8 + 18) * 10, drawcur_y, (const char *)mx->s_bsp_code, true);
			M_ItemPrint2	((8 + 21) * 10, drawcur_y, (const char *)mx->s_map_title_trunc_28_a, true);
			
			drawcur_y +=8;
		} // for

		// Print current map
		if (1) {
			const char *s = "(disconnected)";
			if (cls.state == ca_connected && cls.signon == SIGNONS_4) // MAPS MENU
				s = cl.worldbasename;
			int slen = (int)strlen(s);
			M_Print        (640 - (11   * 8) - 20, 40, "current map");
			M_PrintBronzey (640 - (slen * 8) - 20, 48, s);
		}
	} // endrow > startrow
	else
	{
		M_Print(80, drawcur_y, "No Maps found");
	}

	PPX_DrawSel_End ();
}

#pragma message ("Check out what happens if no servers founds or no maps found and end up pgup hit")
static void M_Maps_Key(cmd_state_t *cmd, int key, int ascii)
{
	int lcase_ascii;

	local_scroll_is_blocked = false;

	switch (key) {
	case K_MOUSE2: // fall thru to K_ESCAPE and then exit
	case K_ESCAPE:
		M_Menu_Main_f(cmd);
		break;

	case K_MOUSE1:
		if (hotspotx_hover == not_found_neg1) 
			break;

		local_cursor = hotspotx_hover + startrow; // fall thru
		{
			int new_cursor = hotspotx_hover + startrow;
			int is_new_cursor = new_cursor != local_cursor;

			local_scroll_is_blocked = true; // PROTECT AGAINST AUTOSCROLL

			local_cursor = new_cursor;

			if (is_new_cursor) {
				// GET OUT!  SET FOCUS TO ITEM
				// Commit_To_Cname ();
				break;
			}

			// Same cursor -- double click in effect.
			// fall thru
			double new_click_time = Sys_DirtyTime();
			double click_delta_time = local_click_time ? (new_click_time - local_click_time) : 0;
			local_click_time = new_click_time;

			if (is_new_cursor == false && click_delta_time && click_delta_time < DOUBLE_CLICK_0_5) {
				// Fall through and load the map
			} else {
				// Entry changed or not fast enough
				break;
			}
		} // block

	case K_ENTER:
		S_LocalSound ("sound/misc/menu2.wav");
		if (local_count) {
			maplist_s *mx = &m_maplist[local_cursor];

			va_super (tmp, 1024, "map %s // %s", mx->s_name_after_maps_folder_a, mx->s_map_title_trunc_28_a);
			
			Cbuf_AddTextLine (cmd, tmp);

			// Baker r0072: Add maps menu map to command history for recall.
			Key_History_Push_String (tmp);

			// Maps is re-entrant
			menu_state_reenter = 1;
			KeyDest_Set (key_game); // key_dest = key_game;
			menu_state_set_nova (m_none);
		}

		break;

	case K_SPACE:
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
		if (local_cursor < 0) // PGUP does not wrap, stops at start
			local_cursor = 0;
		break;

	case K_MWHEELUP:
		local_cursor -= visiblerows / 4;
		if (local_cursor < 0) // K_MWHEELUP does not wrap, stops at start
			local_cursor = 0;
		break;

	case K_PGDN:
		local_cursor += visiblerows / 2;
		if (local_cursor >= local_count) // PGDN does not wrap, stops at end
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
		if (local_cursor < 0) // K_UPARROW wraps around to end EXCEPT ON MEGA LISTS
			local_cursor = 0; // MEGA EXCEPTION
		break;

	case K_DOWNARROW:
		//S_LocalSound ("sound/misc/menu1.wav");
		local_cursor ++;
		if (local_cursor >= local_count) // K_DOWNARROW wraps around to start EXCEPT ON MEGA LISTS
			local_cursor = local_count - 1 ; // MEGA EXCEPTION
		break;

	case K_LEFTARROW:
		break;

	case K_RIGHTARROW:
		break;

	default:
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
#undef 	local_scroll_is_blocked