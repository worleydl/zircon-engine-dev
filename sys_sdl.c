#ifdef CORE_SDL

#ifdef WIN32
	#include <io.h>
	#include "conio.h"
#else
	#include <unistd.h>
	#include <fcntl.h>
	#include <sys/time.h>
#endif

#ifdef __ANDROID__
	#include <android/log.h>

	#ifndef FNDELAY
		#define FNDELAY		O_NDELAY
	#endif
#endif // __ANDROID__

#include <signal.h>

#ifdef _MSC_VER
	#include <SDL2/SDL.h>
#else
	#include <SDL.h>
#endif // _MSC_VER


#ifdef WIN32
	#ifdef _MSC_VER
			#pragma comment(lib, "sdl2.lib")
			#pragma comment(lib, "sdl2main.lib")

	#endif // _MSC_VER

#endif //  WIN32

#include "quakedef.h"

// =======================================================================
// General routines
// =======================================================================

void Sys_Shutdown (void)
{
#ifdef __ANDROID__
	Sys_AllowProfiling(false);
#endif
#ifndef WIN32
	fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);
#endif
	fflush(stdout);
	SDL_Quit();
}


void Sys_Error (const char *error, ...)
{
	va_list argptr;
	char string[MAX_INPUTLINE];

// change stdin to non blocking
#ifndef WIN32
	fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);
#endif

	va_start (argptr,error);
	dpvsnprintf (string, sizeof (string), error, argptr);
	va_end (argptr);

	Con_Printf ("Quake Error: %s\n", string);

#ifdef WIN32
	MessageBox(NULL, string, "Quake Error", MB_OK | MB_SETFOREGROUND | MB_ICONSTOP);
#endif

	Host_Shutdown ();
	exit (1);
}

static int outfd = 1;
void Sys_PrintToTerminal(const char *text)
{
#ifdef __ANDROID__
	if (developer.integer > 0)
	{
		__android_log_write(ANDROID_LOG_DEBUG, com_argv[0], text);
	}
#else // ! __ANDROID__
		if(outfd < 0)
			return;
	#ifdef FNDELAY
		// BUG: for some reason, NDELAY also affects stdout (1) when used on stdin (0).
		// this is because both go to /dev/tty by default!
		{
			int origflags = fcntl (outfd, F_GETFL, 0);
			fcntl (outfd, F_SETFL, origflags & ~FNDELAY);
	#endif // FNDELAY
	#ifdef WIN32
		#define write _write
	#endif // WIN32
			while(*text)
			{
				fs_offset_t written = (fs_offset_t)write(outfd, text, (int)strlen(text));
				if(written <= 0)
					break; // sorry, I cannot do anything about this error - without an output
				text += written;
			}
	#ifdef FNDELAY
			fcntl (outfd, F_SETFL, origflags);
		}
	#endif // FNDELAY
		//fprintf(stdout, "%s", text);
#endif // __ANDROID__
}

char *Sys_ConsoleInput(void)
{
//	if (cls.state == ca_dedicated)
	{
		static char text[MAX_INPUTLINE];
		int len = 0;
#ifdef WIN32
		int c;

		// read a line out
		while (_kbhit ())
		{
			c = _getch ();
			_putch (c);
			if (c == '\r')
			{
				text[len] = 0;
				_putch ('\n');
				len = 0;
				return text;
			}
			if (c == 8)
			{
				if (len)
				{
					_putch (' ');
					_putch (c);
					len--;
					text[len] = 0;
				}
				continue;
			}
			text[len] = c;
			len++;
			text[len] = 0;
			if (len == sizeof (text))
				len = 0;
		}
#else // !WIN32
		fd_set fdset;
		struct timeval timeout;
		FD_ZERO(&fdset);
		FD_SET(0, &fdset); // stdin
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;
		if (select (1, &fdset, NULL, NULL, &timeout) != -1 && FD_ISSET(0, &fdset))
		{
			len = read (0, text, sizeof(text));
			if (len >= 1)
			{
				// rip off the \n and terminate
				text[len-1] = 0;
				return text;
			}
		}
#endif // WIN32
	}
	return NULL;
}

