/*
	Copyright (C) 1996-1997  Id Software, Inc.

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

	See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to:

		Free Software Foundation, Inc.
		59 Temple Place - Suite 330
		Boston, MA  02111-1307, USA
*/

#include "quakedef.h"
#include "cl_video.h"
#include "utf8lib.h"
#include "csprogs.h"

#pragma message ("Seek input from non-US keyboard users on tilde always closing console.  Quakespasm does it, so it is ok right?")
cvar_t con_closeontoggleconsole = {CF_CLIENT | CF_ARCHIVE, "con_closeontoggleconsole","2", "allows toggleconsole binds to close the console as well; when set to 2, this even works when not at the start of the line in console input; when set to 3, this works even if the toggleconsole key is the color tag [Zircon default]"};  // Baker r8193

/*
key up events are sent even if in console mode
*/

char		key_line[MAX_INPUTLINE];
int			key_linepos;
qbool	key_insert = true;	// insert key toggle (for editing)
keydest_t	key_dest;
int			key_consoleactive;
char		*keybindings[MAX_BINDMAPS][MAX_KEYS];

int			history_line;
char		history_savedline[MAX_INPUTLINE];
char		history_searchstring[MAX_INPUTLINE];
qbool	history_matchfound = false;
conbuffer_t history;

extern cvar_t	con_textsize;


#include "keys_history.c.h"

// key modifier states
#define KM_NONE           (!keydown[K_CTRL] && !keydown[K_SHIFT] && !keydown[K_ALT])
#define KM_CTRL_SHIFT_ALT ( keydown[K_CTRL] &&  keydown[K_SHIFT] &&  keydown[K_ALT])
#define KM_CTRL_SHIFT     ( keydown[K_CTRL] &&  keydown[K_SHIFT] && !keydown[K_ALT])
#define KM_CTRL_ALT       ( keydown[K_CTRL] && !keydown[K_SHIFT] &&  keydown[K_ALT])
#define KM_SHIFT_ALT      (!keydown[K_CTRL] &&  keydown[K_SHIFT] &&  keydown[K_ALT])
#define KM_CTRL           ( keydown[K_CTRL] && !keydown[K_SHIFT] && !keydown[K_ALT])
#define KM_SHIFT          (!keydown[K_CTRL] &&  keydown[K_SHIFT] && !keydown[K_ALT])
#define KM_ALT            (!keydown[K_CTRL] && !keydown[K_SHIFT] &&  keydown[K_ALT])

/*
====================
Interactive line editing and console scrollback
====================
*/

signed char chat_mode; // 0 for say, 1 for say_team, -1 for command
char chat_buffer[MAX_INPUTLINE];
int chat_bufferpos = 0;

int Key_AddChar(int unicode, qbool is_console)
{
	char *line;
	char buf[16];
	int len, blen, linepos;

	if (is_console)
	{
		line = key_line;
		linepos = key_linepos;
	}
	else
	{
		line = chat_buffer;
		linepos = chat_bufferpos;
	}

	if (linepos >= MAX_INPUTLINE-1)
		return linepos;

	blen = u8_fromchar(unicode, buf, sizeof(buf));
	if (!blen)
		return linepos;
	len = (int)strlen(&line[linepos]);
	// check insert mode, or always insert if at end of line
	if (key_insert || len == 0)
	{
		if (linepos + len + blen >= MAX_INPUTLINE)
			return linepos;
		// can't use strcpy to move string to right
		len++;
		if (linepos + blen + len >= MAX_INPUTLINE)
			return linepos;
		memmove(&line[linepos + blen], &line[linepos], len);
	}
	else if (linepos + len + blen - u8_bytelen(line + linepos, 1) >= MAX_INPUTLINE)
		return linepos;
	memcpy(line + linepos, buf, blen);
	if (blen > len)
		line[linepos + blen] = 0;
	linepos += blen;
	return linepos;
}

