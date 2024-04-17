// menu_zdev.c.h

#undef local_sort_is_ascending // WHY?

// Edicts
// Textures
// Shader text
// 

// Save Map Progress Age
#define ZDEV_SORTBY_SAVENAME_0		0
#define ZDEV_SORTBY_MAP_1			1
#define ZDEV_SORTBY_PROGRESS_2		2
#define ZDEV_SORTBY_AGE_3			3

#define		local_count					m_zdev2_count // g_saves_filename_noExt.numstrings
#define		local_cursor				m_zdev2_load2cursor
#define 	visiblerows 				m_zdev2_visiblerows
#define 	startrow 					m_zdev2_startrow
#define 	endrow 						m_zdev2_endrow
#define		local_click_time			m_zdev2_click_time
#define		local_scroll_is_blocked		m_zdev2_scroll_is_blocked
#define		local_sort_by				m_zdev2_sort_by
#define		local_sort_is_ascending		m_zdev2_is_sort_ascending
#define		local_sort_is_dirty			m_zdev2_sort_is_dirty

int		m_zdev2_count;
int		m_zdev2_sort_by;
int		m_zdev2_is_sort_ascending;
int		m_zdev2_sort_is_dirty;
int		m_zdev2_oldload_cursor;
int		m_zdev2_load2cursor;
int		m_zdev2_visiblerows;
int		m_zdev2_startrow;
int		m_zdev2_endrow;
int		m_zdev2_sort_map;
float	m_zdev2_click_time;
qbool	m_zdev2_scroll_is_blocked;

// If we were to sort these, we need to sort the indexes.
// By a proxy.

//stringlist_t	g_saves_filename_noExt;
//stringlist_t	g_saves_filename_noExt_crop;
//stringlist_t	g_saves_filename_timestr;
//stringlist_t	g_saves_secondsold;
//stringlist_t	g_saves_mapnames;
//stringlist_t	g_saves_mapnames_crop;
//stringlist_t	g_saves_jpeg_strings;
//stringlist_t	g_saves_comment;
//stringlist_t	g_saves_indexes_4byte;

int zdev_list_sort (const void *pa, const void *pb)
{
	int *sa = (int *)(*(const char **)pa);
	int *sb = (int *)(*(const char **)pb);
	int idx0 = *sa;
	int idx1 = *sb;
	const char *da = NULL;
	const char *db = NULL;
	int negator = m_zdev2_is_sort_ascending == false ? -1 : 1;

	//switch (m_zdev2_sort_by) {
	//default:
	//	break;

	//case SAVELOAD_SORTBY_SAVENAME_0:
	//	da = g_saves_filename_noExt.strings[idx0];
	//	db = g_saves_filename_noExt.strings[idx1];
	//	break;

	//case SAVELOAD_SORTBY_MAP_1:
	//	da = g_saves_mapnames.strings[idx0];
	//	db = g_saves_mapnames.strings[idx1];
	//	break;

	//case SAVELOAD_SORTBY_PROGRESS_2:
	//	da = g_saves_comment.strings[idx0];
	//	db = g_saves_comment.strings[idx1];
	//	break;

	//case SAVELOAD_SORTBY_AGE_3:
	//	da = g_saves_secondsold.strings[idx0];
	//	db = g_saves_secondsold.strings[idx1];
	//	break;
	//} // sw	

	int diff = strcasecmp(da, db);
	//Con_PrintLinef ("Comparing %d " QUOTED_S " vs. %d " QUOTED_S, idx0, da, idx1, db);
	if (diff == 0) {
		// Baker: Deterministic sorting. We cannot allow ties ever.
		// Reason: We want a predictable order every time otherwise the results could jumps around
		// with tie entries randomly filling out the list differently every sort.
		diff = (idx0 - idx1);
	}

	return negator * diff;
}
static int Add_Entry (void *v)
{
//	stringlistappend (&g_saves_mapnames_crop, va32("%-12.12s", mapname) );

//	return is_ok;
	return false;
}

WARP_X_CALLERS_ (M_Menu_Load2_f)
void ZDev_Refresh (void)
{

	//stringlistfreecontents (&g_saves_filename_noExt);
	//stringlistfreecontents (&g_saves_filename_noExt_crop);
	//stringlistfreecontents (&g_saves_filename_timestr);
	//stringlistfreecontents (&g_saves_secondsold);
	//stringlistfreecontents (&g_saves_mapnames);
	//stringlistfreecontents (&g_saves_mapnames_crop);
	//stringlistfreecontents (&g_saves_jpeg_strings);
	//stringlistfreecontents (&g_saves_comment);
	//stringlistfreecontents (&g_saves_indexes_4byte);
	//stringlistfreecontents (&g_saves_secondsold);

	//fssearch_t	*t = FS_Search ("*.sav", fs_caseless_true, fs_quiet_true, fs_pakfile_null, fs_gamedironly_true);

	//if (!t)
	//	return;

	//for (int idx = 0; idx < t->numfilenames; idx ++) {
	//	char *sxy = t->filenames[idx];
	//	// Con_PrintLinef ("%d: %s", idx, sxy);
	//	Add_SaveFile (sxy); // Get_SavesListGameDirOnly
	//} // for

	//FS_FreeSearch_Null_ (t);
}


