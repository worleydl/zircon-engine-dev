// Baker: nocdaudio

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
// Quake is a trademark of Id Software, Inc., (c) 1996 Id Software, Inc. All
// rights reserved.

#include "darkplaces.h"
#include "cdaudio.h"
#include "sound.h"

// used by menu to ghost CD audio slider
cvar_t cdaudioinitialized = {CF_CLIENT | CF_READONLY, "cdaudioinitialized","0","indicates if CD Audio system is active"};
cvar_t cdaudio = {CF_CLIENT | CF_ARCHIVE,"cdaudio","1","CD playing mode (0 = never access CD drive, 1 = play CD tracks if no replacement available, 2 = play fake tracks if no CD track available, 3 = play only real CD tracks, 4 = play real CD tracks even instead of named fake tracks)"};

#define MAX_PLAYLISTS 10
int music_playlist_active = -1;
int music_playlist_playing = 0; // 0 = not playing, 1 = playing, -1 = tried and failed

cvar_t music_playlist_index = {CF_CLIENT, "music_playlist_index", "-1", "selects which of the music_playlist_ variables is the active one, -1 disables playlists"};
cvar_t music_playlist_list[MAX_PLAYLISTS] =
{
	{CF_CLIENT, "music_playlist_list0", "", "list of tracks to play"},
	{CF_CLIENT, "music_playlist_list1", "", "list of tracks to play"},
	{CF_CLIENT, "music_playlist_list2", "", "list of tracks to play"},
	{CF_CLIENT, "music_playlist_list3", "", "list of tracks to play"},
	{CF_CLIENT, "music_playlist_list4", "", "list of tracks to play"},
	{CF_CLIENT, "music_playlist_list5", "", "list of tracks to play"},
	{CF_CLIENT, "music_playlist_list6", "", "list of tracks to play"},
	{CF_CLIENT, "music_playlist_list7", "", "list of tracks to play"},
	{CF_CLIENT, "music_playlist_list8", "", "list of tracks to play"},
	{CF_CLIENT, "music_playlist_list9", "", "list of tracks to play"}
};
cvar_t music_playlist_current[MAX_PLAYLISTS] =
{
	{CF_CLIENT, "music_playlist_current0", "0", "current track index to play in list"},
	{CF_CLIENT, "music_playlist_current1", "0", "current track index to play in list"},
	{CF_CLIENT, "music_playlist_current2", "0", "current track index to play in list"},
	{CF_CLIENT, "music_playlist_current3", "0", "current track index to play in list"},
	{CF_CLIENT, "music_playlist_current4", "0", "current track index to play in list"},
	{CF_CLIENT, "music_playlist_current5", "0", "current track index to play in list"},
	{CF_CLIENT, "music_playlist_current6", "0", "current track index to play in list"},
	{CF_CLIENT, "music_playlist_current7", "0", "current track index to play in list"},
	{CF_CLIENT, "music_playlist_current8", "0", "current track index to play in list"},
	{CF_CLIENT, "music_playlist_current9", "0", "current track index to play in list"},
};
cvar_t music_playlist_random[MAX_PLAYLISTS] =
{
	{CF_CLIENT, "music_playlist_random0", "0", "enables random play order if 1, 0 is sequential play"},
	{CF_CLIENT, "music_playlist_random1", "0", "enables random play order if 1, 0 is sequential play"},
	{CF_CLIENT, "music_playlist_random2", "0", "enables random play order if 1, 0 is sequential play"},
	{CF_CLIENT, "music_playlist_random3", "0", "enables random play order if 1, 0 is sequential play"},
	{CF_CLIENT, "music_playlist_random4", "0", "enables random play order if 1, 0 is sequential play"},
	{CF_CLIENT, "music_playlist_random5", "0", "enables random play order if 1, 0 is sequential play"},
	{CF_CLIENT, "music_playlist_random6", "0", "enables random play order if 1, 0 is sequential play"},
	{CF_CLIENT, "music_playlist_random7", "0", "enables random play order if 1, 0 is sequential play"},
	{CF_CLIENT, "music_playlist_random8", "0", "enables random play order if 1, 0 is sequential play"},
	{CF_CLIENT, "music_playlist_random9", "0", "enables random play order if 1, 0 is sequential play"},
};
cvar_t music_playlist_sampleposition[MAX_PLAYLISTS] =
{
	{CF_CLIENT, "music_playlist_sampleposition0", "-1", "resume position for track, -1 restarts every time"},
	{CF_CLIENT, "music_playlist_sampleposition1", "-1", "resume position for track, -1 restarts every time"},
	{CF_CLIENT, "music_playlist_sampleposition2", "-1", "resume position for track, -1 restarts every time"},
	{CF_CLIENT, "music_playlist_sampleposition3", "-1", "resume position for track, -1 restarts every time"},
	{CF_CLIENT, "music_playlist_sampleposition4", "-1", "resume position for track, -1 restarts every time"},
	{CF_CLIENT, "music_playlist_sampleposition5", "-1", "resume position for track, -1 restarts every time"},
	{CF_CLIENT, "music_playlist_sampleposition6", "-1", "resume position for track, -1 restarts every time"},
	{CF_CLIENT, "music_playlist_sampleposition7", "-1", "resume position for track, -1 restarts every time"},
	{CF_CLIENT, "music_playlist_sampleposition8", "-1", "resume position for track, -1 restarts every time"},
	{CF_CLIENT, "music_playlist_sampleposition9", "-1", "resume position for track, -1 restarts every time"},
};

