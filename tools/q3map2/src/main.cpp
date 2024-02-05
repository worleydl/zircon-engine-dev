/* -------------------------------------------------------------------------------

   Copyright (C) 1999-2007 id Software, Inc. and contributors.
   For a list of contributors, see the accompanying CONTRIBUTORS file.

   This file is part of GtkRadiant.

   GtkRadiant is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   GtkRadiant is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GtkRadiant; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

   -------------------------------------------------------------------------------

   This code has been altered significantly from its original form, to support
   several games based on the Quake III Arena engine, in the form of "Q3Map2."

   ------------------------------------------------------------------------------- */



/* marker */
#define MAIN_C



/* dependencies */
#include "q3map2.h"
#include "table_builder.hpp"

/*
   Random()
   returns a pseudorandom number between 0 and 1
 */

vec_t Random( void ){
	return (vec_t) rand() / RAND_MAX;
}


char *Q_strncpyz( char *dst, const char *src, size_t len ) {
	if ( len == 0 ) {
		abort();
	}

	strncpy( dst, src, len );
	dst[ len - 1 ] = '\0';
	return dst;
}


char *Q_strcat( char *dst, size_t dlen, const char *src ) {
	size_t n = strlen( dst  );

	if ( n > dlen ) {
		abort(); /* buffer overflow */
	}

	return Q_strncpyz( dst + n, src, dlen - n );
}


char *Q_strncat( char *dst, size_t dlen, const char *src, size_t slen ) {
	size_t n = strlen( dst );

	if ( n > dlen ) {
		abort(); /* buffer overflow */
	}

	return Q_strncpyz( dst + n, src, Q_min( slen, dlen - n ) );
}

/*
   ExitQ3Map()
   cleanup routine
 */

static void ExitQ3Map( void ){
	BSPFilesCleanup();
	if ( mapDrawSurfs != NULL ) {
		free( mapDrawSurfs );
	}
}

void printOptions(const std::vector<OptionResult> &options)
{
	TableBuilder table{ 20, 60 };
	for (const auto &option : options)
	{
		if (!option.option.empty())
		{
			table.addRow(option.option + " " + option.value);
		}
		if (!option.desc.empty())
		{
			table.addRow(" " + option.desc);
		}
	}
	auto rows = table.build();
	for (const auto &row : rows)
	{
		Sys_Printf("%s\n", row.c_str());
	}
}

/*
   main()
   q3map mojo...
 */


#if 1
#include 	<direct.h>
#define NEWLINE "\n"

char *File_To_String_Alloc (const char *filename)
{
	char * buffer = 0;
	long length;
	FILE * f = fopen (filename, "rb");

	if (f) {
	  fseek (f, 0, SEEK_END);
	  length = ftell (f);
	  fseek (f, 0, SEEK_SET);
	  buffer = (char *)malloc (length + 1);
	  if (buffer)
	  {
		fread (buffer, 1, length, f);
		buffer[length] = 0;
	  }
	  fclose (f);
	  return buffer;
	}

	return "";
}

#define MAX_ASCII_PRINTABLE_126		126 // TILDE
#define SPACE_CHAR_32 32
void String_Command_String_To_Argv (char *s_cmdline, int *numargc, char **argvz, int maxargs)
{
	// Baker: This converts a commandline in arguments and an argument count.
	// Requires cmd_line, pointer to argc, argv[], maxargs
	while (*s_cmdline && (*numargc < maxargs))
	{
#if 0
		const char *start = s_cmdline;
		int len;
#endif
		// Advance beyond spaces, white space, delete and non-ascii characters
		// ASCII = chars 0-127, where chars > 127 = ANSI codepage 1252
		while (*s_cmdline && (*s_cmdline <= SPACE_CHAR_32 || MAX_ASCII_PRINTABLE_126 <= *s_cmdline ) ) // Was 127, but 127 is DELETE
			s_cmdline++;

		switch (*s_cmdline)
		{
		case 0:  // null terminator
			break;

		case '\"': // quote

			// advance past the quote
			s_cmdline++;

			argvz[*numargc] = s_cmdline;
			(*numargc)++;

			// continue until hit another quote or null terminator
			while (*s_cmdline && *s_cmdline != '\"')
				s_cmdline++;
#if 0
			len = s_cmdline - start;
#endif
			break;

		default:
			argvz[*numargc] = s_cmdline;
			(*numargc)++;

			// Advance until reach space, white space, delete or non-ascii
			while (*s_cmdline && (SPACE_CHAR_32 < *s_cmdline && *s_cmdline <= MAX_ASCII_PRINTABLE_126  ) ) // Was < 127
				s_cmdline++;
#if 0
			len = s_cmdline - start;
#endif
		} // End of switch

		// If more advance the cursor past what should be whitespace
		if (*s_cmdline)
		{
			*s_cmdline = 0;
			s_cmdline++;
		}

	} // end of while (*cmd_line && (*numargc < maxargs)
}

