#ifndef _MSC_VER

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
#endif

#include <signal.h>

#ifdef _MSC_VER
	#include <SDL2/SDL.h>
#else
	#include <SDL.h>
#endif


#ifdef WIN32
#ifdef _MSC_VER
#if SDL_MAJOR_VERSION == 1
#pragma comment(lib, "sdl.lib")
#pragma comment(lib, "sdlmain.lib")
#else
#pragma comment(lib, "sdl2.lib")
#pragma comment(lib, "sdl2main.lib")
#endif
#endif
#endif

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
#else
	if(outfd < 0)
		return;
#ifdef FNDELAY
	// BUG: for some reason, NDELAY also affects stdout (1) when used on stdin (0).
	// this is because both go to /dev/tty by default!
	{
		int origflags = fcntl (outfd, F_GETFL, 0);
		fcntl (outfd, F_SETFL, origflags & ~FNDELAY);
#endif
#ifdef WIN32
#define write _write
#endif
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
#endif
	//fprintf(stdout, "%s", text);
#endif
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
#else
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
#endif
	}
	return NULL;
}

int Sys_SetClipboardData(const char *text_to_clipboard)
{
	return !SDL_SetClipboardText(text_to_clipboard);
}

//char *Sys_GetClipboardData (void)
//{
//#if SDL_MAJOR_VERSION != 1
//	char *data = NULL;
//	char *cliptext;
//
//	cliptext = SDL_GetClipboardText();
//	if (cliptext != NULL) {
//		size_t allocsize;
//		allocsize = strlen(cliptext) + 1;
//		data = (char *)Z_Malloc (allocsize);
//		strlcpy (data, cliptext, allocsize);
//		SDL_free(cliptext);
//	}
//
//	return data;
//#elif defined(WIN32)
//	char *data = NULL;
//	char *cliptext;
//
//	if (OpenClipboard (NULL) != 0)
//	{
//		HANDLE hClipboardData;
//
//		if ((hClipboardData = GetClipboardData (CF_TEXT)) != 0)
//		{
//			if ((cliptext = (char *)GlobalLock (hClipboardData)) != 0)
//			{
//				size_t allocsize;
//				allocsize = GlobalSize (hClipboardData) + 1;
//				data = (char *)Z_Malloc (allocsize);
//				strlcpy (data, cliptext, allocsize);
//				GlobalUnlock (hClipboardData);
//			}
//		}
//		CloseClipboard ();
//	}
//	return data;
//#else
//	return NULL;
//#endif
//}

void Sys_InitConsole (void)
{
}

int main (int argc, char *argv[])
{
	signal(SIGFPE, SIG_IGN);

#ifdef __ANDROID__
	Sys_AllowProfiling(true);
#endif

	com_argc = argc;
	com_argv = (const char **)argv;
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

qboolean sys_supportsdlgetticks = true;
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
#else
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
#endif

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

SBUF___ const char *Sys_Getcwd_SBuf (void) // No trailing slash

{
	static char workingdir[MAX_OSPATH_EX];

	if (getcwd (workingdir, sizeof(workingdir) - 1)) {
		return workingdir;
	} else return NULL;
}

#ifdef _WIN32
#else
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

#else
	#ifdef CORE_SDL
		#pragma comment(lib, "sdl2.lib")
		#pragma comment(lib, "sdl2main.lib")
	#endif // ! CORE_SDL
#endif // ! _MSC_VER