static qbool wasPlaying = false;
static qbool initialized = false;
static qbool enabled = false;
static float cdvolume;
typedef char filename_t[MAX_QPATH];
#ifdef MAXTRACKS
static filename_t remap[MAXTRACKS];
#endif
static unsigned char maxTrack;
static int faketrack = -1;

static float saved_vol = 1.0f;

// exported variables
qbool cdValid = false;
qbool cdPlaying = false;
qbool cdPlayLooping = false;
unsigned char cdPlayTrack;

typedef struct cl_cdstate_s
{
	qbool Valid;
	qbool Playing;
	qbool PlayLooping;
	unsigned char PlayTrack;
}
cl_cdstate_t;

cl_cdstate_t cd;

static void CDAudio_Eject (void)
{
	if (!enabled)
		return;
	
	if(cdaudio.integer == 0)
		return;

	CDAudio_SysEject();
}


static void CDAudio_CloseDoor (void)
{
	if (!enabled)
		return;

	if(cdaudio.integer == 0)
		return;

	CDAudio_SysCloseDoor();
}

static int CDAudio_GetAudioDiskInfo (void)
{
	int ret;

	cdValid = false;

	if(cdaudio.integer == 0)
		return -1;

	ret = CDAudio_SysGetAudioDiskInfo();
	if (ret < 1)
		return -1;

	cdValid = true;
	maxTrack = ret;

	return 0;
}

static qbool CDAudio_Play_real (int track, qbool looping, qbool complain)
{
	if(track < 1)
	{
		if(complain)
			Con_Print("Could not load BGM track.\n");
		return false;
	}

	if (!cdValid)
	{
		CDAudio_GetAudioDiskInfo();
		if (!cdValid)
		{
			if(complain)
				Con_DPrint ("No CD in player.\n");
			return false;
		}
	}

	if (track > maxTrack)
	{
		if(complain)
			Con_DPrintf("CDAudio: Bad track number %u.\n", track);
		return false;
	}

	if (CDAudio_SysPlay(track) == -1)
		return false;

	if(cdaudio.integer != 3)
		Con_DPrintf ("CD track %u playing...\n", track);

	return true;
}

