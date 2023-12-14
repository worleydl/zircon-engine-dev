// cl_parse_chunked.c.h

//
// FTE's chunked download
//

extern void CL_RequestNextDownload (void);

#define MAXBLOCKS_1024		1024	// Must be power of 2
#define DLBLOCKSIZE_1024	1024

int chunked_download_number = 0; // Never reset, bumped up.

int chunked_downloadsize;
int chunked_receivedbytes;
int recievedblock[MAXBLOCKS_1024];
int firstblock;
int blockcycle;

int QW_CL_RequestADownloadChunk(void)
{
	int i;
	int b;

	if (cls.qw_downloadmethod != DL_QWCHUNKS_2) // Paranoia!
		Host_Error_Line ("download not initiated");

	for (i = 0; i < MAXBLOCKS_1024; i++) {
		blockcycle++;

		b = ((blockcycle) & (MAXBLOCKS_1024-1)) + firstblock;

		if (!recievedblock[b&(MAXBLOCKS_1024-1)]) // Don't ask for ones we've already got.
		{
			if (b >= (chunked_downloadsize + DLBLOCKSIZE_1024-1)/DLBLOCKSIZE_1024)	// Don't ask for blocks that are over the size of the file.
				continue;
			return b;
		}
	}

	return -1;
}

void QW_CL_SendChunkDownloadReq(void)
{
	extern cvar_t cl_chunksperframe;
	int i, j, chunks;
	
	chunks = bound (1, cl_chunksperframe.integer, 30);

	for (j = 0; j < chunks; j++) {
		if (cls.qw_downloadmethod != DL_QWCHUNKS_2)
			return;

		i = QW_CL_RequestADownloadChunk();
		// i < 0 mean client complete download, let server know
		// qqshka: download percent optional, server does't really require it, that my extension, hope does't fuck up something

		if (i < 0) {
			char temp[2048];
			const char *s_version_key = InfoString_GetValue(cl.qw_serverinfo, "*version", temp, sizeof(temp));

			if (String_Does_Contain (s_version_key, "MVDSV")) {
				QW_CL_SendClientCommand(q_is_reliable_true, "nextdl %d %d %d", i, cls.qw_downloadpercent, chunked_download_number);
				if (developer_qw.integer) Con_PrintLinef ("Sending reliable MVDSV nextdl");
			} else {
				QW_CL_SendClientCommand (q_is_reliable_true, "stopdownload");
				if (developer_qw.integer) Con_PrintLinef ("Sending reliable stop download");
			}

			cls.qw_downloadpercent = 100;
			QW_CL_FinishDownload(); // this also request next dl
		}
		else
		{
			QW_CL_SendClientCommand (false, "nextdl %d %d %d", i, cls.qw_downloadpercent, chunked_download_number);
		}
	}
}

WARP_X_ (CL_ConnectionlessPacket A2C_PRINT);
WARP_X_ (CL_ParseDownload, CL_QW_ParseDownload)
void QW_CL_Parse_OOB_ChunkedDownload(void)
{
	int j;

	if (developer_qw.integer) Con_PrintLinef ("CL_Parse_OOB_ChunkedDownload");

	for ( j = 0; j < sizeof("\\chunk")-1; j++ )
		MSG_ReadByte (&cl_message);

	//
	// qqshka: well, this is evil.
	// In case of when one file completed download and next started
	// here may be some packets which travel via network,
	// so we got packets from different file, that mean we may assemble wrong data,
	// need somehow discard such packets, i have no idea how, so adding at least this check.
	//

	if (chunked_download_number != MSG_ReadLong (&cl_message)) {
		Con_DPrintLinef ("Dropping OOB chunked message, out of sequence");
		return;
	}

	if (MSG_ReadByte(&cl_message) != qw_svc_download /*41*/ ) {
		Con_DPrintLinef("Something wrong in OOB message and chunked download");
		return;
	}

	QW_CL_ParseDownload (q_is_oob_true);
}

void QW_CL_FinishDownload(void)
{
	if (developer_qw.integer)
		Con_PrintLinef ("CL_FinishDownload");

	if (cls.qw_download_f) {
		fclose (cls.qw_download_f);

		if (cls.qw_downloadpercent == 100) {
			Con_DPrintLinef ("Download took %.1f seconds", Sys_DirtyTime() - cls.qw_downloadspeedtime );

			// rename the temp file to its final name
			if (String_Does_Not_Match(cls.qw_downloadtempname, cls.qw_downloadname))
				if (rename(cls.qw_downloadtempname, cls.qw_downloadname))
					Con_PrintLinef ("Failed to rename %s to %s.", cls.qw_downloadtempname, cls.qw_downloadname);
		} else {
			/* If download didn't complete, remove the unfinished leftover .tmp file ... */
			unlink (cls.qw_downloadtempname);
		}
	}

	
	SET___ cls.qw_download_f = NULL; if (developer_qw.integer) Con_PrintLinef ("Closed qw_download_f CL_FinishDownload");
	cls.qw_downloadpercent = 0;
	cls.qw_downloadmethod = DL_NONE_0;

	// VFS-FIXME: D-Kure: Surely there is somewhere better for this in fs.c
#if 0
	filesystemchanged = true;
#endif

	// get another file if needed

	if (cls.state != ca_disconnected)
		QW_CL_RequestNextDownload ();
}