int Sys_SetClipboardData(const char *text_to_clipboard)
{
	return !SDL_SetClipboardText(text_to_clipboard);
}

void Sys_InitConsole (void)
{
}

//#ifdef WIN32
// Short: Command line to argv
// Notes: None.
// Unit Test:
#define MAX_ASCII_PRINTABLE_126		126 // TILDE
void String_Command_String_To_Argv (char *cmdline, int *numargc, char **argvz, int maxargs)
{
	// Baker: This converts a commandline in arguments and an argument count.
	// Requires cmd_line, pointer to argc, argv[], maxargs
	while (*cmdline && (*numargc < maxargs))
	{
#if 0
		const char *start = cmdline;
		int len;
#endif
		// Advance beyond spaces, white space, delete and non-ascii characters
		// ASCII = chars 0-127, where chars > 127 = ANSI codepage 1252
		while (*cmdline && (*cmdline <= SPACE_CHAR_32 || MAX_ASCII_PRINTABLE_126 <= *cmdline ) ) // Was 127, but 127 is DELETE
			cmdline++;

		switch (*cmdline)
		{
		case 0:  // null terminator
			break;

		case '\"': // quote

			// advance past the quote
			cmdline++;

			argvz[*numargc] = cmdline;
			(*numargc)++;

			// continue until hit another quote or null terminator
			while (*cmdline && *cmdline != '\"')
				cmdline++;
#if 0
			len = cmdline - start;
#endif
			break;

		default:
			argvz[*numargc] = cmdline;
			(*numargc)++;

			// Advance until reach space, white space, delete or non-ascii
			while (*cmdline && (SPACE_CHAR_32 < *cmdline && *cmdline <= MAX_ASCII_PRINTABLE_126  ) ) // Was < 127
				cmdline++;
#if 0
			len = cmdline - start;
#endif
		} // End of switch

		// If more advance the cursor past what should be whitespace
		if (*cmdline)
		{
			*cmdline = 0;
			cmdline++;
		}

	} // end of while (*cmd_line && (*numargc < maxargs)
}

	#define MAX_NUM_Q_ARGVS_50	50
	static int fake_argc; char *fake_argv[MAX_NUM_Q_ARGVS_50];
//#endif // WIN32

int main (int argc, char *argv[])
{
	signal(SIGFPE, SIG_IGN);

#ifdef __ANDROID__
	Sys_AllowProfiling(true);
#endif

	com_argc = argc;
	com_argv = (const char **)argv;

// #ifdef WIN32
	if (com_argc == 1 /*only the exe*/) {
		char	cmdline_fake[MAX_INPUTLINE];
		const char *s_fp = "zircon_command_line.txt"; // File_Getcwd_SBuf not needed
		if (File_Exists (s_fp)) {
			size_t s_temp_size = 0; const char *s_temp = (const char *)File_To_String_Alloc (s_fp, &s_temp_size);
			c_strlcpy (cmdline_fake, "quake_engine "); // arg0 is engine and ignored by COM_CheckParm
			if (s_temp) {
				c_strlcat (cmdline_fake, s_temp);
				String_Edit_Whitespace_To_Space (cmdline_fake); // Make tabs, newlines into spaces.
				
				String_Command_String_To_Argv (/*destructive edit*/ cmdline_fake, &fake_argc, fake_argv, MAX_NUM_Q_ARGVS_50);
				com_argc = fake_argc;
				com_argv = (const char **)fake_argv;

				free ((void *)s_temp);

			} // if
		} // if exists
	} // if 1 cmd arg the exe
// #endif // WIN32

	Sys_ProvideSelfFD();

	// COMMANDLINEOPTION: sdl: -noterminal disables console output on stdout
	if(COM_CheckParm("-noterminal"))
		outfd = -1;
	// COMMANDLINEOPTION: sdl: -stderr moves console output to stderr
	else if(COM_CheckParm("-stderr"))
		outfd = 2;
	else
		outfd = 1;

#ifndef WIN32
	fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);
#endif

	// we don't know which systems we'll want to init, yet...
	SDL_Init(0);

	Host_Main();

	return 0;
}

