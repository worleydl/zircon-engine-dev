/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// cmd.c -- Quake script command processing module

#include "quakedef.h"
#include "thread.h"

cmd_state_t *cmd_local;
cmd_state_t *cmd_serverfromclient;

cmd_userdefined_t cmd_userdefined_all;
cmd_userdefined_t cmd_userdefined_null;

typedef struct cmd_iter_s {
	cmd_state_t *cmd;
}
cmd_iter_t;

static cmd_iter_t *cmd_iter_all;

mempool_t *cbuf_mempool;

// we only run the +whatever commandline arguments once
qbool host_stuffcmdsrun = false;

//=============================================================================

void Cbuf_Lock(cmd_buf_t *cbuf)
{
	Thread_LockMutex(cbuf->lock);
}

void Cbuf_Unlock(cmd_buf_t *cbuf)
{
	Thread_UnlockMutex(cbuf->lock);
}


/*
============
Cmd_Wait_f

Causes execution of the remainder of the command buffer to be delayed until
next frame.  This allows commands like:
bind g "impulse 5 ; +attack ; wait ; -attack ; impulse 2"
============
*/
static void Cmd_Wait_f (cmd_state_t *cmd)
{
	cmd->cbuf->wait = true;
}

/*
============
Cmd_Defer_f

Cause a command to be executed after a delay.
============
*/
static void Cbuf_ParseText(cmd_state_t *cmd, llist_t *head, cmd_input_t *existing, const char *text, qbool allowpending);
static void Cbuf_LinkString(cmd_state_t *cmd, llist_t *head, cmd_input_t *existing, const char *text, qbool leavepending, unsigned int cmdsize);
static void Cmd_Defer_f (cmd_state_t *cmd)
{
	cmd_input_t *current;
	cmd_buf_t *cbuf = cmd->cbuf;
	unsigned int cmdsize;

	if (Cmd_Argc(cmd) == 1)
	{
		if (List_Is_Empty(&cbuf->deferred))
			Con_PrintLinef ("No commands are pending.");
		else
		{
			List_For_Each_Entry(current, &cbuf->deferred, cmd_input_t, list)
				Con_PrintLinef ("-> In %9.2f: %s", current->delay, current->text);
		}
	} else if (Cmd_Argc(cmd) == 2 && String_Does_Match_Caseless("clear", Cmd_Argv(cmd, 1)))
	{
		while(!List_Is_Empty(&cbuf->deferred))
		{
			cbuf->size -= List_Entry(cbuf->deferred.next, cmd_input_t, list)->length;
			List_Move_Tail(cbuf->deferred.next, &cbuf->free);
		}
	}
	else if (Cmd_Argc(cmd) == 3 && (cmdsize = (unsigned int)strlen(Cmd_Argv(cmd, 2))) )
	{
		Cbuf_Lock(cbuf);

		Cbuf_LinkString(cmd, &cbuf->deferred, NULL, Cmd_Argv(cmd, 2), false, cmdsize);
		List_Entry(cbuf->deferred.prev, cmd_input_t, list)->delay = atof(Cmd_Argv(cmd, 1));

		Cbuf_Unlock(cbuf);
	}
	else
	{
		Con_Printf ("usage: defer <seconds> <command>\n"
			   "       defer clear\n");
		return;
	}
}

/*
============
Cmd_Centerprint_f

Print something to the center of the screen using SCR_Centerprint
============
*/
static void Cmd_Centerprint_f (cmd_state_t *cmd)
{
	char msg[MAX_INPUTLINE_16384];
	unsigned int i, c, p;
	c = Cmd_Argc(cmd);
	if (c >= 2)
	{
		strlcpy(msg, Cmd_Argv(cmd,1), sizeof(msg));
		for(i = 2; i < c; ++i)
		{
			strlcat(msg, " ", sizeof(msg));
			strlcat(msg, Cmd_Argv(cmd, i), sizeof(msg));
		}
		c = (unsigned int)strlen(msg);
		for(p = 0, i = 0; i < c; ++i)
		{
			if (msg[i] == '\\')
			{
				if (msg[i+1] == 'n')
					msg[p++] = '\n';
				else if (msg[i+1] == '\\')
					msg[p++] = '\\';
				else {
					msg[p++] = '\\';
					msg[p++] = msg[i+1];
				}
				++i;
			} else {
				msg[p++] = msg[i];
			}
		}
		msg[p] = '\0';
		SCR_CenterPrint(msg);
	}
}

/*
=============================================================================

						COMMAND BUFFER

 * The Quake command-line is super basic. It can be entered in the console
 * or in config files. A semicolon is used to terminate a command and chain
 * them together. Otherwise, a newline delineates command input.
 *
 * In most engines, the Quake command-line is a simple linear text buffer that
 * is parsed when it executes. In Darkplaces, we use a linked list of command
 * input and parse the input on the spot.
 *
 * This was done because Darkplaces allows multiple command interpreters on the
 * same thread. Previously, each interpreter maintained its own buffer and this
 * caused problems related to execution order, and maintaining a single simple
 * buffer for all interpreters makes it non-trivial to keep track of which
 * command should execute on which interpreter.

=============================================================================
*/

/*
============
Cbuf_NodeGet

Returns an existing buffer node for appending or reuse, or allocates a new one
============
*/
static cmd_input_t *Cbuf_NodeGet(cmd_buf_t *cbuf, cmd_input_t *existing)
{
	cmd_input_t *node;
	if (existing && existing->pending)
		node = existing;
	else if (!List_Is_Empty(&cbuf->free))
	{
		node = List_Entry(cbuf->free.next, cmd_input_t, list);
		node->length = node->pending = 0;
	}
	else
	{
		node = (cmd_input_t *)Mem_Alloc(cbuf_mempool, sizeof(cmd_input_t));
		node->list.prev = node->list.next = &node->list;
		node->size = node->length = node->pending = 0;
	}
	return node;
}

/*
============
Cbuf_LinkString

Copies a command string into a buffer node
============
*/
static void Cbuf_LinkString(cmd_state_t *cmd, llist_t *head, cmd_input_t *existing, const char *text, qbool leavepending, unsigned int cmdsize)
{
	cmd_buf_t *cbuf = cmd->cbuf;
	cmd_input_t *node = Cbuf_NodeGet(cbuf, existing);
	unsigned int offset = (unsigned int) node->length; // > 0 if (pending)

	// node will match existing if its text was pending continuation
	if (node != existing)
	{
		node->source = cmd;
		List_Move_Tail(&node->list, head);
	}

	node->length += cmdsize;
	if (node->size < node->length)
	{
		node->text = (char *)Mem_Realloc(cbuf_mempool, node->text, node->length + 1);
		node->size = node->length;
	}
	cbuf->size += cmdsize;

	strlcpy(&node->text[offset], text, cmdsize + 1); // always sets the last char to \0
	//Con_Printf ("^5Cbuf_LinkString(): %s `^7%s^5`\n", node->pending ? "append" : "new", &node->text[offset]);
	node->pending = leavepending;
}

/*
============
Cbuf_ParseText

Parses text to isolate command strings for linking into the buffer
separators: \n \r or unquoted and uncommented ';'
============
*/
static void Cbuf_ParseText(cmd_state_t *cmd, llist_t *head, cmd_input_t *existing, const char *text, qbool allowpending)
{
	unsigned int cmdsize = 0, start = 0, pos;
	qbool quotes = false, comment = false;

	for (pos = 0; text[pos]; ++pos)
	{
		switch(text[pos])
		{
			case ';':
				if (comment || quotes)
					break;
			case '\r':
			case '\n':
				comment = false;
				quotes = false; // matches div0-stable
				if (cmdsize)
				{
					Cbuf_LinkString(cmd, head, existing, &text[start], false, cmdsize);
					cmdsize = 0;
				}
				else if (existing && existing->pending) // all I got was this lousy \n
					existing->pending = false;
				continue; // don't increment cmdsize

			case '/':
				if (!quotes && text[pos + 1] == '/' && (pos == 0 || ISWHITESPACE(text[pos - 1])))
					comment = true;
				break;
			case '"':
				if (!comment && (pos == 0 || text[pos - 1] != '\\'))
					quotes = !quotes;
				break;
		}

		if (!comment)
		{
			if (!cmdsize)
				start = pos;
			++cmdsize;
		}
	}

	if (cmdsize) // the line didn't end yet but we do have a string
		Cbuf_LinkString(cmd, head, existing, &text[start], allowpending, cmdsize);
}

/*
============
Cbuf_AddText

Adds command text at the end of the buffer
============
*/
void Cbuf_AddText (cmd_state_t *cmd, const char *text)
{
	size_t l = strlen(text);
	cmd_buf_t *cbuf = cmd->cbuf;
	llist_t llist = {&llist, &llist};

	Cbuf_Lock(cbuf);

	if (cbuf->maxsize - cbuf->size <= l)
		Con_PrintLinef ("Cbuf_AddText: overflow");
	else
	{
		// If the string terminates but the (last) line doesn't, the node will be left in the pending state (to be continued).
		Cbuf_ParseText(cmd, &llist, (List_Is_Empty(&cbuf->start) ? NULL : List_Entry(cbuf->start.prev, cmd_input_t, list)), text, true);
		List_Splice_Tail(&llist, &cbuf->start);
	}
	Cbuf_Unlock(cbuf);
}

void Cbuf_AddTextLine (cmd_state_t *cmd, const char *text)
{
	char vabuf[16384];
	c_dpsnprintf1 (vabuf, "%s" NEWLINE, text);

	Cbuf_AddText (cmd, vabuf);
}

WARP_X_ (CL_ForwardToServerf)
void Cbuf_AddTextLinef (cmd_state_t *cmd, const char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAX_INPUTLINE_16384];

	va_start		(argptr, fmt);
	dpvsnprintf		(msg, sizeof(msg), fmt, argptr);
	va_end			(argptr);

	Cbuf_AddTextLine (cmd, msg);
}


/*
============
Cbuf_InsertText

Adds command text immediately after the current command
============
*/
void Cbuf_InsertText (cmd_state_t *cmd, const char *text)
{
	cmd_buf_t *cbuf = cmd->cbuf;
	llist_t llist = {&llist, &llist};
	size_t l = strlen(text);

	Cbuf_Lock(cbuf);

	if (cbuf->size + l >= cbuf->maxsize)
		Con_PrintLinef ("Cbuf_InsertText: overflow");
	else
	{
		// bones_was_here assertion: when prepending to the buffer it never makes sense to leave node(s) in the `pending` state,
		// it would have been impossible to append to such text later in the old raw text buffer,
		// and allowing it causes bugs when .cfg files lack \n at EOF (see: https://gitlab.com/xonotic/darkplaces/-/issues/378).
		Cbuf_ParseText(cmd, &llist, (List_Is_Empty(&cbuf->start) ? NULL : List_Entry(cbuf->start.next, cmd_input_t, list)), text, false);
		List_Splice(&llist, &cbuf->start);
	}

	Cbuf_Unlock(cbuf);
}

/*
============
Cbuf_Execute_Deferred --blub
============
*/
static void Cbuf_Execute_Deferred (cmd_buf_t *cbuf)
{
	cmd_input_t *current, *n;
	vec_t eat;

	if (host.realtime - cbuf->deferred_oldtime < 0 || host.realtime - cbuf->deferred_oldtime > 1800)
		cbuf->deferred_oldtime = host.realtime;
	eat = host.realtime - cbuf->deferred_oldtime;
	if (eat < 1/128)
		return;
	cbuf->deferred_oldtime = host.realtime;

	List_For_Each_Entry_Safe(current, n, &cbuf->deferred, cmd_input_t, list)
	{
		current->delay -= eat;
		if (current->delay <= 0)
		{
			Cbuf_AddText(current->source, current->text); // parse deferred string and append its cmdstring(s)
			List_Entry(cbuf->start.prev, cmd_input_t, list)->pending = false; // faster than div0-stable's Cbuf_AddText(";\n");
			List_Move_Tail(&current->list, &cbuf->free); // make deferred string memory available for reuse
			cbuf->size -= current->length;
		}
	}
}