void CDAudio_Play_byName (const char *trackname, qbool looping, qbool tryreal, float startposition)
{
	unsigned int track;
	sfx_t* sfx;
	char filename[MAX_QPATH];

	Host_StartVideo();

	if (!enabled)
		return;

	if(tryreal && strspn(trackname, "0123456789") == strlen(trackname))
	{
		track = (unsigned char) atoi(trackname);
#ifdef MAXTRACKS
		if(track > 0 && track < MAXTRACKS)
			if(*remap[track])
			{
				if(strspn(remap[track], "0123456789") == strlen(remap[track]))
				{
					trackname = remap[track];
				}
				else
				{
					// ignore remappings to fake tracks if we're going to play a real track
					switch(cdaudio.integer)
					{
						case 0: // we never access CD
						case 1: // we have a replacement
							trackname = remap[track];
							break;
						case 2: // we only use fake track replacement if CD track is invalid
							CDAudio_GetAudioDiskInfo();
							if(!cdValid || track > maxTrack)
								trackname = remap[track];
							break;
						case 3: // we always play from CD - ignore this remapping then
						case 4: // we randomize anyway
							break;
					}
				}
			}
#endif
	}

	if(tryreal && strspn(trackname, "0123456789") == strlen(trackname))
	{
		track = (unsigned char) atoi(trackname);
		if (track < 1)
		{
			Con_DPrintf("CDAudio: Bad track number %u.\n", track);
			return;
		}
	}
	else
		track = 0;

	// div0: I assume this code was intentionally there. Maybe turn it into a cvar?
	if (cdPlaying && cdPlayTrack == track && faketrack == -1)
		return;
	CDAudio_Stop ();

	if(track >= 1)
	{
		if(cdaudio.integer == 3) // only play real CD tracks at all
		{
			if(CDAudio_Play_real(track, looping, true))
				goto success;
			return;
		}

		if(cdaudio.integer == 2) // prefer real CD track over fake
		{
			if(CDAudio_Play_real(track, looping, false))
				goto success;
		}
	}

	if(cdaudio.integer == 4) // only play real CD tracks, EVEN instead of fake tracks!
	{
		if(CDAudio_Play_real(track, looping, false))
			goto success;
		
		if(cdValid && maxTrack > 0)
		{
			track = 1 + (rand() % maxTrack);
			if(CDAudio_Play_real(track, looping, true))
				goto success;
		}
		else
		{
			Con_DPrint ("No CD in player.\n");
		}
		return;
	}

	// Try playing a fake track (sound file) first
	if(track >= 1)
	{
		                              dpsnprintf(filename, sizeof(filename), "sound/cdtracks/track%03u.wav", track);
		if (!FS_FileExists(filename)) dpsnprintf(filename, sizeof(filename), "sound/cdtracks/track%03u.ogg", track);
		if (!FS_FileExists(filename)) dpsnprintf(filename, sizeof(filename), "music/track%03u.ogg", track);// added by motorsep
		if (!FS_FileExists(filename)) dpsnprintf(filename, sizeof(filename), "music/cdtracks/track%03u.ogg", track);// added by motorsep
		if (!FS_FileExists(filename)) dpsnprintf(filename, sizeof(filename), "sound/cdtracks/track%02u.wav", track);
		if (!FS_FileExists(filename)) dpsnprintf(filename, sizeof(filename), "sound/cdtracks/track%02u.ogg", track);
		if (!FS_FileExists(filename)) dpsnprintf(filename, sizeof(filename), "music/track%02u.ogg", track);// added by motorsep
		if (!FS_FileExists(filename)) dpsnprintf(filename, sizeof(filename), "music/cdtracks/track%02u.ogg", track);// added by motorsep
	}
	else
	{
		                              dpsnprintf(filename, sizeof(filename), "%s", trackname);
		if (!FS_FileExists(filename)) dpsnprintf(filename, sizeof(filename), "%s.wav", trackname);
		if (!FS_FileExists(filename)) dpsnprintf(filename, sizeof(filename), "%s.ogg", trackname);
		if (!FS_FileExists(filename)) dpsnprintf(filename, sizeof(filename), "sound/%s", trackname);
		if (!FS_FileExists(filename)) dpsnprintf(filename, sizeof(filename), "sound/%s.wav", trackname);
		if (!FS_FileExists(filename)) dpsnprintf(filename, sizeof(filename), "sound/%s.ogg", trackname);
		if (!FS_FileExists(filename)) dpsnprintf(filename, sizeof(filename), "sound/cdtracks/%s", trackname);
		if (!FS_FileExists(filename)) dpsnprintf(filename, sizeof(filename), "sound/cdtracks/%s.wav", trackname);
		if (!FS_FileExists(filename)) dpsnprintf(filename, sizeof(filename), "sound/cdtracks/%s.ogg", trackname);
		if (!FS_FileExists(filename)) dpsnprintf(filename, sizeof(filename), "music/%s.ogg", trackname); // added by motorsep
		if (!FS_FileExists(filename)) dpsnprintf(filename, sizeof(filename), "music/cdtracks/%s.ogg", trackname); // added by motorsep
	}
	if (FS_FileExists(filename) && (sfx = S_PrecacheSound (filename, false, false)))
	{
		faketrack = S_StartSound_StartPosition_Flags (-1, 0, sfx, vec3_origin, cdvolume, 0, startposition, (looping ? CHANNELFLAG_FORCELOOP : 0) | CHANNELFLAG_FULLVOLUME | CHANNELFLAG_LOCALSOUND, 1.0f);
		if (faketrack != -1)
		{
			if(track >= 1)
			{
				if(cdaudio.integer != 0) // we don't need these messages if only fake tracks can be played anyway
					Con_DPrintf ("Fake CD track %u playing...\n", track);
			}
			else
				Con_DPrintf ("BGM track %s playing...\n", trackname);
		}
	}

	// If we can't play a fake CD track, try the real one
	if (faketrack == -1)
	{
		if(cdaudio.integer == 0 || track < 1)
		{
			Con_Print("Could not load BGM track.\n");
			return;
		}
		else
		{
			if(!CDAudio_Play_real(track, looping, true))
				return;
		}
	}

success:
	cdPlayLooping = looping;
	cdPlayTrack = track;
	cdPlaying = true;

	if (cdvolume == 0.0 || bgmvolume.value == 0)
		CDAudio_Pause ();
}