// returns -1 if no key has been recognized
// returns linepos (>= 0) otherwise
// if is_console is true can modify key_line (doesn't change key_linepos)
int Key_Parse_CommonKeys(cmd_state_t *cmd, qbool is_console, int key, int unicode)
{
	char *line;
	int linepos, linestart;
	unsigned int linesize;
	if (is_console)
	{
		line = key_line;
		linepos = key_linepos;
		linesize = sizeof(key_line);
		linestart = 1;
	}
	else
	{
		line = chat_buffer;
		linepos = chat_bufferpos;
		linesize = sizeof(chat_buffer);
		linestart = 0;
	}

	if ((key == 'v' && KM_CTRL) || ((key == K_INSERT || key == K_KP_INSERT) && KM_SHIFT)) {
		char *cbd, *p;

		if (is_console) Partial_Reset_Undo_Selection_Reset (); // SEL/UNDO CTRL-V paste, reset partial 

		if ((cbd = Sys_GetClipboardData()) != 0) {
			int i;
#if 1
			p = cbd;
			while (*p)
			{
				if (*p == '\r' && *(p+1) == '\n')
				{
					*p++ = ';';
					*p++ = ' ';
				}
				else if (*p == '\n' || *p == '\r' || *p == '\b')
					*p++ = ';';
				else
					p++;
			}
#else
			strtok(cbd, "\n\r\b");
#endif
			i = (int)strlen(cbd);
			if (i + linepos >= MAX_INPUTLINE)
				i= MAX_INPUTLINE - linepos - 1;
			if (i > 0) {
				cbd[i] = 0;
				memmove(line + linepos + i, line + linepos, linesize - linepos - i);
				memcpy(line + linepos, cbd, i);
				linepos += i;
			}
			Z_Free(cbd);
		} // CLIPBOARD DATA
		return linepos;
	} // CTRL-V PASTE

	if (key == 'u' && KM_CTRL) { // like vi/readline ^u: delete currently edited line
		return Key_ClearEditLine(is_console); // SEL/UNDO CTRL-U covered since it calls Key_ClearEditLine
	}

	if (key == K_TAB) {
		if (is_console && KM_CTRL) { // append the cvar value to the cvar name
			int		cvar_len, cvar_str_len, chars_to_move;
			char	k;
			char	cvar[MAX_INPUTLINE];
			const char *cvar_str;

			if (is_console) Partial_Reset_Undo_Selection_Reset (); // SEL/UNDO CTRL-TAB (cvar paste), reset partial 

			// go to the start of the variable
			while(--linepos)
			{
				k = line[linepos];
				if (k == '\"' || k == ';' || k == ' ' || k == '\'')
					break;
			}
			linepos++;

			// save the variable name in cvar
			for(cvar_len=0; (k = line[linepos + cvar_len]) != 0; cvar_len++)
			{
				if (k == '\"' || k == ';' || k == ' ' || k == '\'')
					break;
				cvar[cvar_len] = k;
			}
			if (cvar_len==0)
				return linepos;
			cvar[cvar_len] = 0;

			// go to the end of the cvar
			linepos += cvar_len;

			// save the content of the variable in cvar_str
			cvar_str = Cvar_VariableString(&cvars_all, cvar, CF_CLIENT | CF_SERVER);
			cvar_str_len = (int)strlen(cvar_str);
			if (cvar_str_len==0)
				return linepos;

			// insert space and cvar_str in line
			chars_to_move = (int)strlen(&line[linepos]);
			if (linepos + 1 + cvar_str_len + chars_to_move < MAX_INPUTLINE) {
				if (chars_to_move)
					memmove(&line[linepos + 1 + cvar_str_len], &line[linepos], chars_to_move);
				line[linepos++] = ' ';
				memcpy(&line[linepos], cvar_str, cvar_str_len);
				linepos += cvar_str_len;
				line[linepos + chars_to_move] = 0;
			}
			else
				Con_PrintLinef ("Couldn't append cvar value, edit line too long.");

			return linepos;
		} // CTRL + in_console .. append cvar value

		if (is_console) {
			// K_TAB
			return Con_CompleteCommandLine_Zircon (cmd, is_console, keydown[K_SHIFT], q_is_from_nothing_false);
		}

		if (KM_NONE) {
			return Con_CompleteCommandLine(cmd, is_console);
		}
	} // TAB

	// Advanced Console Editing by Radix radix@planetquake.com
	// Added/Modified by EvilTypeGuy eviltypeguy@qeradiant.com
	// Enhanced by [515]
	// Enhanced by terencehill

	// move cursor to the previous character
	if (key == K_LEFTARROW || key == K_KP_LEFTARROW) {
		if (is_console) Partial_Reset_Undo_Selection_Reset (); // SEL/UNDO leftarrow, reset partial 
		if (KM_CTRL) { // move cursor to the previous word
			int		pos;
			char	k;
			if (linepos <= linestart + 1)
				return linestart;
			pos = linepos;

			do {
				k = line[--pos];
				if (!(k == '\"' || k == ';' || k == ' ' || k == '\''))
					break;
			} while(pos > linestart); // skip all "; ' after the word

			if (pos == linestart)
				return linestart;

			do {
				k = line[--pos];
				if (k == '\"' || k == ';' || k == ' ' || k == '\'')
				{
					pos++;
					break;
				}
			} while(pos > linestart);

			linepos = pos;
			return linepos;
		} // KM_CTRL

		if (KM_SHIFT) { // move cursor to the previous character ignoring colors
			int		pos;
			size_t          inchar = 0;
			if (linepos <= linestart + 1)
				return linestart;
			pos = (int)u8_prevbyte(line + linestart, linepos - linestart) + linestart;
			while (pos > linestart)
				if (pos-1 >= linestart && line[pos-1] == STRING_COLOR_TAG && isdigit(line[pos]))
					pos-=2;
				else if (pos-4 >= linestart && line[pos-4] == STRING_COLOR_TAG && line[pos-3] == STRING_COLOR_RGB_TAG_CHAR
						&& isxdigit(line[pos-2]) && isxdigit(line[pos-1]) && isxdigit(line[pos]))
					pos-=5;
				else
				{
					if (pos-1 >= linestart && line[pos-1] == STRING_COLOR_TAG && line[pos] == STRING_COLOR_TAG) // consider ^^ as a character
						pos--;
					pos--;
					break;
				}
			if (pos < linestart)
				return linestart;

			// we need to move to the beginning of the character when in a wide character:
			u8_charidx(line, pos + 1, &inchar);
			linepos = (int)(pos + 1 - inchar);
			return linepos;
		} // KM_SHIFT

		if (KM_NONE) {
			if (linepos <= linestart + 1)
				return linestart;
			// hide ']' from u8_prevbyte otherwise it could go out of bounds
			linepos = (int)u8_prevbyte(line + linestart, linepos - linestart) + linestart;
			return linepos;
		}
	} // LEFT ARROW

	// delete char before cursor
	if ((key == K_BACKSPACE && KM_NONE) || (key == 'h' && KM_CTRL)) {
		if (is_console) Partial_Reset_Undo_Selection_Reset (); // SEL/UNDO backspace, reset partial 

		if (linepos > linestart) {
			// hide ']' from u8_prevbyte otherwise it could go out of bounds
			int newpos = (int)u8_prevbyte(line + linestart, linepos - linestart) + linestart;
			strlcpy(line + newpos, line + linepos, linesize + 1 - linepos);
			linepos = newpos;
		} // if
		return linepos;
	} // BACKSPACE

	// delete char on cursor
	if ((key == K_DELETE || key == K_KP_DELETE) && KM_NONE) {
		if (is_console) Partial_Reset_Undo_Selection_Reset (); // SEL/UNDO delete, reset partial 
		size_t linelen;
		linelen = strlen(line);
		if (linepos < (int)linelen)
			memmove(line + linepos, line + linepos + u8_bytelen(line + linepos, 1), linelen - linepos);
		return linepos;
	} // DELETE

	// move cursor to the next character
	if (key == K_RIGHTARROW || key == K_KP_RIGHTARROW) {
		if (is_console) Partial_Reset_Undo_Selection_Reset (); // SEL/UNDO rightarrow, reset partial 
		if (KM_CTRL) { // move cursor to the next word
			int		pos, len;
			char	k;
			len = (int)strlen(line);
			if (linepos >= len)
				return linepos;
			pos = linepos;

			while(++pos < len) {
				k = line[pos];
				if (k == '\"' || k == ';' || k == ' ' || k == '\'')
					break;
			} // while

			if (pos < len) { // skip all "; ' after the word
				while(++pos < len) {
					k = line[pos];
					if (!(k == '\"' || k == ';' || k == ' ' || k == '\''))
						break;
				} // while
			} // if
			linepos = pos;
			return linepos;
		} // KMCTRL

		if (KM_SHIFT) { // move cursor to the next character ignoring colors
			int		pos, len;
			len = (int)strlen(line);
			if (linepos >= len)
				return linepos;
			pos = linepos;

			// go beyond all initial consecutive color tags, if any
			if (pos < len) {
				while (line[pos] == STRING_COLOR_TAG) {
					if (isdigit(line[pos+1]))
						pos+=2;
					else if (line[pos+1] == STRING_COLOR_RGB_TAG_CHAR && isxdigit(line[pos+2]) && isxdigit(line[pos+3]) && isxdigit(line[pos+4]))
						pos+=5;
					else
						break;
				} // while
			} // if

			// skip the char
			if (line[pos] == STRING_COLOR_TAG && line[pos+1] == STRING_COLOR_TAG) // consider ^^ as a character
				pos++;
			pos += (int)u8_bytelen(line + pos, 1);

			// now go beyond all next consecutive color tags, if any
			if (pos < len) {
				while (line[pos] == STRING_COLOR_TAG) {
					if (isdigit(line[pos+1]))
						pos += 2;
					else if (line[pos+1] == STRING_COLOR_RGB_TAG_CHAR && isxdigit(line[pos+2]) && isxdigit(line[pos+3]) && isxdigit(line[pos+4]))
						pos += 5;
					else
						break;
				} // while
			} // if
			linepos = pos;
			return linepos;
		} // KM_SHIFT

		if (KM_NONE) {
			if (linepos >= (int)strlen(line))
				return linepos;
			linepos += (int)u8_bytelen(line + linepos, 1);
			return linepos;
		} // KM_NONE
	} // RIGHTARROW

	// Baker r0003: Thin cursor / no text overwrite mode	

	//if ((key == K_INSERT || key == K_KP_INSERT) && KM_NONE) // toggle insert mode
	//{
	//	key_insert ^= 1;
	//	return linepos;
	//}

	if (key == K_HOME || key == K_KP_HOME) {
		if (is_console && KM_CTRL) {
			con_backscroll = CON_TEXTSIZE;
			return linepos;
		}
		if (KM_NONE) {
			if (is_console) Partial_Reset_Undo_Selection_Reset (); // SEL/UNDO home that is not CTRL-HOME, reset partial 

			return linestart;
		}
	} // HOME

	if (key == K_END || key == K_KP_END) {
		if (is_console && KM_CTRL) {
			con_backscroll = 0;
			return linepos;
		}
		if (KM_NONE) {
			if (is_console) Partial_Reset_Undo_Selection_Reset (); // SEL/UNDO END that is not CTRL-END, reset partial 

			return (int)strlen(line);
		}
	} // END

	return -1;
}