/*
============
Cbuf_Execute
============
*/
extern qbool prvm_runawaycheck;
static qbool Cmd_PreprocessString(cmd_state_t *cmd, const char *intext, char *outtext, unsigned maxoutlen, cmd_alias_t *alias );
void Cbuf_Execute (cmd_buf_t *cbuf)
{
	cmd_input_t *current;
	char preprocessed[MAX_INPUTLINE_16384];
	char *firstchar;
	unsigned int i = 0;

	// LadyHavoc: making sure the tokenizebuffer doesn't get filled up by repeated crashes
	cbuf->tokenizebufferpos = 0;

	while (!List_Is_Empty(&cbuf->start))
	{
		/*
		 * Delete the text from the command buffer and move remaining
		 * commands down. This is necessary because commands (exec, alias)
		 * can insert data at the beginning of the text buffer
		 */
		current = List_Entry(cbuf->start.next, cmd_input_t, list);

		// Recycle memory so using WASD doesn't cause a malloc and free
		List_Move_Tail(&current->list, &cbuf->free);

		/*
		 * Assume we're rolling with the current command-line and
		 * always set this false because alias expansion or cbuf insertion
		 * without a newline may set this true, and cause weirdness.
		 */
		current->pending = false;

		cbuf->size -= current->length;

		firstchar = current->text;

		// Baker: Advance past white space
		while (*firstchar && ISWHITESPACE(*firstchar))
			firstchar ++;
		if (
		  (String_Does_NOT_Start_With_Caseless_PRE (firstchar, "alias"/*, 5*/)   || !ISWHITESPACE(firstchar[5])) &&
		   (String_Does_NOT_Start_With_Caseless_PRE (firstchar, "bind"/*, 4*/)    || !ISWHITESPACE(firstchar[4])) &&
		   (String_Does_NOT_Start_With_Caseless_PRE (firstchar, "in_bind"/*, 7*/) || !ISWHITESPACE(firstchar[7])))
		{
			if (Cmd_PreprocessString(current->source, current->text, preprocessed, sizeof(preprocessed), NULL )) {
				Cmd_ExecuteString(current->source, preprocessed, src_local, /*lockmutex ?*/ false);
			}
		}
		else
		{
			Cmd_ExecuteString (current->source, current->text, src_local, /*lockmutex ?*/ false);
		}

		current = NULL;

		if (cbuf->wait)
		{
			/*
			 * Skip out while text still remains in
			 * buffer, leaving it for next frame
			 */
			cbuf->wait = false;
			break;
		}

		if (++i == 1000000 && prvm_runawaycheck)
		{
			Con_Printf (CON_WARN "Cbuf_Execute: runaway loop counter hit limit of %d commands, clearing command buffers!\n", i);
			while (!List_Is_Empty(&cbuf->start))
				List_Move_Tail(cbuf->start.next, &cbuf->free);
			while (!List_Is_Empty(&cbuf->deferred))
				List_Move_Tail(cbuf->deferred.next, &cbuf->free);
			cbuf->size = 0;
		}
	}
}

/*
===================
Cbuf_Frame_Input

Add them exactly as if they had been typed at the console
===================
*/
static void Cbuf_Frame_Input(void)
{
	char *line;

#ifdef _WIN32
	if (cls.state != ca_dedicated)
		return;

	while ((line = Sys_ConsoleInput_WinQuake())) // CONSOLUS
		Cbuf_AddText(cmd_local, line);
#else
	while ((line = Sys_ConsoleInput())) // CONSOLUS
		Cbuf_AddText(cmd_local, line);
#endif
}

void Cbuf_Frame(cmd_buf_t *cbuf)
{
	// check for commands typed to the host
	Cbuf_Frame_Input();

//	R_TimeReport("preconsole");

	// execute commands queued with the defer command
	Cbuf_Execute_Deferred(cbuf);
	if (cbuf->size)
	{
		SV_LockThreadMutex();
		Cbuf_Execute(cbuf);
		SV_UnlockThreadMutex();
	}

//	R_TimeReport("console");
}

/*
==============================================================================

						SCRIPT COMMANDS

==============================================================================
*/

/*
===============
Cmd_StuffCmds_f

Adds command line parameters as script statements
Commands lead with a +, and continue until a - or another +
quake +prog jctest.qp +cmd amlev1
quake -nosound +cmd amlev1
===============
*/
static void Cmd_StuffCmds_f (cmd_state_t *cmd)
{
	int		i, j, l;
	// this is for all commandline options combined (and is bounds checked)
	char	build[MAX_INPUTLINE_16384];

	if (Cmd_Argc (cmd) != 1)
	{
		Con_Print("stuffcmds : execute command line parameters\n");
		return;
	}

	// no reason to run the commandline arguments twice
	if (host_stuffcmdsrun)
		return;

	host_stuffcmdsrun = true;
	build[0] = 0;
	l = 0;
	for (i = 0;i < sys.argc;i++)
	{
		if (sys.argv[i] && sys.argv[i][0] == '+' && (sys.argv[i][1] < '0' || sys.argv[i][1] > '9') && l + strlen(sys.argv[i]) - 1 <= sizeof(build) - 1)
		{
			j = 1;
			while (sys.argv[i][j])
				build[l++] = sys.argv[i][j++];
			i++;
			for (;i < sys.argc;i++)
			{
				if (!sys.argv[i])
					continue;
				if ((sys.argv[i][0] == '+' || sys.argv[i][0] == '-') && (sys.argv[i][1] < '0' || sys.argv[i][1] > '9'))
					break;
				if (l + strlen(sys.argv[i]) + 4 > sizeof(build) - 1)
					break;
				build[l++] = ' ';
				if (strchr(sys.argv[i], ' '))
					build[l++] = '\"';
				for (j = 0;sys.argv[i][j];j++)
					build[l++] = sys.argv[i][j];
				if (strchr(sys.argv[i], ' '))
					build[l++] = '\"';
			}
			build[l++] = '\n';
			i--;
		}
	}
	// now terminate the combined string and prepend it to the command buffer
	// we already reserved space for the terminator
	build[l++] = 0;
	Cbuf_InsertText (cmd, build);
}

