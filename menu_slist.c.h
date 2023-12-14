// menu_server_list.c.h

static int slist_cursor;

//
//
//

int serverlist_viewx_dirty;

int serverlist_list_count = 0;
int serverlist_list[SERVERLIST_VIEWLISTSIZE_2048];
char last_nav_cname[128];

WARP_X_ (stringlistsort_cmp)

void SList_Filter_Sort_Changed_c(cvar_t *var)
{
	serverlist_viewx_dirty = true;
}

extern cvar_t net_slist_filter_word;
extern cvar_t net_slist_filter_players_only;
extern cvar_t net_slist_sort_by;
extern cvar_t net_slist_sort_ascending;


WARP_X_ (stringlistsort_cmp)

// Baker: This is deterministic because no 2 servers can have same address
// (And if they did due to multiple master servers, it already got filtered out).
int slist_sort_cname_deterministic (const void *pa, const void *pb)
{
	int negator = net_slist_sort_ascending.integer == 0 ? -1 : 1;
	int a = *(int *)pa;
	int b = *(int *)pb;
	serverlist_entry_t *my_entry_a = ServerList_GetViewEntry(a);
	serverlist_entry_t *my_entry_b = ServerList_GetViewEntry(b);

	const char *s1 = my_entry_a->info.name;
	const char *s2 = my_entry_b->info.name;
	
	return negator * strcasecmp(s1, s2);
}


int slist_sort_ping (const void *pa, const void *pb)
{
	int negator = net_slist_sort_ascending.integer == 0 ? -1 : 1;
	int a = *(int *)pa;
	int b = *(int *)pb;
	serverlist_entry_t *my_entry_a = ServerList_GetViewEntry(a);
	serverlist_entry_t *my_entry_b = ServerList_GetViewEntry(b);

	int n1 = my_entry_a->info.ping;
	int n2 = my_entry_b->info.ping;
	
	int diff = negator * (n1 - n2);

	if (diff == 0) {
		// Baker: Deterministic sorting.
		// We cannot allow ties ever.
		// Reason: We want a predictable order every time.
		return slist_sort_cname_deterministic (pa, pb);
	}

	return diff;
}

int slist_sort_numplayers (const void *pa, const void *pb)
{
	int negator = net_slist_sort_ascending.integer == 0 ? -1 : 1;
	int a = *(int *)pa;
	int b = *(int *)pb;
	serverlist_entry_t *my_entry_a = ServerList_GetViewEntry(a);
	serverlist_entry_t *my_entry_b = ServerList_GetViewEntry(b);

	int n1 = my_entry_a->info.numplayers;
	int n2 = my_entry_b->info.numplayers;
	
	int diff = negator * (n1 - n2);
	if (diff == 0) {
		// Baker: Deterministic sorting.
		// We cannot allow ties ever.
		// Reason: We want a predictable order every time.
		return slist_sort_cname_deterministic (pa, pb);
	}

	return diff;
}

int slist_sort_description (const void *pa, const void *pb)
{
	int negator = net_slist_sort_ascending.integer == 0 ? -1 : 1;
	int a = *(int *)pa;
	int b = *(int *)pb;
	serverlist_entry_t *my_entry_a = ServerList_GetViewEntry(a);
	serverlist_entry_t *my_entry_b = ServerList_GetViewEntry(b);

	const char *s1 = my_entry_a->info.name;
	const char *s2 = my_entry_b->info.name;
	
	int diff = negator * strcasecmp(s1, s2);
	if (diff == 0) {
		// Baker: Deterministic sorting.
		// We cannot allow ties ever.
		// Reason: We want a predictable order every time.
		return slist_sort_cname_deterministic (pa, pb);
	}

	return diff;
}

int slist_sort_gamedir (const void *pa, const void *pb)
{
	int negator = net_slist_sort_ascending.integer == 0 ? -1 : 1;
	int a = *(int *)pa;
	int b = *(int *)pb;
	serverlist_entry_t *my_entry_a = ServerList_GetViewEntry(a);
	serverlist_entry_t *my_entry_b = ServerList_GetViewEntry(b);

	const char *s1 = my_entry_a->info.mod;
	const char *s2 = my_entry_b->info.mod;
	
	int diff = negator * strcasecmp(s1, s2);
	if (diff == 0) {
		// Baker: Deterministic sorting.
		// We cannot allow ties ever.
		// Reason: We want a predictable order every time.
		return slist_sort_cname_deterministic (pa, pb);
	}

	return diff;
}