static void
Key_Console(cmd_state_t *cmd, int key, int unicode)
{
	int linepos;

#if 1
	// Baker r0004: Ctrl + up/down size console like JoeQuake
	if (key == K_UPARROW && keydown[K_CTRL]) {
		Con_AdjustConsoleHeight(-0.05); // Baker: Decrease by 5%
		return;
	}

	if (key == K_DOWNARROW && keydown[K_CTRL]) {
		Con_AdjustConsoleHeight(+0.05); // Baker: Increase by 5%
		return;
	}
#endif

	key = Key_Convert_NumPadKey(key);

	// Forbid Ctrl Alt shortcuts since on Windows they are used to type some characters
	// in certain non-English keyboards using the AltGr key (which emulates Ctrl Alt)
	// Reference: "Why Ctrl+Alt shouldn't be used as a shortcut modifier"
	//            https://blogs.msdn.microsoft.com/oldnewthing/20040329-00/?p=40003
	if (keydown[K_CTRL] && keydown[K_ALT])
		goto add_char;

	linepos = Key_Parse_CommonKeys(cmd, true, key, unicode);
	if (linepos >= 0)
	{
		key_linepos = linepos;
		return;
	}

	if ((key == K_ENTER || key == K_KP_ENTER) && KM_NONE) {
		Cbuf_AddText (cmd, key_line+1);	// skip the ]
		Cbuf_AddText (cmd, "\n");
		Key_History_Push();
		key_linepos = Key_ClearEditLine(true); // SEL/UNDO ENTER --> does a cleareditline
		// force an update, because the command may take some time
		if (cls.state == ca_disconnected)
			CL_UpdateScreen ();
		return;
	}

	if (key == 'l' /* THIS L like LION, not a ONE */ && KM_CTRL) {
		Cbuf_AddTextLine (cmd, "clear");
		return;
	}

	if (key == 'q' && KM_CTRL) { // like zsh ^q: push line to history, don't execute, and clear
		// clear line
		Key_History_Push();
		key_linepos = Key_ClearEditLine(true);  // SEL/UNDO CTRL-Q covered since it calls Key_ClearEditLine

		return;
	}

	// End Advanced Console Editing

	if (((key == K_UPARROW || key == K_KP_UPARROW) && KM_NONE) || (key == 'p' && KM_CTRL)) {
		Partial_Reset_Undo_Selection_Reset (); // SEL/UNDO: Up arrow because changes the line
		Key_History_Up();
		return;
	}

	if (((key == K_DOWNARROW || key == K_KP_DOWNARROW) && KM_NONE) || (key == 'n' && KM_CTRL)) {
		Partial_Reset_Undo_Selection_Reset (); // SEL/UNDO: Down arrow history because changes the line
		Key_History_Down();
		return;
	}

	if (keydown[K_CTRL]) {
		// prints all the matching commands
		if (key == 'f' && KM_CTRL) {
			Key_History_Find_All();
			return;
		}
		// Search forwards/backwards, pointing the history's index to the
		// matching command but without fetching it to let one continue the search.
		// To fetch it, it suffices to just press UP or DOWN.
		if (key == 'r' && KM_CTRL_SHIFT) {
			Key_History_Find_Forwards();
			return;
		}
		if (key == 'r' && KM_CTRL) {
			Key_History_Find_Backwards();
			return;
		}

		// go to the last/first command of the history
		if (key == ',' && KM_CTRL) {
			Key_History_First();
			Partial_Reset_Undo_Selection_Reset (); // SEL/UNDO CTRL-COMMA sets current line
			return;
		}

		if (key == '.' && KM_CTRL) { // SEL/UNDO CTRL-DOT sets current line
			Key_History_Last();
			Partial_Reset_Undo_Selection_Reset (); // SEL/UNDO CTRL-COMMA sets current line
			return;
		}
	}

	if (key == K_PGUP || key == K_KP_PGUP) {
		if (KM_CTRL) {
			con_backscroll += ((vid_conheight.integer >> 2) / con_textsize.integer)-1;
			return;
		}

		if (KM_NONE) {
			con_backscroll += ((vid_conheight.integer >> 1) / con_textsize.integer)-3;
			return;
		}
	} // PGUP

	if (key == K_PGDN || key == K_KP_PGDN) {
		if (KM_CTRL) {
			con_backscroll -= ((vid_conheight.integer >> 2) / con_textsize.integer)-1;
			return;
		}

		if (KM_NONE) {
			con_backscroll -= ((vid_conheight.integer >> 1) / con_textsize.integer)-3;
			return;
		}
	} // PGDN

	if (key == K_MWHEELUP) {
		if (KM_CTRL) {
			con_backscroll += 1;
			return;
		}

		if (KM_SHIFT) {
			con_backscroll += ((vid_conheight.integer >> 2) / con_textsize.integer)-1;
			return;
		}
		
		if (KM_NONE) {
			con_backscroll += 5;
			return;
		}
	} // WHEELUP

	if (key == K_MWHEELDOWN) {
		if (KM_CTRL) {
			con_backscroll -= 1;
			return;
		}

		if (KM_SHIFT) {
			con_backscroll -= ((vid_conheight.integer >> 2) / con_textsize.integer)-1;
			return;
		}

		if (KM_NONE) {
			con_backscroll -= 5;
			return;
		}
	} // WHEELDOWN

	if (keydown[K_CTRL]) {
		// text zoom in
		if ((key == '+' || key == K_KP_PLUS) && KM_CTRL) {
			if (con_textsize.integer < 128)
				Cvar_SetValueQuick(&con_textsize, con_textsize.integer + 1);
			return;
		}
		// text zoom out
		if ((key == '-' || key == K_KP_MINUS) && KM_CTRL) {
			if (con_textsize.integer > 1)
				Cvar_SetValueQuick(&con_textsize, con_textsize.integer - 1);
			return;
		}
		// text zoom reset
		if ((key == '0' || key == K_KP_INSERT) && KM_CTRL) {
			Cvar_SetValueQuick(&con_textsize, atoi(Cvar_VariableDefString(&cvars_all, "con_textsize", CF_CLIENT | CF_SERVER)));
			return;
		}
	}

add_char:

	// non printable
	if (unicode < 32)
		return;

#if 1 // Baker: should reset autocomplete, almost anything that is not TAB or SHIFT-TAB should
	Partial_Reset (); //Con_Undo_Point (1, key == SPACE_CHAR_32);
#endif

	key_linepos = Key_AddChar(unicode, true);
}