void DarkPlaces_Settings_For_Game (cmd_state_t *cmd, int egamemode)
{
	// special defaults for specific games go here, these execute before default.cfg
	// Nehahra pushable crates malfunction in some levels if this is on
	// Nehahra NPC AI is confused by blowupfallenzombies
	switch (egamemode)
	{
	case GAME_NORMAL:
		Cbuf_InsertText (cmd, "\n"
"gl_texturemode gl_linear_mipmap_linear\n" // Baker r9066 ..[Zircon]
"sv_gameplayfix_blowupfallenzombies 0\n"
"sv_gameplayfix_findradiusdistancetobox 0\n"
"sv_gameplayfix_grenadebouncedownslopes 0\n"
"sv_gameplayfix_slidemoveprojectiles 0\n"
"sv_gameplayfix_upwardvelocityclearsongroundflag 0\n"
"sv_gameplayfix_setmodelrealbox 0\n"
"sv_gameplayfix_droptofloorstartsolid 1\n" // Baker r9065: physics correction probably
"sv_gameplayfix_droptofloorstartsolid_nudgetocorrect 0\n"
"sv_gameplayfix_noairborncorpse 0\n"
"sv_gameplayfix_noairborncorpse_allowsuspendeditems 0\n"
"sv_gameplayfix_easierwaterjump 0\n"
"sv_gameplayfix_delayprojectiles 0\n"
"sv_gameplayfix_multiplethinksperframe 0\n"
"sv_gameplayfix_fixedcheckwatertransition 0\n"
"sv_gameplayfix_q1bsptracelinereportstexture 0\n"
"sv_gameplayfix_swiminbmodels 0\n"
"sv_gameplayfix_downtracesupportsongroundflag 0\n"
"sys_ticrate 0.01388889\n"
"r_shadow_gloss 1\n"
"r_shadow_bumpscale_basetexture 0\n"
"csqc_polygons_defaultmaterial_nocullface 0\n"
			);
		break;
	case GAME_NEHAHRA:
		Cbuf_InsertText(cmd, "\n"
"gl_texturemode gl_linear_mipmap_linear\n"
"sv_gameplayfix_blowupfallenzombies 0\n"
"sv_gameplayfix_findradiusdistancetobox 0\n"
"sv_gameplayfix_grenadebouncedownslopes 0\n"
"sv_gameplayfix_slidemoveprojectiles 0\n"
"sv_gameplayfix_upwardvelocityclearsongroundflag 0\n"
"sv_gameplayfix_setmodelrealbox 0\n"
"sv_gameplayfix_droptofloorstartsolid 1\n"
"sv_gameplayfix_droptofloorstartsolid_nudgetocorrect 0\n"
"sv_gameplayfix_noairborncorpse 0\n"
"sv_gameplayfix_noairborncorpse_allowsuspendeditems 0\n"
"sv_gameplayfix_easierwaterjump 0\n"
"sv_gameplayfix_delayprojectiles 0\n"
"sv_gameplayfix_multiplethinksperframe 0\n"
"sv_gameplayfix_fixedcheckwatertransition 0\n"
"sv_gameplayfix_q1bsptracelinereportstexture 0\n"
"sv_gameplayfix_swiminbmodels 0\n"
"sv_gameplayfix_downtracesupportsongroundflag 0\n"
"sys_ticrate 0.01388889\n"
"r_shadow_gloss 1\n"
"r_shadow_bumpscale_basetexture 0\n"
"csqc_polygons_defaultmaterial_nocullface 0\n"
			);
		break;
	// hipnotic mission pack has issues in their 'friendly monster' ai, which seem to attempt to attack themselves for some reason when findradius() returns non-solid entities.
	// hipnotic mission pack has issues with bobbing water entities 'jittering' between different heights on alternate frames at the default 0.0138889 ticrate, 0.02 avoids this issue
	// hipnotic mission pack has issues in their proximity mine sticking code, which causes them to bounce off.
	case GAME_HIPNOTIC:
	case GAME_QUOTH:
		Cbuf_InsertText(cmd, "\n"
"gl_texturemode gl_linear_mipmap_linear\n"
"sv_gameplayfix_blowupfallenzombies 0\n"
"sv_gameplayfix_findradiusdistancetobox 0\n"
"sv_gameplayfix_grenadebouncedownslopes 0\n"
"sv_gameplayfix_slidemoveprojectiles 0\n"
"sv_gameplayfix_upwardvelocityclearsongroundflag 0\n"
"sv_gameplayfix_setmodelrealbox 0\n"
"sv_gameplayfix_droptofloorstartsolid 1\n"
"sv_gameplayfix_droptofloorstartsolid_nudgetocorrect 0\n"
"sv_gameplayfix_noairborncorpse 0\n"
"sv_gameplayfix_noairborncorpse_allowsuspendeditems 0\n"
"sv_gameplayfix_easierwaterjump 0\n"
"sv_gameplayfix_delayprojectiles 0\n"
"sv_gameplayfix_multiplethinksperframe 0\n"
"sv_gameplayfix_fixedcheckwatertransition 0\n"
"sv_gameplayfix_q1bsptracelinereportstexture 0\n"
"sv_gameplayfix_swiminbmodels 0\n"
"sv_gameplayfix_downtracesupportsongroundflag 0\n"
"sys_ticrate 0.02\n"
"r_shadow_gloss 1\n"
"r_shadow_bumpscale_basetexture 0\n"
"csqc_polygons_defaultmaterial_nocullface 0\n"
			);
		break;
	// rogue mission pack has a guardian boss that does not wake up if findradius returns one of the entities around its spawn area
	case GAME_ROGUE:
		Cbuf_InsertText(cmd, "\n"
"gl_texturemode gl_linear_mipmap_linear\n"
"sv_gameplayfix_blowupfallenzombies 0\n"
"sv_gameplayfix_findradiusdistancetobox 0\n"
"sv_gameplayfix_grenadebouncedownslopes 0\n"
"sv_gameplayfix_slidemoveprojectiles 0\n"
"sv_gameplayfix_upwardvelocityclearsongroundflag 0\n"
"sv_gameplayfix_setmodelrealbox 0\n"
"sv_gameplayfix_droptofloorstartsolid 1\n"
"sv_gameplayfix_droptofloorstartsolid_nudgetocorrect 0\n"
"sv_gameplayfix_noairborncorpse 0\n"
"sv_gameplayfix_noairborncorpse_allowsuspendeditems 0\n"
"sv_gameplayfix_easierwaterjump 0\n"
"sv_gameplayfix_delayprojectiles 0\n"
"sv_gameplayfix_multiplethinksperframe 0\n"
"sv_gameplayfix_fixedcheckwatertransition 0\n"
"sv_gameplayfix_q1bsptracelinereportstexture 0\n"
"sv_gameplayfix_swiminbmodels 0\n"
"sv_gameplayfix_downtracesupportsongroundflag 0\n"
"sys_ticrate 0.01388889\n"
"r_shadow_gloss 1\n"
"r_shadow_bumpscale_basetexture 0\n"
"csqc_polygons_defaultmaterial_nocullface 0\n"
			);
		break;
	case GAME_TENEBRAE:
		Cbuf_InsertText(cmd, "\n"
"gl_texturemode gl_linear_mipmap_linear\n"
"sv_gameplayfix_blowupfallenzombies 0\n"
"sv_gameplayfix_findradiusdistancetobox 0\n"
"sv_gameplayfix_grenadebouncedownslopes 0\n"
"sv_gameplayfix_slidemoveprojectiles 0\n"
"sv_gameplayfix_upwardvelocityclearsongroundflag 0\n"
"sv_gameplayfix_setmodelrealbox 0\n"
"sv_gameplayfix_droptofloorstartsolid 1\n"
"sv_gameplayfix_droptofloorstartsolid_nudgetocorrect 0\n"
"sv_gameplayfix_noairborncorpse 0\n"
"sv_gameplayfix_noairborncorpse_allowsuspendeditems 0\n"
"sv_gameplayfix_easierwaterjump 0\n"
"sv_gameplayfix_delayprojectiles 0\n"
"sv_gameplayfix_multiplethinksperframe 0\n"
"sv_gameplayfix_fixedcheckwatertransition 0\n"
"sv_gameplayfix_q1bsptracelinereportstexture 0\n"
"sv_gameplayfix_swiminbmodels 0\n"
"sv_gameplayfix_downtracesupportsongroundflag 0\n"
"sys_ticrate 0.01388889\n"
"r_shadow_gloss 2\n"
"r_shadow_bumpscale_basetexture 4\n"
"csqc_polygons_defaultmaterial_nocullface 0\n"
			);
		break;
	case GAME_NEXUIZ:
		Cbuf_InsertText(cmd, "\n"
"gl_texturemode gl_linear_mipmap_linear\n"
"sv_gameplayfix_blowupfallenzombies 1\n"
"sv_gameplayfix_findradiusdistancetobox 1\n"
"sv_gameplayfix_grenadebouncedownslopes 1\n"
"sv_gameplayfix_slidemoveprojectiles 1\n"
"sv_gameplayfix_upwardvelocityclearsongroundflag 1\n"
"sv_gameplayfix_setmodelrealbox 1\n"
"sv_gameplayfix_droptofloorstartsolid 1\n"
"sv_gameplayfix_droptofloorstartsolid_nudgetocorrect 1\n"
"sv_gameplayfix_noairborncorpse 1\n"
"sv_gameplayfix_noairborncorpse_allowsuspendeditems 1\n"
"sv_gameplayfix_easierwaterjump 1\n"
"sv_gameplayfix_delayprojectiles 1\n"
"sv_gameplayfix_multiplethinksperframe 1\n"
"sv_gameplayfix_fixedcheckwatertransition 1\n"
"sv_gameplayfix_q1bsptracelinereportstexture 1\n"
"sv_gameplayfix_swiminbmodels 1\n"
"sv_gameplayfix_downtracesupportsongroundflag 1\n"
"sys_ticrate 0.01388889\n"
"sv_gameplayfix_q2airaccelerate 1\n"
"sv_gameplayfix_stepmultipletimes 1\n"
"csqc_polygons_defaultmaterial_nocullface 1\n"
"con_chatsound_team_mask 13\n"
			);
		break;
	case GAME_XONOTIC:
	case GAME_VORETOURNAMENT:
		// compatibility for versions prior to 2020-05-25, this can be overridden in newer versions to get the default behavior and be consistent with FTEQW engine
		Cbuf_InsertText(cmd, "\n"
"csqc_polygons_defaultmaterial_nocullface 1\n"
"con_chatsound_team_mask 13\n"
"sv_gameplayfix_customstats 1\n"
"mod_q1bsp_zero_hullsize_cutoff 8.03125\n"
			);
		break;
	// Steel Storm: Burning Retribution csqc misinterprets CSQC_InputEvent if type is a value other than 0 or 1
	case GAME_STEELSTORM:
		Cbuf_InsertText(cmd, "\n"
"gl_texturemode gl_linear_mipmap_linear\n"
"sv_gameplayfix_blowupfallenzombies 1\n"
"sv_gameplayfix_findradiusdistancetobox 1\n"
"sv_gameplayfix_grenadebouncedownslopes 1\n"
"sv_gameplayfix_slidemoveprojectiles 1\n"
"sv_gameplayfix_upwardvelocityclearsongroundflag 1\n"
"sv_gameplayfix_setmodelrealbox 1\n"
"sv_gameplayfix_droptofloorstartsolid 1\n"
"sv_gameplayfix_droptofloorstartsolid_nudgetocorrect 1\n"
"sv_gameplayfix_noairborncorpse 1\n"
"sv_gameplayfix_noairborncorpse_allowsuspendeditems 1\n"
"sv_gameplayfix_easierwaterjump 1\n"
"sv_gameplayfix_delayprojectiles 1\n"
"sv_gameplayfix_multiplethinksperframe 1\n"
"sv_gameplayfix_fixedcheckwatertransition 1\n"
"sv_gameplayfix_q1bsptracelinereportstexture 1\n"
"sv_gameplayfix_swiminbmodels 1\n"
"sv_gameplayfix_downtracesupportsongroundflag 1\n"
"sys_ticrate 0.01388889\n"
"cl_csqc_generatemousemoveevents 0\n"
"csqc_polygons_defaultmaterial_nocullface 1\n"
			);
		break;
	default:
		Cbuf_InsertText(cmd, "\n"
"gl_texturemode gl_linear_mipmap_linear\n"
"sv_gameplayfix_blowupfallenzombies 1\n"
"sv_gameplayfix_findradiusdistancetobox 1\n"
"sv_gameplayfix_grenadebouncedownslopes 1\n"
"sv_gameplayfix_slidemoveprojectiles 1\n"
"sv_gameplayfix_upwardvelocityclearsongroundflag 1\n"
"sv_gameplayfix_setmodelrealbox 1\n"
"sv_gameplayfix_droptofloorstartsolid 1\n"
"sv_gameplayfix_droptofloorstartsolid_nudgetocorrect 1\n"
"sv_gameplayfix_noairborncorpse 1\n"
"sv_gameplayfix_noairborncorpse_allowsuspendeditems 1\n"
"sv_gameplayfix_easierwaterjump 1\n"
"sv_gameplayfix_delayprojectiles 1\n"
"sv_gameplayfix_multiplethinksperframe 1\n"
"sv_gameplayfix_fixedcheckwatertransition 1\n"
"sv_gameplayfix_q1bsptracelinereportstexture 1\n"
"sv_gameplayfix_swiminbmodels 1\n"
"sv_gameplayfix_downtracesupportsongroundflag 1\n"
"sys_ticrate 0.01388889\n"
"csqc_polygons_defaultmaterial_nocullface 0\n"
			);
		break;
	} //sw
}

static unsigned char *MemAllocString (const char *s)
{
	mempool_t *pool = tempmempool;
	int			slen = strlen(s);

	unsigned char *buf = (unsigned char *)Mem_Alloc (pool, slen + ONE_CHAR_1);
	memcpy (buf, s, slen);
	buf[slen] = '\0';

	return buf;
}

static char *s_default_cfg =
	""																		NEWLINE
	"// load keybindings"													NEWLINE
	"//"																	NEWLINE
	"// commands with a leading + will also be called for key up events with"	NEWLINE
	"// the + changed to a -"												NEWLINE
	"unbindall"																NEWLINE
	""																		NEWLINE
	"//"																	NEWLINE
	"// character controls"													NEWLINE
	"//"																	NEWLINE
	""																		NEWLINE
	"bind	ALT				+strafe"										NEWLINE
	""																		NEWLINE
	"bind	,				\"impulse 10\""									NEWLINE
	"bind	.				\"impulse 12\""									NEWLINE
	"bind	PGUP			+lookup"										NEWLINE
	"bind	PGDN			+lookdown"										NEWLINE
	"bind	END				centerview"										NEWLINE
	"bind	w				+forward"										NEWLINE
	"bind	a				+moveleft"										NEWLINE
	"bind	s				+back"											NEWLINE
	"bind	d				+moveright"										NEWLINE
	"bind	q				+moveup"										NEWLINE
	"bind	e				+movedown"										NEWLINE
	"bind	SHIFT			+speed"											NEWLINE
	"bind	CTRL			+attack"										NEWLINE
	"bind	UPARROW			+forward"										NEWLINE
	"bind	DOWNARROW		+back"											NEWLINE
	"bind	LEFTARROW		+left"											NEWLINE
	"bind	RIGHTARROW		+right"											NEWLINE
	""																		NEWLINE
	"bind	SPACE			+jump "											NEWLINE
	"bind	ENTER			+jump"											NEWLINE
	""																		NEWLINE
	"bind	TAB				+showscores"									NEWLINE
	""																		NEWLINE
	"bind	1				\"impulse 1\""									NEWLINE
	"bind	2				\"impulse 2\""									NEWLINE
	"bind	3				\"impulse 3\""									NEWLINE
	"bind	4				\"impulse 4\""									NEWLINE
	"bind	5				\"impulse 5\""									NEWLINE
	"bind	6				\"impulse 6\""									NEWLINE
	"bind	7				\"impulse 7\""									NEWLINE
	"bind	8				\"impulse 8\""									NEWLINE
	""																		NEWLINE
	"bind	0				\"impulse 0\""									NEWLINE
	""																		NEWLINE
	"bind	/				\"impulse 10\"		// change weapon"			NEWLINE
	""																		NEWLINE
	"// zoom"																NEWLINE
	"alias zoom_in \"sensitivity 2;fov 90;wait;fov 70;wait;fov 50;wait;fov 30;wait;fov 10;wait;fov 5;bind F11 zoom_out\"" NEWLINE
	"alias zoom_out \"sensitivity 4;fov 5;wait;fov 10;wait;fov 30;wait;fov 50;wait;fov 70;wait;fov 90;bind F11 zoom_in; sensitivity 3\"" NEWLINE
	"bind F11 zoom_in"														NEWLINE
	""																		NEWLINE
	"// Function keys"														NEWLINE
	"bind	F1			\"help\""											NEWLINE
	"bind	F2			\"menu_save\""										NEWLINE
	"bind	F3			\"menu_load\""										NEWLINE
	"bind	F4			\"menu_options\""									NEWLINE
	"bind	F5			\"menu_multiplayer\""								NEWLINE
	"bind	F6			\"echo Quicksaving...; wait; save quick\""			NEWLINE
	"bind	F9			\"echo Quickloading...; wait; load quick\""			NEWLINE
	//"bind	F10			\"quit\""											NEWLINE
	"bind    F12                     \"screenshot\""						NEWLINE
	"" NEWLINE
	"// mouse options" NEWLINE
	//"bind	\			+mlook"												NEWLINE
	""																		NEWLINE
	"//"																	NEWLINE
	"// client environment commands"										NEWLINE
	"//"																	NEWLINE
	"bind	PAUSE	\"pause\""												NEWLINE
	"bind	ESCAPE	\"togglemenu\""											NEWLINE
	"bind	~		\"toggleconsole\""										NEWLINE
	"bind	`		\"toggleconsole\""										NEWLINE
	""																		NEWLINE
	"bind	t		\"messagemode\""										NEWLINE
	""																		NEWLINE
	"bind	+		\"sizeup\""												NEWLINE
	"bind	=		\"sizeup\""												NEWLINE
	"bind	-		\"sizedown\""											NEWLINE
	""																		NEWLINE
	"bind	INS		+klook"													NEWLINE
	""																		NEWLINE
	"//"																	NEWLINE
	"// mouse buttons"														NEWLINE
	"//"																	NEWLINE
	"bind	MOUSE1	+attack"												NEWLINE
	"bind	MOUSE2	+forward"												NEWLINE
	"bind	MOUSE3	+mlook"													NEWLINE
	""																		NEWLINE
	"//"																	NEWLINE
	"// default cvars"														NEWLINE
	"//"																	NEWLINE
	"viewsize 		100"													NEWLINE
	"gamma 			1.0"													NEWLINE
	"volume 		0.7"													NEWLINE
	"sensitivity 	9"														NEWLINE

