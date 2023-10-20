#ifdef CORE_SDL

#ifdef _WIN32
#include <io.h> // Include this BEFORE darkplaces.h because it uses strncpy which trips DP_STATIC_ASSERT
#include "conio.h"
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#endif

#ifdef __ANDROID__
	#include <android/log.h>
#endif // __ANDROID__

#include <signal.h>

// Include this BEFORE darkplaces.h because it breaks wrapping
// _Static_assert. Cloudwalk has no idea how or why so don't ask.

#include <SDL.h>

#include "darkplaces.h"

#ifdef _WIN32
	#ifdef _MSC_VER
		#pragma comment(lib, "sdl2.lib")
		#pragma comment(lib, "sdl2main.lib")

	#endif // _MSC_VER

#endif //  _WIN32

sys_t sys;

// =======================================================================
// General routines
// =======================================================================

void Sys_Shutdown (void)
{
#ifdef __ANDROID__
	Sys_AllowProfiling(false);
#endif
#ifndef _WIN32
	fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~O_NONBLOCK);
#endif
	fflush(stdout);
	SDL_Quit();
}

static qbool nocrashdialog;
void Sys_Error (const char *error, ...)
{
	va_list argptr;
	char string[MAX_INPUTLINE];

// change stdin to non blocking
#ifndef _WIN32
	fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~O_NONBLOCK);
#endif

	va_start (argptr,error);
	dpvsnprintf (string, sizeof (string), error, argptr);
	va_end (argptr);

	Con_Printf(CON_ERROR "Engine Error: %s\n", string);
	
	if(!nocrashdialog)
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Engine Error", string, NULL);

	//Host_Shutdown ();
	exit (1);
}

void Sys_PrintToTerminal(const char *text)
{
#ifdef __ANDROID__
	#define CORE_ANDROID_LOG_TAG "CoreMain"
	__android_log_print(ANDROID_LOG_INFO, CORE_ANDROID_LOG_TAG, "%s", text);
#else
	// !__ANDROID__
	if(sys.outfd < 0)
		return;

	#ifndef _WIN32
		// BUG: for some reason, NDELAY also affects stdout (1) when used on stdin (0).
		// this is because both go to /dev/tty by default!
		{
			int origflags = fcntl (sys.outfd, F_GETFL, 0);
			fcntl (sys.outfd, F_SETFL, origflags & ~O_NONBLOCK);
	#endif // !_WIN32

	#ifdef _WIN32
		# define write _write
	#endif // _WIN32
		while(*text)
		{
			fs_offset_t written = (fs_offset_t)write(sys.outfd, text, (int)strlen(text));
			if(written <= 0)
				break; // sorry, I cannot do anything about this error - without an output
			text += written;
		}
	#ifndef _WIN32
			fcntl (sys.outfd, F_SETFL, origflags);
		}
	#endif // !_WIN32
	//fprintf(stdout, "%s", text);
#endif // !__ANDROID__
}

char *Sys_ConsoleInput(void)
{
	static char text[MAX_INPUTLINE];
	int len = 0;
#ifdef _WIN32
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
#else // !_WIN32
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
#endif // !_WIN32
	return NULL;
}

char *Sys_GetClipboardData (void)
{
	char *data = NULL;
	char *cliptext;

	cliptext = SDL_GetClipboardText();
	if (cliptext != NULL) {
		size_t allocsize;
		allocsize = min(MAX_INPUTLINE, strlen(cliptext) + 1);
		data = (char *)Z_Malloc (allocsize);
		strlcpy (data, cliptext, allocsize);
		SDL_free(cliptext);
	}

	return data;
}

// Baker r8001: zircon_command_line.txt support
#define MAX_NUM_Q_ARGVS_50	50
static int fake_argc; char *fake_argv[MAX_NUM_Q_ARGVS_50];