//============================================================================

static void
Key_Message (cmd_state_t *cmd, int key, int ascii)
{
	int linepos;
	char vabuf[1024];

	key = Key_Convert_NumPadKey(key);

	if (key == K_ENTER || key == K_KP_ENTER || ascii == 10 || ascii == 13)
	{
		if (chat_mode < 0)
			Cmd_ExecuteString(cmd, chat_buffer, src_local, true); // not Cbuf_AddText to allow semiclons in args; however, this allows no variables then. Use aliases!
		else
			CL_ForwardToServer(va(vabuf, sizeof(vabuf), "%s %s", chat_mode ? "say_team" : "say ", chat_buffer));

		key_dest = key_game;
		chat_bufferpos = Key_ClearEditLine(false);
		return;
	}

	if (key == K_ESCAPE) {
		key_dest = key_game;
		chat_bufferpos = Key_ClearEditLine(false);
		return;
	}

	linepos = Key_Parse_CommonKeys(cmd, false, key, ascii);
	if (linepos >= 0)
	{
		chat_bufferpos = linepos;
		return;
	}

	// ctrl+key generates an ascii value < 32 and shows a char from the charmap
	if (ascii > 0 && ascii < 32 && utf8_enable.integer)
		ascii = 0xE000 + ascii;

	if (!ascii)
		return;							// non printable

	chat_bufferpos = Key_AddChar(ascii, false);
}

//============================================================================


/*
===================
Returns a key number to be used to index keybindings[] by looking at
the given string.  Single ascii characters return themselves, while
the K_* names are matched up.
===================
*/
int
Key_StringToKeynum (const char *str)
{
	Uchar ch;
	const keyname_t  *kn;

	if (!str || !str[0])
		return -1;
	if (!str[1])
		return tolower(str[0]);

	for (kn = keynames; kn->name; kn++) {
		if (String_Does_Match_Caseless (str, kn->name))
			return kn->keynum;
	}

	// non-ascii keys are Unicode codepoints, so give the character if it's valid;
	// error message have more than one character, don't allow it
	ch = u8_getnchar(str, &str, 3);
	return (ch == 0 || *str != 0) ? -1 : (int)ch;
}

/*
===================
Returns a string (either a single ascii char, or a K_* name) for the
given keynum.
FIXME: handle quote special (general escape sequence?)
===================
*/
const char *
Key_KeynumToString (int keynum, char *tinystr, size_t tinystrlength)
{
	const keyname_t  *kn;

	// -1 is an invalid code
	if (keynum < 0)
		return "<KEY NOT FOUND>";

	// search overrides first, because some characters are special
	for (kn = keynames; kn->name; kn++)
		if (keynum == kn->keynum)
			return kn->name;

	// if it is printable, output it as a single character
	if (keynum > 32)
	{
		u8_fromchar(keynum, tinystr, tinystrlength);
		return tinystr;
	}

	// if it is not overridden and not printable, we don't know what to do with it
	return "<UNKNOWN KEYNUM>";
}


qbool
Key_SetBinding (int keynum, int bindmap, const char *binding)
{
	char *newbinding;
	size_t l;

	if (keynum == -1 || keynum >= MAX_KEYS)
		return false;
	if ((bindmap < 0) || (bindmap >= MAX_BINDMAPS))
		return false;

// free old bindings
	if (keybindings[bindmap][keynum]) {
		Z_Free (keybindings[bindmap][keynum]);
		keybindings[bindmap][keynum] = NULL;
	}
	if (!binding[0]) // make "" binds be removed --blub
		return true;
// allocate memory for new binding
	l = strlen (binding);
	newbinding = (char *)Z_Malloc (l + 1);
	memcpy (newbinding, binding, l + 1);
	newbinding[l] = 0;
	keybindings[bindmap][keynum] = newbinding;
	return true;
}