dp_font_t *zdev_dpfont; // fonts_mempool .. font shutdown
void M_Menu_ZDev_f (cmd_state_t *cmd)
{
	KeyDest_Set (key_menu); // key_dest = key_menu;
	menu_state_set_nova (m_zdev);
	m_entersound = true;

	//Get_SavesListGameDirOnly (); // Refresh list

	local_sort_is_ascending = true; // Low high, this means alphabetical?
	m_zdev2_sort_by = 0;// SAVELOAD_SORTBY_SAVENAME_0;
	m_zdev2_sort_is_dirty = true; // FORCE REFRESH

	if (!zdev_dpfont) {
		extern mempool_t *fonts_mempool;
		#pragma message ("Do the fonts need to upload a texture or something?")
		zdev_dpfont = (dp_font_t *)Mem_Alloc(fonts_mempool, sizeof(dp_font_t) * ONE_CHAR_1);

		//fonts/Roboto-Medium.ttf
		//qbool Font_LoadFont(const char *name, dp_font_t *dpfnt, const byte *data_in);
		//qbool is_ok = Font_LoadFont("fonts/NotoSerif-Regular.ttf", zdev_dpfont, DATA_NULL);
		/*qbool is_ok = */
			LoadFontDP(/*override*/ true, "fonts/Roboto-Medium.ttf", zdev_dpfont, 24, /*v_offset*/ 0);
		
		int j = 5;
	}

#if 0
	if (m_zdev2_sort_is_dirty) {
		stringlistsort_custom	(&g_saves_indexes_4byte, fs_make_unique_false, saveload_sort);
		m_zdev2_sort_is_dirty = false;
	}
#endif

	startrow = not_found_neg1, endrow = not_found_neg1;

	if (local_cursor >= local_count)
		local_cursor = local_count - 1;
	if (local_cursor < 0)
		local_cursor = 0;

	menu_state_reenter = 0; m_zdev2_oldload_cursor = -1; m_zdev2_scroll_is_blocked = false;
}