void CDAudio_Play (int track, qbool looping)
{
	char buf[20];
	if (music_playlist_index.integer >= 0)
		return;
	dpsnprintf(buf, sizeof(buf), "%d", (int) track);
	CDAudio_Play_byName(buf, looping, true, 0);
}

float CDAudio_GetPosition (void)
{
	if(faketrack != -1)
		return S_GetChannelPosition(faketrack);
	return -1;
}

static void CDAudio_StopPlaylistTrack(void);

void CDAudio_Stop (void)
{
	if (!enabled)
		return;

	// save the playlist position
	CDAudio_StopPlaylistTrack();

	if (faketrack != -1)
	{
		S_StopChannel (faketrack, true, true);
		faketrack = -1;
	}
	else if (cdPlaying && (CDAudio_SysStop() == -1))
		return;
	else if(wasPlaying)
	{
		CDAudio_Resume(); // needed by SDL - can't stop while paused there (causing pause/stop to fail after play, pause, stop, play otherwise)
		if (cdPlaying && (CDAudio_SysStop() == -1))
			return;
	}

	wasPlaying = false;
	cdPlaying = false;
}

void CDAudio_Pause (void)
{
	if (!enabled || !cdPlaying)
		return;

	if (faketrack != -1)
		S_SetChannelFlag (faketrack, CHANNELFLAG_PAUSED, true);
	else if (CDAudio_SysPause() == -1)
		return;

	wasPlaying = cdPlaying;
	cdPlaying = false;
}


void CDAudio_Resume (void)
{
	if (!enabled || cdPlaying || !wasPlaying)
		return;

	if (faketrack != -1)
		S_SetChannelFlag (faketrack, CHANNELFLAG_PAUSED, false);
	else if (CDAudio_SysResume() == -1)
		return;
	cdPlaying = true;
}