void Key_GetBindMap(int *fg, int *bg)
{
	if (fg)
		*fg = key_bmap;
	if (bg)
		*bg = key_bmap2;
}

qbool Key_SetBindMap(int fg, int bg)
{
	if (fg >= MAX_BINDMAPS)
		return false;
	if (bg >= MAX_BINDMAPS)
		return false;
	if (fg >= 0)
		key_bmap = fg;
	if (bg >= 0)
		key_bmap2 = bg;
	return true;
}

static void
Key_In_Unbind_f(cmd_state_t *cmd)
{
	int         b, m;
	char *errchar = NULL;

	if (Cmd_Argc (cmd) != 3) {
		Con_Print("in_unbind <bindmap> <key> : remove commands from a key\n");
		return;
	}

	m = strtol(Cmd_Argv(cmd, 1), &errchar, 0);
	if ((m < 0) || (m >= MAX_BINDMAPS) || (errchar && *errchar)) {
		Con_Printf ("%s isn't a valid bindmap\n", Cmd_Argv(cmd, 1));
		return;
	}

	b = Key_StringToKeynum (Cmd_Argv(cmd, 2));
	if (b == -1) {
		Con_Printf ("\"%s\" isn't a valid key\n", Cmd_Argv(cmd, 2));
		return;
	}

	if (!Key_SetBinding (b, m, ""))
		Con_Printf ("Key_SetBinding failed for unknown reason\n");
}

static void
Key_In_Bind_f(cmd_state_t *cmd)
{
	int         i, c, b, m;
	char        line[MAX_INPUTLINE];
	char *errchar = NULL;

	c = Cmd_Argc (cmd);

	if (c != 3 && c != 4) {
		Con_Print("in_bind <bindmap> <key> [command] : attach a command to a key\n");
		return;
	}

	m = strtol(Cmd_Argv(cmd, 1), &errchar, 0);
	if ((m < 0) || (m >= MAX_BINDMAPS) || (errchar && *errchar)) {
		Con_Printf ("%s isn't a valid bindmap\n", Cmd_Argv(cmd, 1));
		return;
	}

	b = Key_StringToKeynum (Cmd_Argv(cmd, 2));
	if (b == -1 || b >= MAX_KEYS) {
		Con_Printf ("\"%s\" isn't a valid key\n", Cmd_Argv(cmd, 2));
		return;
	}

	if (c == 3) {
		if (keybindings[m][b])
			Con_Printf ("\"%s\" = \"%s\"\n", Cmd_Argv(cmd, 2), keybindings[m][b]);
		else
			Con_Printf ("\"%s\" is not bound\n", Cmd_Argv(cmd, 2));
		return;
	}
// copy the rest of the command line
	line[0] = 0;							// start out with a null string
	for (i = 3; i < c; i++) {
		strlcat (line, Cmd_Argv(cmd, i), sizeof (line));
		if (i != (c - 1))
			strlcat (line, " ", sizeof (line));
	}

	if (!Key_SetBinding (b, m, line))
		Con_Printf ("Key_SetBinding failed for unknown reason\n");
}

static void
Key_In_Bindmap_f(cmd_state_t *cmd)
{
	int         m1, m2, c;
	char *errchar = NULL;

	c = Cmd_Argc (cmd);

	if (c != 3) {
		Con_Print("in_bindmap <bindmap> <fallback>: set current bindmap and fallback\n");
		return;
	}

	m1 = strtol(Cmd_Argv(cmd, 1), &errchar, 0);
	if ((m1 < 0) || (m1 >= MAX_BINDMAPS) || (errchar && *errchar)) {
		Con_Printf ("%s isn't a valid bindmap\n", Cmd_Argv(cmd, 1));
		return;
	}

	m2 = strtol(Cmd_Argv(cmd, 2), &errchar, 0);
	if ((m2 < 0) || (m2 >= MAX_BINDMAPS) || (errchar && *errchar)) {
		Con_Printf ("%s isn't a valid bindmap\n", Cmd_Argv(cmd, 2));
		return;
	}

	key_bmap = m1;
	key_bmap2 = m2;
}

static void
Key_Unbind_f(cmd_state_t *cmd)
{
	int         b;

	if (Cmd_Argc (cmd) != 2) {
		Con_Print("unbind <key> : remove commands from a key\n");
		return;
	}

	b = Key_StringToKeynum (Cmd_Argv(cmd, 1));
	if (b == -1) {
		Con_Printf ("\"%s\" isn't a valid key\n", Cmd_Argv(cmd, 1));
		return;
	}

	if (!Key_SetBinding (b, 0, ""))
		Con_Printf ("Key_SetBinding failed for unknown reason\n");
}

static void
Key_Unbindall_f(cmd_state_t *cmd)
{
	int         i, j;

	for (j = 0; j < MAX_BINDMAPS; j++)
		for (i = 0; i < (int)(sizeof(keybindings[0])/sizeof(keybindings[0][0])); i++)
			if (keybindings[j][i])
				Key_SetBinding (i, j, "");
}

static void
Key_PrintBindList(int j)
{
	char bindbuf[MAX_INPUTLINE];
	char tinystr[TINYSTR_LEN_4];
	const char *p;
	int i;

	for (i = 0; i < (int)(sizeof(keybindings[0])/sizeof(keybindings[0][0])); i++)
	{
		p = keybindings[j][i];
		if (p)
		{
			Cmd_QuoteString(bindbuf, sizeof(bindbuf), p, "\"\\", false);
			if (j == 0)
				Con_Printf ("^3%s ^7= \"%s\"\n", Key_KeynumToString (i, tinystr, TINYSTR_LEN_4), bindbuf);
			else
				Con_PrintLinef (CON_BRONZE "bindmap %d: "CON_BRONZE "%s " CON_WHITE "= " QUOTED_S, j, Key_KeynumToString (i, tinystr, TINYSTR_LEN_4), bindbuf);
		}
	}
}

static void
Key_In_BindList_f(cmd_state_t *cmd)
{
	int m;
	char *errchar = NULL;

	if (Cmd_Argc(cmd) >= 2)
	{
		m = strtol(Cmd_Argv(cmd, 1), &errchar, 0);
		if ((m < 0) || (m >= MAX_BINDMAPS) || (errchar && *errchar)) {
			Con_PrintLinef ("%s isn't a valid bindmap", Cmd_Argv(cmd, 1));
			return;
		}
		Key_PrintBindList(m);
	}
	else
	{
		for (m = 0; m < MAX_BINDMAPS; m++)
			Key_PrintBindList(m);
	}
}

