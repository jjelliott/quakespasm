/*
 * Background music handling for Quakespasm (adapted from uHexen2)
 * Handles streaming music as raw sound samples and runs the midi driver
 *
 * Copyright (C) 1999-2005 Id Software, Inc.
 * Copyright (C) 2010-2018 O.Sezer <sezero@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "quakedef.h"
#include "snd_codec.h"
#include "bgmusic.h"

#define MUSIC_DIRNAME	"music"
#define MUSICDIR_PATH_ID	(~0u)

qboolean	bgmloop;
cvar_t		bgm_extmusic = {"bgm_extmusic", "1", CVAR_ARCHIVE};
cvar_t		snd_musicdir = {"snd_musicdir", "", CVAR_ARCHIVE};

static qboolean	no_extmusic= false;
static float	old_volume = -1.0f;

typedef enum _bgm_player
{
	BGM_NONE = -1,
	BGM_MIDIDRV = 1,
	BGM_STREAMER
} bgm_player_t;

typedef struct music_handler_s
{
	unsigned int	type;	/* 1U << n (see snd_codec.h)	*/
	bgm_player_t	player;	/* Enumerated bgm player type	*/
	int	is_available;	/* -1 means not present		*/
	const char	*ext;	/* Expected file extension	*/
	const char	*dir;	/* Where to look for music file */
	struct music_handler_s	*next;
} music_handler_t;

static music_handler_t wanted_handlers[] =
{
	{ CODECTYPE_VORBIS,BGM_STREAMER,-1,  "ogg", MUSIC_DIRNAME, NULL },
	{ CODECTYPE_OPUS, BGM_STREAMER, -1, "opus", MUSIC_DIRNAME, NULL },
	{ CODECTYPE_MP3,  BGM_STREAMER, -1,  "mp3", MUSIC_DIRNAME, NULL },
	{ CODECTYPE_FLAC, BGM_STREAMER, -1, "flac", MUSIC_DIRNAME, NULL },
	{ CODECTYPE_WAV,  BGM_STREAMER, -1,  "wav", MUSIC_DIRNAME, NULL },
	{ CODECTYPE_MOD,  BGM_STREAMER, -1,  "it",  MUSIC_DIRNAME, NULL },
	{ CODECTYPE_MOD,  BGM_STREAMER, -1,  "s3m", MUSIC_DIRNAME, NULL },
	{ CODECTYPE_MOD,  BGM_STREAMER, -1,  "xm",  MUSIC_DIRNAME, NULL },
	{ CODECTYPE_MOD,  BGM_STREAMER, -1,  "mod", MUSIC_DIRNAME, NULL },
	{ CODECTYPE_UMX,  BGM_STREAMER, -1,  "umx", MUSIC_DIRNAME, NULL },
	{ CODECTYPE_NONE, BGM_NONE,     -1,   NULL,         NULL,  NULL }
};

static music_handler_t *music_handlers = NULL;

#define ANY_CODECTYPE	0xFFFFFFFF
#define CDRIP_TYPES	(CODECTYPE_VORBIS | CODECTYPE_MP3 | CODECTYPE_FLAC | CODECTYPE_WAV | CODECTYPE_OPUS)
#define CDRIPTYPE(x)	(((x) & CDRIP_TYPES) != 0)

static snd_stream_t *bgmstream = NULL;

typedef enum
{
	BGM_REQUEST_NONE = 0,
	BGM_REQUEST_FILE,
	BGM_REQUEST_CDTRACK
} bgm_request_t;

static bgm_request_t bgm_current_type = BGM_REQUEST_NONE;
static char bgm_current_file[MAX_QPATH];
static byte bgm_current_cdtrack;
static qboolean bgm_current_cdlooping;
static qboolean bgm_musicdir_callback_active = false;

static void BGM_StopPlayback (void);
static void BGM_RestartCurrent (void);

static qboolean BGM_IsValidMusicDir (const char *dir)
{
	if (!dir || !*dir)
		return false;

	if (!strcmp(dir, ".") || strstr(dir, "..") || strstr(dir, "/") ||
		strstr(dir, "\\") || strstr(dir, ":"))
		return false;

	return true;
}

static void BGM_MusicDir_Callback (cvar_t *var)
{
	qboolean restart = false;

	if (bgm_musicdir_callback_active)
		return;

	bgm_musicdir_callback_active = true;

	if (host_initialized)
		restart = (bgm_current_type != BGM_REQUEST_NONE);

	if (!BGM_IsValidMusicDir(var->string))
	{
		if (*var->string)
		{
			Con_Printf("snd_musicdir should be a single directory name, not a path\n");
			Cvar_SetROM(var->name, "");
		}
		goto _done;
	}

_done:
	if (restart)
		BGM_RestartCurrent();

	bgm_musicdir_callback_active = false;
}

