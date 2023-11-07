// menu_quit.c.h


//=============================================================================
/* QUIT MENU */

static const char *m_quit_message[9];
static int		m_quit_prevstate;
static qbool	wasInMenus;


static int M_QuitMessage(const char *line1, const char *line2, const char *line3, const char *line4, const char *line5, const char *line6, const char *line7, const char *line8)
{
	m_quit_message[0] = line1;
	m_quit_message[1] = line2;
	m_quit_message[2] = line3;
	m_quit_message[3] = line4;
	m_quit_message[4] = line5;
	m_quit_message[5] = line6;
	m_quit_message[6] = line7;
	m_quit_message[7] = line8;
	m_quit_message[8] = NULL;
	return 1;
}

static int M_ChooseQuitMessage(int request)
{
	M_QuitMessage("Are you sure you want to quit?",
		"Press Y to quit, N to stay",
		NULL,NULL,NULL,NULL,NULL,NULL);
	return 0; // Baker: Why does this have a return value?  So a quit message can do multiple lines in a for loop.
}

void M_Menu_Quit_f(cmd_state_t *cmd)
{
	int n;
	if (m_state == m_quit)
		return;
	wasInMenus = (key_dest == key_menu || key_dest == key_menu_grabbed);
	key_dest = key_menu;
	m_quit_prevstate = m_state;
	menu_state_set_nova (m_quit);
	m_entersound = true;
	// count how many there are
	for (n = 1;M_ChooseQuitMessage(n);n++);
	// choose one
	M_ChooseQuitMessage(rand() % n);
}


static void M_Quit_Key(cmd_state_t *cmd, int key, int ascii)
{
	switch (key) {
	case K_ESCAPE:	case K_MOUSE2:
	case 'n':
	case 'N':
		if (wasInMenus)
		{
			m_state = (enum m_state_e)m_quit_prevstate;
			m_entersound = true;
		}
		else
		{
			key_dest = key_game;
			menu_state_set_nova (m_none);
		}
		break;
	case K_MOUSE1:
	case 'Y':
	case 'y':
		host.state = host_shutdown;
		break;

	default:
		break;
	}
}

static void M_Quit_Draw (void)
{
	int i, l, linelength, firstline, lastline, lines;
	for (i = 0, linelength = 0, firstline = 9999, lastline = -1;m_quit_message[i];i++)
	{
		if ((l = (int)strlen(m_quit_message[i])))
		{
			if (firstline > i)
				firstline = i;
			if (lastline < i)
				lastline = i;
			if (linelength < l)
				linelength = l;
		}
	}
	lines = (lastline - firstline) + 1;
	M_Background(linelength * 8 + 16, lines * 8 + 16, q_darken_true);
	if (!m_missingdata) //since this is a fallback menu for missing data, it is very hard to read with the box
		M_DrawTextBox(0, 0, linelength, lines); //this is less obtrusive than hacking up the M_DrawTextBox function
	for (i = 0, l = firstline;i < lines;i++, l++)
		M_Print(8 + 4 * (linelength - strlen(m_quit_message[l])), 8 + 8 * i, m_quit_message[l]);
}

static int M_ChooseNoSaveMessage()
{
	return M_QuitMessage("Nothing to save!","OK",NULL,NULL,NULL,NULL,NULL,NULL);
}

//void M_Menu_NoSave_f(cmd_state_t *cmd)
//{
//	key_dest = key_menu;
//	
//	menu_state_set_nova (m_nosave);
//	m_entersound = true;
//	M_ChooseNoSaveMessage();
//}
//
//
//static void M_NoSave_Key (cmd_state_t* cmd, int key, int ascii)
//{
//	switch (key) {
//	case K_MOUSE2:
//	case K_ESCAPE:	
//	case K_MOUSE1: 
//	case K_ENTER:
//		M_Menu_SinglePlayer_f (cmd);
//	} // sw
//}
//
//static void M_NoSave_Draw (void)
//{
//	int i, l, linelength, firstline, lastline, lines;
//	for (i = 0, linelength = 0, firstline = 9999, lastline = -1;m_quit_message[i];i++)
//	{
//		if ((l = (int)strlen(m_quit_message[i])))
//		{
//			if (firstline > i)
//				firstline = i;
//			if (lastline < i)
//				lastline = i;
//			if (linelength < l)
//				linelength = l;
//		}
//	}
//	lines = (lastline - firstline) + 1;
//	M_Background(linelength * 8 + 16, lines * 8 + 16);
//	if (!m_missingdata) //since this is a fallback menu for missing data, it is very hard to read with the box
//		M_DrawTextBox(0, 0, linelength, lines); //this is less obtrusive than hacking up the M_DrawTextBox function
//	for (i = 0, l = firstline;i < lines;i++, l++)
//		M_Print(8 + 4 * (linelength - strlen(m_quit_message[l])), 8 + 8 * i, m_quit_message[l]);
//}