int main (int argc, char *argv[])
{
	signal(SIGFPE, SIG_IGN);

#ifdef __ANDROID__
	Sys_AllowProfiling(true);
#endif

	sys.selffd = -1;
	sys.argc = argc;
	sys.argv = (const char **)argv;

	//
    // Baker: zircon_command_line.txt
    //
    
#ifdef CORE_XCODE // MACOSX
	// This is to make zircon_command_line.txt work for a Mac .app
    if (strstr(sys_argv[0], ".app/") && strstr(sys_argv[0], "/Xcode/") == NULL) {
        char *split;
        strlcpy(fs_basedir, com_argv[0], sizeof(fs_basedir));
        split = strstr(fs_basedir, ".app/");

        // Baker: find first '/' after .app, 0 it out
        while (split > fs_basedir && *split != '/')
            split--;
        *split = 0;
        
        // Change to dir of .app, should make zircon_command_line.txt work
        chdir (fs_basedir);
    } // if .app in name, should be 100%
#endif // XCODE
    
	if (sys.argc == 1 /*only the exe*/) {
		char	cmdline_fake[MAX_INPUTLINE];
		
		const char *s_fp = 
#ifdef __ANDROID__
		
			"/sdcard/zircon/"
#endif
			"zircon_command_line.txt";

		if (File_Exists (s_fp)) {
			size_t s_temp_size = 0; const char *s_temp = (const char *)File_To_String_Alloc (s_fp, &s_temp_size);
			c_strlcpy (cmdline_fake, "quake_engine "); // arg0 is engine and ignored by Sys_CheckParm
			if (s_temp) {
				c_strlcat (cmdline_fake, s_temp);
				String_Edit_Whitespace_To_Space (cmdline_fake); // Make tabs, newlines into spaces.
				
				String_Command_String_To_Argv (/*destructive edit*/ cmdline_fake, &fake_argc, fake_argv, MAX_NUM_Q_ARGVS_50);
				sys.argc = fake_argc;
				sys.argv = (const char **)fake_argv;

				free ((void *)s_temp);
			} // if
		} // if exists
	} // if 1 cmd arg the exe

	//
    // End Baker: zircon_command_line.txt
    //


	// Sys_Error this early in startup might screw with automated
	// workflows or something if we show the dialog by default.
	nocrashdialog = true;

	Sys_ProvideSelfFD();

	// COMMANDLINEOPTION: -nocrashdialog disables "Engine Error" crash dialog boxes
	if(!Sys_CheckParm("-nocrashdialog"))
		nocrashdialog = false;
	// COMMANDLINEOPTION: sdl: -noterminal disables console output on stdout
	if(Sys_CheckParm("-noterminal"))
		sys.outfd = -1;
	// COMMANDLINEOPTION: sdl: -stderr moves console output to stderr
	else if(Sys_CheckParm("-stderr"))
		sys.outfd = 2;
	else
		sys.outfd = 1;

#ifndef _WIN32
	fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | O_NONBLOCK);
#endif // !_WIN32

	// we don't know which systems we'll want to init, yet...
	SDL_Init(0);

	// used by everything
	Memory_Init();

	Host_Main();

	Sys_Quit(0);
	
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

// Baker r3102: "folder" command
#ifdef _WIN32
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
#else // !_WIN32
	// Folder must exist.  It must be a folder.
	int Sys_Folder_Open_Folder_Must_Exist (const char *path_to_file)
	{
#ifdef CORE_XCODE // MACOSX
        int _Shell_Folder_Open_Folder_Must_Exist (const char *path_to_file);
        return _Shell_Folder_Open_Folder_Must_Exist(path_to_file);
#else
	//  xdg-open is a desktop-independent tool for configuring the default applications of a user
		if ( fork() == 0) {
			execl ("/usr/bin/xdg-open", "xdg-open", path_to_file, (char *)0);
			//cleanup_on_exit();  /* clean up before exiting */
			exit(3);
		}
#endif // LINUX etc
		return true;
	}
#endif // _WIN32

// copies given text to clipboard.  Text can't be NULL
int Sys_Clipboard_Set_Text (const char *text_to_clipboard)
{
#ifdef CONFIG_MENU	// We are using this as the litmus test that we have a client
	return SDL_SetClipboardText(text_to_clipboard) == 0;
#else
	return 0;
#endif

}


#define MAX_OSPATH_EX_1024 1024 // Hopefully large enough

#ifdef _WIN32
	// empty
	SBUF___ const char *Sys_Getcwd_SBuf (void) // No trailing slash
	{
		static char workingdir[MAX_OSPATH_EX_1024];
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
		static char workingdir[MAX_OSPATH_EX_1024];

		if (getcwd (workingdir, sizeof(workingdir) - 1)) {
			return workingdir;
		} else return NULL;
	}

	const char *Sys_Binary_URL_SBuf (void)
	{
		static char binary_url[MAX_OSPATH_EX_1024];
		if (!binary_url[0]) {
			pid_t pid = getpid();
			int length;

			char linkname[MAX_OSPATH_EX_1024];
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