int slist_sort_map (const void *pa, const void *pb)
{
	int negator = net_slist_sort_ascending.integer == 0 ? -1 : 1;
	int a = *(int *)pa;
	int b = *(int *)pb;
	serverlist_entry_t *my_entry_a = ServerList_GetViewEntry(a);
	serverlist_entry_t *my_entry_b = ServerList_GetViewEntry(b);

	const char *s1 = my_entry_a->info.map;
	const char *s2 = my_entry_b->info.map;
	
	int diff = negator * strcasecmp(s1, s2);
	if (diff == 0) {
		// Baker: Deterministic sorting.
		// We cannot allow ties ever.
		// Reason: We want a predictable order every time otherwise the server browser list jumps around
		// with tie entries randomly filling out the list differently every frame.
		return slist_sort_cname_deterministic (pa, pb);
	}

	return diff;
}

void Commit_To_Cname (void)
{
	int idx_nova = serverlist_list[slist_cursor];
	serverlist_entry_t *my_entry= ServerList_GetViewEntry(idx_nova);
	c_strlcpy (last_nav_cname, my_entry->info.cname);
}


void M_ServerList_Rebuild (void)
{
	serverlist_list_count = 0; // Clear
	
	// Find qualifying entries ..
	for (int idx = 0 ; idx < serverlist_viewlist_count; idx ++) {
		//serverlist_viewlist[i] = serverlist_viewlist[ i - 1 ];
		serverlist_entry_t *my_entry= ServerList_GetViewEntry(idx);

		// Baker: Server without map name is disqualified (seems to be qwfwd or something)
		if (my_entry->info.map[0] == 0)
			continue; // DISQUALIFIED

//		int is_qualified = true;

		// Word Filter?
		if (net_slist_filter_word.string[0]) {
			// Must contain
//			int has_word = true;
			if (String_Does_Contain_Caseless (my_entry->info.cname, net_slist_filter_word.string))
				goto keep_me;
			if (String_Does_Contain_Caseless (my_entry->info.name /*description*/, net_slist_filter_word.string))
				goto keep_me;
			if (String_Does_Contain_Caseless (my_entry->info.mod /*description*/, net_slist_filter_word.string))
				goto keep_me;
			if (String_Does_Contain_Caseless (my_entry->info.map, net_slist_filter_word.string))
				goto keep_me;

			continue; // Disqualified
		}

keep_me:
		// Players only?
		if (net_slist_filter_players_only.value) {
			if (my_entry->info.numplayers == 0)
				continue; // DISQUALIFIED
		}

		// ADD:
		serverlist_list[serverlist_list_count] = idx; serverlist_list_count ++;
	} // for

#if 000
	for (int idx = 0 ; idx < serverlist_list_count; idx ++) {
		int idx_nova = serverlist_list[idx];
		serverlist_entry_t *my_entry= ServerList_GetViewEntry(idx_nova);
		Con_PrintLinef ("RAW idx %03d of %03d/ idx_nova %03d %s", idx, serverlist_list_count, idx_nova, my_entry->info.name);
	}
#endif

	// Sort the list.  If no sort specified, don't.
	// "net_slist_sort_by", "0", "0 is ping, 1 is players, 2 name, 3 game 4 map"};
	int sort_type = net_slist_sort_by.value;

	if (serverlist_list_count == 0)
		goto empty;

//	qsort(&serverlist_list[0], serverlist_list_count, sizeof(serverlist_list[0]), slist_sort_ping);
//	goto empty;
	switch (sort_type) {
	default /*ping*/:				qsort(&serverlist_list[0], serverlist_list_count, sizeof(serverlist_list[0]), slist_sort_ping);
									break;
	case 1 /*numplayers*/:			qsort(&serverlist_list[0], serverlist_list_count, sizeof(serverlist_list[0]), slist_sort_numplayers);
									break;
	case 2 /*description*/:			qsort(&serverlist_list[0], serverlist_list_count, sizeof(serverlist_list[0]), slist_sort_description);
									break;
	case 3 /*gamedir*/:				qsort(&serverlist_list[0], serverlist_list_count, sizeof(serverlist_list[0]), slist_sort_gamedir);
									break;
	case 4 /*map*/:					qsort(&serverlist_list[0], serverlist_list_count, sizeof(serverlist_list[0]), slist_sort_map);
									break;
	} // sw

empty:
	// Determine listindex
#if 0
	for (int idx = 0 ; idx < serverlist_list_count; idx ++) {
		int idx_nova = serverlist_list[idx];
		serverlist_entry_t *my_entry= ServerList_GetViewEntry(idx_nova);
		Con_PrintLinef ("SORTED idx %03d of %03d/ idx_nova %03d %s", idx, serverlist_list_count, idx_nova, my_entry->info.name);
	}	
#endif

	if (last_nav_cname[0] == 0 || serverlist_list_count == 0) {
		goto cant_find;
	}

	
	for (int idx = 0 ; idx < serverlist_list_count; idx ++) {
		int idx_nova = serverlist_list[idx];
		serverlist_entry_t *this_entry = ServerList_GetViewEntry(idx_nova);
		if (String_Does_Match (this_entry->info.cname, last_nav_cname)) {
			// Found it
			slist_cursor = idx;
			return;
		}
	} // for	

cant_find:
	slist_cursor = 0;
}


