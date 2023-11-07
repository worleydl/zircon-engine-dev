// console_autocomplete.c.h


void Partial_Reset (void)
{
	void Sys_PrintToTerminal2(const char *text);
	freenull_ (_g_autocomplete.s_search_partial_a);
	Sys_PrintToTerminal2 ("Partial Reset\n");
	//Con_PrintLinef ("Partial Reset");

}

// Performed by:
WARP_X_ (Key_ClearEditLine if console which is called by Con_ToggleConsole)
// Con_ToggleConsole resets history to -1 
// Partial reset needs to be performed in the console
// by damn near any key that is not TAB or shift TAB

// What keys should not reset autocomplete
// 1. TAB and shift TAB
// 2. ALT-TAB
// 3. Probably ALT-ENTER
// 4. Mousewheel scrolling through history
// 5. Setting zoom like CTRL-ZERO or CTRL-PLUS or CTRL-MINUS

void Partial_Reset_Undo_Selection_Reset (void)
{
	Partial_Reset ();
}

autocomplete_t _g_autocomplete;

int GetXTexMode_Count (const char *s_prefix)
{
	int			count = 0, i;
	// This list has to be alpha sorted for
	// autocomplete to work right
	const char *slist[] =  {
		"GL_LINEAR",
		"GL_LINEAR_MIPMAP_LINEAR",
		"GL_LINEAR_MIPMAP_NEAREST",
		"GL_NEAREST",
		"GL_NEAREST_MIPMAP_LINEAR",
		"GL_NEAREST_MIPMAP_NEAREST",
	};

	int	nummy = (int)ARRAY_COUNT(slist);

	for (i = 0; i < nummy; i++) {
		const char *sxy =  slist[i];
		if (String_Does_Start_With_Caseless (sxy, s_prefix) == false)
			continue;

		SPARTIAL_EVAL_

	} // i
	return count;
}


/*
GetMapList

Made by [515]
Prints not only map filename, but also
its format (q1/q2/q3/hl) and even its message
*/
//[515]: here is an ugly hack.. two gotos... oh my... *but it works*
//LadyHavoc: rewrote bsp type detection, rewrote message extraction to do proper worldspawn parsing
//LadyHavoc: added .ent file loading, and redesigned error handling to still try the .ent file even if the map format is not recognized, this also eliminated one goto
//LadyHavoc: FIXME: man this GetMapList is STILL ugly code even after my cleanups...

int m_maplist_count;
maplist_s m_maplist[MAXMAPLIST_4096];