#if 1
	"cl_forwardspeed 	400"												NEWLINE
	"cl_backspeed 		400"												NEWLINE
#endif

	""																		NEWLINE
	;

#if 0
static char *s_quake_rc =
		"// load the base configuration"					NEWLINE
		"exec default.cfg"									NEWLINE
		""													NEWLINE
		"// load the last saved configuration"				NEWLINE
		"exec config.cfg"									NEWLINE
		""													NEWLINE
		"// run a user script file if present"				NEWLINE
		"exec autoexec.cfg"									NEWLINE
		""													NEWLINE
		"//"												NEWLINE
		"// stuff command line statements"					NEWLINE
		"//"												NEWLINE
		"stuffcmds"											NEWLINE
		""													NEWLINE
		"// start demos if not allready running a server"	NEWLINE
		"startdemos demo1 demo2 demo3"						NEWLINE
	; // Ender
#endif

static void Cmd_Exec (cmd_state_t *cmd, const char *filename)
{
	char *f;
	//size_t filenameLen = strlen(filename);
	qbool is_default_cfg = String_Does_Match (filename, "default.cfg") ||
							String_Does_End_With(filename, "/default.cfg");
	// (filenameLen >= 12 && String_Does_Match(filename + filenameLen - 12, "/default.cfg"));

#if 0
	qbool is_quake_rc = String_Does_Match (filename, "quake.rc") ||
							String_Does_End_With(filename, "/quake.rc");
#endif

	if (String_Does_Match(filename, "config.cfg")) {
		filename = CONFIGFILENAME;
		if (Sys_CheckParm("-noconfig"))
			return; // don't execute config.cfg
	}

	f = (char *)FS_LoadFile (filename, tempmempool, fs_quiet_FALSE, fs_size_ptr_null);


	if (!f && fs_have_qex && is_default_cfg) {
		f = (char *)MemAllocString (s_default_cfg);
	}

	if (!f) {
		Con_PrintLinef ("couldn't exec %s", filename);
		return;
	}
	Con_PrintLinef ("execing %s", filename);

	// if executing default.cfg for the first time, lock the cvar defaults
	// it may seem backwards to insert this text BEFORE the default.cfg
	// but Cbuf_InsertText inserts before, so this actually ends up after it.
	if (is_default_cfg) {
		Cbuf_InsertText (cmd, NEWLINE "cvar_lockdefaults" NEWLINE);
	}

	Cbuf_InsertText (cmd, f);
	Mem_Free(f);

	if (is_default_cfg) {
		DarkPlaces_Settings_For_Game (cmd, gamemode);
	} // if
}

/*
===============
Cmd_Exec_f
===============
*/
static void Cmd_Exec_f (cmd_state_t *cmd)
{
	fssearch_t *s;

	if (Cmd_Argc(cmd) != 2) {
		Con_PrintLinef ("exec <filename> : execute a script file");
		return;
	}

	const char *s_execfile = Cmd_Argv(cmd, 1);
	s = FS_Search (s_execfile, fs_caseless_true, fs_quiet_true, fs_pakfile_null, fs_gamedironly_false);

	if (!s && String_Does_Match (s_execfile, "default.cfg") && fs_have_qex) {
		Cmd_Exec (cmd, s_execfile);
		goto default_cfg_qex_skip;
	}

	if (!s || !s->numfilenames) {
		Con_PrintLinef ("couldn't exec %s", s_execfile);
		return;
	}

	for (int j = 0; j < s->numfilenames; j ++) {
		const char *s_filename = s->filenames[j];
		Cmd_Exec (cmd, s_filename);
	}

default_cfg_qex_skip:
	FS_FreeSearch(s);
}


/*
===============
Cmd_Echo_f

Just prints the rest of the line to the console
===============
*/
static void Cmd_Echo_f (cmd_state_t *cmd)
{
	int		i;

	for (i=1 ; i<Cmd_Argc(cmd) ; i++)
		Con_Printf ("%s ",Cmd_Argv(cmd, i));
	Con_Print("\n");
}

// DRESK - 5/14/06
// Support Doom3-style Toggle Console Command
/*
===============
Cmd_Toggle_f

Toggles a specified console variable amongst the values specified (default is 0 and 1)
===============
*/

// Baker r3173: inc, dec, cvar_reset commands
static void Cmd_Inc_f(cmd_state_t *cmd)
{
	cvar_state_t *cvars = cmd->cvars;
	int numargs = Cmd_Argc(cmd);

	if (numargs == 1 || numargs > 3) {
		Con_PrintLinef ("Inc Console Variable - Usage" NEWLINE
			"  inc <variable> [amt] - increases variable");
		return;
	}

	cvar_t *cvCVar = Cvar_FindVar(cvars, Cmd_Argv(cmd, 1), ALL_FLAGS_ANTIZERO);

	if (!cvCVar) {
		Con_PrintLinef ("Variable %s not found", Cmd_Argv(cmd, 1));
		return;
	}

	if (Have_Flag(cvCVar->flags, CF_PERSISTENT)) {
		Con_PrintLinef ("%s is CF_PERSISTENT", Cmd_Argv(cmd, 1));
		return;
	}

	if (Have_Flag(cvCVar->flags, CF_READONLY)) {
		Con_PrintLinef ("%s is CF_READONLY", Cmd_Argv(cmd, 1));
		return;
	}

	float amt, val;

	switch (numargs) {
	case 2:	val = cvCVar->value + 1;
			Cvar_SetValueQuick(cvCVar, val);
			break;

	case 3:	amt = atof(Cmd_Argv(cmd, 2));
			val = cvCVar->value + amt;
			Cvar_SetValueQuick(cvCVar, val);
			break;
	}

}

static void Cmd_Dec_f(cmd_state_t *cmd)
{
	cvar_state_t *cvars = cmd->cvars;
	int numargs = Cmd_Argc(cmd);

	if (numargs == 1 || numargs > 3) {
		Con_PrintLinef ("Dec Console Variable - Usage" NEWLINE
			"  dec <variable> [amt] - decreases variable");
		return;
	}

	cvar_t *cvCVar = Cvar_FindVar(cvars, Cmd_Argv(cmd, 1), ALL_FLAGS_ANTIZERO);

	if (!cvCVar) {
		Con_PrintLinef ("Variable %s not found", Cmd_Argv(cmd, 1));
		return;
	}

	if (Have_Flag(cvCVar->flags, CF_PERSISTENT)) {
		Con_PrintLinef ("%s is CF_PERSISTENT", Cmd_Argv(cmd, 1));
		return;
	}

	if (Have_Flag(cvCVar->flags, CF_READONLY)) {
		Con_PrintLinef ("%s is CF_READONLY", Cmd_Argv(cmd, 1));
		return;
	}

	float amt, val;
	switch (numargs) {
	case 2:	val = cvCVar->value + 1;
		Cvar_SetValueQuick(cvCVar, val);
		break;

	case 3: amt = atof(Cmd_Argv(cmd, 2));
		val = cvCVar->value - amt;
		Cvar_SetValueQuick(cvCVar, val);
		break;
	}

}

static void Cmd_Toggle_f(cmd_state_t *cmd)
{
	// Acquire Number of Arguments
	int nNumArgs = Cmd_Argc(cmd);

	if (nNumArgs == 1)
		// No Arguments Specified; Print Usage
		Con_PrintLinef ("Toggle Console Variable - Usage" NEWLINE
						"  toggle <variable> - toggles between 0 and 1" NEWLINE
						"  toggle <variable> <value> - toggles between 0 and <value>" NEWLINE
						"  toggle <variable> [string 1] [string 2]...[string n] - cycles through all strings");
	else
	{ // Correct Arguments Specified
		// Acquire Potential CVar
		cvar_t *cvCVar = Cvar_FindVar(cmd->cvars, Cmd_Argv(cmd, 1), cmd->cvars_flagsmask);

		if (cvCVar != NULL)
		{ // Valid CVar
			if (nNumArgs == 2)
			{ // Default Usage
				if (cvCVar->integer)
					Cvar_SetValueQuick(cvCVar, 0);
				else
					Cvar_SetValueQuick(cvCVar, 1);
			}
			else
			if (nNumArgs == 3)
			{ // 0 and Specified Usage
				if (cvCVar->integer == atoi(Cmd_Argv(cmd, 2) ) )
					// CVar is Specified Value; // Reset to 0
					Cvar_SetValueQuick(cvCVar, 0);
				else
				if (cvCVar->integer == 0)
					// CVar is 0; Specify Value
					Cvar_SetQuick(cvCVar, Cmd_Argv(cmd, 2) );
				else
					// CVar does not match; Reset to 0
					Cvar_SetValueQuick(cvCVar, 0);
			}
			else
			{ // Variable Values Specified
				int nCnt;
				int bFound = 0;

				for(nCnt = 2; nCnt < nNumArgs; nCnt++)
				{ // Cycle through Values
					if ( strcmp(cvCVar->string, Cmd_Argv(cmd, nCnt) ) == 0)
					{ // Current Value Located; Increment to Next
						if ( (nCnt + 1) == nNumArgs)
							// Max Value Reached; Reset
							Cvar_SetQuick(cvCVar, Cmd_Argv(cmd, 2) );
						else
							// Next Value
							Cvar_SetQuick(cvCVar, Cmd_Argv(cmd, nCnt + 1) );

						// End Loop
						nCnt = nNumArgs;
						// Assign Found
						bFound = 1;
					}
				}
				if (!bFound)
					// Value not Found; Reset to Original
					Cvar_SetQuick(cvCVar, Cmd_Argv(cmd, 2) );
			}

		}
		else
		{ // Invalid CVar
			Con_PrintLinef ("ERROR : CVar " QUOTED_S " not found", Cmd_Argv(cmd, 1) );
		}
	}
}

/*
===============
Cmd_Alias_f

Creates a new command that executes a command string (possibly ; seperated)
===============
*/
static void Cmd_Alias_f (cmd_state_t *cmd)
{
	cmd_alias_t	*a;
	char		line[MAX_INPUTLINE_16384];
	int			i, c;
	const char		*s;
	size_t		alloclen;

	if (Cmd_Argc(cmd) == 1)
	{
		Con_Print("Current alias commands:\n");
		for (a = cmd->userdefined->alias ; a ; a=a->next)
			Con_Printf ("%s : %s", a->name, a->value);
		return;
	}

	s = Cmd_Argv(cmd, 1);
	if (strlen(s) >= MAX_ALIAS_NAME_32)
	{
		Con_Print("Alias name is too long\n");
		return;
	}

	// if the alias already exists, reuse it
	for (a = cmd->userdefined->alias ; a ; a=a->next)
	{
		if (String_Does_Match(s, a->name))
		{
			Z_Free (a->value);
			break;
		}
	}

	if (!a)
	{
		cmd_alias_t *prev, *current;

		a = (cmd_alias_t *)Z_Malloc (sizeof(cmd_alias_t));
		strlcpy (a->name, s, sizeof (a->name));
		// insert it at the right alphanumeric position
		for( prev = NULL, current = cmd->userdefined->alias ; current && strcmp( current->name, a->name ) < 0 ; prev = current, current = current->next )
			;
		if ( prev ) {
			prev->next = a;
		} else {
			cmd->userdefined->alias = a;
		}
		a->next = current;
	}


// copy the rest of the command line
	line[0] = 0;		// start out with a null string
	c = Cmd_Argc(cmd);
	for (i=2 ; i < c ; i++)
	{
		if (i != 2)
			strlcat (line, " ", sizeof (line));
		strlcat (line, Cmd_Argv(cmd, i), sizeof (line));
	}
	strlcat (line, "\n", sizeof (line));

	alloclen = strlen (line) + 1;
	if (alloclen >= 2)
		line[alloclen - 2] = '\n'; // to make sure a newline is appended even if too long
	a->value = (char *)Z_Malloc (alloclen);
	memcpy (a->value, line, alloclen);
}