//#define		local_count		serverlist_viewlist_count
#define		local_count		serverlist_list_count
#define		local_cursor	slist_cursor
#define 	visiblerows 	m_slist_visiblerows
#define 	startrow 		m_slist_startrow
#define 	endrow 			m_slist_endrow


int m_slist_visiblerows;
int m_slist_startrow;
int m_slist_endrow;

// Ok how does this work ..
// serverlist_viewx_dirty = true;
// Stage 1: draw
// VID_TouchscreenArea( 0,   0, 160,  64,  64, "gfx/gui/touch_menu_button.tga"
// font?


WARP_X_ (Net_SlistQW_f, Net_Slist_f ServerList_QueryList)
// Baker: serverlist_viewlist is a subset of serverlist_cache (basically)
WARP_X_ (serverlist_viewlist serverlist_viewlist_count serverlist_cache)
WARP_X_ (NetConn_ClientParsePacket_ServerList_UpdateCache which line 1 is written)
void M_Menu_ServerList_f(cmd_state_t *cmd)
{
	key_dest = key_menu;
	menu_state_set_nova (m_slist);
	m_entersound = true;
	M_Update_Return_Reason("");

	if (menu_state_reenter == 0) {
		if (lanConfig_cursor == 2)
			Net_SlistQW_f(cmd);
		else
			Net_Slist_f(cmd);
	}

	startrow = not_found_neg1, endrow = not_found_neg1;

	if (local_cursor >= local_count)
		local_cursor = local_count - 1;
	if (local_cursor < 0)
		local_cursor = 0;
//	local_cursor = 0;
	if (menu_state_reenter)
		menu_state_reenter = 0;
}


static void M_ServerList_Draw (void)
{
	int n, statnumplayers, statmaxplayers;
	cachepic_t *p0;
	const char *s;
	char vabuf[1024];

	if (serverlist_viewx_dirty) {
		// Rebuild list
		M_ServerList_Rebuild ();
		serverlist_viewx_dirty = false;
	}


	drawidx = 0; drawsel_idx = not_found_neg1;  // PPX_Start - does not have frame cursor

	M_Background(640, vid_conheight.integer, q_darken_true);

	ServerList_GetPlayerStatistics(&statnumplayers, &statmaxplayers);
	s = va(vabuf, sizeof(vabuf), "%d/%d masters %d/%d servers %d/%d players", masterreplycount, masterquerycount, serverreplycount, serverquerycount, statnumplayers, statmaxplayers);
	M_PrintBronzey((640 - strlen(s) * 8) / 2, 32, s);
	if (*m_return_reason)
		M_Print(16, menu_height - 8, m_return_reason);

	//^7%-21.21s %-19.19s ^%c%-17.17s^7 %-20.20s
	M_PrintBronzey(0, 48, "  Ping Players  Name                     Game       Map");

	// scroll the list as the cursor moves

	drawcur_y = 60;
	visiblerows = (int)((menu_height - (/*rows*/ (2 + 1) * 8) - drawcur_y) / 8);

	// Baker: Do it this way because a short list may have more visible rows than the list count
	// so using bound doesn't work.
	startrow = local_cursor - (visiblerows / 2);
	if (startrow > local_count - visiblerows)
		startrow = local_count - visiblerows;
	if (startrow < 0)
		startrow = 0; // visiblerows can exceed local_count

	endrow = Smallest(startrow + visiblerows, local_count);

	p0 = Draw_CachePic ("gfx/p_multi");
	M_DrawPic((640 - Draw_GetPicWidth(p0)) / 2, 4, "gfx/p_multi", NO_HOTSPOTS_0, NA0, NA0);

	WARP_X_ (NetConn_ClientParsePacket_ServerList_UpdateCache)

	if (endrow > startrow) {
		for (n = startrow; n < endrow; n++) {
			if (!in_range_beyond (0, n, local_count) )
				continue;

			if (n == local_cursor)
				drawsel_idx = (n - startrow) /*relative*/;

			int idx_nova = serverlist_list[n];
			serverlist_entry_t *entry_nova = ServerList_GetViewEntry(idx_nova);

			Hotspots_Add2 (menu_x + 0, menu_y + drawcur_y, 62 * 8, (8 * 1) + 1, 1, hotspottype_button, /*idx*/ n); // PPX DUR
			M_PrintColored(0, drawcur_y, entry_nova->line1);

			drawcur_y += 8;
		} // for
	} // endrow > startrow
	else if (host.realtime - masterquerytime > 10)
	{
		if (masterquerycount)
			M_Print(0, drawcur_y, "No servers found");
		else
			M_Print(0, drawcur_y, "No master servers found (network problem?)");
	}
	else
	{
		if (serverquerycount)
			M_Print(0, drawcur_y, "Querying servers");
		else
			M_Print(0, drawcur_y, "Querying master servers");
	}

	PPX_DrawSel_End ();

	if (local_count) {
		int idx =
			hotspotx_hover == not_found_neg1 ?
			local_cursor : hotspotx_hover + startrow;

		//serverlist_entry_t *ex = ServerList_GetViewEntry(idx);
		int idx_nova = serverlist_list[idx];
		serverlist_entry_t *entry_nova = ServerList_GetViewEntry(idx_nova);


		drawcur_y = menu_height - (/*rows*/ (1) * 8) - 4;

		s = va(vabuf, sizeof(vabuf), S_FMT_LEFT_PAD_40, entry_nova->info.name);
		M_PrintBronzey	( (2 +  0) * 8, drawcur_y, s);

		s = va(vabuf, sizeof(vabuf), S_FMT_LEFT_PAD_40, entry_nova->info.cname);
		M_Print			( (2 + 40) * 8 , drawcur_y, s);
	} // if
}