#endif

int main(int argc, char **argv) {
	int i, r;
	double start, end;


	/* we want consistent 'randomness' */
	srand( 0 );

	/* start timer */
	start = I_FloatTime();

	/* this was changed to emit version number over the network */
	// printf( Q3MAP_VERSION "\n" );

	/* set exit call */
	atexit( ExitQ3Map );

	/* set game to default (q3a) */
	game = &games[0];

	/* read general options first */
	for ( i = 1; i < argc; i++ )
	{
		/* -help */
		if (!Q_stricmp(argv[i], "-h") || !Q_stricmp(argv[i], "--help")
			|| !Q_stricmp(argv[i], "-help")) {
			HelpMain(argv[i + 1]);
			return 0;
		}

		/* -connect */
		if (!Q_stricmp(argv[i], "-connect")) {
			i++;
			Broadcast_Setup(argv[i]);
		}

		/* verbose */
		else if (!Q_stricmp(argv[i], "-v")) {
			if (!verbose) {
				verbose = qtrue;
			}
		}

		/* force */
		else if (!Q_stricmp(argv[i], "-force")) {
			force = qtrue;
		}

		/* patch subdivisions */
		else if (!Q_stricmp(argv[i], "-subdivisions")) {
			i++;
			patchSubdivisions = atoi(argv[i]);
			if (patchSubdivisions <= 0) {
				patchSubdivisions = 1;
			}
		}

		/* threads */
		else if (!Q_stricmp(argv[i], "-threads")) {
			i++;
			numthreads = atoi(argv[i]);
		}

		else if (Q_stricmp(argv[i], "-game") == 0) {
			if (++i >= argc) {
				Error("Out of arguments: No game specified after %s", argv[i - 1]);
			}
			game = GetGame(argv[i]);
			if (game == NULL) {
				game = &games[0];
			}
		}
	}

	/* init model library */
	PicoInit();
	PicoSetMallocFunc( safe_malloc );
	PicoSetFreeFunc( free );
	PicoSetPrintFunc( PicoPrintFunc );
	PicoSetLoadFileFunc( PicoLoadFileFunc );
	PicoSetFreeFileFunc( free );

	/* set number of threads */
	ThreadSetDefault();

	/* generate sinusoid jitter table */
	for ( i = 0; i < MAX_JITTERS; i++ )
	{
		jitters[ i ] = sin( i * 139.54152147 );
		//%	Sys_Printf( "Jitter %4d: %f\n", i, jitters[ i ] );
	}

	/* we print out two versions, q3map's main version (since it evolves a bit out of GtkRadiant)
	   and we put the GtkRadiant version to make it easy to track with what version of Radiant it was built with */

	Sys_Printf("\n");
	Sys_Printf("Quake III .map compiler " MAPCOMPILER_VERSION " "  __DATE__ " " __TIME__ "\n");
	Sys_Printf("Patches by Aciz and ryven\n");
	Sys_Printf("https://github.com/isRyven/map-compiler\n");
	Sys_Printf("\n");
	Sys_Printf("Q3Map              - v1.0r (c) 1999 Id Software Inc.  \n");
	Sys_Printf("Q3Map (ydnar)      - v2.5.17                          \n");
	Sys_Printf("Q3Map (NetRadiant) - v2.5.17n                         \n");
	Sys_Printf("%s\n\n", Q3MAP_MOTD);


	//
	// Baker: Look for extra
	//

	// Q: What is working directory?
   char cwd_buf[1024];
   if (_getcwd(cwd_buf, sizeof(cwd_buf)) != NULL) {
	   Sys_Printf("Current working dir: %s" NEWLINE, cwd_buf);
   }
   else {
	   Sys_Printf("Baker: Unable to get current working directory" NEWLINE);
   }

   Sys_Printf("Baker: argc is %d" NEWLINE, argc);
	
   int is_extra = false;
   int is_auto = false;
	int light_argc = -1;
	/* read general options first */
	for ( i = 1; i < argc; i++ ) {
		Sys_Printf ("Baker: argv %02d \"%s\"" NEWLINE, i, argv[i]);
		if (!Q_stricmp(argv[i], "-extra"))
			is_extra = true;
		if (!Q_stricmp(argv[i], "-light"))
			light_argc = i;
		if (!Q_stricmp(argv[i], "-auto"))
			is_auto = true;
	} // for

	Sys_Printf("Baker: Read argc" NEWLINE);

	// If extras
	if (light_argc == -1) {
		Sys_Printf("Baker: -light NOT detected" NEWLINE);
		goto nofake_light_commandline;
	}

	if (is_auto == false) {
		Sys_Printf("Baker: -auto NOT detected" NEWLINE);
		goto nofake_light_commandline;
	}

	Sys_Printf("Baker: -light detected" NEWLINE);

	char *s_config = is_extra ? "#light_extra.txt" : "#light_normal.txt";
	char *cmdline_fake_extra = File_To_String_Alloc (s_config);

	if (!cmdline_fake_extra || !cmdline_fake_extra[0]) {
		Sys_Printf("Baker: could not open %s" NEWLINE, s_config);
		goto nofake_light_commandline;
	}

	Sys_Printf("%s: %s" NEWLINE, s_config, cmdline_fake_extra);
	const char *s_mapname = argv[argc - 1];
	int smash = argc - 1;

	Sys_Printf("map name is \"%s\"" NEWLINE, s_mapname);

	// Baker: Read extras command line
	{
		#define MAX_NUM_Q_ARGVS_50	50
		static int fake_argc; char *fake_argv[MAX_NUM_Q_ARGVS_50];

		Sys_Printf("%s: %s" NEWLINE, s_config, cmdline_fake_extra);

		String_Command_String_To_Argv (/*destructive edit*/ cmdline_fake_extra, &fake_argc, fake_argv, MAX_NUM_Q_ARGVS_50);

		Sys_Printf("Start argc %d, adding %d" NEWLINE, argc, fake_argc);

		argc --;
		for (int j = 0; j < fake_argc; j++) {
			argv[smash + j] = fake_argv[j];
			argc++;
		} // for

		// Baker: We need to put this on the end
		argv[argc] = (char *)s_mapname;
		argc++;
		Sys_Printf("Added to \"%s\" to end" NEWLINE, s_mapname);

		Sys_Printf("End argc %d" NEWLINE, argc);
		for (int j = 1; j < argc; j++) {
			Sys_Printf("argv %03d: %s" NEWLINE, j, argv[j]);
		} // for

	}

nofake_light_commandline:

	// Baker: END MOD

	/* ydnar: new path initialization */
	InitPaths( argc, argv );

	/* set game options */
	if ( !patchSubdivisions ) {
		patchSubdivisions = game->patchSubdivisions;
	}

	/* check if we have enough options left to attempt something */
	if ( argc < 2 ) {
		Error( "Usage: %s [general options] [options] mapfile", argv[ 0 ] );
	}

	for (int i = 0; i < argc; i++) {
		/* fixaas */
		if (!Q_stricmp(argv[i], "-fixaas")) {
			r = FixAASMain(argc - 1, argv + 1);
			break;
		}

		/* analyze */
		else if (!Q_stricmp(argv[i], "-analyze")) {
			r = AnalyzeBSPMain(argc - 1, argv + 1);
			break;
		}

		/* info */
		else if (!Q_stricmp(argv[i], "-info")) {
			r = BSPInfoMain(argc - 2, argv + 2);
			break;
		}

		/* vis */
		else if (!Q_stricmp(argv[i], "-vis")) {
			r = VisMain(argc - 1, argv + 1);
			break;
		}

		/* light */
		else if (!Q_stricmp(argv[i], "-light")) {
			r = LightMain(argc - 1, argv + 1);
			break;
		}

		/* QBall: export entities */
		else if (!Q_stricmp(argv[i], "-exportents")) {
			r = ExportEntitiesMain(argc - 1, argv + 1);
			break;
		}

		/* ydnar: lightmap export */
		else if (!Q_stricmp(argv[i], "-export")) {
			r = ExportLightmapsMain(argc - 1, argv + 1);
			break;
		}

		/* ydnar: lightmap import */
		else if (!Q_stricmp(argv[i], "-import")) {
			r = ImportLightmapsMain(argc - 1, argv + 1);
			break;
		}

		/* ydnar: bsp scaling */
		else if (!Q_stricmp(argv[i], "-scale")) {
			r = ScaleBSPMain(argc - 1, argv + 1);
			break;
		}

		/* ydnar: bsp conversion */
		else if (!Q_stricmp(argv[i], "-convert")) {
			r = ConvertBSPMain(argc - 1, argv + 1);
			break;
		}

		/* div0: minimap */
		else if (!Q_stricmp(argv[i], "-minimap")) {
			r = MiniMapBSPMain(argc - 1, argv + 1);
			break;
		}

		if (i + 1 == argc) {
			r = BSPMain(argc, argv);
		}
	}

	/* emit time */
	end = I_FloatTime();
	Sys_Printf( "%9.0f seconds elapsed\n", end - start );

	/* shut down connection */
	Broadcast_Shutdown();

	/* return any error code */
	return r;
}