/*
===============
Cmd_UnAlias_f

Remove existing aliases.
===============
*/
static void Cmd_UnAlias_f (cmd_state_t *cmd)
{
	cmd_alias_t	*a, *p;
	int i;
	const char *s;

	if (Cmd_Argc(cmd) == 1)
	{
		Con_Print("unalias: Usage: unalias alias1 [alias2 ...]\n");
		return;
	}

	for(i = 1; i < Cmd_Argc(cmd); ++i)
	{
		s = Cmd_Argv(cmd, i);
		p = NULL;
		for(a = cmd->userdefined->alias; a; p = a, a = a->next)
		{
			if (String_Does_Match(s, a->name))
			{
				if (a->initstate) // we can not remove init aliases
					continue;
				if (a == cmd->userdefined->alias)
					cmd->userdefined->alias = a->next;
				if (p)
					p->next = a->next;
				Z_Free(a->value);
				Z_Free(a);
				break;
			}
		}
		if (!a)
			Con_Printf ("unalias: %s alias not found\n", s);
	}
}

/*
=============================================================================

					COMMAND EXECUTION

=============================================================================
*/

static const char *Cmd_GetDirectCvarValue(cmd_state_t *cmd, const char *varname, cmd_alias_t *alias, qbool *is_multiple)
{
	cvar_t *cvar;
	long argno;
	char *endptr;
	static char vabuf[1024]; // cmd_mutex

	if (is_multiple)
		*is_multiple = false;

	if (!varname || !*varname)
		return NULL;

	if (alias)
	{
		if (String_Does_Match(varname, "*"))
		{
			if (is_multiple)
				*is_multiple = true;
			return Cmd_Args(cmd);
		}
		else if (String_Does_Match(varname, "#"))
		{
			return va(vabuf, sizeof(vabuf), "%d", Cmd_Argc(cmd));
		}
		else if (varname[strlen(varname) - 1] == '-')
		{
			argno = strtol(varname, &endptr, 10);
			if (endptr == varname + strlen(varname) - 1)
			{
				// whole string is a number, apart from the -
				const char *p = Cmd_Args(cmd);
				for(; argno > 1; --argno)
					if (!COM_ParseToken_Console(&p))
						break;
				if (p)
				{
					if (is_multiple)
						*is_multiple = true;

					// kill pre-argument whitespace
					for (;*p && ISWHITESPACE(*p);p++)
						;

					return p;
				}
			}
		}
		else
		{
			argno = strtol(varname, &endptr, 10);
			if (*endptr == 0)
			{
				// whole string is a number
				// NOTE: we already made sure we don't have an empty cvar name!
				if (argno >= 0 && argno < Cmd_Argc(cmd))
					return Cmd_Argv(cmd, argno);
			}
		}
	}

	if ((cvar = Cvar_FindVar(cmd->cvars, varname, cmd->cvars_flagsmask)) && !(cvar->flags & CF_PRIVATE))
		return cvar->string;

	return NULL;
}

qbool Cmd_QuoteString(char *out, size_t outlen, const char *in, const char *quoteset, qbool putquotes)
{
	qbool quote_quot = !!strchr(quoteset, '"');
	qbool quote_backslash = !!strchr(quoteset, '\\');
	qbool quote_dollar = !!strchr(quoteset, '$');

	if (putquotes)
	{
		if (outlen <= 2)
		{
			*out++ = 0;
			return false;
		}
		*out++ = '"'; --outlen;
		--outlen;
	}

	while(*in)
	{
		if (*in == '"' && quote_quot)
		{
			if (outlen <= 2)
				goto fail;
			*out++ = '\\'; --outlen;
			*out++ = '"'; --outlen;
		}
		else if (*in == '\\' && quote_backslash)
		{
			if (outlen <= 2)
				goto fail;
			*out++ = '\\'; --outlen;
			*out++ = '\\'; --outlen;
		}
		else if (*in == '$' && quote_dollar)
		{
			if (outlen <= 2)
				goto fail;
			*out++ = '$'; --outlen;
			*out++ = '$'; --outlen;
		}
		else
		{
			if (outlen <= 1)
				goto fail;
			*out++ = *in; --outlen;
		}
		++in;
	}
	if (putquotes)
		*out++ = '"';
	*out++ = 0;
	return true;
fail:
	if (putquotes)
		*out++ = '"';
	*out++ = 0;
	return false;
}

static const char *Cmd_GetCvarValue(cmd_state_t *cmd, const char *var, size_t varlen, cmd_alias_t *alias)
{
	static char varname[MAX_INPUTLINE_16384]; // cmd_mutex
	static char varval[MAX_INPUTLINE_16384]; // cmd_mutex
	const char *varstr = NULL;
	char *varfunc;
	qbool required = false;
	qbool optional = false;
	static char asis[] = "asis"; // just to suppress const char warnings

	if (varlen >= MAX_INPUTLINE_16384)
		varlen = MAX_INPUTLINE_16384 - 1;
	memcpy(varname, var, varlen);
	varname[varlen] = 0;
	varfunc = strchr(varname, ' ');

	if (varfunc)
	{
		*varfunc = 0;
		++varfunc;
	}

	if (*var == 0)
	{
		// empty cvar name?
		if (alias)
			Con_PrintLinef (CON_WARN "Warning: Could not expand $ in alias %s", alias->name);
		else
			Con_PrintLinef (CON_WARN "Warning: Could not expand $");
		return "$";
	}

	if (varfunc)
	{
		char *p;
		// ? means optional
		while((p = strchr(varfunc, '?')))
		{
			optional = true;
			memmove(p, p+1, strlen(p)); // with final NUL
		}
		// ! means required
		while((p = strchr(varfunc, '!')))
		{
			required = true;
			memmove(p, p+1, strlen(p)); // with final NUL
		}
		// kill spaces
		while((p = strchr(varfunc, ' ')))
		{
			memmove(p, p+1, strlen(p)); // with final NUL
		}
		// if no function is left, NULL it
		if (!*varfunc)
			varfunc = NULL;
	}

	if (varname[0] == '$')
		varstr = Cmd_GetDirectCvarValue(cmd, Cmd_GetDirectCvarValue(cmd, varname + 1, alias, NULL), alias, NULL);
	else
	{
		qbool is_multiple = false;
		// Exception: $* and $n- don't use the quoted form by default
		varstr = Cmd_GetDirectCvarValue(cmd, varname, alias, &is_multiple);
		if (is_multiple)
			if (!varfunc)
				varfunc = asis;
	}

	if (!varstr)
	{
		if (required)
		{
			if (alias)
				Con_PrintLinef (CON_ERROR "Error: Could not expand $%s in alias %s", varname, alias->name);
			else
				Con_PrintLinef (CON_ERROR "Error: Could not expand $%s", varname);
			return NULL;
		}
		else if (optional)
		{
			return "";
		}
		else
		{
			if (alias)
				Con_PrintLinef (CON_WARN "Warning: Could not expand $%s in alias %s\n", varname, alias->name);
			else
				Con_PrintLinef (CON_WARN "Warning: Could not expand $%s", varname);
			dpsnprintf(varval, sizeof(varval), "$%s", varname);
			return varval;
		}
	}

	if (!varfunc || String_Does_Match(varfunc, "q")) // note: quoted form is default, use "asis" to override!
	{
		// quote it so it can be used inside double quotes
		// we just need to replace " by \", and of course, double backslashes
		Cmd_QuoteString(varval, sizeof(varval), varstr, "\"\\", false);
		return varval;
	}
	else if (String_Does_Match(varfunc, "asis"))
	{
		return varstr;
	}
	else
		Con_PrintLinef ("Unknown variable function %s", varfunc);

	return varstr;
}

/*
Cmd_PreprocessString

Preprocesses strings and replaces $*, $param#, $cvar accordingly. Also strips comments.
*/
static qbool Cmd_PreprocessString(cmd_state_t *cmd, const char *intext, char *outtext, unsigned maxoutlen, cmd_alias_t *alias ) {
	const char *in;
	size_t eat, varlen;
	unsigned outlen;
	const char *val;

	// don't crash if there's no room in the outtext buffer
	if ( maxoutlen == 0 ) {
		return false;
	}
	maxoutlen--; // because of \0

	in = intext;
	outlen = 0;

	while( *in && outlen < maxoutlen ) {
		if ( *in == '$' ) {
			// this is some kind of expansion, see what comes after the $
			in++;

			// The console does the following preprocessing:
			//
			// - $$ is transformed to a single dollar sign.
			// - $var or ${var} are expanded to the contents of the named cvar,
			//   with quotation marks and backslashes quoted so it can safely
			//   be used inside quotation marks (and it should always be used
			//   that way)
			// - ${var asis} inserts the cvar value as is, without doing this
			//   quoting
			// - ${var ?} silently expands to the empty string if
			//   $var does not exist
			// - ${var !} fails expansion and executes nothing if
			//   $var does not exist
			// - prefix the cvar name with a dollar sign to do indirection;
			//   for example, if $x has the value timelimit, ${$x} will return
			//   the value of $timelimit
			// - when expanding an alias, the special variable name $* refers
			//   to all alias parameters, and a number refers to that numbered
			//   alias parameter, where the name of the alias is $0, the first
			//   parameter is $1 and so on; as a special case, $* inserts all
			//   parameters, without extra quoting, so one can use $* to just
			//   pass all parameters around. All parameters starting from $n
			//   can be referred to as $n- (so $* is equivalent to $1-).
			// - ${* q} and ${n- q} force quoting anyway
			//
			// Note: when expanding an alias, cvar expansion is done in the SAME step
			// as alias expansion so that alias parameters or cvar values containing
			// dollar signs have no unwanted bad side effects. However, this needs to
			// be accounted for when writing complex aliases. For example,
			//   alias foo "set x NEW; echo $x"
			// actually expands to
			//   "set x NEW; echo OLD"
			// and will print OLD! To work around this, use a second alias:
			//   alias foo "set x NEW; foo2"
			//   alias foo2 "echo $x"
			//
			// Also note: lines starting with alias are exempt from cvar expansion.
			// If you want cvar expansion, write "alias" instead:
			//
			//   set x 1
			//   alias foo "echo $x"
			//   "alias" bar "echo $x"
			//   set x 2
			//
			// foo will print 2, because the variable $x will be expanded when the alias
			// gets expanded. bar will print 1, because the variable $x was expanded
			// at definition time. foo can be equivalently defined as
			//
			//   "alias" foo "echo $$x"
			//
			// because at definition time, $$ will get replaced to a single $.

			if ( *in == '$' ) {
				val = "$";
				eat = 1;
			} else if (*in == '{') {
				varlen = strcspn(in + 1, "}");
				if (in[varlen + 1] == '}')
				{
					val = Cmd_GetCvarValue(cmd, in + 1, varlen, alias);
					if (!val)
						return false;
					eat = varlen + 2;
				}
				else
				{
					// ran out of data?
					val = NULL;
					eat = varlen + 1;
				}
			} else {
				varlen = strspn(in, "#*0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-");
				val = Cmd_GetCvarValue(cmd, in, varlen, alias);
				if (!val)
					return false;
				eat = varlen;
			}
			if (val)
			{
				// insert the cvar value
				while(*val && outlen < maxoutlen)
					outtext[outlen++] = *val++;
				in += eat;
			}
			else
			{
				// copy the unexpanded text
				outtext[outlen++] = '$';
				while(eat && outlen < maxoutlen)
				{
					outtext[outlen++] = *in++;
					--eat;
				}
			}
		}
		else
			outtext[outlen++] = *in++;
	}
	outtext[outlen] = 0;
	return true;
}