static void CD_f (void)
{
	const char *command;
#ifdef MAXTRACKS
	int ret;
	int n;
#endif

	command = Cmd_Argv (1);

	if (strcasecmp(command, "remap") != 0)
		Host_StartVideo();

	if (strcasecmp(command, "on") == 0)
	{
		enabled = true;
		return;
	}

	if (strcasecmp(command, "off") == 0)
	{
		CDAudio_Stop();
		enabled = false;
		return;
	}

	if (strcasecmp(command, "reset") == 0)
	{
		enabled = true;
		CDAudio_Stop();
#ifdef MAXTRACKS
		for (n = 0; n < MAXTRACKS; n++)
			*remap[n] = 0; // empty string, that is, unremapped
#endif
		CDAudio_GetAudioDiskInfo();
		return;
	}

	if (strcasecmp(command, "rescan") == 0)
	{
		CDAudio_Shutdown();
		CDAudio_Startup();
		return;
	}

	if (strcasecmp(command, "remap") == 0)
	{
#ifdef MAXTRACKS
		ret = Cmd_Argc() - 2;
		if (ret <= 0)
		{
			for (n = 1; n < MAXTRACKS; n++)
				if (*remap[n])
					Con_Printf("  %u -> %s\n", n, remap[n]);
			return;
		}
		for (n = 1; n <= ret; n++)
			strlcpy(remap[n], Cmd_Argv (n+1), sizeof(*remap));
#endif
		return;
	}

	if (strcasecmp(command, "close") == 0)
	{
		CDAudio_CloseDoor();
		return;
	}

	if (strcasecmp(command, "play") == 0)
	{
		if (music_playlist_index.integer >= 0)
			return;
		CDAudio_Play_byName(Cmd_Argv (2), false, true, (Cmd_Argc() > 3) ? atof( Cmd_Argv(3) ) : 0);
		return;
	}

	if (strcasecmp(command, "loop") == 0)
	{
		if (music_playlist_index.integer >= 0)
			return;
		CDAudio_Play_byName(Cmd_Argv (2), true, true, (Cmd_Argc() > 3) ? atof( Cmd_Argv(3) ) : 0);
		return;
	}

	if (strcasecmp(command, "stop") == 0)
	{
		if (music_playlist_index.integer >= 0)
			return;
		CDAudio_Stop();
		return;
	}

	if (strcasecmp(command, "pause") == 0)
	{
		if (music_playlist_index.integer >= 0)
			return;
		CDAudio_Pause();
		return;
	}

	if (strcasecmp(command, "resume") == 0)
	{
		if (music_playlist_index.integer >= 0)
			return;
		CDAudio_Resume();
		return;
	}

	if (strcasecmp(command, "eject") == 0)
	{
		if (faketrack == -1)
			CDAudio_Stop();
		CDAudio_Eject();
		cdValid = false;
		return;
	}

	if (strcasecmp(command, "info") == 0)
	{
		CDAudio_GetAudioDiskInfo ();
		if (cdValid)
			Con_Printf("%u tracks on CD.\n", maxTrack);
		else
			Con_Print ("No CD in player.\n");
		if (cdPlaying)
			Con_Printf("Currently %s track %u\n", cdPlayLooping ? "looping" : "playing", cdPlayTrack);
		else if (wasPlaying)
			Con_Printf("Paused %s track %u\n", cdPlayLooping ? "looping" : "playing", cdPlayTrack);
		if (cdvolume >= 0)
			Con_Printf("Volume is %f\n", cdvolume);
		else
			Con_Printf("Can't get CD volume\n");
		return;
	}

	Con_Printf("CD commands:\n");
	Con_Printf("cd on - enables CD audio system\n");
	Con_Printf("cd off - stops and disables CD audio system\n");
	Con_Printf("cd reset - resets CD audio system (clears track remapping and re-reads disc information)\n");
	Con_Printf("cd rescan - rescans disks in drives (to use another disc)\n");
	Con_Printf("cd remap <remap1> [remap2] [remap3] [...] - chooses (possibly emulated) CD tracks to play when a map asks for a particular track, this has many uses\n");
	Con_Printf("cd close - closes CD tray\n");
	Con_Printf("cd eject - stops playing music and opens CD tray to allow you to change disc\n");
	Con_Printf("cd play <tracknumber> <startposition> - plays selected track in remapping table\n");
	Con_Printf("cd loop <tracknumber> <startposition> - plays and repeats selected track in remapping table\n");
	Con_Printf("cd stop - stops playing current CD track\n");
	Con_Printf("cd pause - pauses CD playback\n");
	Con_Printf("cd resume - unpauses CD playback\n");
	Con_Printf("cd info - prints basic disc information (number of tracks, currently playing track, volume level)\n");
}

static void CDAudio_SetVolume (float newvol)
{
	// If the volume hasn't changed
	if (newvol == cdvolume)
		return;

	// If the CD has been muted
	if (newvol == 0.0f)
		CDAudio_Pause ();
	else
	{
		// If the CD has been unmuted
		if (cdvolume == 0.0f)
			CDAudio_Resume ();

		if (faketrack != -1)
			S_SetChannelVolume (faketrack, newvol);
		else
			CDAudio_SysSetVolume (newvol * mastervolume.value);
	}

	cdvolume = newvol;
}

static void CDAudio_StopPlaylistTrack(void)
{
	if (music_playlist_active >= 0 && music_playlist_active < MAX_PLAYLISTS && music_playlist_sampleposition[music_playlist_active].value >= 0)
	{
		// save position for resume
		float position = CDAudio_GetPosition();
		Cvar_SetValueQuick(&music_playlist_sampleposition[music_playlist_active], position >= 0 ? position : 0);
	}
	music_playlist_active = -1;
	music_playlist_playing = 0; // not playing
}