static qboolean BGM_BuildMusicDirPath (char *path, size_t pathsize,
	const char *base, const char *filename)
{
	if (!BGM_IsValidMusicDir(snd_musicdir.string))
		return false;

	q_snprintf(path, pathsize, "%s/%s/%s", base, snd_musicdir.string, filename);
	return true;
}

static qboolean BGM_MusicDirFileExists (const char *filename, unsigned int *path_id)
{
	char path[MAX_OSPATH];

	if (BGM_BuildMusicDirPath(path, sizeof(path), host_parms->userdir, filename) &&
		(Sys_FileType(path) & FS_ENT_FILE))
	{
		if (path_id)
			*path_id = MUSICDIR_PATH_ID;
		return true;
	}

	if (host_parms->userdir != host_parms->basedir &&
		BGM_BuildMusicDirPath(path, sizeof(path), com_basedir, filename) &&
		(Sys_FileType(path) & FS_ENT_FILE))
	{
		if (path_id)
			*path_id = MUSICDIR_PATH_ID;
		return true;
	}

	return false;
}

static snd_stream_t *BGM_OpenStream (const char *filename, unsigned int type)
{
	char path[MAX_OSPATH];

	if (BGM_BuildMusicDirPath(path, sizeof(path), host_parms->userdir, filename))
	{
		bgmstream = S_CodecOpenStreamTypeDirect(path, filename, type, bgmloop);
		if (bgmstream)
			return bgmstream;
	}

	if (host_parms->userdir != host_parms->basedir &&
		BGM_BuildMusicDirPath(path, sizeof(path), com_basedir, filename))
	{
		bgmstream = S_CodecOpenStreamTypeDirect(path, filename, type, bgmloop);
		if (bgmstream)
			return bgmstream;
	}

	return S_CodecOpenStreamType(filename, type, bgmloop);
}

static void BGM_Play_f (void)
{
	if (Cmd_Argc() == 2) {
		BGM_Play (Cmd_Argv(1));
	}
	else {
		Con_Printf ("music <musicfile>\n");
	}
}

static void BGM_Pause_f (void)
{
	BGM_Pause ();
}

static void BGM_Resume_f (void)
{
	BGM_Resume ();
}

static void BGM_Loop_f (void)
{
	if (Cmd_Argc() == 2) {
		if (q_strcasecmp(Cmd_Argv(1),  "0") == 0 ||
		    q_strcasecmp(Cmd_Argv(1),"off") == 0)
			bgmloop = false;
		else if (q_strcasecmp(Cmd_Argv(1), "1") == 0 ||
			 q_strcasecmp(Cmd_Argv(1),"on") == 0)
			bgmloop = true;
		else if (q_strcasecmp(Cmd_Argv(1),"toggle") == 0)
			bgmloop = !bgmloop;

		if (bgmstream) bgmstream->loop = bgmloop;
	}

	if (bgmloop)
		Con_Printf("Music will be looped\n");
	else
		Con_Printf("Music will not be looped\n");
}

static void BGM_Stop_f (void)
{
	BGM_Stop();
}

static void BGM_Jump_f (void)
{
	if (Cmd_Argc() != 2) {
		Con_Printf ("music_jump <ordernum>\n");
	}
	else if (bgmstream) {
		S_CodecJumpToOrder(bgmstream, atoi(Cmd_Argv(1)));
	}
}

qboolean BGM_Init (void)
{
	music_handler_t *handlers = NULL;
	int i;

	Cvar_RegisterVariable(&bgm_extmusic);
	Cvar_RegisterVariable(&snd_musicdir);
	Cvar_SetCallback(&snd_musicdir, BGM_MusicDir_Callback);
	BGM_MusicDir_Callback(&snd_musicdir);
	Cmd_AddCommand("music", BGM_Play_f);
	Cmd_AddCommand("music_pause", BGM_Pause_f);
	Cmd_AddCommand("music_resume", BGM_Resume_f);
	Cmd_AddCommand("music_loop", BGM_Loop_f);
	Cmd_AddCommand("music_stop", BGM_Stop_f);
	Cmd_AddCommand("music_jump", BGM_Jump_f);

	if (COM_CheckParm("-noextmusic") != 0)
		no_extmusic = true;

	bgmloop = true;

	for (i = 0; wanted_handlers[i].type != CODECTYPE_NONE; i++)
	{
		switch (wanted_handlers[i].player)
		{
		case BGM_MIDIDRV:
		/* not supported in quake */
			break;
		case BGM_STREAMER:
			wanted_handlers[i].is_available =
				S_CodecIsAvailable(wanted_handlers[i].type);
			break;
		case BGM_NONE:
		default:
			break;
		}
		if (wanted_handlers[i].is_available != -1)
		{
			if (handlers)
			{
				handlers->next = &wanted_handlers[i];
				handlers = handlers->next;
			}
			else
			{
				music_handlers = &wanted_handlers[i];
				handlers = music_handlers;
			}
		}
	}

	return true;
}

