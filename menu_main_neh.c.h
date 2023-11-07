// menu_main_neh.c.h

			switch (NehGameType)
			{
			case TYPE_BOTH:
				switch (m_main_cursor)
				{
				case 0:
					M_Menu_SinglePlayer_f (cmd);
					break;

				case 1:
					M_Menu_Demos_f (cmd);
					break;

				case 2:
					M_Menu_MultiPlayer_f (cmd);
					break;

				case 3:
					M_Menu_Options_Classic_f (cmd);
					break;

				case 4:
					key_dest = key_game;
					if (sv.active)
						Cbuf_AddTextLine (cmd, "disconnect");
					Cbuf_AddTextLine (cmd, "playdemo endcred");
					break;

				case 5:
					M_Menu_Quit_f (cmd);
					break;
				}
				break;
			case TYPE_GAME:
				switch (m_main_cursor)
				{
				case 0:
					M_Menu_SinglePlayer_f (cmd);
					break;

				case 1:
					M_Menu_MultiPlayer_f (cmd);
					break;

				case 2:
					M_Menu_Options_Classic_f (cmd);
					break;

				case 3:
					key_dest = key_game;
					if (sv.active)
						Cbuf_AddTextLine (cmd, "disconnect");
					Cbuf_AddTextLine (cmd, "playdemo endcred");
					break;

				case 4:
					M_Menu_Quit_f (cmd);
					break;
				}
				break;
			case TYPE_DEMO:
				switch (m_main_cursor)
				{
				case 0:
					M_Menu_Demos_f (cmd);
					break;

				case 1:
					key_dest = key_game;
					if (sv.active)
						Cbuf_AddTextLine (cmd, "disconnect");
					Cbuf_AddTextLine (cmd, "playdemo endcred");
					break;

				case 2:
					M_Menu_Options_Classic_f (cmd);
					break;

				case 3:
					M_Menu_Quit_f (cmd);
					break;
				}
				break;
			} // sw