static void
Key_BindList_f(cmd_state_t *cmd)
{
	Key_PrintBindList(0);
}

static void
Key_Bind_f(cmd_state_t *cmd)
{
	int         i, c, b;
	char        line[MAX_INPUTLINE];

	c = Cmd_Argc (cmd);

	if (c != 2 && c != 3) {
		Con_Print("bind <key> [command] : attach a command to a key\n");
		return;
	}
	b = Key_StringToKeynum (Cmd_Argv(cmd, 1));
	if (b == -1 || b >= MAX_KEYS) {
		Con_Printf ("\"%s\" isn't a valid key\n", Cmd_Argv(cmd, 1));
		return;
	}

	if (c == 2) {
		if (keybindings[0][b])
			Con_Printf ("\"%s\" = \"%s\"\n", Cmd_Argv(cmd, 1), keybindings[0][b]);
		else
			Con_Printf ("\"%s\" is not bound\n", Cmd_Argv(cmd, 1));
		return;
	}
// copy the rest of the command line
	line[0] = 0;							// start out with a null string
	for (i = 2; i < c; i++) {
		strlcat (line, Cmd_Argv(cmd, i), sizeof (line));
		if (i != (c - 1))
			strlcat (line, " ", sizeof (line));
	}

	if (!Key_SetBinding (b, 0, line))
		Con_Printf ("Key_SetBinding failed for unknown reason\n");
}

/*
============
Writes lines containing "bind key value"
============
*/
void
Key_WriteBindings (qfile_t *f)
{
	int         i, j;
	char bindbuf[MAX_INPUTLINE];
	char tinystr[TINYSTR_LEN_4];
	const char *p;

	// Override default binds
	FS_Printf(f, "unbindall\n");

	for (j = 0; j < MAX_BINDMAPS; j++)
	{
		for (i = 0; i < (int)(sizeof(keybindings[0])/sizeof(keybindings[0][0])); i++)
		{
			p = keybindings[j][i];
			if (p)
			{
				Cmd_QuoteString(bindbuf, sizeof(bindbuf), p, "\"\\", false); // don't need to escape $ because cvars are not expanded inside bind
				if (j == 0)
					FS_Printf(f, "bind %s \"%s\"\n", Key_KeynumToString (i, tinystr, TINYSTR_LEN_4), bindbuf);
				else
					FS_Printf(f, "in_bind %d %s \"%s\"\n", j, Key_KeynumToString (i, tinystr, TINYSTR_LEN_4), bindbuf);
			}
		}
	}
}


void
Key_Init (void)
{
	Key_History_Init();
	key_linepos = Key_ClearEditLine(true);

//
// register our functions
//
	Cmd_AddCommand(CF_CLIENT, "in_bind", Key_In_Bind_f, "binds a command to the specified key in the selected bindmap");
	Cmd_AddCommand(CF_CLIENT, "in_unbind", Key_In_Unbind_f, "removes command on the specified key in the selected bindmap");
	Cmd_AddCommand(CF_CLIENT, "in_bindlist", Key_In_BindList_f, "bindlist: displays bound keys for all bindmaps, or the given bindmap");
	Cmd_AddCommand(CF_CLIENT, "in_bindmap", Key_In_Bindmap_f, "selects active foreground and background (used only if a key is not bound in the foreground) bindmaps for typing");
	Cmd_AddCommand(CF_CLIENT, "in_releaseall", Key_ReleaseAll_f, "releases all currently pressed keys (debug command)");

	Cmd_AddCommand(CF_CLIENT, "bind", Key_Bind_f, "binds a command to the specified key in bindmap 0");
	Cmd_AddCommand(CF_CLIENT, "unbind", Key_Unbind_f, "removes a command on the specified key in bindmap 0");
	Cmd_AddCommand(CF_CLIENT, "bindlist", Key_BindList_f, "bindlist: displays bound keys for bindmap 0 bindmaps");
	Cmd_AddCommand(CF_CLIENT, "unbindall", Key_Unbindall_f, "removes all commands from all keys in all bindmaps (leaving only shift-escape and escape)");

	Cmd_AddCommand(CF_CLIENT, "history", Key_History_f, "prints the history of executed commands (history X prints the last X entries, history -c clears the whole history)");

	Cvar_RegisterVariable (&con_closeontoggleconsole);
}

void
Key_Shutdown (void)
{
	Key_History_Shutdown();
}

const char *Key_GetBind (int key, int bindmap)
{
	const char *bind;
	if (key < 0 || key >= MAX_KEYS)
		return NULL;
	if (bindmap >= MAX_BINDMAPS)
		return NULL;
	if (bindmap >= 0)
	{
		bind = keybindings[bindmap][key];
	}
	else
	{
		bind = keybindings[key_bmap][key];
		if (!bind)
			bind = keybindings[key_bmap2][key];
	}
	return bind;
}

void Key_FindKeysForCommand (const char *command, int *keys, int numkeys, int bindmap)
{
	int		count;
	int		j;
	const char	*b;

	for (j = 0;j < numkeys;j++)
		keys[j] = -1;

	if (bindmap >= MAX_BINDMAPS)
		return;

	count = 0;

	for (j = 0; j < MAX_KEYS; ++j)
	{
		b = Key_GetBind(j, bindmap);
		if (!b)
			continue;
		if (String_Does_Match (b, command) )
		{
			keys[count++] = j;
			if (count == numkeys)
				break;
		}
	}
}

/*
===================
Called by the system between frames for both key up and key down events
Should NOT be called during an interrupt!
===================
*/
static char tbl_keyascii[MAX_KEYS];
static keydest_t tbl_keydest[MAX_KEYS];

typedef struct eventqueueitem_s
{
	int key;
	int ascii;
	qbool down;
}
eventqueueitem_t;
static int events_blocked = 0;
static eventqueueitem_t eventqueue[32];
unsigned eventqueue_idx = 0;