WARP_X_ (svc_downloaddata CL_ParseDownload String_Edit_Delete_At );
void QW_CL_ParseChunkedDownload(int is_oob)
{
	char *svname;
	int totalsize;
	int chunknum;
	unsigned char data[DLBLOCKSIZE_1024];
	double tm;

	chunknum = MSG_ReadLong(&cl_message);
	//if (000001) Con_PrintLinef ("Chunknum %d (oob %d)", chunknum, is_oob);
	if (chunknum < 0) {
		totalsize = MSG_ReadLong	(&cl_message);
		svname    = MSG_ReadString	(&cl_message, cl_readstring, sizeof(cl_readstring));

		Con_PrintLinef ("Total size %s: svname " QUOTED_S, String_Num_To_Thousands (totalsize), svname);
		if (cls.qw_download_f) { 
			// Ensure FILE is closed
			if (totalsize != -3) // -3 = dl stopped, so this known issue, do not warn
				Con_PrintLinef ("cls.download shouldn't have been set");

			fclose (cls.qw_download_f);
			SET___ cls.qw_download_f = NULL; Con_PrintLinef ("fclose wrongly opened qw_download_f CL_ParseChunkedDownload -3");
			cls.qw_downloadpercent = 0;
		}

		if (cls.demoplayback)
			return;

		if (totalsize < 0) {
			switch (totalsize) {
				case -3: Con_DPrintLinef ("Server cancel downloading file %s", svname);				break;
				case -2: Con_PrintLinef  ("Server permissions deny downloading file %s", svname);	break;
				default: Con_PrintLinef  ("Couldn't find file %s on the server", svname);			break;
			}

			QW_CL_FinishDownload(); // this also request next dl
			return;
		}

		if (cls.qw_downloadmethod == DL_QWCHUNKS_2)
			Host_Error_Line ("Received second download - " QUOTED_S, svname);

// Baker: This is commented out in ezQuake
// FIXME: damn, fixme!!!!!
//		if (strcasecmp(cls.downloadname, svname))
//			Host_Error("Server sent the wrong download - \"%s\" instead of \"%s\"\n", svname, cls.downloadname);

		// Start the new download
		FS_CreatePath (cls.qw_downloadtempname);

		// ezQuake open
		SET___ cls.qw_download_f = fopen (cls.qw_downloadtempname, "wb");
		if (developer_qw.integer) Con_PrintLinef ("Opened qw_download_f");
		if (cls.qw_download_f == NULL)  {
			Con_PrintLinef ("Failed to open tempname %s", cls.qw_downloadtempname);

			QW_CL_FinishDownload(); // This also requests next dl.
			return;
		}

		cls.qw_downloadmethod  = DL_QWCHUNKS_2;
		cls.qw_downloadpercent = 0;
		cls.qw_downloadstarttime = Sys_DirtyTime ();

		chunked_download_number++;

		chunked_downloadsize        = totalsize;

		firstblock    = 0;
		chunked_receivedbytes = 0;
		blockcycle    = -1;	//so it requests 0 first. :)
		memset(recievedblock, 0, sizeof(recievedblock));
		return;
	}

	// ez: MSG_ReadData (data, DLBLOCKSIZE_1024);
	MSG_ReadBytes (&cl_message, DLBLOCKSIZE_1024, data);

	if (!cls.qw_download_f) { 
		return;
	}

	if (cls.qw_downloadmethod != DL_QWCHUNKS_2)
		Host_Error_Line ("cls.downloadmethod != DL_QWCHUNKS_2");

	if (cls.demoplayback) {	
		// Err, yeah, when playing demos we don't actually pay any attention to this.
		return;
	}

	if (chunknum < firstblock)
	{
		if (developer_qw.integer) Con_PrintLinef ("Received invalid chunk %d < firstblock (%d), ignoring ...", chunknum, firstblock);
		return;
	}

	if (chunknum - firstblock >= MAXBLOCKS_1024) {
		if (developer_qw.integer) Con_PrintLinef ("Received invalid chunk %d .. chunknum - firstblock >= MAXBLOCKS_1024, ignoring ...", chunknum);
		return;
	}

	if (recievedblock[chunknum&(MAXBLOCKS_1024-1)]) {
		if (developer_qw.integer) Con_PrintLinef ("Received duplicate chunk %d, ignoring ...", chunknum);
		return;
	}

	chunked_receivedbytes += DLBLOCKSIZE_1024;
	recievedblock[chunknum&(MAXBLOCKS_1024-1)] = true;

	while (recievedblock[firstblock&(MAXBLOCKS_1024-1)]) {
		recievedblock[firstblock&(MAXBLOCKS_1024-1)] = false;
		firstblock++;
	}

	fseek (cls.qw_download_f, chunknum * DLBLOCKSIZE_1024, SEEK_SET);
	if (chunked_downloadsize - chunknum * DLBLOCKSIZE_1024 < DLBLOCKSIZE_1024)	//final block is actually meant to be smaller than we recieve.
		fwrite (data, 1, chunked_downloadsize - chunknum * DLBLOCKSIZE_1024, cls.qw_download_f);
	else
		fwrite (data, 1, DLBLOCKSIZE_1024, cls.qw_download_f);

	cls.qw_downloadpercent = chunked_receivedbytes/(float)chunked_downloadsize*100;
	cls.qw_downloadspeedcount += DLBLOCKSIZE_1024;

	tm = Sys_DirtyTime() - cls.qw_downloadstarttime; // how long we dl-ing
	cls.qw_downloadspeedrate = (tm ? chunked_receivedbytes / 1024 / tm : 0); // some average dl speed in KB/s
}