static void CDAudio_StartPlaylist(qbool resume)
{
	const char *list;
	const char *t;
	int index;
	int current;
	int randomplay;
	int count;
	int listindex;
	float position;
	char trackname[MAX_QPATH];
	CDAudio_Stop();
	index = music_playlist_index.integer;
	if (index >= 0 && index < MAX_PLAYLISTS && bgmvolume.value > 0)
	{
		list = music_playlist_list[index].string;
		current = music_playlist_current[index].integer;
		randomplay = music_playlist_random[index].integer;
		position = music_playlist_sampleposition[index].value;
		count = 0;
		trackname[0] = 0;
		if (list && list[0])
		{
			for (t = list;;count++)
			{
				if (!COM_ParseToken_Console(&t))
					break;
				// if we don't find the desired track, use the first one
				if (count == 0)
					strlcpy(trackname, com_token, sizeof(trackname));
			}
		}
		if (count > 0)
		{
			// position < 0 means never resume track
			if (position < 0)
				position = 0;
			// advance to next track in playlist if the last one ended
			if (!resume)
			{
				position = 0;
				current++;
				if (randomplay)
					current = (int)lhrandom(0, count);
			}
			// wrap playlist position if needed
			if (current >= count)
				current = 0;
			// set current
			Cvar_SetValueQuick(&music_playlist_current[index], current);
			// get the Nth trackname
			if (current >= 0 && current < count)
			{
				for (listindex = 0, t = list;;listindex++)
				{
					if (!COM_ParseToken_Console(&t))
						break;
					if (listindex == current)
					{
						strlcpy(trackname, com_token, sizeof(trackname));
						break;
					}
				}
			}
			if (trackname[0])
			{
				CDAudio_Play_byName(trackname, false, false, position);
				if (faketrack != -1)
					music_playlist_active = index;
			}
		}
	}
	music_playlist_playing = music_playlist_active >= 0 ? 1 : -1;
}

void CDAudio_Update (void)
{
	static int lastplaylist = -1;
	if (!enabled)
		return;

	CDAudio_SetVolume (bgmvolume.value);
	if (music_playlist_playing > 0 && CDAudio_GetPosition() < 0)
	{
		// this track ended, start a new track from the beginning
		CDAudio_StartPlaylist(false);
		lastplaylist = music_playlist_index.integer;
	}
	else if (lastplaylist != music_playlist_index.integer
	|| (bgmvolume.value > 0 && !music_playlist_playing && music_playlist_index.integer >= 0))
	{
		// active playlist changed, save position and switch track
		CDAudio_StartPlaylist(true);
		lastplaylist = music_playlist_index.integer;
	}

	if (faketrack == -1 && cdaudio.integer != 0 && bgmvolume.value != 0)
		CDAudio_SysUpdate();
}

int CDAudio_Init (void)
{
	int i;

	if (cls.state == ca_dedicated)
		return -1;

// COMMANDLINEOPTION: Sound: -nocdaudio disables CD audio support
	if (Sys_CheckParm("-nocdaudio")) // Baker 7005
		return -1;

	CDAudio_SysInit();

#ifdef MAXTRACKS
	for (i = 0; i < MAXTRACKS; i++)
		*remap[i] = 0;
#endif

	Cvar_RegisterVariable(&cdaudio);
	Cvar_RegisterVariable(&cdaudioinitialized);
	Cvar_SetValueQuick(&cdaudioinitialized, true);
	enabled = true;

	Cvar_RegisterVariable(&music_playlist_index);
	for (i = 0;i < MAX_PLAYLISTS;i++)
	{
		Cvar_RegisterVariable(&music_playlist_list[i]);
		Cvar_RegisterVariable(&music_playlist_current[i]);
		Cvar_RegisterVariable(&music_playlist_random[i]);
		Cvar_RegisterVariable(&music_playlist_sampleposition[i]);
	}

	Cmd_AddCommand("cd", CD_f, "execute a CD drive command (cd on/off/reset/remap/close/play/loop/stop/pause/resume/eject/info) - use cd by itself for usage");

	return 0;
}

int CDAudio_Startup (void)
{
	//if (Sys_CheckParm("-nocdaudio")) // Baker 7005
		return -1;

	CDAudio_SysStartup ();

	if (CDAudio_GetAudioDiskInfo())
	{
		Con_Print("CDAudio_Init: No CD in player.\n");
		cdValid = false;
	}

	saved_vol = CDAudio_SysGetVolume ();
	if (saved_vol < 0.0f)
	{
		Con_Print ("Can't get initial CD volume\n");
		saved_vol = 1.0f;
	}
	else
		Con_Printf ("Initial CD volume: %g\n", saved_vol);

	initialized = true;

	Con_Print("CD Audio Initialized\n");

	return 0;
}

void CDAudio_Shutdown (void)
{
	if (!initialized)
		return;

	CDAudio_SysSetVolume (saved_vol);

	CDAudio_Stop();
	CDAudio_SysShutdown();
	initialized = false;
}
