#if !defined(_WIN32) && !defined(MACOSX) && !defined(CORE_SDL)

#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>

#include <signal.h>

#include "darkplaces.h"



// =======================================================================
// General routines
// =======================================================================

void Sys_Shutdown (void)
{
	fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~O_NONBLOCK);

	fflush(stdout);
}

/*
===============================================================================

SYSTEM IO

===============================================================================
*/

void Sys_Error (const char *error, ...)
{
	va_list argptr;
	char text[MAX_INPUTLINE_16384];

// change stdin to non blocking
	fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~O_NONBLOCK);

	va_start (argptr,error);
	dpvsnprintf (text, sizeof (text), error, argptr);
	va_end (argptr);

	Con_PrintLinef (CON_ERROR "Engine Error: %s", text);

	//Host_Shutdown ();
	exit (1);
}

void Sys_PrintToTerminal(const char *text)
{
	if(sys.outfd < 0)
		return;
	// BUG: for some reason, NDELAY also affects stdout (1) when used on stdin (0).
	// this is because both go to /dev/tty by default!
	{
		int origflags = fcntl (sys.outfd, F_GETFL, 0);
		fcntl (sys.outfd, F_SETFL, origflags & ~O_NONBLOCK);

		while(*text)
		{
			fs_offset_t written = (fs_offset_t)write(sys.outfd, text, (int)strlen(text));
			if(written <= 0)
				break; // sorry, I cannot do anything about this error - without an output
			text += written;
		}
		fcntl (sys.outfd, F_SETFL, origflags);
	}
	//fprintf(stdout, "%s", text);
}

char *Sys_ConsoleInput(void)
{
	static char text[MAX_INPUTLINE_16384];
	static unsigned int len = 0;
	fd_set fdset;
	struct timeval timeout;
	FD_ZERO(&fdset);
	FD_SET(0, &fdset); // stdin
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	if (select (1, &fdset, NULL, NULL, &timeout) != -1 && FD_ISSET(0, &fdset))
	{
		len = read (0, text, sizeof(text) - 1);
		if (len >= 1)
		{
			// rip off the \n and terminate
			// div0: WHY? console code can deal with \n just fine
			// this caused problems with pasting stuff into a terminal window
			// so, not ripping off the \n, but STILL keeping a NUL terminator
			text[len] = 0;
			return text;
		}
	}

	return NULL;
}

// Returns 1 on success, 0 on failure
int Sys_SetClipboardData(const char *text_to_clipboard)
{
	return false; // Dedicated server, this fails
}

char *Sys_GetClipboardData_Alloc (void)
{
	return NULL;
}

int main (int argc, char **argv)
{
	signal(SIGFPE, SIG_IGN);
	sys.selffd = -1;
	sys.argc = argc;
	sys.argv = (const char **)argv;
	Sys_ProvideSelfFD();

	// COMMANDLINEOPTION: sdl: -noterminal disables console output on stdout
	if(Sys_CheckParm("-noterminal"))
		sys.outfd = -1;
	// COMMANDLINEOPTION: sdl: -stderr moves console output to stderr
	else if(Sys_CheckParm("-stderr"))
		sys.outfd = 2;
	else
		sys.outfd = 1;

	fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | O_NONBLOCK);

	// used by everything
	Memory_Init();

	Host_Main();

	Sys_Quit(0);

	return 0;
}

qbool sys_supportsdlgetticks = false;
unsigned int Sys_SDL_GetTicks (void)
{
	Sys_Error("Called Sys_SDL_GetTicks on non-SDL target");
	return 0;
}
void Sys_SDL_Delay (unsigned int milliseconds)
{
	Sys_Error("Called Sys_SDL_Delay on non-SDL target");
}

SBUF___ char *Sys_Getcwd_SBuf (void) // No trailing slash

{
	static char workingdir[MAX_OSPATH_EX_1024];

	if (getcwd (workingdir, sizeof(workingdir) - 1)) {
		return workingdir;
	} else return NULL;
}

// Folder must exist.  It must be a folder.
int Sys_Folder_Open_Folder_Must_Exist (const char *path_to_file)
{
//  xdg-open is a desktop-independent tool for configuring the default applications of a user
	if ( fork() == 0) {
		execl ("/usr/bin/xdg-open", "xdg-open", path_to_file, (char *)0);
		//cleanup_on_exit();  /* clean up before exiting */
		exit(3);
	}
	return true;
}

// copies given text to clipboard.  Text can't be NULL
int Sys_Clipboard_Set_Text (const char *text_to_clipboard)
{
	return 0;
}

int Sys_Clipboard_Set_Image_BGRA_Is_Ok (const unsigned *bgra, int width, int height)
{
    return false;
}

#endif  // !CORE_SDL