// a helper to simulate release of ALL keys
void Key_ReleaseAll(void)
{
	extern kbutton_t	in_mlook, in_klook;
	extern kbutton_t	in_left, in_right, in_forward, in_back;
	extern kbutton_t	in_lookup, in_lookdown, in_moveleft, in_moveright;
	extern kbutton_t	in_strafe, in_speed, in_jump, in_attack, in_use;
	extern kbutton_t	in_up, in_down;
	// LadyHavoc: added 6 new buttons
	extern kbutton_t	in_button3, in_button4, in_button5, in_button6, in_button7, in_button8;
	//even more
	extern kbutton_t	in_button9, in_button10, in_button11, in_button12, in_button13, in_button14, in_button15, in_button16;


#define ClearBtnState(x) \
	if (x.state) { \
		memset (&x, 0, sizeof(x)); \
	} // Ender

	Con_DPrintLinef ("Key_ReleaseAll");
	int key;
	// clear the event queue first
	eventqueue_idx = 0;
	// then send all down events (possibly into the event queue)
	for (key = 0; key < MAX_KEYS; ++key) {
		// Baker: What if keydown[key] > 1?
		if (keydown[key])
			Key_Event(/*scancode*/ key, /*ascii*/ 0, /*isdown*/ false);
		if (developer.integer > 0 && keydown[key]) {
			Con_DPrintLinef ("Key_ReleaseAll key %d is still down", key);
		}

	}
	// now all keys are guaranteed up (once the event queue is unblocked)
	// and only future events count

	ClearBtnState(in_mlook);
	ClearBtnState(in_klook);
	ClearBtnState(in_left);
	ClearBtnState(in_right);
	ClearBtnState(in_moveleft);
	ClearBtnState(in_moveright);
	ClearBtnState(in_forward);
	ClearBtnState(in_back);
	ClearBtnState(in_lookup);
	ClearBtnState(in_lookdown);
	ClearBtnState(in_strafe);
	ClearBtnState(in_speed);
	ClearBtnState(in_jump);
	ClearBtnState(in_attack);
	ClearBtnState(in_use);
	ClearBtnState(in_button3);
	ClearBtnState(in_button4);
	ClearBtnState(in_button5);
	ClearBtnState(in_button6);
	ClearBtnState(in_button7);
	ClearBtnState(in_button8);
	ClearBtnState(in_button9);
	ClearBtnState(in_button10);
	ClearBtnState(in_button11);
	ClearBtnState(in_button12);
	ClearBtnState(in_button13);
	ClearBtnState(in_button14);
	ClearBtnState(in_button15);
	ClearBtnState(in_button16);

}

void Key_ReleaseAll_f(cmd_state_t* cmd)
{
	Key_ReleaseAll();
}



static void Key_EventQueue_Add(int key, int ascii, qbool down)
{
	if (eventqueue_idx < sizeof(eventqueue) / sizeof(*eventqueue))
	{
		eventqueue[eventqueue_idx].key = key;
		eventqueue[eventqueue_idx].ascii = ascii;
		eventqueue[eventqueue_idx].down = down;
		++eventqueue_idx;
	}
}

void Key_EventQueue_Block(void)
{
	// block key events until call to Unblock
	events_blocked = true;
}

void Key_EventQueue_Unblock(void)
{
	// unblocks key events again
	unsigned i;
	events_blocked = false;
	for(i = 0; i < eventqueue_idx; ++i)
		Key_Event(eventqueue[i].key, eventqueue[i].ascii, eventqueue[i].down);
	eventqueue_idx = 0;
}

// Baker r0001 - ALT-ENTER support

qbool ignore_enter_up = false; // Baker 2000

void VID_Alt_Enter_f (void) // Baker: ALT-ENTER
{
#ifdef __ANDROID__
	Con_PrintLinef ("vid_restart not supported for this build");

	return;
#endif // __ANDROID__

	if (vid.fullscreen) {
		// Full-screen to window
		Cvar_SetValueQuick (&vid_width, vid_window_width.integer);
		Cvar_SetValueQuick (&vid_height, vid_window_height.integer);
		Cvar_SetValueQuick (&vid_fullscreen, 0);
	}
	else {
		Cvar_SetValueQuick (&vid_width, vid_fullscreen_width.integer);
		Cvar_SetValueQuick (&vid_height, vid_fullscreen_height.integer);
		Cvar_SetValueQuick (&vid_fullscreen, 1);
	}
	Cbuf_AddText (cmd_local, "vid_restart" NEWLINE);
}