qbool GetMapList (const char *s_partial, char *completedname, 
	int completednamebufferlength, int is_menu_fill, 
	int is_zautocomplete, int is_suppress_print)
{
	fssearch_t	*t;
	char		s_pattern[1024];
	char		s_map_title[1024];
	int			j, k, our_max_length, p, partial_length, our_min_length;
	unsigned char *lengths_array;
	qfile_t		*f;
	unsigned char buf[1024];

	if (is_menu_fill) {
		m_maplist_count = 0;
	}

	c_dpsnprintf1 (s_pattern, "maps/%s*.bsp", s_partial);
	
	#pragma message ("Do a FS_Search_Concat?")
	t = FS_Search(s_pattern, fs_caseless_true, fs_quiet_true, fs_pakfile_null);

	if (!t)
		return false;
	if (t->numfilenames > 1) {
		if (is_menu_fill == false && is_suppress_print == false) 
			Con_PrintLinef (CON_BRONZE " %d maps found:", t->numfilenames); // 2bronze
	}
	lengths_array = (unsigned char *)Z_Malloc(t->numfilenames);
	our_min_length = 666;
	for (our_max_length = j = 0; j < t->numfilenames; j++) {
		k = (int)strlen(t->filenames[j]);
		k -= 9; // Why 9?  maps/ is 5, .bsp is 4
		if (our_max_length < k)
			our_max_length = k;
		else
		if (our_min_length > k)
			our_min_length = k;
		lengths_array[j] = k;
	}
	partial_length = (int)strlen(s_partial);
	for (j = 0; j < t->numfilenames; j++) {
		int lumpofs = 0, lumplen = 0;
		char *entities = NULL;
		const char *data = NULL;
		char keyname[64];
		char entfilename[MAX_QPATH];
		char desc[64];
		int map_format_code = 0;
		int is_playable = false;
		desc[0] = 0;
		c_strlcpy (s_map_title, "^1ERROR: open failed^7");
		p = 0;
		f = FS_OpenVirtualFile(t->filenames[j], fs_quiet_true);
		if (f) {
			c_strlcpy (s_map_title, "^1ERROR: not a known map format^7");
			memset(buf, 0, 1024);
			FS_Read(f, buf, 1024);
			if (!memcmp(buf, "IBSP", 4)) {
				p = LittleLong(((int *)buf)[1]);
				if (p == Q3BSPVERSION) {
					q3dheader_t *header = (q3dheader_t *)buf;
					lumpofs = LittleLong(header->lumps[Q3LUMP_ENTITIES].fileofs);
					lumplen = LittleLong(header->lumps[Q3LUMP_ENTITIES].filelen);
					c_dpsnprintf1 (desc, "Q3BSP%d", p); 
					map_format_code = 3;
				}
				else if (p == Q2BSPVERSION) {
					q2dheader_t *header = (q2dheader_t *)buf;
					lumpofs = LittleLong(header->lumps[Q2LUMP_ENTITIES].fileofs);
					lumplen = LittleLong(header->lumps[Q2LUMP_ENTITIES].filelen);
					c_dpsnprintf1 (desc, "Q2BSP%d", p); 
					map_format_code = 2;
				}
				else {
					c_dpsnprintf1 (desc, "IBSP%d", p); 
					map_format_code = 4;
				}
			} else if (BuffLittleLong(buf) == BSPVERSION /*29*/) {
				lumpofs = BuffLittleLong(buf + 4 + 8 * LUMP_ENTITIES);
				lumplen = BuffLittleLong(buf + 4 + 8 * LUMP_ENTITIES + 4);
				c_strlcpy (desc, "BSP29"); 
				map_format_code = 1;
			} else if (BuffLittleLong(buf) == 30 /*Half-Life*/) {
				lumpofs = BuffLittleLong(buf + 4 + 8 * LUMP_ENTITIES);
				lumplen = BuffLittleLong(buf + 4 + 8 * LUMP_ENTITIES + 4);
				c_strlcpy (desc, "BSPHL"); 
				map_format_code = -1;
			} else if (!memcmp(buf, "BSP2", 4)) {
				lumpofs = BuffLittleLong(buf + 4 + 8 * LUMP_ENTITIES);
				lumplen = BuffLittleLong(buf + 4 + 8 * LUMP_ENTITIES + 4);
				c_strlcpy (desc, "BSP2"); 
				map_format_code = 1;
			} else if (!memcmp(buf, "2PSB", 4)) {
				lumpofs = BuffLittleLong(buf + 4 + 8 * LUMP_ENTITIES);
				lumplen = BuffLittleLong(buf + 4 + 8 * LUMP_ENTITIES + 4);
				c_strlcpy (desc, "BSP2RMQe"); 
				map_format_code = 1;
			} else if (!memcmp(buf, "VBSP", 4)) {
				hl2dheader_t *header = (hl2dheader_t *)buf;
				lumpofs = LittleLong(header->lumps[HL2LUMP_ENTITIES].fileofs);
				lumplen = LittleLong(header->lumps[HL2LUMP_ENTITIES].filelen);
				int versi = LittleLong(((int *)buf)[1]);
				c_dpsnprintf1 (desc, "VBSP%d", versi); 
				map_format_code = 5;
			} else {
				c_dpsnprintf1(desc, "unknown%d", BuffLittleLong(buf)); 
				map_format_code = -2;
			}
			c_strlcpy(entfilename, t->filenames[j]);
			memcpy(entfilename + strlen(entfilename) - 4, ".ent", 5);
			entities = (char *)FS_LoadFile(entfilename, tempmempool, fs_quiet_true, fs_size_ptr_null);

			if (!entities && lumplen >= 10) {
				FS_Seek(f, lumpofs, SEEK_SET);
				entities = (char *)Z_Malloc(lumplen + 1);
				FS_Read(f, entities, lumplen);
			}
			if (entities) {
				// if there are entities to parse, a missing message key just
				// means there is no title, so clear the message string now
				s_map_title[0] = 0;
				is_playable = String_Does_Contain (entities, "info_player_start") || 
					String_Does_Contain (entities, "info_player_deathmatch");

				data = entities;
				for (;;) {
					int this_len;
					if (!COM_ParseToken_Simple(&data, false, false, true))
						break;
					if (com_token[0] == '{')
						continue;
					if (com_token[0] == '}')
						break;

					// skip leading whitespace
					for (k = 0; com_token[k] && ISWHITESPACE(com_token[k]);k++);
					for (this_len = 0; this_len < (int)sizeof(keyname) - 1 && com_token[k+this_len] && !ISWHITESPACE(com_token[k+this_len]);this_len++) {
						keyname[this_len] = com_token[k + this_len];
					} // for
					keyname[this_len] = 0;
					if (!COM_ParseToken_Simple(&data, false, false, true))
						break;
					if (developer_extra.integer)
						Con_DPrintLinef ("key: %s %s", keyname, com_token);
					if (String_Does_Match(keyname, "message")) {
						// get the map title
						c_strlcpy (s_map_title, com_token);
						break;
					} // if "message"
				} // for
			} // if entities
		}
		if (entities)
			Z_Free(entities);
		if (f)
			FS_Close(f);

		*(t->filenames[j] + lengths_array[j] + 5) = 0;

		// Ignore unplayable map
		if (!is_playable)
			continue;

		if (is_zautocomplete) {
			const char *sxy = t->filenames[j] + 5;

			SPARTIAL_EVAL_
		}
		
		if (is_menu_fill == false && is_suppress_print == false) {
			// If we print, print only maps with spawnpoints
			// This should avoid healthboxes and such
			if (is_zautocomplete == false) {
				// Zircon autocomplete does not print here on 2nd autocomplete
				// For example, press TAB it prints
				// Press TAB again, it does not
				Con_PrintLinef ("%16s (%-8s) %s", t->filenames[j] + 5, desc, s_map_title);
			}
		} else {
			// Map fill
			if (m_maplist_count < (int)ARRAY_COUNT(m_maplist) /* map with no title should still list */ /*&& s_map_title[0]*/) {
				char stru[16];
				char stru28[28];

				maplist_s *mx = &m_maplist[m_maplist_count];
				if (mx->sm_a)			{ free (mx->sm_a);		mx->sm_a	= NULL;	}
				if (mx->smtru_a)		{ free (mx->smtru_a);	mx->smtru_a	= NULL;	}
				if (mx->smsg_a)			{ free (mx->smsg_a);	mx->smsg_a	= NULL;	}
				//if (mx->sqbsp)		{ free (mx->smsg_a);	mx->smsg_a	= NULL;	}
				//unsigned char *sqbsp;

				c_strlcpy (stru, t->filenames[j] + 5);
				c_strlcpy (stru28, s_map_title);

				mx->sm_a	= (unsigned char *)strdup	(t->filenames[j] + 5);
				mx->smtru_a	= (unsigned char *)strdup	(stru);
				mx->smsg_a	= (unsigned char *)strdup	(stru28 /*message*/);
				mx->sqbsp	= (unsigned char *) ( (map_format_code == 3 || map_format_code == 4) ? "Q3" : "");
				m_maplist_count ++;
			} // if m_maplist_count < max
		} // if menu fill
	} // for numfilenames
	if (is_menu_fill == false && is_suppress_print == false) 
		Con_Print("\n");

	for (p = partial_length; p < our_min_length; p ++)
	{
		k = *(t->filenames[0] + 5 + p);
		if (k == 0)
			goto endcomplete;
		for(j = 1; j < t->numfilenames; j++)
			if (*(t->filenames[j] + 5 + p) != k)
				goto endcomplete;
	}
endcomplete:
	if (p > partial_length && completedname && completednamebufferlength > 0) {
		memset(completedname, 0, completednamebufferlength);
		memcpy(completedname, (t->filenames[0] + 5), Smallest(p, completednamebufferlength - 1));
	}
	Z_Free(lengths_array);
	FS_FreeSearch(t);
	return p > partial_length;
}
