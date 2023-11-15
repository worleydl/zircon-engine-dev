// menu_server_list.c.h

#define		local_count		serverlist_viewcount
#define		local_cursor	slist_cursor
#define 	visiblerows 	m_slist_visiblerows
#define 	startrow 		m_slist_startrow
#define 	endrow 			m_slist_endrow

static int slist_cursor;

int m_slist_visiblerows;
int m_slist_startrow;
int m_slist_endrow;

#pragma message ("pausedemo must reset on new map")

#pragma message ("protocol nehahra movie does not work")

#pragma message ("Server list cursor is borked")
#pragma message ("Server list clear on switch between Quakeworld and DakrPlaces")
#pragma message ("Caption at bottom of screen the address of server where cursor is")
void M_Menu_ServerList_f(cmd_state_t *cmd)
{
	key_dest = key_menu;
	menu_state_set_nova (m_slist);
	m_entersound = true;
	M_Update_Return_Reason("");
	if (lanConfig_cursor == 2)
		Net_SlistQW_f(cmd);
	else
		Net_Slist_f(cmd);
	startrow = not_found_neg1, endrow = not_found_neg1;
	local_cursor = 0;
}

static void M_ServerList_Draw (void)
{
	int n, statnumplayers, statmaxplayers;
	cachepic_t *p0;
	const char *s;
	char vabuf[1024];

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
				
			serverlist_entry_t *entry = ServerList_GetViewEntry(n);
			
			Hotspots_Add2 (menu_x + 0, menu_y + drawcur_y, 62 * 8, (8 * 1) + 1, 1, hotspottype_button, /*idx*/ n); // PPX DUR
			M_PrintColored(0, drawcur_y, entry->line1);
			
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
 
		serverlist_entry_t *ex = ServerList_GetViewEntry(idx);

		drawcur_y = menu_height - (/*rows*/ (1) * 8) - 4;
		
		s = va(vabuf, sizeof(vabuf), S_FMT_LEFT_PAD_40, ex->info.name);
		M_PrintBronzey	( (2 +  0) * 8, drawcur_y, s);

		s = va(vabuf, sizeof(vabuf), S_FMT_LEFT_PAD_40, ex->info.cname);
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

	case K_ENTER:
		S_LocalSound ("sound/misc/menu2.wav");
		if (local_count) {
			char vabuf[1024];
			Cbuf_AddTextLine (cmd, va(vabuf, sizeof(vabuf), "connect " QUOTED_S, ServerList_GetViewEntry(local_cursor)->info.cname));
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
		local_cursor++;
		if (local_cursor >= local_count)
			local_cursor = local_count - 1;
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