void
Key_Event (int key, int ascii, qbool down)
{
	cmd_state_t *cmd = cmd_local;
	const char *bind;
	qbool q;
	keydest_t keydest = key_dest;
	char vabuf[1024];

	if (key < 0 || key >= MAX_KEYS)
		return;

	if (events_blocked)
	{
		Key_EventQueue_Add(key, ascii, down);
		return;
	}

// Baker r0001 - ALT-ENTER support
#if 1 // ALT-ENTER
	if (key == K_ENTER) {
		if (keydown[K_ALT] && down) { // Baker 2000
			VID_Alt_Enter_f();
			ignore_enter_up = true;
			return; // Didn't happen!
		}
		else if (ignore_enter_up && !down)
		{
			ignore_enter_up = false;
			if (!down) return; // Didn't happen!
		}
	}
#endif

	// get key binding
	bind = keybindings[key_bmap][key];
	if (!bind)
		bind = keybindings[key_bmap2][key];

	if (developer_insane.integer)
		Con_DPrintf ("Key_Event(%d, '%c', %s) keydown %d bind \"%s\"\n", key, ascii ? ascii : '?', down ? "down" : "up", keydown[key], bind ? bind : "");

	if (key_consoleactive)
		keydest = key_console;

	if (down)
	{
		// increment key repeat count each time a down is received so that things
		// which want to ignore key repeat can ignore it
		keydown[key] = min(keydown[key] + 1, 2);
		if (keydown[key] == 1) {
			tbl_keyascii[key] = ascii;
			tbl_keydest[key] = keydest;
		} else {
			ascii = tbl_keyascii[key];
			keydest = tbl_keydest[key];
		}
	}
	else
	{
		// clear repeat count now that the key is released
		keydown[key] = 0;
		keydest = tbl_keydest[key];
		ascii = tbl_keyascii[key];
	}

	if (keydest == key_void)
		return;

	// key_consoleactive is a flag not a key_dest because the console is a
	// high priority overlay ontop of the normal screen (designed as a safety
	// feature so that developers and users can rescue themselves from a bad
	// situation).
	//
	// this also means that toggling the console on/off does not lose the old
	// key_dest state

	// specially handle escape (togglemenu) and shift-escape (toggleconsole)
	// engine bindings, these are not handled as normal binds so that the user
	// can recover from a completely empty bindmap
	if (key == K_ESCAPE)
	{
		// ignore key repeats on escape
		if (keydown[key] > 1)
			return;

		// escape does these things:
		// key_consoleactive - close console
		// key_message - abort messagemode
		// key_menu - go to parent menu (or key_game)
		// key_game - open menu

		// in all modes shift-escape toggles console
		if (keydown[K_SHIFT])
		{
			if (down)
			{
				Con_ToggleConsole (); // Baker: SHIFT-ESC toggle of console.
				tbl_keydest[key] = key_void; // esc release should go nowhere (especially not to key_menu or key_game)
			}
			return;
		}

		switch (keydest)
		{
			case key_console:
				if (down) {
					if (Have_Flag (key_consoleactive, KEY_CONSOLEACTIVE_FORCED_4)) {
						Flag_Remove_From (key_consoleactive, KEY_CONSOLEACTIVE_USER_1);
#ifdef CONFIG_MENU
						MR_ToggleMenu(1);
#endif
					}
					else
						Con_ToggleConsole (); // Baker: We are in the console, a key is pressed and the console is not forced (we are connected)
											  //  So the user has the console open and now we close it.
				}
				break;

			case key_message:
				if (down)
					Key_Message (cmd, key, ascii); // that'll close the message input
				break;

			case key_menu:
			case key_menu_grabbed:
#ifdef CONFIG_MENU
				MR_KeyEvent (key, ascii, down);
#endif
				break;

			case key_game:
				// csqc has priority over toggle menu if it wants to (e.g. handling escape for UI stuff in-game.. :sick:)
				q = CL_VM_InputEvent(down ? 0 : 1, key, ascii);
#ifdef CONFIG_MENU
				if (!q && down)
					MR_ToggleMenu(1);
#endif
				break;

			default:
				Con_Printf ("Key_Event: Bad key_dest\n");
		}
		return;
	}

	// send function keydowns to interpreter no matter what mode is (unless the menu has specifically grabbed the keyboard, for rebinding keys)
	// VorteX: Omnicide does bind F* keys
	if (keydest != key_menu_grabbed)
	if (key >= K_F1 && key <= K_F12 && gamemode != GAME_BLOODOMNICIDE)
	{
		if (bind)
		{
			if (keydown[key] == 1 && down)
			{
				// button commands add keynum as a parm
				if (bind[0] == '+')
					Cbuf_InsertText(cmd, va(vabuf, sizeof(vabuf), "%s %d\n", bind, key));
				else
					Cbuf_InsertText(cmd, bind);
			}
			else if (bind[0] == '+' && !down && keydown[key] == 0)
				Cbuf_InsertText(cmd, va(vabuf, sizeof(vabuf), "-%s %d\n", bind + 1, key));
		}
		return;
	}

#if 1
	// Baker: If we are disconnected or fully connected open menu (don't open menu during signon process -- but does that achieve that?  And does it matter?).
	if (down && 
		isin2(key, K_MOUSE1, K_MOUSE2) && 
		(keydest == key_console || (keydest == key_game && cls.demoplayback)) &&
		(cls.state != ca_connected ||
		(cls.state == ca_connected && cls.signon == SIGNONS))) {

		WARP_X_(M_ToggleMenu)
		if (Have_Flag(key_consoleactive, KEY_CONSOLEACTIVE_USER_1)) // conexit
			Con_ToggleConsole();

		WARP_X_ (M_ToggleMenu) // Goes to M_ToggleMenu without CSQC 
		MR_ToggleMenu(1);
		return;
	}
#endif



	// send input to console if it wants it
	if (keydest == key_console)
	{
		if (!down)
			return;
		// con_closeontoggleconsole enables toggleconsole keys to close the
		// console, as long as they are not the color prefix character
		// (special exemption for german keyboard layouts)
		if (con_closeontoggleconsole.integer && 
			bind && 
			String_Does_Start_With (bind, "toggleconsole") && 
			Have_Flag (key_consoleactive, KEY_CONSOLEACTIVE_USER_1) && 
			(con_closeontoggleconsole.integer >= ((ascii != STRING_COLOR_TAG) ? 2 : 3) || key_linepos == 1))
		{
			Con_ToggleConsole (); // Baker: This is high priority toggle console, right?
			return;
		}

		if (Sys_CheckParm ("-noconsole"))
			return; // only allow the key bind to turn off console

		Key_Console (cmd, key, ascii);
		return;
	}

	// handle toggleconsole in menu too
	if (keydest == key_menu)
	{
		if (down && con_closeontoggleconsole.integer && bind && !strncmp(bind, "toggleconsole", strlen("toggleconsole")) && ascii != STRING_COLOR_TAG)
		{
			Cbuf_InsertText(cmd, "toggleconsole\n");  // Deferred to next frame so we're not sending the text event to the console.
			tbl_keydest[key] = key_void; // key release should go nowhere (especially not to key_menu or key_game)
			return;
		}
	}

	// ignore binds while a video is played, let the video system handle the key event
	if (cl_videoplaying)
	{
		if (gamemode == GAME_BLOODOMNICIDE) // menu controls key events
#ifdef CONFIG_MENU
			MR_KeyEvent(key, ascii, down);
#else
			{
			}
#endif
		else
			CL_Video_KeyEvent (key, ascii, keydown[key] != 0);
		return;
	}

	// anything else is a key press into the game, chat line, or menu
	switch (keydest)
	{
		case key_message:
			if (down)
				Key_Message (cmd, key, ascii);
			break;
		case key_menu:
		case key_menu_grabbed:
#ifdef CONFIG_MENU
			MR_KeyEvent (key, ascii, down);
#endif
			break;
		case key_game:
			q = CL_VM_InputEvent(down ? 0 : 1, key, ascii);
			// ignore key repeats on binds and only send the bind if the event hasnt been already processed by csqc
			if (!q && bind)
			{
				if (keydown[key] == 1 && down)
				{
					// button commands add keynum as a parm
					if (bind[0] == '+')
						Cbuf_InsertText(cmd, va(vabuf, sizeof(vabuf), "%s %d\n", bind, key));
					else
						Cbuf_InsertText(cmd, bind);
				}
				else if (bind[0] == '+' && !down && keydown[key] == 0)
					Cbuf_InsertText(cmd, va(vabuf, sizeof(vabuf), "-%s %d\n", bind + 1, key));
			}
			break;
		default:
			Con_Printf ("Key_Event: Bad key_dest\n");
	}
}