/*
============
Cmd_ExecuteAlias

Called for aliases and fills in the alias into the cbuffer
============
*/
static void Cmd_ExecuteAlias (cmd_state_t *cmd, cmd_alias_t *alias)
{
	static char buffer[ MAX_INPUTLINE_16384 ]; // cmd_mutex
	static char buffer2[ MAX_INPUTLINE_16384 ]; // cmd_mutex
	qbool ret = Cmd_PreprocessString( cmd, alias->value, buffer, sizeof(buffer) - 2, alias );
	if (!ret)
		return;
	// insert at start of command buffer, so that aliases execute in order
	// (fixes bug introduced by Black on 20050705)

	// Note: Cbuf_PreprocessString will be called on this string AGAIN! So we
	// have to make sure that no second variable expansion takes place, otherwise
	// alias parameters containing dollar signs can have bad effects.
	Cmd_QuoteString(buffer2, sizeof(buffer2), buffer, "$", false);
	Cbuf_InsertText(cmd, buffer2);
}

/*
========
Cmd_List

	CmdList Added by EvilTypeGuy eviltypeguy@qeradiant.com
	Thanks to Matthias "Maddes" Buecher, http://www.inside3d.com/qip/

========
*/
static void Cmd_List_f (cmd_state_t *cmd)
{
	cmd_function_t *func;
	const char *partial;
	size_t len;
	int count;
	qbool ispattern;

	if (Cmd_Argc(cmd) > 1)
	{
		partial = Cmd_Argv(cmd, 1);
		len = strlen(partial);
		ispattern = (strchr(partial, '*') || strchr(partial, '?'));
	}
	else
	{
		partial = NULL;
		len = 0;
		ispattern = false;
	}

	count = 0;
	for (func = cmd->userdefined->qc_functions; func; func = func->next)
	{
		if (partial && (ispattern ? !matchpattern_with_separator(func->name, partial, false, "", false) : strncmp(partial, func->name, len)))
			continue;
		Con_PrintLinef ("%s : %s", func->name, func->description);
		count++;
	}
	for (func = cmd->engine_functions; func; func = func->next)
	{
		if (partial && (ispattern ? !matchpattern_with_separator(func->name, partial, false, "", false) : strncmp(partial, func->name, len)))
			continue;
		Con_PrintLinef ("%s : %s", func->name, func->description);
		count++;
	}

	if (len)
	{
		if (ispattern)
			Con_Printf ("%d Command%s matching " QUOTED_S "\n\n", count, (count > 1) ? "s" : "", partial);
		else
			Con_Printf ("%d Command%s beginning with " QUOTED_S "\n\n", count, (count > 1) ? "s" : "", partial);
	}
	else
		Con_Printf ("%d Command%s\n\n", count, (count > 1) ? "s" : "");
}

static void Cmd_Apropos_f(cmd_state_t *cmd)
{
	cmd_function_t *func;
	cvar_t *cvar;
	cmd_alias_t *alias;
	const char *partial;
	int count;
	qbool ispattern;
	char vabuf[1024];

	if (Cmd_Argc(cmd) > 1)
		partial = Cmd_Args(cmd);
	else
	{
		Con_PrintLinef ("usage: %s <string>", Cmd_Argv(cmd, 0));
		return;
	}

	ispattern = partial && (strchr(partial, '*') || strchr(partial, '?'));
	if (!ispattern)
		partial = va(vabuf, sizeof(vabuf), "*%s*", partial);

	count = 0;
	for (cvar = cmd->cvars->vars; cvar; cvar = cvar->next)
	{
		if (matchpattern_with_separator(cvar->name, partial, true, "", false) ||
		    matchpattern_with_separator(cvar->description, partial, true, "", false))
		{
			Con_Printf ("cvar ");
			Cvar_PrintHelp(cvar, cvar->name, true);
			count++;
		}
		for (char **cvar_alias = cvar->aliases; cvar_alias && *cvar_alias; cvar_alias++)
		{
			if (matchpattern_with_separator(*cvar_alias, partial, true, "", false))
			{
				Con_Printf ("cvar ");
				Cvar_PrintHelp(cvar, *cvar_alias, true);
				count++;
			}
		}
	}
	for (func = cmd->userdefined->qc_functions; func; func = func->next)
	{
		if (!matchpattern_with_separator(func->name, partial, true, "", false))
			if (!matchpattern_with_separator(func->description, partial, true, "", false))
				continue;
		Con_PrintLinef ("command "CON_BRONZE "%s" CON_WHITE ": %s", func->name, func->description);
		count++;
	}
	for (func = cmd->engine_functions; func; func = func->next)
	{
		if (!matchpattern_with_separator(func->name, partial, true, "", false))
		if (!matchpattern_with_separator(func->description, partial, true, "", false))
			continue;
		Con_PrintLinef ("command " CON_BRONZE "%s" CON_WHITE ": %s", func->name, func->description); // Baker bronze
		count++;
	}
	for (alias = cmd->userdefined->alias; alias; alias = alias->next)
	{
		// procede here a bit differently as an alias value always got a final \n
		if (!matchpattern_with_separator(alias->name, partial, true, "", false))
		if (!matchpattern_with_separator(alias->value, partial, true, "\n", false)) // when \n is as separator wildcards don't match it
			continue;
		Con_Printf ("alias " CON_BRONZE "%s" CON_WHITE ": %s", alias->name, alias->value); // do not print an extra \n
		count++;
	}
	Con_Printf ("%d result%s\n\n", count, (count > 1) ? "s" : "");
}

static cmd_state_t *Cmd_AddInterpreter(cmd_buf_t *cbuf, cvar_state_t *cvars, int cvars_flagsmask, int cmds_flagsmask, cmd_userdefined_t *userdefined)
{
	cmd_state_t *cmd = (cmd_state_t *)Mem_Alloc(tempmempool, sizeof(cmd_state_t));

	cmd->mempool = Mem_AllocPool("commands", 0, NULL);
	// space for commands and script files
	cmd->cbuf = cbuf;
	cmd->null_string = "";

	cmd->cvars = cvars;
	cmd->cvars_flagsmask = cvars_flagsmask;
	cmd->cmd_flags = cmds_flagsmask;
	cmd->userdefined = userdefined;

	return cmd;
}



/*
============
Cmd_Argc
============
*/
inline int Cmd_Argc (cmd_state_t *cmd)
{
	return cmd->argc;
}

/*
============
Cmd_Argv
============
*/
inline const char *Cmd_Argv(cmd_state_t *cmd, int arg)
{
	if (arg >= cmd->argc )
		return cmd->null_string;
	return cmd->argv[arg];
}

/*
============
Cmd_Args
============
*/
inline const char *Cmd_Args (cmd_state_t *cmd)
{
	return cmd->args;
}

/*
============
Cmd_TokenizeString

Parses the given string into command line tokens.
============
*/
// AK: This function should only be called from ExcuteString because the current design is a bit of an hack
static void Cmd_TokenizeString (cmd_state_t *cmd, const char *text)
{
	int l;

	cmd->argc = 0;
	cmd->args = NULL;
	cmd->cmdline = NULL;

	while (1)
	{
		// skip whitespace up to a /n
		while (*text && ISWHITESPACE(*text) && *text != '\r' && *text != '\n')
			text++;

		// line endings:
		// UNIX: \n
		// Mac: \r
		// Windows: \r\n
		if (*text == '\n' || *text == '\r')
		{
			// a newline separates commands in the buffer
			if (*text == '\r' && text[1] == '\n')
				text++;
			text++;
			break;
		}

		if (!*text)
			return;

		if (!cmd->argc)
			cmd->cmdline = text;
		if (cmd->argc == 1)
			cmd->args = text;

		if (!COM_ParseToken_Console(&text))
			return;

		if (cmd->argc < MAX_ARGS_80)
		{
			l = (int)strlen(com_token) + 1;
			if (cmd->cbuf->tokenizebufferpos + l > CMD_TOKENIZELENGTH)
			{
				Con_PrintLinef ("Cmd_TokenizeString: ran out of %d character buffer space for command arguments", CMD_TOKENIZELENGTH);
				break;
			}
			memcpy (cmd->cbuf->tokenizebuffer + cmd->cbuf->tokenizebufferpos, com_token, l);
			cmd->argv[cmd->argc] = cmd->cbuf->tokenizebuffer + cmd->cbuf->tokenizebufferpos;
			cmd->cbuf->tokenizebufferpos += l;
			cmd->argc++;
		}
	}
}


/*
============
Cmd_AddCommand
============
*/
void Cmd_AddCommand(int flags, const char *cmd_name, xcommand_t function, const char *description)
{
	cmd_function_t *func;
	cmd_function_t *prev, *current;
	cmd_state_t *cmd;
	int i;

	for (i = 0; i < 2; i++)
	{
		cmd = cmd_iter_all[i].cmd;
		if (flags & cmd->cmd_flags)
		{
			// fail if the command is a variable name
			if (Cvar_FindVar(cmd->cvars, cmd_name, ~0))
			{
				Con_PrintLinef ("Cmd_AddCommand: %s already defined as a var", cmd_name);
				return;
			}

			if (function)
			{
				// fail if the command already exists in this interpreter
				for (func = cmd->engine_functions; func; func = func->next)
				{
					if (String_Does_Match(cmd_name, func->name))
					{
						Con_PrintLinef ("Cmd_AddCommand: %s already defined", cmd_name);
						continue;
					}
				}

				func = (cmd_function_t *)Mem_Alloc(cmd->mempool, sizeof(cmd_function_t));
				func->flags = flags;
				func->name = cmd_name;
				func->function = function;
				func->description = description;
				func->next = cmd->engine_functions;

				// insert it at the right alphanumeric position
				for (prev = NULL, current = cmd->engine_functions; current && strcmp(current->name, func->name) < 0; prev = current, current = current->next)
					;
				if (prev) {
					prev->next = func;
				}
				else {
					cmd->engine_functions = func;
				}
				func->next = current;
			}
			else
			{
				// mark qcfunc if the function already exists in the qc_functions list
				for (func = cmd->userdefined->qc_functions; func; func = func->next)
				{
					if (String_Does_Match(cmd_name, func->name))
					{
						func->qcfunc = true; //[515]: csqc
						continue;
					}
				}

				func = (cmd_function_t *)Mem_Alloc(cmd->mempool, sizeof(cmd_function_t));
				func->flags = flags;
				func->name = cmd_name;
				func->function = function;
				func->description = description;
				func->qcfunc = true; //[515]: csqc
				func->next = cmd->userdefined->qc_functions;

				// bones_was_here: if this QC command overrides an engine command, store its pointer
				// to avoid doing this search at invocation if QC declines to handle this command.
				for (cmd_function_t *f = cmd->engine_functions; f; f = f->next)
				{
					if (String_Does_Match(cmd_name, f->name))
					{
						Con_DPrintf ("Adding QC override of engine command %s\n", cmd_name);
						func->overridden = f;
						break;
					}
				}

				// insert it at the right alphanumeric position
				for (prev = NULL, current = cmd->userdefined->qc_functions; current && strcmp(current->name, func->name) < 0; prev = current, current = current->next)
					;
				if (prev) {
					prev->next = func;
				}
				else {
					cmd->userdefined->qc_functions = func;
				}
				func->next = current;
			}
		}
	}
}

/*
============
Cmd_Exists
============
*/
qbool Cmd_Exists (cmd_state_t *cmd, const char *cmd_name)
{
	cmd_function_t	*func;

	for (func = cmd->userdefined->qc_functions; func; func = func->next)
		if (String_Does_Match(cmd_name, func->name))
			return true;

	for (func=cmd->engine_functions ; func ; func=func->next)
		if (String_Does_Match (cmd_name,func->name))
			return true;

	return false;
}