qbool sys_supportsdlgetticks = true;
unsigned int Sys_SDL_GetTicks (void)
{
	return SDL_GetTicks();
}
void Sys_SDL_Delay (unsigned int milliseconds)
{
	SDL_Delay(milliseconds);
}

#ifdef WIN32
	#include "Shellapi.h" // Never needed this before?
	// Folder must exist.  It must be a folder.
	static char *windows_style_url_alloc (const char *path_to_file)
	{
		RETURNING_ALLOC___ char *windows_style_url_o = strdup (path_to_file);
		File_URL_Edit_SlashesBack_Like_Windows (windows_style_url_o);
		RETURNING___ return windows_style_url_o;
	}

	int Sys_Folder_Open_Folder_Must_Exist (const char *path_to_file)
	{
		AUTO_ALLOC___ char *windows_style_url_a = windows_style_url_alloc (path_to_file);
		int ret = ShellExecute(0, "Open", "explorer.exe", windows_style_url_a, NULL, SW_NORMAL) != 0;
		AUTO_FREE___ free (windows_style_url_a);
		return ret;
	}
#else // !WIN32
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
#endif // WIN32

// copies given image to clipbard
int Sys_Clipboard_Set_Image (unsigned *rgba, int width, int height)
{
	return 1; // I have not discovered a method for this for Linux yet
}

// copies given text to clipboard.  Text can't be NULL
int Sys_Clipboard_Set_Text (const char *text_to_clipboard)
{
#ifdef CONFIG_MENU	// We are using this as the litmus test that we have a client
	return SDL_SetClipboardText(text_to_clipboard) == 0;
#else
	return 0;
#endif

}

// This must return Z_Malloc text

char *Sys_GetClipboardData (void)
{

#ifdef CONFIG_MENU
	static char sbuf[MAX_INPUTLINE /*16384*/];
	char *_text = SDL_GetClipboardText();  //(!SDL_HasClipboardText())
	if (_text) {
		int slen = strlen(_text);
		int slen2 = Smallest( (MAX_INPUTLINE - 1), slen);
//		char *text_out  (char *)Z_Malloc (slen + ONE_CHAR_1);
		strlcpy (sbuf, _text, slen2 + ONE_CHAR_1);
		return sbuf;
	}
	return NULL;
#else
	return NULL;
#endif
}

#define MAX_OSPATH_EX 1024 // Hopefully large enough

#ifdef _WIN32
	// empty
	SBUF___ const char *Sys_Getcwd_SBuf (void) // No trailing slash
	{
		static char workingdir[MAX_OSPATH_EX];
		int ok = 0;

		if (GetCurrentDirectory (sizeof(workingdir), workingdir)) { // No trailing slash in this or getcwd
			if (workingdir[strlen(workingdir)-1] == '/')
				workingdir[strlen(workingdir)-1] = 0;

			File_URL_Edit_SlashesForward_Like_Unix (workingdir);

			ok = 1;
		}

		if (ok == 0) return NULL;
			//log_fatal ("Couldn't determine current directory");

		return workingdir;
	}
#else // ! _WIN32

	

	SBUF___ const char *Sys_Getcwd_SBuf (void) // No trailing slash

	{
		static char workingdir[MAX_OSPATH_EX];

		if (getcwd (workingdir, sizeof(workingdir) - 1)) {
			return workingdir;
		} else return NULL;
	}

	const char *Sys_Binary_URL_SBuf (void)
	{
		static char binary_url[MAX_OSPATH_EX];
		if (!binary_url[0]) {
			pid_t pid = getpid();
			int length;

			char linkname[MAX_OSPATH_EX];
			dpsnprintf (linkname, sizeof(linkname), "/proc/%d/exe", pid);

			length = readlink (linkname, binary_url, sizeof(binary_url)-1);

    		// In case of an error, leave the handling up to the caller
			if (length == -1 || length >= (int)sizeof(binary_url) ) {
				//log_fatal ("Couldn't determine executable directory");
				return NULL; //binary_url;
			}

    		binary_url[length] = 0;
		}
		return binary_url;
	}
#endif // _WIN32

#endif // CORE_SDL