static void M_ZDev_Draw (void)
{
#if 0 // DIRTY UPDATE
	if (m_zdev2_sort_is_dirty) {
		WARP_X_ (saveload_sort)
		WARP_X_ (m_zdev2_is_sort_ascending)
		WARP_X_ (m_zdev2_sort_by)
		stringlistsort_custom	(&g_saves_indexes_4byte, fs_make_unique_false, saveload_sort);
		m_zdev2_sort_is_dirty = false;
	}
#endif
	drawidx = 0; drawsel_idx = not_found_neg1;  // PPX_Start - does not have frame cursor

	M_Background (640, vid_conheight.integer, q_darken_true);

	// scroll the list as the cursor moves
	drawcur_y = 7 * 8; // 56
	
	int usable_height = menu_height /*640*/ - drawcur_y /*row_0_at_48*/;
	visiblerows = (usable_height / 8) - 3; // Baker: Room for bottom

	// Baker: Do it this way because a short list may have more visible rows than the list count
	// so using bound doesn't work. 
	if (local_scroll_is_blocked == false) {
		startrow = local_cursor - (visiblerows / 2);

		if (startrow > local_count - visiblerows)	
			startrow = local_count - visiblerows;
		if (startrow < 0)	
			startrow = 0; // visiblerows can exceed local_count
	}
	
	endrow = Smallest (startrow + visiblerows, local_count);

	//cachepic_t *p0 = Draw_CachePic ("gfx/p_load"); // size of this is 104 x 24
#if 1
	vec3_t pos = { 32, 32 };
	vec3_t scale = { 1, 1 };
	vec3_t rgb = { 1,1,1};
	float scale_x = scale_1_0, scale_y = scale_1_0 ; // Hmmm.
	//WARP_X_ (LoadFont VM_drawstring scale_x getdrawfontscale /*defaults 1*/ )
		//ft2_font_t *font vs.  dp_font_t -> ft2

	// getdrawfontscale(prog, &sx, &sy);
	// dp_font_t *zdev_dpfont=Font_LoadFont; // ?

	// We do have 2D conscale in effect here!
	if (zdev_dpfont) {
		WARP_X_ ()
		DrawQ_String_Scale (pos[0], pos[1], "Developer Tools", 0, scale[0], scale[1], scale_x, 
			scale_y, rgb[0], rgb[1], rgb[2], alpha_1_0, 
			DRAWFLAG_NORMAL_0, /*outcolorptr*/ NULL, /*ignorecolorcodes?*/ true, zdev_dpfont
		);
	}
#else
	M_Print16 (32, 32, "Developer Tools");
#endif
	WARP_X_ (VM_drawstring)

	// drawcur_y = 8 * 6; is set above
	// row_0_at_48
	if (endrow > startrow) {
		for (int n = startrow; n < endrow; n++) {
			// local cursor index translated to sort index
			// int pidx = *((int *)&g_saves_indexes_4byte.strings[/*the index here*/ n][0]);

			if (false == in_range_beyond (0, n, local_count))
				continue;

#if 0
			WARP_X_ (M_Dump_f)
			

			if (n == local_cursor) 
				drawsel_idx = (n - startrow) /*relative*/;

			// This adds a hotspot to the menu item, it draws nothing ...
			Hotspots_Add2	(menu_x + column_1 - 8/*(9 * 8)*/, menu_y + drawcur_y, row_width_red_376, 8, 1, hotspottype_button, n);
			
			// 12 12 
			WARP_X_ (M_Dump_f)
			M_ItemPrint		(column_1, drawcur_y, g_saves_filename_noExt_crop.strings[pidx], q_unghosted_true);
			M_ItemPrint		(column_2, drawcur_y, g_saves_mapnames_crop.strings[pidx], q_unghosted_true);
			M_ItemPrint		(column_3, drawcur_y, g_saves_comment.strings[pidx], q_unghosted_true);
			M_ItemPrint		(column_4, drawcur_y, g_saves_filename_timestr.strings[pidx], q_unghosted_true);
#endif			
			drawcur_y += 8;
		} // for
	} // endrow > startrow
	else
	{
		M_Print(80, drawcur_y, "Empty List");
	}

#if 0 // COLUMN HEADERS

	// Determine underline color based on ascending
	float underline_color3[3] = {0.32, 0.32, 0.32}; // White
	if (local_sort_is_ascending) {
		underline_color3[1] = underline_color3[2] = 0; // RED
	}

	// DRAW COLUMN HEADERS
drawcolumnheaders:

	drawcur_y = title_row_0;
	int headidx		= 0;  // Hotspot idx, we are going -1 and lower
	//int draw_cur_h	= 8;
	
	#define DRAW_UNDERLINE_COLUMN \
		DrawQ_Fill(menu_x + drawcur_x, menu_y + drawcur_y + 9, draw_cur_w, 1 /*draw_cur_h*/, VECTOR3_SEND(underline_color3),  \
			q_alpha_1, DRAWFLAG_NORMAL_0) // Ender

	#define HOTSPOT_DRAW_ADD \
		Hotspots_Add2 (menu_x + drawcur_x, menu_y + drawcur_y, draw_cur_w, (8 * 1) + 1, /*count*/ 1,  hotspottype_listitem, --headidx); \
		drawcur_x += (draw_cur_w + /*space*/ 1 * 8) // Ender

	int drawcur_x	= column_1; //2 * 8;

	int draw_cur_w = 12 /*chars*/ * 8;
	if (local_sort_by == SAVELOAD_SORTBY_SAVENAME_0) {DRAW_UNDERLINE_COLUMN;}
		M_PrintBronzey(drawcur_x, drawcur_y, "Save");	HOTSPOT_DRAW_ADD;

	draw_cur_w = 12 /*chars*/ * 8; // column width
	if (local_sort_by == SAVELOAD_SORTBY_MAP_1) {DRAW_UNDERLINE_COLUMN;}
		M_PrintBronzey(drawcur_x, drawcur_y, "Map");	HOTSPOT_DRAW_ADD;
		//drawcur_x += (/*1 spaces*/ 1 * 8);  // SPACES
	
	draw_cur_w = 9 /*chars*/ * 8; // column width
	if (local_sort_by == SAVELOAD_SORTBY_PROGRESS_2) {DRAW_UNDERLINE_COLUMN;}
		M_PrintBronzey(drawcur_x, drawcur_y, "Progress");	HOTSPOT_DRAW_ADD;
		//drawcur_x += (/*1 spaces*/ 1 * 8);
	
	draw_cur_w = 9 /*chars*/ * 8;
	if (local_sort_by == SAVELOAD_SORTBY_AGE_3) {DRAW_UNDERLINE_COLUMN;}
		M_PrintBronzey(drawcur_x, drawcur_y, "Age");	HOTSPOT_DRAW_ADD;
		//drawcur_x += (/*1 spaces*/ 1 * 8);
	
#endif

	PPX_DrawSel_End ();

	// Load new pic if applicable
	// This is not the draw phase
#if 0
	// Baker: This would be to mouse move preview pics, we aren't doing that
	int effective_cursor = hotspotx_hover == not_found_neg1 ? m_zdev2_load2cursor : hotspotx_hover + startrow;
#else
	// Baker: Column headers are mouse interactive only and do not affect local cursor
	int effective_cursor = local_cursor;
#endif


}