/*
============
Cmd_CompleteCommand
============
*/
const char *Cmd_CompleteCommand (cmd_state_t *cmd, const char *partial, int is_from_nothing)
{
	cmd_function_t *func;
	size_t len;

	len = strlen(partial);

	if (!len && is_from_nothing == false)
		return NULL;

// check functions
	for (func = cmd->userdefined->qc_functions; func; func = func->next)
		if (String_Does_Start_With_Caseless (func->name, partial))
			return func->name;

	for (func = cmd->engine_functions; func; func = func->next)
		if (String_Does_Start_With_Caseless (func->name, partial))
			return func->name;

	return NULL;
}

/*
	Cmd_CompleteCountPossible

	New function for tab-completion system
	Added by EvilTypeGuy
	Thanks to Fett erich@heintz.com
	Thanks to taniwha

*/

int Cmd_CompleteCountPossible (cmd_state_t *cmd, const char *partial, int is_from_nothing)
{
	cmd_function_t *func;
	size_t len;
	int h;

	h = 0;
	len = strlen(partial);

	if (!len && is_from_nothing == false)
		return 0;

	// Loop through the command list and count all partial matches
	for (func = cmd->userdefined->qc_functions; func; func = func->next) {
		if (String_Does_Start_With_Caseless (func->name, partial)) {
			const char *sxy = func->name;

			SPARTIAL_EVAL_

			h++; // qualified
		} // qualified
	} // for

	for (func = cmd->engine_functions; func; func = func->next) {
		if (String_Does_Start_With_Caseless(func->name, partial)) {
			const char *sxy = func->name;
			SPARTIAL_EVAL_
			h++; // qualified
		} // qualified
	} // for

	return h;
}

/*
	Cmd_CompleteBuildList

	New function for tab-completion system
	Added by EvilTypeGuy
	Thanks to Fett erich@heintz.com
	Thanks to taniwha

*/
const char **Cmd_CompleteBuildList (cmd_state_t *cmd, const char *partial, int is_from_nothing)
{
	cmd_function_t *func;
	//size_t len = 0;
	size_t bpos = 0;
	size_t sizeofbuf = (Cmd_CompleteCountPossible (cmd, partial, is_from_nothing) + 1) * sizeof (const char *);
	const char **buf;

	//len = strlen(partial);
	buf = (const char **)Mem_Alloc(tempmempool, sizeofbuf + sizeof (const char *));
	// Loop through the functions lists and print all matches
	for (func = cmd->userdefined->qc_functions; func; func = func->next) {
		if (String_Does_Start_With_Caseless (func->name, partial))
			buf[bpos++] = func->name;
	} // for
	for (func = cmd->engine_functions; func; func = func->next) {
		if (String_Does_Start_With_Caseless (func->name, partial))
			buf[bpos++] = func->name;
	} // for

	buf[bpos] = NULL;
	return buf;
}

// written by LadyHavoc
void Cmd_CompleteCommandPrint (cmd_state_t *cmd, const char *partial, int is_from_nothing)
{
	cmd_function_t *func;
	//size_t len = strlen(partial);

	// Loop through the command list and print all matches
	for (func = cmd->userdefined->qc_functions; func; func = func->next) {
		if (String_Does_Start_With_Caseless (func->name, partial))
			Con_PrintLinef (CON_BRONZE "%s" CON_WHITE ": %s", func->name, func->description);
	} // for

	for (func = cmd->engine_functions; func; func = func->next) {
		if (String_Does_Start_With_Caseless (func->name, partial))
			Con_PrintLinef (CON_BRONZE "%s" CON_WHITE ": %s", func->name, func->description);
	} // for
}

/*
	Cmd_CompleteAlias

	New function for tab-completion system
	Added by EvilTypeGuy
	Thanks to Fett erich@heintz.com
	Thanks to taniwha

*/
const char *Cmd_CompleteAlias (cmd_state_t *cmd, const char *partial, int is_from_nothing)
{
	cmd_alias_t *alias;
	size_t len;

	len = strlen(partial);

	if (!len && is_from_nothing == false)
		return NULL;

	// Check functions
	for (alias = cmd->userdefined->alias; alias; alias = alias->next)
		if (String_Does_Start_With_Caseless (alias->name, partial))
			return alias->name;

	return NULL;
}

// written by LadyHavoc
void Cmd_CompleteAliasPrint (cmd_state_t *cmd, const char *partial, int is_from_nothing)
{
	cmd_alias_t *alias;

	// Loop through the alias list and print all matches
	for (alias = cmd->userdefined->alias; alias; alias = alias->next)
		if (String_Does_Start_With_Caseless (alias->name, partial))
			Con_PrintLinef (CON_BRONZE "%s" CON_WHITE ": %s", alias->name, alias->value);
}


/*
	Cmd_CompleteAliasCountPossible

	New function for tab-completion system
	Added by EvilTypeGuy
	Thanks to Fett erich@heintz.com
	Thanks to taniwha

*/
int Cmd_CompleteAliasCountPossible (cmd_state_t *cmd, const char *partial, int is_from_nothing)
{
	cmd_alias_t	*alias;
	size_t		len;
	int			h;

	h = 0;

	len = strlen(partial);

	if (!len && is_from_nothing == false)
		return 0;

	// Loop through the command list and count all partial matches
	for (alias = cmd->userdefined->alias; alias; alias = alias->next) {
		if (String_Does_Start_With_Caseless(alias->name, partial)) {
			const char *sxy = alias->name;
			h++;
			SPARTIAL_EVAL_
		} // if
	} // for

	return h;
}

/*
	Cmd_CompleteAliasBuildList

	New function for tab-completion system
	Added by EvilTypeGuy
	Thanks to Fett erich@heintz.com
	Thanks to taniwha

*/
const char **Cmd_CompleteAliasBuildList (cmd_state_t *cmd, const char *partial, int is_from_nothing)
{
	cmd_alias_t *alias;
	//size_t len = 0;
	size_t bpos = 0;
	size_t sizeofbuf = (Cmd_CompleteAliasCountPossible (cmd, partial, is_from_nothing) + 1) * sizeof (const char *);
	const char **buf;

	//len = strlen(partial);
	buf = (const char **)Mem_Alloc(tempmempool, sizeofbuf + sizeof (const char *));
	// Loop through the alias list and print all matches
	for (alias = cmd->userdefined->alias; alias; alias = alias->next)
		if (String_Does_Start_With_Caseless (alias->name, partial))
			buf[bpos++] = alias->name;

	buf[bpos] = NULL;
	return buf;
}

// TODO: Make this more generic?
void Cmd_ClearCSQCCommands (cmd_state_t *cmd)
{
	cmd_function_t *func;
	cmd_function_t **next = &cmd->userdefined->qc_functions;

	while(*next)
	{
		func = *next;
		*next = func->next;
		Z_Free(func);
	}
}

extern cvar_t sv_cheats;

/*
 * Cloudwalk FIXME: This idea sounded great in my head but...
 * How do we handle commands that can be received by the client,
 * but which the server can also execute locally?
 *
 * If we create a callback where the engine will forward to server
 * but try to execute the command locally if it's dedicated,
 * we're back to intermixing client and server code which I'm
 * trying to avoid. There's no other way I can think of to
 * implement that behavior that doesn't involve an #ifdef, or
 * making a mess of hooks.
 */
qbool Cmd_Callback(cmd_state_t *cmd, cmd_function_t *func, const char *text, cmd_source_t src)
{
	if (func->function)
		func->function(cmd);
	else
		Con_PrintLinef ("Command " QUOTED_S " can not be executed", Cmd_Argv(cmd, 0));
	return true;
}

qbool Cmd_CL_Callback(cmd_state_t *cmd, cmd_function_t *func, const char *text, cmd_source_t src)
{
	// TODO: Assign these functions to QC commands directly?
	if (func->qcfunc)
	{
		if (((func->flags & CF_CLIENT) && CL_VM_ConsoleCommand(text)) ||
		   ((func->flags & CF_SERVER) && SV_VM_ConsoleCommand(text)))
			return true;

		if (func->overridden) // If this QC command overrides an engine command,
			func = func->overridden; // fall back to that command.
	}

	// Baker: prespawn, god, say, status, tell, pause, ping, give some ent_create/remove stuff, sv_startdownload
	if (Have_Flag (func->flags, CF_SERVER_FROM_CLIENT))
	{
		if (host_isclient.integer)
		{
			CL_ForwardToServer_f(cmd);
			return true;
		}
		else if (!(func->flags & CF_SERVER))
		{
			Con_PrintLinef ("Cannot execute client commands from a dedicated server console.");
			return true;
		}
	}
	return Cmd_Callback(cmd, func, text, src);
}

// Baker: What is the return value here?
// Well denied cheats returned true before so we do that.
qbool Cmd_SV_Callback(cmd_state_t *cmd, cmd_function_t *func, const char *text, cmd_source_t src)
{
	if (func->qcfunc && (func->flags & CF_SERVER))
		return SV_VM_ConsoleCommand(text);
	else if (src == src_client)
	{
		if ((func->flags & CF_CHEAT)) {
			prvm_prog_t *prog = SVVM_prog;
			if (!sv_cheats.integer) {
				SV_ClientPrintf ("No cheats allowed. The server must have sv_cheats set to 1" NEWLINE);
				return true;
			}
			else if (sv_cheats.integer < 2 && PRVM_serverglobalfloat(deathmatch)) {
				SV_ClientPrintf ("No cheats allowed in deathmatch. The server must have sv_cheats set to 2" NEWLINE);
				return true;
			}
			// is ok .. fall through
		} // if
		func->function(cmd);
		return true;
	}
	return false;
}

qbool Cmd_SV_NotFound(cmd_state_t *cmd, cmd_function_t *func, const char *text, cmd_source_t src)
{
	if (cmd->source == src_client) {
		Con_PrintLinef ("Client " QUOTED_S " tried to execute " QUOTED_S, host_client->name, text);
		return true;
	}
	return false;
}
/*
============
Cmd_ExecuteString

A complete command line has been parsed, so try to execute it
FIXME: lookupnoadd the token to speed search?
============
*/
void Cmd_ExecuteString (cmd_state_t *cmd, const char *text, cmd_source_t src, qbool lockmutex)
{
	int oldpos;
	cmd_function_t *func;
	cmd_alias_t *a;
	if (lockmutex)
		Cbuf_Lock(cmd->cbuf);
	oldpos = cmd->cbuf->tokenizebufferpos;
	cmd->source = src;

#if 0
	if (developer_execstring.integer)
		Con_PrintLinef ("exec: " QUOTED_S, text);
#endif
	Cmd_TokenizeString (cmd, text);

// execute the command line
	if (!Cmd_Argc(cmd))
		goto done; // no tokens

	const char *s_command_wanted = Cmd_Argv(cmd, 0);

// check functions
	for (func = cmd->userdefined->qc_functions; func; func = func->next)
	{
		if (String_Does_Match_Caseless(func->name, s_command_wanted))
		{
			if (cmd->Handle(cmd, func, text, src))
				goto done;
		}
	}

	for (func = cmd->engine_functions; func; func=func->next)
	{
		if (String_Does_Match_Caseless (func->name, s_command_wanted))
		{
			if (cmd->Handle(cmd, func, text, src))
				goto done;
		}
	}

	// if it's a client command and no command was found, say so.
	if (cmd->NotFound)
	{
		if (cmd->NotFound(cmd, func, text, src))
			goto done;
	}

// check alias
	for (a=cmd->userdefined->alias ; a ; a=a->next)
	{
		if (String_Does_Match_Caseless (a->name, s_command_wanted))
		{
			Cmd_ExecuteAlias(cmd, a);
			goto done;
		}
	}

// check cvars
	if (!Cvar_Command(cmd) && host.superframecount > 0) {
		if (is_in_loadconfig && String_Does_Match (s_command_wanted, "gamma")) {
			// Baker: Sick of "unknown command gamma" during Quake gamedir change startup
			// Do not print
			// Must be in loadconfig only
		} else {
			Con_PrintLinef ("Unknown command " QUOTED_S, s_command_wanted);
		}
	}
done:
	cmd->cbuf->tokenizebufferpos = oldpos;
	if (lockmutex)
		Cbuf_Unlock(cmd->cbuf);
}