static void M_ServerList_Key(cmd_state_t *cmd, int k, int ascii)
{
	switch (k) {
	case K_MOUSE2: // fall thru
	case K_ESCAPE:
		M_Menu_LanConfig_f(cmd);
		break;

	case K_MOUSE1:
		if (hotspotx_hover == not_found_neg1)
			break;

		local_cursor = hotspotx_hover + startrow; // fall thru
		Commit_To_Cname ();

	case K_ENTER:
		S_LocalSound ("sound/misc/menu2.wav");
		if (local_count) {
			char vabuf[1024];

			int idx_nova = serverlist_list[local_cursor];
			serverlist_entry_t *entry_nova = ServerList_GetViewEntry(idx_nova);

			char *s_ipaddy = entry_nova->info.cname;
			char *s_name = entry_nova->info.name;
			menu_state_reenter = (lanConfig_cursor == /*qw*/ 2) ? 3 : 2; 
			
			// Baker: Issuing a connect will exit the menu
			// So we set the re-entrance state "menu_state_reenter" first

			// Baker: Prevent annoyingly long console lines
			va (vabuf, 80 /*sizeof(vabuf)*/, "connect " QUOTED_S " // %s", s_ipaddy, s_name);
			Cbuf_AddTextLine (cmd, vabuf);

			// Baker r0072: Add maps menu map to command history for recall.
			Key_History_Push_String (vabuf);

		}

		break;

	case K_SPACE: // This is "refresh list"

		if (lanConfig_cursor == 2)
			Net_SlistQW_f(cmd);
		else
			Net_Slist_f(cmd);
		break;

	case K_HOME:
		if (local_count)
			local_cursor = 0;
		Commit_To_Cname ();
		break;

	case K_END:
		if (local_count)
			local_cursor = local_count - 1;
		Commit_To_Cname ();
		break;

	case K_PGUP:
		local_cursor -= visiblerows / 2;
		if (local_cursor < 0)
			local_cursor = 0;
		Commit_To_Cname ();
		break;

	case K_MWHEELUP:
		local_cursor -= visiblerows / 4;
		if (local_cursor < 0)
			local_cursor = 0;
		Commit_To_Cname ();
		break;

	case K_PGDN:
		local_cursor += visiblerows / 2;
		if (local_cursor >= local_count)
			local_cursor = local_count - 1;
		Commit_To_Cname ();
		break;

	case K_MWHEELDOWN:
		local_cursor += visiblerows / 4;
		if (local_cursor >= local_count)
			local_cursor = local_count - 1;
		Commit_To_Cname ();
		break;

	case K_UPARROW:
		//S_LocalSound ("sound/misc/menu1.wav");
		local_cursor--;
		if (local_cursor < 0)
			local_cursor = 0;
		Commit_To_Cname ();
		break;

	case K_DOWNARROW:
		//S_LocalSound ("sound/misc/menu1.wav");
		local_cursor++;
		if (local_cursor >= local_count)
			local_cursor = local_count - 1;
		Commit_To_Cname ();
		break;

	case K_LEFTARROW:
		break;

	case K_RIGHTARROW:
		break;

	default:
		break;
	} // sw

}

#undef local_count
#undef local_cursor
#undef visiblerows
#undef 	startrow
#undef 	endrow