void BGM_Shutdown (void)
{
	BGM_Stop();
/* sever our connections to
 * midi_drv and snd_codec */
	music_handlers = NULL;
}

static void BGM_Play_noext (const char *filename, unsigned int allowed_types)
{
	char tmp[MAX_QPATH];
	music_handler_t *handler;

	handler = music_handlers;
	while (handler)
	{
		if (! (handler->type & allowed_types))
		{
			handler = handler->next;
			continue;
		}
		if (!handler->is_available)
		{
			handler = handler->next;
			continue;
		}
		q_snprintf(tmp, sizeof(tmp), "%s/%s.%s",
			   handler->dir, filename, handler->ext);
		switch (handler->player)
		{
		case BGM_MIDIDRV:
		/* not supported in quake */
			break;
		case BGM_STREAMER:
			bgmstream = BGM_OpenStream(tmp, handler->type);
			if (bgmstream)
				return;		/* success */
			break;
		case BGM_NONE:
		default:
			break;
		}
		handler = handler->next;
	}

	Con_Printf("Couldn't handle music file %s\n", filename);
}

void BGM_Play (const char *filename)
{
	char tmp[MAX_QPATH];
	const char *ext;
	music_handler_t *handler;

	BGM_StopPlayback();
	bgm_current_type = BGM_REQUEST_FILE;
	if (filename)
		q_strlcpy(bgm_current_file, filename, sizeof(bgm_current_file));
	else
		bgm_current_file[0] = '\0';

	if (music_handlers == NULL)
		return;

	if (!filename || !*filename)
	{
		Con_DPrintf("null music file name\n");
		return;
	}

	ext = COM_FileGetExtension(filename);
	if (! *ext)	/* try all things */
	{
		BGM_Play_noext(filename, ANY_CODECTYPE);
		return;
	}

	handler = music_handlers;
	while (handler)
	{
		if (handler->is_available &&
		    !q_strcasecmp(ext, handler->ext))
			break;
		handler = handler->next;
	}
	if (!handler)
	{
		Con_Printf("Unhandled extension for %s\n", filename);
		return;
	}
	q_snprintf(tmp, sizeof(tmp), "%s/%s", handler->dir, filename);
	switch (handler->player)
	{
	case BGM_MIDIDRV:
	/* not supported in quake */
		break;
	case BGM_STREAMER:
		bgmstream = BGM_OpenStream(tmp, handler->type);
		if (bgmstream)
			return;		/* success */
		break;
	case BGM_NONE:
	default:
		break;
	}

	Con_Printf("Couldn't handle music file %s\n", filename);
}

void BGM_PlayCDtrack (byte track, qboolean looping)
{
/* instead of searching by the order of music_handlers, do so by
 * the order of searchpath priority: the file from the searchpath
 * with the highest path_id is most likely from our own gamedir
 * itself. This way, if a mod has track02 as a *.mp3 file, which
 * is below *.ogg in the music_handler order, the mp3 will still
 * have priority over track02.ogg from, say, id1.
 */
	char tmp[MAX_QPATH];
	const char *ext;
	unsigned int path_id, prev_id, type;
	music_handler_t *handler;

	BGM_StopPlayback();
	bgm_current_type = BGM_REQUEST_CDTRACK;
	bgm_current_cdtrack = track;
	bgm_current_cdlooping = looping;
	if (CDAudio_Play(track, looping) == 0)
		return;			/* success */

	if (music_handlers == NULL)
		return;

	if (no_extmusic || !bgm_extmusic.value)
		return;

	prev_id = 0;
	type = 0;
	ext  = NULL;
	handler = music_handlers;
	while (handler)
	{
		if (! handler->is_available)
			goto _next;
	//	if (! CDRIPTYPE(handler->type))
	//		goto _next;
		q_snprintf(tmp, sizeof(tmp), "%s/track%02d.%s",
				MUSIC_DIRNAME, (int)track, handler->ext);
		if (!BGM_MusicDirFileExists(tmp, &path_id) &&
			!COM_FileExists(tmp, &path_id))
			goto _next;
		if (path_id > prev_id)
		{
			prev_id = path_id;
			type = handler->type;
			ext = handler->ext;
		}
	_next:
		handler = handler->next;
	}
	if (ext == NULL)
		Con_Printf("Couldn't find a cdrip for track %d\n", (int)track);
	else
	{
		q_snprintf(tmp, sizeof(tmp), "%s/track%02d.%s",
				MUSIC_DIRNAME, (int)track, ext);
		bgmstream = BGM_OpenStream(tmp, type);
		if (! bgmstream)
			Con_Printf("Couldn't handle music file %s\n", tmp);
	}
}