int Cmd_Is_Lead_Word_A_Command_Cvar_Alias (cmd_state_t *cmd, const char *text)
{
// Baker: a command like "disconnect; menu_load" .. we are getting "disconnect;" as the lead word.
	const char *t = text;
	int got_token = COM_ParseToken_QuakeC (&t, /*returnnewline?*/ false);

	//Cmd_TokenizeString (cmd, text);
	const char *s_command_wanted = com_token;

// execute the command line
	if (got_token == false)
		return 0; // no tokens

// check functions

	WARP_X_ (examples: )
	for (cmd_function_t *func = cmd->userdefined->qc_functions; func; func = func->next) {
		if (String_Does_Match_Caseless(func->name, s_command_wanted))
			return true;
	} // for cmd

	WARP_X_ (examples: wait cprint sv_downloads )
	for (cmd_function_t *func = cmd->engine_functions; func; func = func->next) {
		if (String_Does_Match_Caseless (func->name, s_command_wanted))
			return true;
	} // for engine_functions // what is that?

	// if it's a client command and no command was found, say so.
	//if (cmd->NotFound) {
	//	if (cmd->NotFound(cmd, func, text, src))
	//		goto done;
	//}

// check alias
	for (cmd_alias_t *a = cmd->userdefined->alias; a ; a = a->next) {
		if (String_Does_Match_Caseless (a->name, s_command_wanted))
			return true;
	} // alias

// check cvars
	//cvar_state_t *cvars = cmd->cvars;
	WARP_X_ (what is flags mask )
	if (Cvar_FindVar(cmd->cvars, s_command_wanted, cmd->cvars_flagsmask) )
		return true;


	return false;
}

/*
================
Cmd_CheckParm

Returns the position (1 to argc-1) in the command's argument list
where the given parameter apears, or 0 if not present
================
*/

int Cmd_CheckParm (cmd_state_t *cmd, const char *parm)
{
	int i;

	if (!parm)
	{
		Con_PrintLinef ("Cmd_CheckParm: NULL");
		return 0;
	}

	for (i = 1; i < Cmd_Argc (cmd); i++)
		if (String_Does_Match_Caseless (parm, Cmd_Argv(cmd, i)))
			return i;

	return 0;
}



void Cmd_Host_Init_SaveInitState(void)
{
	cmd_iter_t *cmd_iter;
	for (cmd_iter = cmd_iter_all; cmd_iter->cmd; cmd_iter++) {
		cmd_state_t *cmd = cmd_iter->cmd;
		cmd_function_t *f;
		cmd_alias_t *a;
		for (f = cmd->userdefined->qc_functions; f; f = f->next)
			f->initstate = true;
		for (f = cmd->engine_functions; f; f = f->next)
			f->initstate = true;
		for (a = cmd->userdefined->alias; a; a = a->next)
		{
			a->initstate = true;
			a->initialvalue = Mem_strdup(zonemempool, a->value);
		}
	}
	Cvar_SaveInitState(&cvars_all);
}

void Cmd_RestoreInitState(void)
{
	cmd_iter_t *cmd_iter;
	for (cmd_iter = cmd_iter_all; cmd_iter->cmd; cmd_iter++) {
		cmd_state_t *cmd = cmd_iter->cmd;
		cmd_function_t *f, **fp;
		cmd_alias_t *a, **ap;
		for (fp = &cmd->userdefined->qc_functions; (f = *fp);)
		{
			if (f->initstate)
				fp = &f->next;
			else
			{
				// destroy this command, it didn't exist at init
				Con_DPrintLinef ("Cmd_RestoreInitState: Destroying command %s", f->name);
				*fp = f->next;
				Z_Free(f);
			}
		}
		for (fp = &cmd->engine_functions; (f = *fp);)
		{
			if (f->initstate)
				fp = &f->next;
			else
			{
				// destroy this command, it didn't exist at init
				Con_DPrintLinef ("Cmd_RestoreInitState: Destroying command %s", f->name);
				*fp = f->next;
				Z_Free(f);
			}
		}
		for (ap = &cmd->userdefined->alias; (a = *ap);)
		{
			if (a->initstate)
			{
				// restore this alias, it existed at init
				if (strcmp(a->value ? a->value : "", a->initialvalue ? a->initialvalue : ""))
				{
					Con_DPrintf ("Cmd_RestoreInitState: Restoring alias %s\n", a->name);
					if (a->value)
						Z_Free(a->value);
					a->value = Mem_strdup(zonemempool, a->initialvalue);
				}
				ap = &a->next;
			}
			else
			{
				// free this alias, it didn't exist at init...
				Con_DPrintf ("Cmd_RestoreInitState: Destroying alias %s\n", a->name);
				*ap = a->next;
				if (a->value)
					Z_Free(a->value);
				Z_Free(a);
			}
		}
	}
	Cvar_RestoreInitState(&cvars_all);
}

void Cmd_NoOperation_f(cmd_state_t *cmd)
{
}

/*
============
Cmd_Shutdown
============
*/
void Cmd_Shutdown(void)
{
	cmd_iter_t *cmd_iter;
	for (cmd_iter = cmd_iter_all; cmd_iter->cmd; cmd_iter++)
	{
		cmd_state_t *cmd = cmd_iter->cmd;

		if (cmd->cbuf->lock)
		{
			// we usually have this locked when we get here from Host_Quit_f
			Cbuf_Unlock(cmd->cbuf);
		}

		Mem_FreePool(&cmd->mempool);
	}
}

/*
============
Cmd_Init
============
*/


void Cmd_Init(void)
{
	cmd_buf_t *cbuf;
	cbuf_mempool = Mem_AllocPool("Command buffer", 0, NULL);
	cbuf = (cmd_buf_t *)Mem_Alloc(cbuf_mempool, sizeof(cmd_buf_t));
	cbuf->maxsize = CMDBUFSIZE_0_6_MB;
	cbuf->lock = Thread_CreateMutex();
	cbuf->wait = false;
	host.cbuf = cbuf;

	cbuf->start.prev = cbuf->start.next = &(cbuf->start);
	cbuf->deferred.prev = cbuf->deferred.next = &(cbuf->deferred);
	cbuf->free.prev = cbuf->free.next = &(cbuf->free);

	// FIXME: Get rid of cmd_iter_all eventually. This is just a hack to reduce the amount of work to make the interpreters dynamic.
	cmd_iter_all = (cmd_iter_t *)Mem_Alloc(tempmempool, sizeof(cmd_iter_t) * 3);

	// local console
	cmd_iter_all[0].cmd = cmd_local = Cmd_AddInterpreter(cbuf, &cvars_all, CF_CLIENT | CF_SERVER, CF_CLIENT | CF_CLIENT_FROM_SERVER | CF_SERVER_FROM_CLIENT, &cmd_userdefined_all);
	cmd_local->Handle = Cmd_CL_Callback;
	cmd_local->NotFound = NULL;

	// server commands received from clients have no reason to access cvars, cvar expansion seems perilous.
	cmd_iter_all[1].cmd = cmd_serverfromclient = Cmd_AddInterpreter(cbuf, &cvars_null, 0, CF_SERVER_FROM_CLIENT | CF_USERINFO, &cmd_userdefined_null);
	cmd_serverfromclient->Handle = Cmd_SV_Callback;
	cmd_serverfromclient->NotFound = Cmd_SV_NotFound;

	cmd_iter_all[2].cmd = NULL;
//
// register our commands
//
	// client-only commands
	Cmd_AddCommand(CF_SHARED, "wait", Cmd_Wait_f, "make script execution wait for next rendered frame");
	Cmd_AddCommand(CF_CLIENT, "cprint", Cmd_Centerprint_f, "print something at the screen center"); // Baker: Really?

	// maintenance commands used for upkeep of cvars and saved configs
	Cmd_AddCommand(CF_SHARED, "stuffcmds", Cmd_StuffCmds_f, "execute commandline parameters (must be present in quake.rc script)");
	Cmd_AddCommand(CF_SHARED, "cvar_lockdefaults", Cvar_LockDefaults_f, "stores the current values of all cvars into their default values, only used once during startup after parsing default.cfg");
	Cmd_AddCommand(CF_SHARED, "cvar_resettodefaults_all", Cvar_ResetToDefaults_All_f, "sets all cvars to their locked default values");
	Cmd_AddCommand(CF_SHARED, "cvar_resettodefaults_nosaveonly", Cvar_ResetToDefaults_NoSaveOnly_f, "sets all non-saved cvars to their locked default values (variables that will not be saved to config.cfg)");
	Cmd_AddCommand(CF_SHARED, "cvar_resettodefaults_saveonly", Cvar_ResetToDefaults_SaveOnly_f, "sets all saved cvars to their locked default values (variables that will be saved to config.cfg)");



	// general console commands used in multiple environments
	Cmd_AddCommand(CF_SHARED, "exec", Cmd_Exec_f, "execute a script file");
	Cmd_AddCommand(CF_SHARED, "echo",Cmd_Echo_f, "print a message to the console (useful in scripts)");
	Cmd_AddCommand(CF_SHARED, "alias",Cmd_Alias_f, "create a script function (parameters are passed in as $X (being X a number), $* for all parameters, $X- for all parameters starting from $X). Without arguments show the list of all alias");
	Cmd_AddCommand(CF_SHARED, "unalias",Cmd_UnAlias_f, "remove an alias");
	Cmd_AddCommand(CF_SHARED, "set", Cvar_Set_f, "create or change the value of a console variable");
	Cmd_AddCommand(CF_SHARED, "seta", Cvar_SetA_f, "create or change the value of a console variable that will be saved to config.cfg");
	Cmd_AddCommand(CF_SHARED, "unset", Cvar_Del_f, "delete a cvar (does not work for static ones like _cl_name, or read-only ones)");

#ifdef FILLALLCVARSWITHRUBBISH
	Cmd_AddCommand(CF_SHARED, "fillallcvarswithrubbish", Cvar_FillAll_f, "fill all cvars with a specified number of characters to provoke buffer overruns");
#endif /* FILLALLCVARSWITHRUBBISH */

	// 2000-01-09 CmdList, CvarList commands By Matthias "Maddes" Buecher
	// Added/Modified by EvilTypeGuy eviltypeguy@qeradiant.com
	Cmd_AddCommand(CF_SHARED, "cmdlist", Cmd_List_f, "lists all console commands beginning with the specified prefix or matching the specified wildcard pattern");
	Cmd_AddCommand(CF_SHARED, "cvarlist", Cvar_List_f, "lists all console variables beginning with the specified prefix or matching the specified wildcard pattern, 'cvarlist changed' will print only changed cvars");
	Cmd_AddCommand(CF_SHARED, "apropos", Cmd_Apropos_f, "lists all console variables/commands/aliases containing the specified string in the name or description");
	Cmd_AddCommand(CF_SHARED, "find", Cmd_Apropos_f, "lists all console variables/commands/aliases containing the specified string in the name or description");

	Cmd_AddCommand(CF_SHARED, "defer", Cmd_Defer_f, "execute a command in the future");

	// DRESK - 5/14/06
	// Support Doom3-style Toggle Command

	// Baker r3173: inc, dec, cvar_reset commands
	Cmd_AddCommand(CF_SHARED | CF_CLIENT_FROM_SERVER, "toggle", Cmd_Toggle_f, "toggles a console variable's values (use for more info)");
	Cmd_AddCommand (CF_SHARED | CF_CLIENT_FROM_SERVER, "inc", Cmd_Inc_f, "Increases value of cvar by 1 or provided amount [Zircon]"); // Baker r1246: inc, dec
	Cmd_AddCommand (CF_SHARED | CF_CLIENT_FROM_SERVER, "dec", Cmd_Dec_f, "Decreases value of cvar by 1 or provided amount [Zircon]"); // Baker r1246: inc, dec
	Cmd_AddCommand (CF_SHARED | CF_CLIENT_FROM_SERVER, "cvar_reset", Cvar_Reset_f, "lists all console variables beginning with the specified prefix or matching the specified wildcard pattern"); // Baker r3173: cvar_reset
}