static void M_ZDev_Key(cmd_state_t *cmd, int key, int ascii)
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

#if 0 // DID WE CLICK A COLUMN HEADER THEN SET SORT AND GET OUT --- BLOCK
		{
			// This detects MOUSE collision with column headers
			// And sets the sort
			hotspotx_s *h = &hotspotxs[hotspotx_hover];
			if (h->hotspottype == hotspottype_listitem) {
				int sort_idx_for_hotspot = -(h->trueidx + 1);
				int did_click_the_current_sort_index = m_zdev2_sort_by == sort_idx_for_hotspot;

				m_zdev2_sort_by = sort_idx_for_hotspot;
				m_zdev2_sort_is_dirty = true;

				// IF WE CLICK THE CURRENT SORT INDEX, REVERSE IT
				if (did_click_the_current_sort_index) {
					// If clicked what is already the key, toggle it.
					m_zdev2_is_sort_ascending = !m_zdev2_is_sort_ascending;
				} else {
					int shall_invert = isin2 (m_zdev2_sort_by, SAVELOAD_SORTBY_AGE_3, SAVELOAD_SORTBY_PROGRESS_2);
					m_zdev2_is_sort_ascending = shall_invert ? false : true; // Low to high
				}

				break; // HIT A COLUMN HEADER, GET OUT.
			}
		}
#endif
#if 1 // DOUBLE CLICK LOGIC
		{
			int new_cursor		= hotspotx_hover + startrow;
			int is_new_cursor	= new_cursor != local_cursor;

			local_scroll_is_blocked = true; // PROTECT AGAINST AUTOSCROLL
			local_cursor = new_cursor;

			if (is_new_cursor)
				break; // GET OUT!  SET FOCUS TO ITEM

			// SEE IF DOUBLE CLICK, IF SO FIRE K_ENTER ACTION
			double new_click_time	= Sys_DirtyTime();
			double click_delta_time = local_click_time ? (new_click_time - local_click_time) : 0;
			int is_double_click		= click_delta_time && (click_delta_time < DOUBLE_CLICK_0_5);

			local_click_time = new_click_time;

			// New cursor or not a double click, get out otherwise fall through to K_ENTER action
			if (is_double_click == false)
				break; 

			// Fall through to fire K_ENTER
		} // block
#endif

	case K_ENTER:
		S_LocalSound ("sound/misc/menu2.wav");
#if 0
		if (local_count) {

			// local cursor index translated to sort index
			int pidx = *((int *)&g_saves_indexes_4byte.strings[local_cursor][0]);
			const char *s_savename = g_saves_filename_noExt.strings[pidx];// pIdx here
			const char *s_map = g_saves_mapnames.strings[pidx];// pIdx here

			va_super (tmp, 1024, "load %s // %s", s_savename, s_map);

#if 0 // Not reentrant
			//menu_state_reenter = 1;
#endif
			Cbuf_AddTextLine (cmd, tmp);

			Key_History_Push_String (tmp);

			KeyDest_Set (key_game); // key_dest = key_game;
			menu_state_set_nova (m_none);
		}
#endif

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
			// Baker: Find the next item starting with 'a' or whatever key someone pressed (wraparound seek)
			char sprefix[2] = { (char)lcase_ascii, 0 };

			for (int myidx = local_cursor, iters = 0; iters < local_count; iters ++) {
				myidx ++;
				if (myidx >= local_count)
					myidx = 0;
				
				char *sx = g_saves_filename_noExt_crop.strings[myidx];

				if (false == String_Does_Start_With_Caseless (sx, sprefix))
					continue;

				local_cursor = myidx;
				break;
			} // for iters
		} // if a-z
		break;
	} // sw

}

#undef	local_count	
#undef	local_cursor
#undef 	visiblerows
#undef 	startrow
#undef 	endrow
#undef  local_click_time
#undef  local_sort_by
#undef  local_is_sort_ascending
#undef	local_sort_is_dirty
#undef  local_scroll_is_blocked