static void BGM_StopPlayback (void)
{
	if (bgmstream)
	{
		bgmstream->status = STREAM_NONE;
		S_CodecCloseStream(bgmstream);
		bgmstream = NULL;
		s_rawend = 0;
	}
}

static void BGM_RestartCurrent (void)
{
	char current_file[MAX_QPATH];
	byte current_cdtrack;
	qboolean current_cdlooping;
	bgm_request_t current_type;

	current_type = bgm_current_type;
	q_strlcpy(current_file, bgm_current_file, sizeof(current_file));
	current_cdtrack = bgm_current_cdtrack;
	current_cdlooping = bgm_current_cdlooping;

	switch (current_type)
	{
	case BGM_REQUEST_FILE:
		BGM_Play(current_file);
		break;
	case BGM_REQUEST_CDTRACK:
		BGM_PlayCDtrack(current_cdtrack, current_cdlooping);
		break;
	case BGM_REQUEST_NONE:
	default:
		break;
	}
}

void BGM_Stop (void)
{
	bgm_current_type = BGM_REQUEST_NONE;
	bgm_current_file[0] = '\0';
	bgm_current_cdtrack = 0;
	bgm_current_cdlooping = false;
	BGM_StopPlayback();
}

void BGM_Pause (void)
{
	if (bgmstream)
	{
		if (bgmstream->status == STREAM_PLAY)
			bgmstream->status = STREAM_PAUSE;
	}
}

void BGM_Resume (void)
{
	if (bgmstream)
	{
		if (bgmstream->status == STREAM_PAUSE)
			bgmstream->status = STREAM_PLAY;
	}
}

static void BGM_UpdateStream (void)
{
	qboolean did_rewind = false;
	int	res;	/* Number of bytes read. */
	int	bufferSamples;
	int	fileSamples;
	int	fileBytes;
	byte	raw[16384];

	if (bgmstream->status != STREAM_PLAY)
		return;

	/* don't bother playing anything if musicvolume is 0 */
	if (bgmvolume.value <= 0)
		return;

	/* see how many samples should be copied into the raw buffer */
	if (s_rawend < paintedtime)
		s_rawend = paintedtime;

	while (s_rawend < paintedtime + MAX_RAW_SAMPLES)
	{
		bufferSamples = MAX_RAW_SAMPLES - (s_rawend - paintedtime);

		/* decide how much data needs to be read from the file */
		fileSamples = bufferSamples * bgmstream->info.rate / shm->speed;
		if (!fileSamples)
			return;

		/* our max buffer size */
		fileBytes = fileSamples * (bgmstream->info.width * bgmstream->info.channels);
		if (fileBytes > (int) sizeof(raw))
		{
			fileBytes = (int) sizeof(raw);
			fileSamples = fileBytes /
					  (bgmstream->info.width * bgmstream->info.channels);
		}

		/* Read */
		res = S_CodecReadStream(bgmstream, fileBytes, raw);
		if (res < fileBytes)
		{
			fileBytes = res;
			fileSamples = res / (bgmstream->info.width * bgmstream->info.channels);
		}

		if (res > 0)	/* data: add to raw buffer */
		{
			S_RawSamples(fileSamples, bgmstream->info.rate,
							bgmstream->info.width,
							bgmstream->info.channels,
							raw, bgmvolume.value);
			did_rewind = false;
		}
		else if (res == 0)	/* EOF */
		{
			if (bgmloop)
			{
				if (did_rewind)
				{
					Con_Printf("Stream keeps returning EOF.\n");
					BGM_Stop();
					return;
				}

				res = S_CodecRewindStream(bgmstream);
				if (res != 0)
				{
					Con_Printf("Stream seek error (%i), stopping.\n", res);
					BGM_Stop();
					return;
				}
				did_rewind = true;
			}
			else
			{
				BGM_Stop();
				return;
			}
		}
		else	/* res < 0: some read error */
		{
			Con_Printf("Stream read error (%i), stopping.\n", res);
			BGM_Stop();
			return;
		}
	}
}

void BGM_Update (void)
{
	if (old_volume != bgmvolume.value)
	{
		if (bgmvolume.value < 0)
			Cvar_SetQuick (&bgmvolume, "0");
		else if (bgmvolume.value > 1)
			Cvar_SetQuick (&bgmvolume, "1");
		old_volume = bgmvolume.value;
	}
	if (bgmstream)
		BGM_UpdateStream ();
}
