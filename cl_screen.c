// cl_screen.c


#include "quakedef.h"
#include "cl_video.h"
#include "image.h"
#include "jpeg.h"
#include "image_png.h"
#include "cl_collision.h"
#include "libcurl.h"
#include "csprogs.h"
#include "r_stats.h"
#ifdef CONFIG_VIDEO_CAPTURE
#include "cap_avi.h"
#include "cap_ogg.h"
#endif

// we have to include snd_main.h here only to get access to snd_renderbuffer->format.speed when writing the AVI headers
#include "snd_main.h"

cvar_t scr_viewsize = {CF_CLIENT | CF_ARCHIVE, "viewsize","100", "how large the view should be, 110 disables inventory bar, 120 disables status bar"};
cvar_t scr_fov = {CF_CLIENT | CF_ARCHIVE, "fov","90", "field of vision, 1-170 degrees, default 90, some players use 110-130"};
cvar_t scr_conalpha = {CF_CLIENT | CF_ARCHIVE, "scr_conalpha", "1", "opacity of console background gfx/conback"};
cvar_t scr_conalphafactor = {CF_CLIENT | CF_ARCHIVE, "scr_conalphafactor", "1", "opacity of console background gfx/conback relative to scr_conalpha; when 0, gfx/conback is not drawn"};
cvar_t scr_conalpha2factor = {CF_CLIENT | CF_ARCHIVE, "scr_conalpha2factor", "0", "opacity of console background gfx/conback2 relative to scr_conalpha; when 0, gfx/conback2 is not drawn"};
cvar_t scr_conalpha3factor = {CF_CLIENT | CF_ARCHIVE, "scr_conalpha3factor", "0", "opacity of console background gfx/conback3 relative to scr_conalpha; when 0, gfx/conback3 is not drawn"};
cvar_t scr_conbrightness = {CF_CLIENT | CF_ARCHIVE, "scr_conbrightness", "1", "brightness of console background (0 = black, 1 = image)"};
cvar_t scr_conforcewhiledisconnected = {CF_CLIENT, "scr_conforcewhiledisconnected", "1", "1 forces fullscreen console while disconnected, 2 also forces it when the listen server has started but the client is still loading"}; // SEPUS
cvar_t scr_conheight = {CF_CLIENT | CF_ARCHIVE, "scr_conheight", "0.5", "fraction of screen height occupied by console (reduced as necessary for visibility of loading progress and infobar)"};
cvar_t scr_conscroll_x = {CF_CLIENT | CF_ARCHIVE, "scr_conscroll_x", "0", "scroll speed of gfx/conback in x direction"};
cvar_t scr_conscroll_y = {CF_CLIENT | CF_ARCHIVE, "scr_conscroll_y", "0", "scroll speed of gfx/conback in y direction"};
cvar_t scr_conscroll2_x = {CF_CLIENT | CF_ARCHIVE, "scr_conscroll2_x", "0", "scroll speed of gfx/conback2 in x direction"};
cvar_t scr_conscroll2_y = {CF_CLIENT | CF_ARCHIVE, "scr_conscroll2_y", "0", "scroll speed of gfx/conback2 in y direction"};
cvar_t scr_conscroll3_x = {CF_CLIENT | CF_ARCHIVE, "scr_conscroll3_x", "0", "scroll speed of gfx/conback3 in x direction"};
cvar_t scr_conscroll3_y = {CF_CLIENT | CF_ARCHIVE, "scr_conscroll3_y", "0", "scroll speed of gfx/conback3 in y direction"};
cvar_t csqc_full_width_height = {CF_CLIENT | CF_ARCHIVE, "csqc_full_width_height", "0", "csqc always gets a full client-sized canvas, vid_conwidth and vid_conheight are vid.width vid.height during csqc draws. 0 - Don't 1 - Yes [Zircon]"}; // Baker r7003 - option for csqc to get full vid.width/vid.height canvas
cvar_t csqc_full_width_height_available = {CF_CLIENT /*| CF_READONLY*/, "csqc_full_width_height_available", "1", "Indicates ability for csqc to gain access full width/height resolution, otherwise a mod would want to set vid_conwidth in csqc like Xonotic [Zircon]"}; // Baker r7004
#ifdef CONFIG_MENU
cvar_t scr_menuforcewhiledisconnected = {CF_CLIENT, "scr_menuforcewhiledisconnected", "0", "forces menu while disconnected"};
#endif
cvar_t scr_centertime = {CF_CLIENT, "scr_centertime","2", "how long centerprint messages show"};
cvar_t scr_showram = {CF_CLIENT | CF_ARCHIVE, "showram","1", "show ram icon if low on surface cache memory (not used)"};
cvar_t scr_showturtle = {CF_CLIENT | CF_ARCHIVE, "showturtle","0", "show turtle icon when framerate is too low"};
cvar_t scr_showpause = {CF_CLIENT | CF_ARCHIVE, "showpause","1", "show pause icon when game is paused"};
cvar_t scr_showbrand = {CF_CLIENT, "showbrand","0", "shows gfx/brand.tga in a corner of the screen (different values select different positions, including centered)"};
cvar_t scr_printspeed = {CF_CLIENT, "scr_printspeed","0", "speed of intermission printing (episode end texts), a value of 0 disables the slow printing"};
cvar_t scr_loadingscreen_background = {CF_CLIENT, "scr_loadingscreen_background","0", "show the last visible background during loading screen (costs one screenful of video memory)"};
cvar_t scr_loadingscreen_scale = {CF_CLIENT, "scr_loadingscreen_scale","1", "scale factor of the background"};
cvar_t scr_loadingscreen_scale_base = {CF_CLIENT, "scr_loadingscreen_scale_base","0", "0 = console pixels, 1 = video pixels"};
cvar_t scr_loadingscreen_scale_limit = {CF_CLIENT, "scr_loadingscreen_scale_limit","0", "0 = no limit, 1 = until first edge hits screen edge, 2 = until last edge hits screen edge, 3 = until width hits screen width, 4 = until height hits screen height"};
cvar_t scr_loadingscreen_picture = {CF_CLIENT, "scr_loadingscreen_picture", "gfx/loading", "picture shown during loading"};
cvar_t scr_loadingscreen_count = {CF_CLIENT, "scr_loadingscreen_count","1", "number of loading screen files to use randomly (named loading.tga, loading2.tga, loading3.tga, ...)"};
cvar_t scr_loadingscreen_firstforstartup = {CF_CLIENT, "scr_loadingscreen_firstforstartup","0", "remove loading.tga from random scr_loadingscreen_count selection and only display it on client startup, 0 = normal, 1 = firstforstartup"};

cvar_t scr_loadingscreen_barcolor = {CF_CLIENT, "scr_loadingscreen_barcolor", "0.25 0.25 0.25", "rgb color of loadingscreen progress bar [Zircon default]"}; // Baker r8003 gray progress bar is the default color.

cvar_t scr_loadingscreen_barheight = {CF_CLIENT, "scr_loadingscreen_barheight", "0", "the height of the loadingscreen progress bar [Zircon default]"}; // Baker r8082: Was going to set 0, but some of the slow loading giant Quake maps, this increases confidence it is actually loading and not frozen
cvar_t scr_loadingscreen_maxfps = {CF_CLIENT, "scr_loadingscreen_maxfps", "10", "maximum FPS for loading screen so it will not update very often (this reduces loading time with lots of models)"}; // Baker r1462: Faster load times.
cvar_t scr_infobar_height = {CF_CLIENT, "scr_infobar_height", "8", "the height of the infobar items"};
//cvar_t vid_conwidthauto = {CF_CLIENT | CF_ARCHIVE, "vid_conwidthauto", "1", "automatically update vid_conwidth to match aspect ratio"}; // Baker r0005: 2d sizing needs this removed as is not using value in calc
cvar_t vid_conwidth = {CF_CLIENT | CF_ARCHIVE, "vid_conwidth", "640", "virtual width of 2D graphics system (note: changes may be overwritten, see vid_conwidthauto)"};
cvar_t vid_conheight = {CF_CLIENT | CF_ARCHIVE, "vid_conheight", "480", "virtual height of 2D graphics system"};

cvar_t vid_pixelheight = {CF_CLIENT | CF_ARCHIVE, "vid_pixelheight", "1", "adjusts vertical field of vision to account for non-square pixels (1280x1024 on a CRT monitor for example)"};
cvar_t scr_screenshot_jpeg = {CF_CLIENT | CF_ARCHIVE, "scr_screenshot_jpeg","1", "save jpeg instead of targa"};
cvar_t scr_screenshot_jpeg_quality = {CF_CLIENT | CF_ARCHIVE, "scr_screenshot_jpeg_quality","0.9", "image quality of saved jpeg"};
cvar_t scr_screenshot_png = {CF_CLIENT | CF_ARCHIVE, "scr_screenshot_png","0", "save png instead of targa"};
cvar_t scr_screenshot_gammaboost = {CF_CLIENT | CF_ARCHIVE, "scr_screenshot_gammaboost","1", "gamma correction on saved screenshots and videos, 1.0 saves unmodified images"};
cvar_t scr_screenshot_alpha = {CF_CLIENT, "scr_screenshot_alpha","0", "try to write an alpha channel to screenshots (debugging feature)"};
cvar_t scr_screenshot_timestamp = {CF_CLIENT | CF_ARCHIVE, "scr_screenshot_timestamp", "1", "use a timestamp based number of the type YYYYMMDDHHMMSSsss instead of sequential numbering"};
// scr_screenshot_name is defined in fs.c
#ifdef CONFIG_VIDEO_CAPTURE
cvar_t cl_capturevideo = {CF_CLIENT, "cl_capturevideo", "0", "enables saving of video to a .avi file using uncompressed I420 colorspace and PCM audio, note that scr_screenshot_gammaboost affects the brightness of the output)"};
cvar_t cl_capturevideo_demo_stop = {CF_CLIENT | CF_ARCHIVE, "cl_capturevideo_demo_stop", "1", "automatically stops video recording when demo ends"};
cvar_t cl_capturevideo_printfps = {CF_CLIENT | CF_ARCHIVE, "cl_capturevideo_printfps", "1", "prints the frames per second captured in capturevideo (is only written to the log file, not to the console, as that would be visible on the video)"};
cvar_t cl_capturevideo_width = {CF_CLIENT | CF_ARCHIVE, "cl_capturevideo_width", "0", "scales all frames to this resolution before saving the video"};
cvar_t cl_capturevideo_height = {CF_CLIENT | CF_ARCHIVE, "cl_capturevideo_height", "0", "scales all frames to this resolution before saving the video"};
cvar_t cl_capturevideo_realtime = {CF_CLIENT, "cl_capturevideo_realtime", "0", "causes video saving to operate in realtime (mostly useful while playing, not while capturing demos), this can produce a much lower quality video due to poor sound/video sync and will abort saving if your machine stalls for over a minute"};
cvar_t cl_capturevideo_fps = {CF_CLIENT | CF_ARCHIVE, "cl_capturevideo_fps", "30", "how many frames per second to save (29.97 for NTSC, 30 for typical PC video, 15 can be useful)"};
cvar_t cl_capturevideo_nameformat = {CF_CLIENT | CF_ARCHIVE, "cl_capturevideo_nameformat", "dpvideo", "prefix for saved videos (the date is encoded using strftime escapes)"};
cvar_t cl_capturevideo_number = {CF_CLIENT | CF_ARCHIVE, "cl_capturevideo_number", "1", "number to append to video filename, incremented each time a capture begins"};
cvar_t cl_capturevideo_ogg = {CF_CLIENT | CF_ARCHIVE, "cl_capturevideo_ogg", "1", "save captured video data as Ogg/Vorbis/Theora streams"};
cvar_t cl_capturevideo_framestep = {CF_CLIENT | CF_ARCHIVE, "cl_capturevideo_framestep", "1", "when set to n >= 1, render n frames to capture one (useful for motion blur like effects)"};
#endif
cvar_t r_letterbox = {CF_CLIENT, "r_letterbox", "0", "reduces vertical height of view to simulate a letterboxed movie effect (can be used by mods for cutscenes)"};
cvar_t r_stereo_separation = {CF_CLIENT, "r_stereo_separation", "4", "separation distance of eyes in the world (negative values are only useful for cross-eyed viewing)"};
cvar_t r_stereo_sidebyside = {CF_CLIENT, "r_stereo_sidebyside", "0", "side by side views for those who can't afford glasses but can afford eye strain (note: use a negative r_stereo_separation if you want cross-eyed viewing)"};
cvar_t r_stereo_horizontal = {CF_CLIENT, "r_stereo_horizontal", "0", "aspect skewed side by side view for special decoder/display hardware"};
cvar_t r_stereo_vertical = {CF_CLIENT, "r_stereo_vertical", "0", "aspect skewed top and bottom view for special decoder/display hardware"};
cvar_t r_stereo_redblue = {CF_CLIENT, "r_stereo_redblue", "0", "red/blue anaglyph stereo glasses (note: most of these glasses are actually red/cyan, try that one too)"};
cvar_t r_stereo_redcyan = {CF_CLIENT, "r_stereo_redcyan", "0", "red/cyan anaglyph stereo glasses, the kind given away at drive-in movies like Creature From The Black Lagoon In 3D"};
cvar_t r_stereo_redgreen = {CF_CLIENT, "r_stereo_redgreen", "0", "red/green anaglyph stereo glasses (for those who don't mind yellow)"};
cvar_t r_stereo_angle = {CF_CLIENT, "r_stereo_angle", "0", "separation angle of eyes (makes the views look different directions, as an example, 90 gives a 90 degree separation where the views are 45 degrees left and 45 degrees right)"};
cvar_t scr_stipple = {CF_CLIENT, "scr_stipple", "0", "interlacing-like stippling of the display"};
cvar_t scr_refresh = {CF_CLIENT, "scr_refresh", "1", "allows you to completely shut off rendering for benchmarking purposes"};
cvar_t scr_screenshot_name_in_mapdir = {CF_CLIENT | CF_ARCHIVE, "scr_screenshot_name_in_mapdir", "0", "if set to 1, screenshots are placed in a subdirectory named like the map they are from"};
cvar_t net_graph = {CF_CLIENT | CF_ARCHIVE, "net_graph", "0", "shows a graph of packet sizes and other information, 0 = off, 1 = show client netgraph, 2 = show client and server netgraphs (when hosting a server)"};
cvar_t cl_demo_mousegrab = {CF_CLIENT, "cl_demo_mousegrab", "0", "Allows reading the mouse input while playing demos. Useful for camera mods developed in csqc. (0: never, 1: always)"};
cvar_t timedemo_screenshotframelist = {CF_CLIENT, "timedemo_screenshotframelist", "", "when performing a timedemo, take screenshots of each frame in this space-separated list - example: 1 201 401"};
cvar_t vid_touchscreen_outlinealpha = {CF_CLIENT, "vid_touchscreen_outlinealpha", "0", "opacity of touchscreen area outlines"};
cvar_t vid_touchscreen_overlayalpha = {CF_CLIENT, "vid_touchscreen_overlayalpha", "0.25", "opacity of touchscreen area icons"};

cvar_t tool_inspector = {CF_CLIENT, "tool_inspector", "0", "view visible entity QC information [Zircon]"}; // Baker r0106: tool inspector
cvar_t tool_marker = {CF_CLIENT, "tool_marker", "0", "set a position to display on-screen, this value must be quoted since it is a since string [Zircon]"}; // Baker r0109: tool marker

extern cvar_t sbar_info_pos;
extern cvar_t r_fog_clear;

int jpeg_supported = false;

qbool	scr_initialized;		// ready to draw

int scr_loading = false;  // we are in a loading screen

unsigned int        scr_con_current;
static unsigned int scr_con_margin_bottom;

extern int	con_vislines;

extern int cl_punchangle_applied;

static void SCR_ScreenShot_f(cmd_state_t *cmd);
static void R_Envmap_f(cmd_state_t *cmd);

// backend
void R_ClearScreen(qbool fogcolor);

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

char		scr_centerstring[MAX_INPUTLINE_16384];
float		scr_centertime_start;	// for slow victory printing
float		scr_centertime_off;
int			scr_center_lines;
int			scr_erase_lines;
int			scr_erase_center;
char        scr_infobarstring[MAX_INPUTLINE_16384];
float       scr_infobartime_off;

/*
==============
SCR_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void SCR_CenterPrint(const char *str)
{
	Con_LogCenterPrint (str); // Baker r1421: centerprint logging to console
	c_strlcpy (scr_centerstring, str);
	scr_centertime_off = scr_centertime.value;
	scr_centertime_start = cl.time;

// count the number of lines for centering
	scr_center_lines = 1;
	while (*str)
	{
		if (*str == '\n')
			scr_center_lines++;
		str++;
	}
}


static void SCR_DrawCenterString (void)
{
	char	*start;
	int		x, y;
	int		remaining;
	int		color;

	if (cl.intermission == 2) // in finale,
		if (sb_showscores) // make TAB hide the finale message (sb_showscores overrides finale in sbar.c)
			return;

	if (scr_centertime.value <= 0 && !cl.intermission)
		return;

// the finale prints the characters one at a time, except if printspeed is an absurdly high value
	if (cl.intermission && scr_printspeed.value > 0 && scr_printspeed.value < 1000000)
		remaining = (int)(scr_printspeed.value * (cl.time - scr_centertime_start));
	else
		remaining = 9999;

	scr_erase_center = 0;
	start = scr_centerstring;

	if (remaining < 1)
		return;

	if (scr_center_lines <= 4)
		y = (int)(vid_conheight.integer*0.35);
	else
		y = 48;

	color = -1;
	do
	{
		// scan the number of characters on the line, not counting color codes
		char *newline = strchr(start, '\n');
		int l = newline ? (newline - start) : (int)strlen(start);
		float width = DrawQ_TextWidth(start, l, 8, 8, false, FONT_CENTERPRINT);

		x = (int) (vid_conwidth.integer - width)/2;
		if (l > 0)
		{
			if (remaining < l)
				l = remaining;
			DrawQ_String(x, y, start, l, 8, 8, 1, 1, 1, 1, 0, &color, false, FONT_CENTERPRINT);
			remaining -= l;
			if (remaining <= 0)
				return;
		}
		y += 8;

		if (!newline)
			break;
		start = newline + 1; // skip the \n
	} while (1);
}

static void SCR_CheckDrawCenterString (void)
{
	if (scr_center_lines > scr_erase_lines)
		scr_erase_lines = scr_center_lines;

	if (cl.time > cl.oldtime)
		scr_centertime_off -= cl.time - cl.oldtime;

	// don't draw if this is a normal stats-screen intermission,
	// only if it is not an intermission, or a finale intermission
	if (cl.intermission == 1)
		return;
	if (scr_centertime_off <= 0 && !cl.intermission)
		return;
	if (key_dest != key_game)
		return;

	SCR_DrawCenterString ();
}

static void SCR_DrawNetGraph_DrawGraph (int graphx, int graphy, int graphwidth, int graphheight, float graphscale, int graphlimit, const char *label, float textsize, int packetcounter, netgraphitem_t *netgraph)
{
	netgraphitem_t *graph;
	int j, x, y;
	int totalbytes = 0;
	char bytesstring[128];
	float g[NETGRAPH_PACKETS][7];
	float *a;
	float *b;
	DrawQ_Fill(graphx, graphy, graphwidth, graphheight + textsize * 2, 0, 0, 0, 0.5, 0);
	// draw the bar graph itself
	memset(g, 0, sizeof(g));
	for (j = 0;j < NETGRAPH_PACKETS;j++)
	{
		graph = netgraph + j;
		g[j][0] = 1.0f - 0.25f * (host.realtime - graph->time);
		g[j][1] = 1.0f;
		g[j][2] = 1.0f;
		g[j][3] = 1.0f;
		g[j][4] = 1.0f;
		g[j][5] = 1.0f;
		g[j][6] = 1.0f;
		if (graph->unreliablebytes == NETGRAPH_LOSTPACKET)
			g[j][1] = 0.00f;
		else if (graph->unreliablebytes == NETGRAPH_CHOKEDPACKET)
			g[j][2] = 0.90f;
		else
		{
			if (netgraph[j].time >= netgraph[(j+NETGRAPH_PACKETS-1)%NETGRAPH_PACKETS].time)
				if (graph->unreliablebytes + graph->reliablebytes + graph->ackbytes >= graphlimit * (netgraph[j].time - netgraph[(j+NETGRAPH_PACKETS-1)%NETGRAPH_PACKETS].time))
					g[j][2] = 0.98f;
			g[j][3] = 1.0f    - graph->unreliablebytes * graphscale;
			g[j][4] = g[j][3] - graph->reliablebytes   * graphscale;
			g[j][5] = g[j][4] - graph->ackbytes        * graphscale;
			// count bytes in the last second
			if (host.realtime - graph->time < 1.0f)
				totalbytes += graph->unreliablebytes + graph->reliablebytes + graph->ackbytes;
		}
		if (graph->cleartime >= 0)
			g[j][6] = 0.5f + 0.5f * (2.0 / M_PI) * atan((M_PI / 2.0) * (graph->cleartime - graph->time));
		g[j][1] = bound(0.0f, g[j][1], 1.0f);
		g[j][2] = bound(0.0f, g[j][2], 1.0f);
		g[j][3] = bound(0.0f, g[j][3], 1.0f);
		g[j][4] = bound(0.0f, g[j][4], 1.0f);
		g[j][5] = bound(0.0f, g[j][5], 1.0f);
		g[j][6] = bound(0.0f, g[j][6], 1.0f);
	}
	// render the lines for the graph
	for (j = 0;j < NETGRAPH_PACKETS;j++)
	{
		a = g[j];
		b = g[(j+1)%NETGRAPH_PACKETS];
		if (a[0] < 0.0f || b[0] > 1.0f || b[0] < a[0])
			continue;
		DrawQ_Line(1, graphx + graphwidth * a[0], graphy + graphheight * a[2], graphx + graphwidth * b[0], graphy + graphheight * b[2], 1.0f, 1.0f, 1.0f, 1.0f, 0);
		DrawQ_Line(1, graphx + graphwidth * a[0], graphy + graphheight * a[1], graphx + graphwidth * b[0], graphy + graphheight * b[1], 1.0f, 0.0f, 0.0f, 1.0f, 0);
		DrawQ_Line(1, graphx + graphwidth * a[0], graphy + graphheight * a[5], graphx + graphwidth * b[0], graphy + graphheight * b[5], 0.0f, 1.0f, 0.0f, 1.0f, 0);
		DrawQ_Line(1, graphx + graphwidth * a[0], graphy + graphheight * a[4], graphx + graphwidth * b[0], graphy + graphheight * b[4], 1.0f, 1.0f, 1.0f, 1.0f, 0);
		DrawQ_Line(1, graphx + graphwidth * a[0], graphy + graphheight * a[3], graphx + graphwidth * b[0], graphy + graphheight * b[3], 1.0f, 0.5f, 0.0f, 1.0f, 0);
		DrawQ_Line(1, graphx + graphwidth * a[0], graphy + graphheight * a[6], graphx + graphwidth * b[0], graphy + graphheight * b[6], 0.0f, 0.0f, 1.0f, 1.0f, 0);
	}
	x = graphx;
	y = graphy + graphheight;
	dpsnprintf(bytesstring, sizeof(bytesstring), "%d", totalbytes);
	DrawQ_String(x, y, label      , 0, textsize, textsize, 1.0f, 1.0f, 1.0f, 1.0f, 0, NULL, false, FONT_DEFAULT);y += textsize;
	DrawQ_String(x, y, bytesstring, 0, textsize, textsize, 1.0f, 1.0f, 1.0f, 1.0f, 0, NULL, false, FONT_DEFAULT);y += textsize;
}

/*
==============
SCR_DrawNetGraph
==============
*/
static void SCR_DrawNetGraph (void)
{
	int i, separator1, separator2, graphwidth, graphheight, netgraph_x, netgraph_y, textsize, index, netgraphsperrow, graphlimit;
	float graphscale;
	netconn_t *c;
	char vabuf[1024];

	if (cls.state != ca_connected)
		return;
	if (!cls.netcon)
		return;
	if (!net_graph.integer)
		return;

	separator1 = 2;
	separator2 = 4;
	textsize = 8;
	graphwidth = 120;
	graphheight = 70;
	graphscale = 1.0f / 1500.0f;
	graphlimit = cl_rate.integer;

	netgraphsperrow = (vid_conwidth.integer + separator2) / (graphwidth * 2 + separator1 + separator2);
	netgraphsperrow = max(netgraphsperrow, 1);

	index = 0;
	netgraph_x = (vid_conwidth.integer + separator2) - (1 + (index % netgraphsperrow)) * (graphwidth * 2 + separator1 + separator2);
	netgraph_y = (vid_conheight.integer - 48 - sbar_info_pos.integer + separator2) - (1 + (index / netgraphsperrow)) * (graphheight + textsize + separator2);
	c = cls.netcon;
	SCR_DrawNetGraph_DrawGraph(netgraph_x                          , netgraph_y, graphwidth, graphheight, graphscale, graphlimit, "incoming", textsize, c->incoming_packetcounter, c->incoming_netgraph);
	SCR_DrawNetGraph_DrawGraph(netgraph_x + graphwidth + separator1, netgraph_y, graphwidth, graphheight, graphscale, graphlimit, "outgoing", textsize, c->outgoing_packetcounter, c->outgoing_netgraph);
	index++;

	if (sv.active && net_graph.integer >= 2)
	{
		for (i = 0;i < svs.maxclients;i++)
		{
			c = svs.clients[i].netconnection;
			if (!c)
				continue;
			netgraph_x = (vid_conwidth.integer + separator2) - (1 + (index % netgraphsperrow)) * (graphwidth * 2 + separator1 + separator2);
			netgraph_y = (vid_conheight.integer - 48 + separator2) - (1 + (index / netgraphsperrow)) * (graphheight + textsize + separator2);
			SCR_DrawNetGraph_DrawGraph(netgraph_x                          , netgraph_y, graphwidth, graphheight, graphscale, graphlimit, va(vabuf, sizeof(vabuf), "%s", svs.clients[i].name), textsize, c->outgoing_packetcounter, c->outgoing_netgraph);
			SCR_DrawNetGraph_DrawGraph(netgraph_x + graphwidth + separator1, netgraph_y, graphwidth, graphheight, graphscale, graphlimit, ""                           , textsize, c->incoming_packetcounter, c->incoming_netgraph);
			index++;
		}
	}
}

/*
==============
SCR_DrawTurtle
==============
*/
static void SCR_DrawTurtle (void)
{
	static int	count;

	if (cls.state != ca_connected)
		return;

	if (!scr_showturtle.integer)
		return;

	if (cl.realframetime < 0.1)
	{
		count = 0;
		return;
	}

	count++;
	if (count < 3)
		return;

	DrawQ_Pic (0, 0, Draw_CachePic ("gfx/turtle"), 0, 0, 1, 1, 1, 1, 0);
}

/*
==============
SCR_DrawNet
==============
*/
static void SCR_DrawNet (void)
{
	if (cls.state != ca_connected)
		return;
	if (host.realtime - cl.last_received_message < 0.3)
		return;
	if (cls.demoplayback)
		return;

	DrawQ_Pic (64, 0, Draw_CachePic ("gfx/net"), 0, 0, 1, 1, 1, 1, 0);
}

/*
==============
DrawPause
==============
*/
static void SCR_DrawPause (void)
{
	cachepic_t	*pic;

	if (cls.state != ca_connected)
		return;

	if (!scr_showpause.integer)		// turn off for screenshots
		return;

	if (!cl.paused)
		return;

	pic = Draw_CachePic ("gfx/pause");
	DrawQ_Pic ((vid_conwidth.integer - Draw_GetPicWidth(pic))/2, (vid_conheight.integer - Draw_GetPicHeight(pic))/2, pic, 0, 0, 1, 1, 1, 1, 0);
}

/*
==============
SCR_DrawBrand
==============
*/
static void SCR_DrawBrand (void)
{
	cachepic_t	*pic;
	float		x, y;

	if (!scr_showbrand.value)
		return;

	pic = Draw_CachePic ("gfx/brand");

	switch ((int)scr_showbrand.value)
	{
	case 1:	// bottom left
		x = 0;
		y = vid_conheight.integer - Draw_GetPicHeight(pic);
		break;
	case 2:	// bottom centre
		x = (vid_conwidth.integer - Draw_GetPicWidth(pic)) / 2;
		y = vid_conheight.integer - Draw_GetPicHeight(pic);
		break;
	case 3:	// bottom right
		x = vid_conwidth.integer - Draw_GetPicWidth(pic);
		y = vid_conheight.integer - Draw_GetPicHeight(pic);
		break;
	case 4:	// centre right
		x = vid_conwidth.integer - Draw_GetPicWidth(pic);
		y = (vid_conheight.integer - Draw_GetPicHeight(pic)) / 2;
		break;
	case 5:	// top right
		x = vid_conwidth.integer - Draw_GetPicWidth(pic);
		y = 0;
		break;
	case 6:	// top centre
		x = (vid_conwidth.integer - Draw_GetPicWidth(pic)) / 2;
		y = 0;
		break;
	case 7:	// top left
		x = 0;
		y = 0;
		break;
	case 8:	// centre left
		x = 0;
		y = (vid_conheight.integer - Draw_GetPicHeight(pic)) / 2;
		break;
	default:
		return;
	}

	DrawQ_Pic (x, y, pic, 0, 0, 1, 1, 1, 1, 0);
}

/*
==============
SCR_DrawQWDownload
==============
*/
static int SCR_DrawQWDownload(int offset)
{
	// sync with SCR_InfobarHeight
	int len;
	float x, y;
	float size = scr_infobar_height.value;
	char temp[256];

	if (!cls.qw_downloadname[0])
	{
		cls.qw_downloadspeedrate = 0;
		cls.qw_downloadspeedtime = host.realtime;
		cls.qw_downloadspeedcount = 0;
		return 0;
	}
	if (host.realtime >= cls.qw_downloadspeedtime + 1)
	{
		cls.qw_downloadspeedrate = cls.qw_downloadspeedcount;
		cls.qw_downloadspeedtime = host.realtime;
		cls.qw_downloadspeedcount = 0;
	}
	if (cls.protocol == PROTOCOL_QUAKEWORLD)
		dpsnprintf(temp, sizeof(temp), "Downloading %s %3i%% (%d) at %d bytes/s", cls.qw_downloadname, cls.qw_downloadpercent, cls.qw_downloadmemorycursize, cls.qw_downloadspeedrate);
	else
		dpsnprintf(temp, sizeof(temp), "Downloading %s %3i%% (%d/%d) at %d bytes/s", cls.qw_downloadname, cls.qw_downloadpercent, cls.qw_downloadmemorycursize, cls.qw_downloadmemorymaxsize, cls.qw_downloadspeedrate);
	len = (int)strlen(temp);
	x = (vid_conwidth.integer - DrawQ_TextWidth(temp, len, size, size, true, FONT_INFOBAR)) / 2;
	y = vid_conheight.integer - size - offset;
	DrawQ_Fill(0, y, vid_conwidth.integer, size, 0, 0, 0, cls.signon == SIGNONS_4 ? 0.5 : 1, 0);
	DrawQ_String(x, y, temp, len, size, size, 1, 1, 1, 1, 0, NULL, true, FONT_INFOBAR);
	return size;
}
/*
==============
SCR_DrawInfobarString
==============
*/
static int SCR_DrawInfobarString(int offset)
{
	int len;
	float x, y;
	float size = scr_infobar_height.value;

	len = (int)strlen(scr_infobarstring);
	x = (vid_conwidth.integer - DrawQ_TextWidth(scr_infobarstring, len, size, size, false, FONT_INFOBAR)) / 2;
	y = vid_conheight.integer - size - offset;
	DrawQ_Fill(0, y, vid_conwidth.integer, size, 0, 0, 0, cls.signon == SIGNONS_4 ? 0.5 : 1, 0);
	DrawQ_String(x, y, scr_infobarstring, len, size, size, 1, 1, 1, 1, 0, NULL, false, FONT_INFOBAR);
	return size;
}

/*
==============
SCR_DrawCurlDownload
==============
*/
static int SCR_DrawCurlDownload(int offset)
{
	// sync with SCR_InfobarHeight
	int len;
	int nDownloads;
	int i;
	float x, y;
	float size = scr_infobar_height.value;
	Curl_downloadinfo_t *downinfo;
	char temp[256];
	char addinfobuf[128];
	const char *addinfo;

	downinfo = Curl_GetDownloadInfo(&nDownloads, &addinfo, addinfobuf, sizeof(addinfobuf));
	if (!downinfo)
		return 0;

	y = vid_conheight.integer - size * nDownloads - offset;

	if (addinfo)
	{
		len = (int)strlen(addinfo);
		x = (vid_conwidth.integer - DrawQ_TextWidth(addinfo, len, size, size, true, FONT_INFOBAR)) / 2;
		DrawQ_Fill(0, y - size, vid_conwidth.integer, size, 1, 1, 1, cls.signon == SIGNONS_4 ? 0.8 : 1, 0);
		DrawQ_String(x, y - size, addinfo, len, size, size, 0, 0, 0, 1, 0, NULL, true, FONT_INFOBAR);
	}

	for(i = 0; i != nDownloads; ++i)
	{
		if (downinfo[i].queued)
			dpsnprintf(temp, sizeof(temp), "Still in queue: %s", downinfo[i].filename);
		else if (downinfo[i].progress <= 0)
			dpsnprintf(temp, sizeof(temp), "Downloading %s ...  ???.?%% @ %.1f KiB/s", downinfo[i].filename, downinfo[i].speed / 1024.0);
		else
			dpsnprintf(temp, sizeof(temp), "Downloading %s ...  %5.1f%% @ %.1f KiB/s", downinfo[i].filename, 100.0 * downinfo[i].progress, downinfo[i].speed / 1024.0);
		len = (int)strlen(temp);
		x = (vid_conwidth.integer - DrawQ_TextWidth(temp, len, size, size, true, FONT_INFOBAR)) / 2;
		DrawQ_Fill(0, y + i * size, vid_conwidth.integer, size, 0, 0, 0, cls.signon == SIGNONS_4 ? 0.5 : 1, 0);
		DrawQ_String(x, y + i * size, temp, len, size, size, 1, 1, 1, 1, 0, NULL, true, FONT_INFOBAR);
	}

	Z_Free(downinfo);

	return size * (nDownloads + (addinfo ? 1 : 0));
}

/*
==============
SCR_DrawInfobar
==============
*/
static void SCR_DrawInfobar(void)
{
	unsigned int offset = 0;
	offset += SCR_DrawQWDownload(offset);
	offset += SCR_DrawCurlDownload(offset);
	if (scr_infobartime_off > 0)
		offset += SCR_DrawInfobarString(offset);
	if (!offset && scr_loading) {
		int effective_loading = scr_loadingscreen_barheight.integer;
		if (!effective_loading && scr_loading && cl_signon_start_time
			&& Sys_DirtyTime() - cl_signon_start_time > 1.5)
			effective_loading = 8;

		offset = effective_loading;
	}
	if (offset != scr_con_margin_bottom)
		Con_DPrintLinef ("broken console margin calculation: %d != %d", offset, scr_con_margin_bottom);
}

static int SCR_InfobarHeight(void)
{
	int offset = 0;
	Curl_downloadinfo_t *downinfo;
	const char *addinfo;
	int nDownloads;
	char addinfobuf[128];

	if (cl.time > cl.oldtime)
		scr_infobartime_off -= cl.time - cl.oldtime;
	if (scr_infobartime_off > 0)
		offset += 1;
	if (cls.qw_downloadname[0])
		offset += 1;

	downinfo = Curl_GetDownloadInfo(&nDownloads, &addinfo, addinfobuf, sizeof(addinfobuf));
	if (downinfo)
	{
		offset += (nDownloads + (addinfo ? 1 : 0));
		Z_Free(downinfo);
	}
	offset *= scr_infobar_height.value;

	return offset;
}

/*
==============
SCR_InfoBar_f
==============
*/
static void SCR_InfoBar_f (cmd_state_t *cmd)
{
	if (Cmd_Argc(cmd) == 3) {
		scr_infobartime_off = atof(Cmd_Argv(cmd, 1));
		c_strlcpy (scr_infobarstring, Cmd_Argv(cmd, 2) );
		Con_LogCenterPrint (scr_infobarstring); // Baker r1421: centerprint logging to console
	} else {
		Con_PrintLinef ("usage:" NEWLINE "infobar expiretime " QUOTED_STR ("string") );
	}
}
//=============================================================================

/*
==================
SCR_SetUpToDrawConsole
==================
*/

static void SCR_SetUpToDrawConsole (void)
{
#ifdef CONFIG_MENU
	static int framecounter = 0;
#endif

	Con_CheckResize ();

#ifdef CONFIG_MENU
	if (scr_menuforcewhiledisconnected.integer && key_dest == key_game && cls.state == ca_disconnected)
	{
		if (framecounter >= 2)
			MR_ToggleMenu(1); // conexit
		else
			framecounter++;
	}
	else
		framecounter = 0;
#endif

	if (scr_conforcewhiledisconnected.integer >= 2 && key_dest == key_game && cls.signon != SIGNONS_4)
		Flag_Add_To(key_consoleactive, KEY_CONSOLEACTIVE_FORCED_4);
	else if (scr_conforcewhiledisconnected.integer >= 1 && key_dest == key_game && cls.signon != SIGNONS_4 && !sv.active) {
		Flag_Add_To(key_consoleactive, KEY_CONSOLEACTIVE_FORCED_4); // |= KEY_CONSOLEACTIVE_FORCED;
	}
	else
		Flag_Remove_From(key_consoleactive, KEY_CONSOLEACTIVE_FORCED_4); // &= ~KEY_CONSOLEACTIVE_FORCED_4;

	// decide on the height of the console
	if (Have_Flag(key_consoleactive, KEY_CONSOLEACTIVE_USER_1)) {
		// Baker r0004: Ctrl + up/down size console like JoeQuake
		scr_con_current = vid_conheight.integer *scr_conheight.value * console_user_pct; // Baker 1032.2
	}
	else
		scr_con_current = 0; // none visible
}

/*
==================
SCR_DrawConsole
==================
*/
int scr_skip_once;

void SCR_DrawConsole (void)
{
	static int mfirst;
	int effective_loading = scr_loadingscreen_barheight.integer;
	if (!effective_loading && 
		scr_loading && 
		cl_signon_start_time &&
		((Sys_DirtyTime() - cl_signon_start_time) > 1.5) )
		effective_loading = 8;


	// infobar and loading progress are not drawn simultaneously
	scr_con_margin_bottom = scr_loading ? effective_loading : SCR_InfobarHeight();

	if (mfirst == 0) {
		mfirst = 1;
		goto no_draw_0;
	}
	if (scr_skip_once) {
		scr_skip_once = false;
		goto no_draw_0;
	}

	if (Have_Flag (key_consoleactive, KEY_CONSOLEACTIVE_FORCED_4)) {
		// full screen
		#if 1
			if (cls.state == ca_connected && in_range (2, cls.signon, 3) && 
				scr_loading) {
				goto no_draw_0;
			}
		#endif
		Con_DrawConsole (vid_conheight.integer - scr_con_margin_bottom);
	}
	else if (scr_con_current)
		Con_DrawConsole (Smallest(scr_con_current, vid_conheight.integer - scr_con_margin_bottom));
	else {
no_draw_0:
		con_vislines = 0;
	}
}

//=============================================================================

/*
=================
SCR_SizeUp_f

Keybinding command
=================
*/
static void SCR_SizeUp_f (cmd_state_t *cmd)
{
	Cvar_SetValueQuick (&scr_viewsize, scr_viewsize.value + 10);
}


/*
=================
SCR_SizeDown_f

Keybinding command
=================
*/
static void SCR_SizeDown_f (cmd_state_t *cmd)
{
	Cvar_SetValueQuick(&scr_viewsize, scr_viewsize.value - 10);
}

#ifdef CONFIG_VIDEO_CAPTURE
void SCR_CaptureVideo_EndVideo(void);
#endif
void CL_Screen_Shutdown(void)
{
#ifdef CONFIG_VIDEO_CAPTURE
	SCR_CaptureVideo_EndVideo();
#endif
}


/*
==================
SCR_ScreenShot_f
==================
*/
void SCR_ScreenShot_f (cmd_state_t *cmd)
{
	static int shotnumber;
	static char old_prefix_name[MAX_QPATH_128];
	char prefix_name[MAX_QPATH_128];
	char filename[MAX_QPATH_128];
	unsigned char *buffer1;
	unsigned char *buffer2;
	qbool jpeg = (scr_screenshot_jpeg.integer != 0);
	qbool png = (scr_screenshot_png.integer != 0) && !jpeg;
	char vabuf[1024];

	if (Cmd_Argc(cmd) == 2) {
		const char *ext;
		c_strlcpy(filename, Cmd_Argv(cmd, 1));
		ext = FS_FileExtension(filename);
		if (String_Does_Match_Caseless(ext, "jpg"))
		{
			jpeg = true;
			png = false;
		}
		else if (String_Does_Match_Caseless(ext, "tga"))
		{
			jpeg = false;
			png = false;
		}
		else if (String_Does_Match_Caseless(ext, "png"))
		{
			jpeg = false;
			png = true;
		}
		else
		{
			Con_Printf ("screenshot: supplied filename must end in .jpg or .tga or .png\n");
			return;
		}
	}
	else if (scr_screenshot_timestamp.integer)
	{
		int shotnumber100;

		// TODO maybe make capturevideo and screenshot use similar name patterns?
		if (scr_screenshot_name_in_mapdir.integer && cl.worldbasename[0])
			dpsnprintf(prefix_name, sizeof(prefix_name), "%s/%s%s", cl.worldbasename, scr_screenshot_name.string, Sys_TimeString("%Y%m%d%H%M%S"));
		else
			dpsnprintf(prefix_name, sizeof(prefix_name), "%s%s", scr_screenshot_name.string, Sys_TimeString("%Y%m%d%H%M%S"));

		// find a file name to save it to
		for (shotnumber100 = 0;shotnumber100 < 100;shotnumber100++)
			if (!FS_SysFileExists(va(vabuf, sizeof(vabuf), "%s/screenshots/%s-%02d.tga", fs_gamedir, prefix_name, shotnumber100))
			 && !FS_SysFileExists(va(vabuf, sizeof(vabuf), "%s/screenshots/%s-%02d.jpg", fs_gamedir, prefix_name, shotnumber100))
			 && !FS_SysFileExists(va(vabuf, sizeof(vabuf), "%s/screenshots/%s-%02d.png", fs_gamedir, prefix_name, shotnumber100)))
				break;
		if (shotnumber100 >= 100)
		{
			Con_Print("Couldn't create the image file - already 100 shots taken this second!\n");
			return;
		}

		dpsnprintf(filename, sizeof(filename), "screenshots/%s-%02d.%s", prefix_name, shotnumber100, jpeg ? "jpg" : png ? "png" : "tga");
	}
	else
	{
		// TODO maybe make capturevideo and screenshot use similar name patterns?
		if (scr_screenshot_name_in_mapdir.integer && cl.worldbasename[0])
			dpsnprintf(prefix_name, sizeof(prefix_name), "%s/%s", cl.worldbasename, Sys_TimeString(scr_screenshot_name.string));
		else
			dpsnprintf(prefix_name, sizeof(prefix_name), "%s", Sys_TimeString(scr_screenshot_name.string));

		// if prefix changed, gamedir or map changed, reset the shotnumber so
		// we scan again
		// FIXME: should probably do this whenever FS_Rescan or something like that occurs?
		if (strcmp(old_prefix_name, prefix_name))
		{
			dpsnprintf(old_prefix_name, sizeof(old_prefix_name), "%s", prefix_name );
			shotnumber = 0;
		}

		// find a file name to save it to
		for (;shotnumber < 1000000;shotnumber++)
			if (!FS_SysFileExists(va(vabuf, sizeof(vabuf), "%s/screenshots/%s%06d.tga", fs_gamedir, prefix_name, shotnumber))
			 && !FS_SysFileExists(va(vabuf, sizeof(vabuf), "%s/screenshots/%s%06d.jpg", fs_gamedir, prefix_name, shotnumber))
			 && !FS_SysFileExists(va(vabuf, sizeof(vabuf), "%s/screenshots/%s%06d.png", fs_gamedir, prefix_name, shotnumber)))
				break;
		if (shotnumber >= 1000000)
		{
			Con_Print("Couldn't create the image file - you already have 1000000 screenshots!\n");
			return;
		}

		dpsnprintf(filename, sizeof(filename), "screenshots/%s%06d.%s", prefix_name, shotnumber, jpeg ? "jpg" : png ? "png" : "tga");

		shotnumber++;
	}

	buffer1 = (unsigned char *)Mem_Alloc(tempmempool, vid.width * vid.height * 4);
	buffer2 = (unsigned char *)Mem_Alloc(tempmempool, vid.width * vid.height * (scr_screenshot_alpha.integer ? 4 : 3));

	if (SCR_ScreenShot (filename, buffer1, buffer2, 0, 0, vid.width, vid.height, false, false, false, jpeg, png, true, scr_screenshot_alpha.integer != 0))
		Con_PrintLinef ("Wrote %s", filename);
	else
	{
		Con_PrintLinef (CON_ERROR "Unable to write %s", filename);
		if (jpeg || png)
		{
			if (SCR_ScreenShot (filename, buffer1, buffer2, 0, 0, vid.width, vid.height, false, false, false, false, false, true, scr_screenshot_alpha.integer != 0))
			{
				strlcpy(filename + strlen(filename) - 3, "tga", 4);
				Con_PrintLinef ("Wrote %s", filename);
			}
		}
	}

	Mem_Free (buffer1);
	Mem_Free (buffer2);
}

#ifdef CONFIG_VIDEO_CAPTURE
static void SCR_CaptureVideo_BeginVideo(void)
{
	double r, g, b;
	unsigned int i;
	int width = cl_capturevideo_width.integer, height = cl_capturevideo_height.integer;
	if (cls.capturevideo.active)
		return;
	memset(&cls.capturevideo, 0, sizeof(cls.capturevideo));
	// soundrate is figured out on the first SoundFrame

	if (width == 0 && height != 0)
		width = (int) (height * (double)vid.width / ((double)vid.height * vid_pixelheight.value)); // keep aspect
	if (width != 0 && height == 0)
		height = (int) (width * ((double)vid.height * vid_pixelheight.value) / (double)vid.width); // keep aspect

	if (width < 2 || width > vid.width) // can't scale up
		width = vid.width;
	if (height < 2 || height > vid.height) // can't scale up
		height = vid.height;

	// ensure it's all even; if not, scale down a little
	if (width % 1)
		--width;
	if (height % 1)
		--height;

	cls.capturevideo.width = width;
	cls.capturevideo.height = height;
	cls.capturevideo.active = true;
	cls.capturevideo.framerate = bound(1, cl_capturevideo_fps.value, 1001) * bound(1, cl_capturevideo_framestep.integer, 64);
	cls.capturevideo.framestep = cl_capturevideo_framestep.integer;
	cls.capturevideo.soundrate = S_GetSoundRate();
	cls.capturevideo.soundchannels = S_GetSoundChannels();
	cls.capturevideo.startrealtime = host.realtime;
	cls.capturevideo.frame = cls.capturevideo.lastfpsframe = 0;
	cls.capturevideo.starttime = cls.capturevideo.lastfpstime = host.realtime;
	cls.capturevideo.soundsampleframe = 0;
	cls.capturevideo.is_realtime = cl_capturevideo_realtime.integer != 0;
	cls.capturevideo.screenbuffer = (unsigned char *)Mem_Alloc(tempmempool, vid.width * vid.height * 4);
	cls.capturevideo.outbuffer = (unsigned char *)Mem_Alloc(tempmempool, width * height * (4+4) + 18);
	dpsnprintf(cls.capturevideo.basename, sizeof(cls.capturevideo.basename), "video/%s%03i", Sys_TimeString(cl_capturevideo_nameformat.string), cl_capturevideo_number.integer);
	Cvar_SetValueQuick(&cl_capturevideo_number, cl_capturevideo_number.integer + 1);

	/*
	for (i = 0;i < 256;i++)
	{
		unsigned char j = (unsigned char)bound(0, 255*pow(i/255.0, gamma), 255);
		cls.capturevideo.rgbgammatable[0][i] = j;
		cls.capturevideo.rgbgammatable[1][i] = j;
		cls.capturevideo.rgbgammatable[2][i] = j;
	}
	*/
/*
R = Y + 1.4075 * (Cr - 128);
G = Y + -0.3455 * (Cb - 128) + -0.7169 * (Cr - 128);
B = Y + 1.7790 * (Cb - 128);
Y = R *  .299 + G *  .587 + B *  .114;
Cb = R * -.169 + G * -.332 + B *  .500 + 128.;
Cr = R *  .500 + G * -.419 + B * -.0813 + 128.;
*/

	// identity gamma table
	BuildGammaTable16(1.0f, 1.0f, 1.0f, 0.0f, 1.0f, cls.capturevideo.vidramp, 256);
	BuildGammaTable16(1.0f, 1.0f, 1.0f, 0.0f, 1.0f, cls.capturevideo.vidramp + 256, 256);
	BuildGammaTable16(1.0f, 1.0f, 1.0f, 0.0f, 1.0f, cls.capturevideo.vidramp + 256*2, 256);
	if (scr_screenshot_gammaboost.value != 1)
	{
		double igamma = 1 / scr_screenshot_gammaboost.value;
		for (i = 0;i < 256 * 3;i++)
			cls.capturevideo.vidramp[i] = (unsigned short) (0.5 + pow(cls.capturevideo.vidramp[i] * (1.0 / 65535.0), igamma) * 65535.0);
	}

	for (i = 0;i < 256;i++)
	{
		r = 255*cls.capturevideo.vidramp[i]/65535.0;
		g = 255*cls.capturevideo.vidramp[i+256]/65535.0;
		b = 255*cls.capturevideo.vidramp[i+512]/65535.0;
		// NOTE: we have to round DOWN here, or integer overflows happen. Sorry for slightly wrong looking colors sometimes...
		// Y weights from RGB
		cls.capturevideo.rgbtoyuvscaletable[0][0][i] = (short)(r *  0.299);
		cls.capturevideo.rgbtoyuvscaletable[0][1][i] = (short)(g *  0.587);
		cls.capturevideo.rgbtoyuvscaletable[0][2][i] = (short)(b *  0.114);
		// Cb weights from RGB
		cls.capturevideo.rgbtoyuvscaletable[1][0][i] = (short)(r * -0.169);
		cls.capturevideo.rgbtoyuvscaletable[1][1][i] = (short)(g * -0.332);
		cls.capturevideo.rgbtoyuvscaletable[1][2][i] = (short)(b *  0.500);
		// Cr weights from RGB
		cls.capturevideo.rgbtoyuvscaletable[2][0][i] = (short)(r *  0.500);
		cls.capturevideo.rgbtoyuvscaletable[2][1][i] = (short)(g * -0.419);
		cls.capturevideo.rgbtoyuvscaletable[2][2][i] = (short)(b * -0.0813);
		// range reduction of YCbCr to valid signal range
		cls.capturevideo.yuvnormalizetable[0][i] = 16 + i * (236-16) / 256;
		cls.capturevideo.yuvnormalizetable[1][i] = 16 + i * (240-16) / 256;
		cls.capturevideo.yuvnormalizetable[2][i] = 16 + i * (240-16) / 256;
	}

	if (cl_capturevideo_ogg.integer)
	{
		if (SCR_CaptureVideo_Ogg_Available())
		{
			SCR_CaptureVideo_Ogg_BeginVideo();
			return;
		}
		else
			Con_Print("cl_capturevideo_ogg: libraries not available. Capturing in AVI instead.\n");
	}

	SCR_CaptureVideo_Avi_BeginVideo();
}

void SCR_CaptureVideo_EndVideo(void)
{
	if (!cls.capturevideo.active)
		return;
	cls.capturevideo.active = false;

	Con_Printf ("Finishing capture of %s.%s (%d frames, %d audio frames)\n", cls.capturevideo.basename, cls.capturevideo.formatextension, cls.capturevideo.frame, cls.capturevideo.soundsampleframe);

	if (cls.capturevideo.videofile)
	{
		cls.capturevideo.endvideo();
	}

	if (cls.capturevideo.screenbuffer)
	{
		Mem_Free (cls.capturevideo.screenbuffer);
		cls.capturevideo.screenbuffer = NULL;
	}

	if (cls.capturevideo.outbuffer)
	{
		Mem_Free (cls.capturevideo.outbuffer);
		cls.capturevideo.outbuffer = NULL;
	}

	memset(&cls.capturevideo, 0, sizeof(cls.capturevideo));
}

static void SCR_ScaleDownBGRA(unsigned char *in, int inw, int inh, unsigned char *out, int outw, int outh)
{
	// TODO optimize this function

	int x, y;
	float area;

	// memcpy is faster than me
	if (inw == outw && inh == outh)
	{
		memcpy(out, in, 4 * inw * inh);
		return;
	}

	// otherwise: a box filter
	area = (float)outw * (float)outh / (float)inw / (float)inh;
	for(y = 0; y < outh; ++y)
	{
		float iny0 =  y    / (float)outh * inh; int iny0_i = (int) floor(iny0);
		float iny1 = (y+1) / (float)outh * inh; int iny1_i = (int) ceil(iny1);
		for(x = 0; x < outw; ++x)
		{
			float inx0 =  x    / (float)outw * inw; int inx0_i = (int) floor(inx0);
			float inx1 = (x+1) / (float)outw * inw; int inx1_i = (int) ceil(inx1);
			float r = 0, g = 0, b = 0, alpha = 0;
			int xx, yy;

			for(yy = iny0_i; yy < iny1_i; ++yy)
			{
				float ya = min(yy+1, iny1) - max(iny0, yy);
				for(xx = inx0_i; xx < inx1_i; ++xx)
				{
					float a = ya * (min(xx+1, inx1) - max(inx0, xx));
					r += a * in[4*(xx + inw * yy)+0];
					g += a * in[4*(xx + inw * yy)+1];
					b += a * in[4*(xx + inw * yy)+2];
					alpha += a * in[4*(xx + inw * yy)+3];
				}
			}

			out[4*(x + outw * y)+0] = (unsigned char) (r * area);
			out[4*(x + outw * y)+1] = (unsigned char) (g * area);
			out[4*(x + outw * y)+2] = (unsigned char) (b * area);
			out[4*(x + outw * y)+3] = (unsigned char) (alpha * area);
		}
	}
}

static void SCR_CaptureVideo_VideoFrame(int newframestepframenum)
{
	int x = 0, y = 0;
	int width = cls.capturevideo.width, height = cls.capturevideo.height;

	if (newframestepframenum == cls.capturevideo.framestepframe)
		return;

	CHECKGLERROR
	// speed is critical here, so do saving as directly as possible

	GL_ReadPixelsBGRA(x, y, vid.width, vid.height, cls.capturevideo.screenbuffer);

	SCR_ScaleDownBGRA (cls.capturevideo.screenbuffer, vid.width, vid.height, cls.capturevideo.outbuffer, width, height);

	cls.capturevideo.videoframes(newframestepframenum - cls.capturevideo.framestepframe);
	cls.capturevideo.framestepframe = newframestepframenum;

	if (cl_capturevideo_printfps.integer)
	{
		char buf[80];
		double t = host.realtime;
		if (t > cls.capturevideo.lastfpstime + 1)
		{
			double fps1 = (cls.capturevideo.frame - cls.capturevideo.lastfpsframe) / (t - cls.capturevideo.lastfpstime + 0.0000001);
			double fps  = (cls.capturevideo.frame                                ) / (t - cls.capturevideo.starttime   + 0.0000001);
			dpsnprintf(buf, sizeof(buf), "capturevideo: (%.1fs) last second %.3ffps, total %.3ffps\n", cls.capturevideo.frame / cls.capturevideo.framerate, fps1, fps);
			Sys_PrintToTerminal(buf);
			cls.capturevideo.lastfpstime = t;
			cls.capturevideo.lastfpsframe = cls.capturevideo.frame;
		}
	}
}

void SCR_CaptureVideo_SoundFrame(const portable_sampleframe_t *paintbuffer, size_t length)
{
	cls.capturevideo.soundsampleframe += (int)length;
	cls.capturevideo.soundframe(paintbuffer, length);
}

static void SCR_CaptureVideo(void)
{
	int newframenum;
	if (cl_capturevideo.integer)
	{
		if (!cls.capturevideo.active)
			SCR_CaptureVideo_BeginVideo();
		if (cls.capturevideo.framerate != cl_capturevideo_fps.value * cl_capturevideo_framestep.integer)
		{
			Con_Printf ("You can not change the video framerate while recording a video.\n");
			Cvar_SetValueQuick(&cl_capturevideo_fps, cls.capturevideo.framerate / (double) cl_capturevideo_framestep.integer);
		}
		// for AVI saving we have to make sure that sound is saved before video
		if (cls.capturevideo.soundrate && !cls.capturevideo.soundsampleframe)
			return;
		if (cls.capturevideo.is_realtime)
		{
			// preserve sound sync by duplicating frames when running slow
			newframenum = (int)((host.realtime - cls.capturevideo.startrealtime) * cls.capturevideo.framerate);
		}
		else
			newframenum = cls.capturevideo.frame + 1;
		// if falling behind more than one second, stop
		if (newframenum - cls.capturevideo.frame > 60 * (int)ceil(cls.capturevideo.framerate))
		{
			Cvar_SetValueQuick(&cl_capturevideo, 0);
			Con_Printf ("video saving failed on frame %d, your machine is too slow for this capture speed.\n", cls.capturevideo.frame);
			SCR_CaptureVideo_EndVideo();
			return;
		}
		// write frames
		SCR_CaptureVideo_VideoFrame(newframenum / cls.capturevideo.framestep);
		cls.capturevideo.frame = newframenum;
		if (cls.capturevideo.error)
		{
			Cvar_SetValueQuick(&cl_capturevideo, 0);
			Con_Printf ("video saving failed on frame %d, out of disk space? stopping video capture.\n", cls.capturevideo.frame);
			SCR_CaptureVideo_EndVideo();
		}
	}
	else if (cls.capturevideo.active)
		SCR_CaptureVideo_EndVideo();
}
#endif

/*
===============
R_Envmap_f

Grab six views for environment mapping tests
===============
*/
struct envmapinfo_s
{
	float angles[3];
	const char *name;
	qbool flipx, flipy, flipdiagonaly;
}
envmapinfo[12] =
{
	{{  0,   0, 0}, "rt", false, false, false},
	{{  0, 270, 0}, "ft", false, false, false},
	{{  0, 180, 0}, "lf", false, false, false},
	{{  0,  90, 0}, "bk", false, false, false},
	{{-90, 180, 0}, "up",  true,  true, false},
	{{ 90, 180, 0}, "dn",  true,  true, false},

	{{  0,   0, 0}, "px",  true,  true,  true},
	{{  0,  90, 0}, "py", false,  true, false},
	{{  0, 180, 0}, "nx", false, false,  true},
	{{  0, 270, 0}, "ny",  true, false, false},
	{{-90, 180, 0}, "pz", false, false,  true},
	{{ 90, 180, 0}, "nz", false, false,  true}
};

static void R_Envmap_f (cmd_state_t *cmd)
{
	int j, size;
	char filename[MAX_QPATH_128], basename[MAX_QPATH_128];
	unsigned char *buffer1;
	unsigned char *buffer2;
	r_rendertarget_t *rt;

	if (Cmd_Argc(cmd) != 3)
	{
		Con_Print("envmap <basename> <size>: save out 6 cubic environment map images, usable with loadsky, note that size must one of 128, 256, 512, or 1024 and can't be bigger than your current resolution\n");
		return;
	}

	if (cls.state != ca_connected) {
		Con_PrintLinef ("envmap: No map loaded");
		return;
	}

	c_strlcpy (basename, Cmd_Argv(cmd, 1));
	size = atoi(Cmd_Argv(cmd, 2));

	if (size != 128 && size != 256 && size != 512 && size != 1024)
	{
		Con_Print("envmap: size must be one of 128, 256, 512, or 1024\n");
		return;
	}
	if (size > vid.width || size > vid.height)
	{
		Con_Print("envmap: your resolution is not big enough to render that size\n");
		return;
	}

	r_refdef.envmap = true;

	R_UpdateVariables();

	r_refdef.view.x = 0;
	r_refdef.view.y = 0;
	r_refdef.view.z = 0;
	r_refdef.view.width = size;
	r_refdef.view.height = size;
	r_refdef.view.depth = 1;
	r_refdef.view.useperspective = true;
	r_refdef.view.isoverlay = false;
	r_refdef.view.ismain = true;

	r_refdef.view.frustum_x = 1; // tan(45 * M_PI / 180.0);
	r_refdef.view.frustum_y = 1; // tan(45 * M_PI / 180.0);
	r_refdef.view.ortho_x = 90; // abused as angle by VM_CL_R_SetView
	r_refdef.view.ortho_y = 90; // abused as angle by VM_CL_R_SetView

	buffer1 = (unsigned char *)Mem_Alloc(tempmempool, size * size * 4);
	buffer2 = (unsigned char *)Mem_Alloc(tempmempool, size * size * 3);

	// TODO: use TEXTYPE_COLORBUFFER16F and output to .exr files as well?
	rt = R_RenderTarget_Get(size, size, TEXTYPE_DEPTHBUFFER24STENCIL8, true, TEXTYPE_COLORBUFFER, TEXTYPE_UNUSED, TEXTYPE_UNUSED, TEXTYPE_UNUSED);
	CL_UpdateEntityShading();
	for (j = 0;j < 12;j++)
	{
		dpsnprintf(filename, sizeof(filename), "env/%s%s.tga", basename, envmapinfo[j].name);
		Matrix4x4_CreateFromQuakeEntity(&r_refdef.view.matrix, r_refdef.view.origin[0], r_refdef.view.origin[1], r_refdef.view.origin[2], envmapinfo[j].angles[0], envmapinfo[j].angles[1], envmapinfo[j].angles[2], 1);
		r_refdef.view.quality = 1;
		r_refdef.view.clear = true;
		R_Mesh_Start();
		R_RenderView(rt->fbo, rt->depthtexture, rt->colortexture[0], 0, 0, size, size);
		R_Mesh_Finish();
		SCR_ScreenShot(filename, buffer1, buffer2, 0, vid.height - (r_refdef.view.y + r_refdef.view.height), size, size, envmapinfo[j].flipx, envmapinfo[j].flipy, envmapinfo[j].flipdiagonaly, false, false, false, false);
	}

	Mem_Free (buffer1);
	Mem_Free (buffer2);

	r_refdef.envmap = false;
}

//=============================================================================

void SHOWLMP_decodehide(void)
{
	int i;
	char *lmplabel;
	lmplabel = MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring));
	for (i = 0;i < cl.num_showlmps;i++)
		if (cl.showlmps[i].isactive && strcmp(cl.showlmps[i].label, lmplabel) == 0)
		{
			cl.showlmps[i].isactive = false;
			return;
		}
}

void SHOWLMP_decodeshow(void)
{
	int k;
	char lmplabel[256], picname[256];
	float x, y;
	strlcpy (lmplabel,MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring)), sizeof (lmplabel));
	strlcpy (picname, MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring)), sizeof (picname));
	if (gamemode == GAME_NEHAHRA) // LadyHavoc: nasty old legacy junk
	{
		x = MSG_ReadByte(&cl_message);
		y = MSG_ReadByte(&cl_message);
	}
	else
	{
		x = MSG_ReadShort(&cl_message);
		y = MSG_ReadShort(&cl_message);
	}
	if (!cl.showlmps || cl.num_showlmps >= cl.max_showlmps)
	{
		showlmp_t *oldshowlmps = cl.showlmps;
		cl.max_showlmps += 16;
		cl.showlmps = (showlmp_t *) Mem_Alloc(cls.levelmempool, cl.max_showlmps * sizeof(showlmp_t));
		if (oldshowlmps)
		{
			if (cl.num_showlmps)
				memcpy(cl.showlmps, oldshowlmps, cl.num_showlmps * sizeof(showlmp_t));
			Mem_Free(oldshowlmps);
		}
	}
	for (k = 0;k < cl.max_showlmps;k++)
		if (cl.showlmps[k].isactive && String_Does_Match(cl.showlmps[k].label, lmplabel))
			break;
	if (k == cl.max_showlmps)
		for (k = 0;k < cl.max_showlmps;k++)
			if (!cl.showlmps[k].isactive)
				break;
	cl.showlmps[k].isactive = true;
	strlcpy (cl.showlmps[k].label, lmplabel, sizeof (cl.showlmps[k].label));
	strlcpy (cl.showlmps[k].pic, picname, sizeof (cl.showlmps[k].pic));
	cl.showlmps[k].x = x;
	cl.showlmps[k].y = y;
	cl.num_showlmps = max(cl.num_showlmps, k + 1);
}

void SHOWLMP_drawall(void)
{
	int i;
	for (i = 0;i < cl.num_showlmps;i++)
		if (cl.showlmps[i].isactive)
			DrawQ_Pic(cl.showlmps[i].x, cl.showlmps[i].y, Draw_CachePic_Flags (cl.showlmps[i].pic, CACHEPICFLAG_NOTPERSISTENT), 0, 0, 1, 1, 1, 1, 0);
}

/*
==============================================================================

						SCREEN SHOTS

==============================================================================
*/

// buffer1: 4*w*h
// buffer2: 3*w*h (or 4*w*h if screenshotting alpha too)
qbool SCR_ScreenShot(char *filename, unsigned char *buffer1, unsigned char *buffer2, int x, int y, int width, int height, qbool flipx, qbool flipy, qbool flipdiagonal, qbool jpeg, qbool png, qbool gammacorrect, qbool keep_alpha)
{
	int	indices[4] = {0,1,2,3}; // BGRA
	qbool ret;

	GL_ReadPixelsBGRA(x, y, width, height, buffer1);

	if (gammacorrect && (scr_screenshot_gammaboost.value != 1))
	{
		int i;
		double igamma = 1.0 / scr_screenshot_gammaboost.value;
		unsigned short vidramp[256 * 3];
		// identity gamma table
		BuildGammaTable16(1.0f, 1.0f, 1.0f, 0.0f, 1.0f, vidramp, 256);
		BuildGammaTable16(1.0f, 1.0f, 1.0f, 0.0f, 1.0f, vidramp + 256, 256);
		BuildGammaTable16(1.0f, 1.0f, 1.0f, 0.0f, 1.0f, vidramp + 256*2, 256);
		if (scr_screenshot_gammaboost.value != 1)
		{
			for (i = 0;i < 256 * 3;i++)
				vidramp[i] = (unsigned short) (0.5 + pow(vidramp[i] * (1.0 / 65535.0), igamma) * 65535.0);
		}
		for (i = 0;i < width*height*4;i += 4)
		{
			buffer1[i] = (unsigned char) (vidramp[buffer1[i] + 512] * 255.0 / 65535.0 + 0.5); // B
			buffer1[i+1] = (unsigned char) (vidramp[buffer1[i+1] + 256] * 255.0 / 65535.0 + 0.5); // G
			buffer1[i+2] = (unsigned char) (vidramp[buffer1[i+2]] * 255.0 / 65535.0 + 0.5); // R
			// A
		}
	}

	if (keep_alpha && !jpeg)
	{
		if (!png)
			flipy = !flipy; // TGA: not preflipped
		Image_CopyMux (buffer2, buffer1, width, height, flipx, flipy, flipdiagonal, 4, 4, indices);
		if (png)
			ret = PNG_SaveImage_preflipped (filename, width, height, true, buffer2);
		else
			ret = Image_WriteTGABGRA(filename, width, height, buffer2);
	}
	else
	{
		if (jpeg)
		{
			indices[0] = 2;
			indices[2] = 0; // RGB
		}
		Image_CopyMux (buffer2, buffer1, width, height, flipx, flipy, flipdiagonal, 3, 4, indices);
		if (jpeg)
			ret = JPEG_SaveImage_preflipped (filename, width, height, buffer2);
		else if (png)
			ret = PNG_SaveImage_preflipped (filename, width, height, false, buffer2);
		else
			ret = Image_WriteTGABGR_preflipped (filename, width, height, buffer2);
	}

	return ret;
}

//=============================================================================

int scr_numtouchscreenareas;
scr_touchscreenarea_t scr_touchscreenareas[128];

static void SCR_DrawTouchscreenOverlay(void)
{
	int i;
	scr_touchscreenarea_t *a;
	cachepic_t *pic;
	for (i = 0, a = scr_touchscreenareas;i < scr_numtouchscreenareas;i++, a++)
	{
		if (vid_touchscreen_outlinealpha.value > 0 && a->rect[0] >= 0 && a->rect[1] >= 0 && a->rect[2] >= 4 && a->rect[3] >= 4)
		{
			DrawQ_Fill(a->rect[0] +              2, a->rect[1]                 , a->rect[2] - 4,          1    , 1, 1, 1, vid_touchscreen_outlinealpha.value * (0.5f + 0.5f * a->active), 0);
			DrawQ_Fill(a->rect[0] +              1, a->rect[1] +              1, a->rect[2] - 2,          1    , 1, 1, 1, vid_touchscreen_outlinealpha.value * (0.5f + 0.5f * a->active), 0);
			DrawQ_Fill(a->rect[0]                 , a->rect[1] +              2,          2    , a->rect[3] - 2, 1, 1, 1, vid_touchscreen_outlinealpha.value * (0.5f + 0.5f * a->active), 0);
			DrawQ_Fill(a->rect[0] + a->rect[2] - 2, a->rect[1] +              2,          2    , a->rect[3] - 2, 1, 1, 1, vid_touchscreen_outlinealpha.value * (0.5f + 0.5f * a->active), 0);
			DrawQ_Fill(a->rect[0] +              1, a->rect[1] + a->rect[3] - 2, a->rect[2] - 2,          1    , 1, 1, 1, vid_touchscreen_outlinealpha.value * (0.5f + 0.5f * a->active), 0);
			DrawQ_Fill(a->rect[0] +              2, a->rect[1] + a->rect[3] - 1, a->rect[2] - 4,          1    , 1, 1, 1, vid_touchscreen_outlinealpha.value * (0.5f + 0.5f * a->active), 0);
		}
		pic = a->pic ? Draw_CachePic_Flags(a->pic, CACHEPICFLAG_FAILONMISSING_256) : NULL;
		if (Draw_IsPicLoaded(pic))
			DrawQ_Pic(a->rect[0], a->rect[1], pic, a->rect[2], a->rect[3], 1, 1, 1, vid_touchscreen_overlayalpha.value * (0.5f + 0.5f * a->active), 0);
		if (a->text && a->text[0])
		{
			int textwidth = DrawQ_TextWidth(a->text, 0, a->textheight, a->textheight, false, FONT_CHAT);
			DrawQ_String(a->rect[0] + (a->rect[2] - textwidth) * 0.5f, a->rect[1] + (a->rect[3] - a->textheight) * 0.5f, a->text, 0, a->textheight, a->textheight, 1.0f, 1.0f, 1.0f, vid_touchscreen_overlayalpha.value, 0, NULL, false, FONT_CHAT);
		}
	}
}

void R_ClearScreen(qbool fogcolor)
{
	float clearcolor[4];
	if (scr_screenshot_alpha.integer)
		// clear to transparency (so png screenshots can contain alpha channel, useful for building model pictures)
		Vector4Set(clearcolor, 0.0f, 0.0f, 0.0f, 0.0f);
	else
		// clear to opaque black (if we're being composited it might otherwise render as transparent)
		Vector4Set(clearcolor, 0.0f, 0.0f, 0.0f, 1.0f);
	if (fogcolor && r_fog_clear.integer)
	{
		R_UpdateFog();
		VectorCopy(r_refdef.fogcolor, clearcolor);
	}
	// clear depth is 1.0
	// clear the screen
	GL_Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | (vid.stencil ? GL_STENCIL_BUFFER_BIT : 0), clearcolor, 1.0f, 0);
}

int r_stereo_side;
extern cvar_t v_isometric;
extern cvar_t v_isometric_verticalfov;

typedef struct loadingscreenstack_s
{
	struct loadingscreenstack_s *prev;
	char msg[MAX_QPATH_128];
	float absolute_loading_amount_min; // this corresponds to relative completion 0 of this item
	float absolute_loading_amount_len; // this corresponds to relative completion 1 of this item
	float relative_completion; // 0 .. 1
}
loadingscreenstack_t;
static loadingscreenstack_t *loadingscreenstack = NULL;
rtexture_t *loadingscreentexture = NULL; // last framebuffer before loading screen, kept for the background
static float loadingscreentexture_vertex3f[12];
static float loadingscreentexture_texcoord2f[8];
static int loadingscreenpic_number = 0;

static void SCR_DrawLoadingScreen(void);


static void SCR_ClearLoadingScreenTexture(void)
{
	if (loadingscreentexture)
		R_FreeTexture(loadingscreentexture);
	loadingscreentexture = NULL;
}

extern rtexturepool_t *r_main_texturepool;
static void SCR_SetLoadingScreenTexture(void)
{
	int w, h;
	float loadingscreentexture_w;
	float loadingscreentexture_h;

	SCR_ClearLoadingScreenTexture();

	w = vid.width; h = vid.height;
	loadingscreentexture_w = loadingscreentexture_h = 1;

	loadingscreentexture = R_LoadTexture2D(r_main_texturepool, "loadingscreentexture", w, h, NULL, TEXTYPE_COLORBUFFER, TEXF_RENDERTARGET | TEXF_FORCENEAREST | TEXF_CLAMP, -1, NULL);
	R_Mesh_CopyToTexture(loadingscreentexture, 0, 0, 0, 0, vid.width, vid.height);

	loadingscreentexture_vertex3f[2] = loadingscreentexture_vertex3f[5] = loadingscreentexture_vertex3f[8] = loadingscreentexture_vertex3f[11] = 0;
	loadingscreentexture_vertex3f[0] = loadingscreentexture_vertex3f[9] = 0;
	loadingscreentexture_vertex3f[1] = loadingscreentexture_vertex3f[4] = 0;
	loadingscreentexture_vertex3f[3] = loadingscreentexture_vertex3f[6] = vid_conwidth.integer;
	loadingscreentexture_vertex3f[7] = loadingscreentexture_vertex3f[10] = vid_conheight.integer;
	loadingscreentexture_texcoord2f[0] = 0;loadingscreentexture_texcoord2f[1] = loadingscreentexture_h;
	loadingscreentexture_texcoord2f[2] = loadingscreentexture_w;loadingscreentexture_texcoord2f[3] = loadingscreentexture_h;
	loadingscreentexture_texcoord2f[4] = loadingscreentexture_w;loadingscreentexture_texcoord2f[5] = 0;
	loadingscreentexture_texcoord2f[6] = 0;loadingscreentexture_texcoord2f[7] = 0;
}

static void SCR_ChooseLoadingPic(qbool startup)
{
	if (startup && scr_loadingscreen_firstforstartup.integer)
		loadingscreenpic_number = 0;
	else if (scr_loadingscreen_firstforstartup.integer)
		if (scr_loadingscreen_count.integer > 1)
			loadingscreenpic_number = rand() % (scr_loadingscreen_count.integer - 1) + 1;
		else
			loadingscreenpic_number = 0;
	else
		loadingscreenpic_number = rand() % (scr_loadingscreen_count.integer > 1 ? scr_loadingscreen_count.integer : 1);
}

/*
===============
SCR_BeginLoadingPlaque

================
*/
void SCR_BeginLoadingPlaque(qbool startup)
{
	loadingscreenstack_t dummy_status;
	scr_skip_once = true;

	// we need to push a dummy status so CL_UpdateScreen knows we have things to load...
	if (!loadingscreenstack)
	{
		dummy_status.msg[0] = '\0';
		dummy_status.absolute_loading_amount_min = 0;
		loadingscreenstack = &dummy_status;
	}

	SCR_DeferLoadingPlaque(startup);
	if (scr_loadingscreen_background.integer)
		SCR_SetLoadingScreenTexture();
	CL_UpdateScreen();

	if (loadingscreenstack == &dummy_status)
		loadingscreenstack = NULL;
}

void SCR_DeferLoadingPlaque(qbool startup)
{
	SCR_ChooseLoadingPic(startup);
	scr_loading = true;
}

void SCR_EndLoadingPlaque(void)
{
	scr_loading = false;
	SCR_ClearLoadingScreenTexture();
}


void SCR_PushLoadingScreen (const char *msg, float len_in_parent)
{
	loadingscreenstack_t *s = (loadingscreenstack_t *) Z_Malloc(sizeof(loadingscreenstack_t));
	s->prev = loadingscreenstack;
	loadingscreenstack = s;

	scr_skip_once = true;

	c_strlcpy(s->msg, msg);
	s->relative_completion = 0;

	if (s->prev)
	{
		s->absolute_loading_amount_min = s->prev->absolute_loading_amount_min + s->prev->absolute_loading_amount_len * s->prev->relative_completion;
		s->absolute_loading_amount_len = s->prev->absolute_loading_amount_len * len_in_parent;
		if (s->absolute_loading_amount_len > s->prev->absolute_loading_amount_min + s->prev->absolute_loading_amount_len - s->absolute_loading_amount_min)
			s->absolute_loading_amount_len = s->prev->absolute_loading_amount_min + s->prev->absolute_loading_amount_len - s->absolute_loading_amount_min;
	}
	else
	{
		s->absolute_loading_amount_min = 0;
		s->absolute_loading_amount_len = 1;
	}

	if (scr_loading)
		CL_UpdateScreen();
}

void SCR_PopLoadingScreen (qbool redraw)
{
	loadingscreenstack_t *s = loadingscreenstack;

	if (!s)
	{
		Con_DPrintLinef ("Popping a loading screen item from an empty stack!");
		return;
	}

	loadingscreenstack = s->prev;
	if (s->prev)
		s->prev->relative_completion = (s->absolute_loading_amount_min + s->absolute_loading_amount_len - s->prev->absolute_loading_amount_min) / s->prev->absolute_loading_amount_len;
	else {
		// Baker: zg something is ruining relay of completion %, null.spr ?  a sound?  who knows.  Engine's fault somehow.
	}
	Z_Free(s);

	if (scr_loading && redraw)
		CL_UpdateScreen();
}

void SCR_ClearLoadingScreen (qbool redraw)
{
	while(loadingscreenstack)
		SCR_PopLoadingScreen(redraw && !loadingscreenstack->prev);
}

static float SCR_DrawLoadingStack_r(loadingscreenstack_t *s, float y, float size)
{
	float x;
	size_t len;
	float total;

	total = 0;
#if 0
	if (s)
	{
		total += SCR_DrawLoadingStack_r(s->prev, y, 8);
		y -= total;
		if (!s->prev || strcmp(s->msg, s->prev->msg))
		{
			len = strlen(s->msg);
			x = (vid_conwidth.integer - DrawQ_TextWidth(s->msg, len, size, size, true, FONT_INFOBAR)) / 2;
			y -= size;
			DrawQ_String(x, y, s->msg, len, size, size, 1, 1, 1, 1, 0, NULL, true, FONT_INFOBAR);
			total += size;
		}
	}
#else
	if (s)
	{
		len = strlen(s->msg);
		x = (vid_conwidth.integer - DrawQ_TextWidth(s->msg, len, size, size, true, FONT_INFOBAR)) / 2;
		y -= size;
		DrawQ_String(x, y, s->msg, len, size, size, 1, 1, 1, 1, 0, NULL, true, FONT_INFOBAR);
		total += size;
	}
#endif
	return total;
}

static void SCR_DrawLoadingStack(void)
{
	float verts[12];
	float colors[16];

	int effective_loading = scr_loadingscreen_barheight.integer;
	if (!effective_loading && 
		scr_loading && 
		cl_signon_start_time &&
		((Sys_DirtyTime() - cl_signon_start_time) > 1.5) )
		effective_loading = 8;


	SCR_DrawLoadingStack_r(loadingscreenstack, vid_conheight.integer, effective_loading);
	if (loadingscreenstack)
	{
		// height = 32; // sorry, using the actual one is ugly
		GL_BlendFunc(GL_SRC_ALPHA, GL_ONE);
		GL_DepthRange(0, 1);
		GL_PolygonOffset(0, 0);
		GL_DepthTest(false);
		//R_Mesh_ResetTextureState();
		verts[2] = verts[5] = verts[8] = verts[11] = 0;
		verts[0] = verts[9] = 0;
		verts[1] = verts[4] = vid_conheight.integer - effective_loading;
		verts[3] = verts[6] = vid_conwidth.integer * loadingscreenstack->absolute_loading_amount_min;
		verts[7] = verts[10] = vid_conheight.integer;
		
#if _MSC_VER >= 1400
#define sscanf sscanf_s
#endif
		colors[0] = 0; colors[1] = 0; colors[2] = 0; colors[3] = 1;
		colors[4] = 0; colors[5] = 0; colors[6] = 0; colors[7] = 1;
		sscanf(scr_loadingscreen_barcolor.string, "%f %f %f", &colors[8], &colors[9], &colors[10]); colors[11] = 1;
		sscanf(scr_loadingscreen_barcolor.string, "%f %f %f", &colors[12], &colors[13], &colors[14]);  colors[15] = 1;

		R_Mesh_PrepareVertices_Generic_Arrays(4, verts, colors, NULL);
		R_SetupShader_Generic_NoTexture(true, true);
		R_Mesh_Draw(0, 4, 0, 2, polygonelement3i, NULL, 0, polygonelement3s, NULL, 0);
	}
}

qbool R_Stereo_ColorMasking(void)
{
	return r_stereo_redblue.integer || r_stereo_redgreen.integer || r_stereo_redcyan.integer;
}

qbool R_Stereo_Active(void)
{
	return (vid.stereobuffer || r_stereo_sidebyside.integer || r_stereo_horizontal.integer || r_stereo_vertical.integer || R_Stereo_ColorMasking());
}

static void SCR_DrawLoadingScreen(void)
{
	cachepic_t* loadingscreenpic;
	float loadingscreenpic_vertex3f[12];
	float loadingscreenpic_texcoord2f[8];
	float x, y, w, h, sw, sh, f;
	char vabuf[1024];

	GL_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	GL_DepthRange(0, 1);
	GL_PolygonOffset(0, 0);
	GL_DepthTest(false);
	GL_Color(1, 1, 1, 1);

	if (loadingscreentexture)
	{
		R_Mesh_PrepareVertices_Generic_Arrays(4, loadingscreentexture_vertex3f, NULL, loadingscreentexture_texcoord2f);
		R_SetupShader_Generic(loadingscreentexture, false, true, true);
		R_Mesh_Draw(0, 4, 0, 2, polygonelement3i, NULL, 0, polygonelement3s, NULL, 0);
	}

	loadingscreenpic = Draw_CachePic_Flags(loadingscreenpic_number ? va(vabuf, sizeof(vabuf), "%s%d", scr_loadingscreen_picture.string, loadingscreenpic_number + 1) : scr_loadingscreen_picture.string, loadingscreenpic_number ? CACHEPICFLAG_NOTPERSISTENT : 0);
	w = Draw_GetPicWidth(loadingscreenpic);
	h = Draw_GetPicHeight(loadingscreenpic);

	// apply scale
	w *= scr_loadingscreen_scale.value;
	h *= scr_loadingscreen_scale.value;

	// apply scale base
	if (scr_loadingscreen_scale_base.integer)
	{
		w *= vid_conwidth.integer / (float)vid.width;
		h *= vid_conheight.integer / (float)vid.height;
	}

	// apply scale limit
	sw = w / vid_conwidth.integer;
	sh = h / vid_conheight.integer;
	f = 1;
	switch (scr_loadingscreen_scale_limit.integer)
	{
	case 1:
		f = max(sw, sh);
		break;
	case 2:
		f = min(sw, sh);
		break;
	case 3:
		f = sw;
		break;
	case 4:
		f = sh;
		break;
	}
	if (f > 1)
	{
		w /= f;
		h /= f;
	}

	x = (vid_conwidth.integer - w) / 2;
	y = (vid_conheight.integer - h) / 2;
	loadingscreenpic_vertex3f[2] = loadingscreenpic_vertex3f[5] = loadingscreenpic_vertex3f[8] = loadingscreenpic_vertex3f[11] = 0;
	loadingscreenpic_vertex3f[0] = loadingscreenpic_vertex3f[9] = x;
	loadingscreenpic_vertex3f[1] = loadingscreenpic_vertex3f[4] = y;
	loadingscreenpic_vertex3f[3] = loadingscreenpic_vertex3f[6] = x + w;
	loadingscreenpic_vertex3f[7] = loadingscreenpic_vertex3f[10] = y + h;
	loadingscreenpic_texcoord2f[0] = 0; loadingscreenpic_texcoord2f[1] = 0;
	loadingscreenpic_texcoord2f[2] = 1; loadingscreenpic_texcoord2f[3] = 0;
	loadingscreenpic_texcoord2f[4] = 1; loadingscreenpic_texcoord2f[5] = 1;
	loadingscreenpic_texcoord2f[6] = 0; loadingscreenpic_texcoord2f[7] = 1;

	R_Mesh_PrepareVertices_Generic_Arrays(4, loadingscreenpic_vertex3f, NULL, loadingscreenpic_texcoord2f);
	R_SetupShader_Generic(Draw_GetPicTexture(loadingscreenpic), true, true, false);
	R_Mesh_Draw(0, 4, 0, 2, polygonelement3i, NULL, 0, polygonelement3s, NULL, 0);

	SCR_DrawLoadingStack();
}

// Baker r0005: Autoscale 360p
float   yfactors;
float    yfactor_mag_360;                    // output
float    scale_width_360;
float    scale_height_360;

int        old_vid_height;
int        old_vid_width;
qbool    old_vid_fullscreen;
int old_vid_kickme;

// Allow adjustment over automatic math
float    old_vid_fullscreen_conscale;
float    old_vid_window_conscale;

void scale_360_calc (void)
{
	if (old_vid_kickme) goto doitanyway;
    if (old_vid_height == vid.height &&
		old_vid_width == vid.width &&
        old_vid_fullscreen == vid.fullscreen &&
        old_vid_fullscreen_conscale == vid_fullscreen_conscale.value &&
        old_vid_window_conscale == vid_window_conscale.value)
        return;

doitanyway:
	if (old_vid_kickme) old_vid_kickme--;    

    old_vid_width				  = vid.width;
	old_vid_height                = vid.height;
    old_vid_fullscreen            = vid.fullscreen;
    old_vid_fullscreen_conscale    = vid_fullscreen_conscale.value;
    old_vid_window_conscale        = vid_window_conscale.value;

    cvar_t *pcvar;
    pcvar  = vid.fullscreen ? &vid_fullscreen_conscale : &vid_window_conscale;

    yfactors = vid.height / 360.0f;
    yfactors = Q_rint (yfactors); // 720 + is magnification of 2 or more
    if (yfactors < 0)
        yfactors = 1;

    /*Con_PrintLinef ("conscale %d %d fs? %d conscale %f",
                    vid.width,
                    vid.height,
                    vid.fullscreen,
                    pcvar->value);
    Con_PrintLinef ("yfactors %f",
                    yfactors);
    */
    float multo = bound (0.5, pcvar->value, 2);

    if (yfactors)
        yfactor_mag_360 = (1 / yfactors) * multo;
    else yfactor_mag_360 = 1;

	Con_DPrintLinef ("yfactor_mag_360 %f", yfactor_mag_360);

	Cvar_SetValueQuick(&vid_conheight, 399.5); // Evile!

    scale_width_360 =  vid.width * yfactor_mag_360;
    scale_height_360 = vid.height * yfactor_mag_360;
}

// Baker
static void CL_UpdateScreen_SCR_UpdateVars(void)
{
draw9:

	scale_360_calc ();

	Cvar_SetValueQuick (&vid_conwidth,  scale_width_360);
	Cvar_SetValueQuick (&vid_conheight, scale_height_360);


cldraw2d3:
	// Set the canvas here

	if (CLVM_prog->loaded && csqc_full_width_height.integer) {
		// CSQC loaded and wants full screen

		// Regular Quake never hits here.  no csqc
		Cvar_SetValueQuick (&vid_conwidth,  vid.width);
		Cvar_SetValueQuick (&vid_conheight, vid.height);
	} else if (1 /*vid_conscale_auto.integer*/) {
		// So CSQC can receive scaleauto without skip a frame			
		Cvar_SetValueQuick (&vid_conwidth,  scale_width_360);
		Cvar_SetValueQuick (&vid_conheight, scale_height_360);
	}
	
	// bound viewsize
	if (scr_viewsize.value < 30)
		Cvar_SetValueQuick(&scr_viewsize, 30);
	if (scr_viewsize.value > 120)
		Cvar_SetValueQuick(&scr_viewsize, 120);

	// bound field of view
	if (scr_fov.value < 1)
		Cvar_SetValueQuick(&scr_fov, 1);
	if (scr_fov.value > 170)
		Cvar_SetValueQuick(&scr_fov, 170);

	// intermission is always full screen
	if (cl.intermission)
		sb_lines = 0;
	else
	{
		if (scr_viewsize.value >= 120)
			sb_lines = 0;		// no status bar at all
		else if (scr_viewsize.value >= 110)
			sb_lines = 24;		// no inventory
		else
			sb_lines = 24 + 16 + 8;
	}
}

#if 1

entity_t *find_e_render (entity_render_t *exy, int *enumy)
{
	int i;

	for (i = 1; i < cl.num_entities; i ++) {
		if (cl.entities_active[i]) {
			entity_t *e = cl.entities + i;
			entity_render_t *exy2 = &e->render;
			if (exy2 == exy) {
				(*enumy) = i;
				return e;
			} // if
		} // if
	} // i
	return NULL;
}

void CL_Tool_Inspector (void)
{
	SV_LockThreadMutex (); // 

	int conwidth_2d_phase = scale_width_360;
	int conheight_2d_phase = scale_height_360;

	WARP_X_ (PRVM_ED_Write)
				
	prvm_prog_t *prog = SVVM_prog;
//	vec_t vieworigin;
	
//	Matrix4x4_OriginFromMatrix(&r_refdef.view.matrix, vieworigin);

	stringlist_t	matchedSet;
	stringlistinit  (&matchedSet); // this does not allocate

	for (int ent_num = 0; ent_num < r_refdef.scene.numentities; ent_num++) {
		if (!r_refdef.viewcache.entityvisible[ent_num])
			continue;
		entity_render_t *e_this = r_refdef.scene.entities[ent_num];
		
		if (e_this->model && e_this->model->Draw != NULL) {
			int edict_num = 0;
			entity_t *e = find_e_render(e_this, &edict_num);
			
			if (e) {
				prvm_edict_t *ed = PRVM_EDICT_NUM(edict_num);
				if (ed->free == false) {
					//const char *s_cls = PRVM_GetString(prog, PRVM_alledictstring(ed, classname));
					const char	*s_mdl = PRVM_GetString(prog, PRVM_alledictstring(ed, model));
					prvm_vec_t *e_orig = PRVM_serveredictvector(ed, origin);

					vec3_t v_screen2d= {0};
					int ismap = s_mdl && s_mdl[0] == '*';
					float dist;
					vec3_t dist3;

					if (ismap) {
						prvm_vec_t *v1 = PRVM_serveredictvector(ed, absmin);
						prvm_vec_t *v2 = PRVM_serveredictvector(ed, absmax);
						vec3_t e_box_center = { (v1[0] + v2[0])/2.0, 
							          (v1[1] + v2[1])/2.0, 
									  (v1[2] + v2[2])/2.0
								};
						Math_Project (e_box_center, v_screen2d);
						VectorSubtract(e_box_center, r_refdef.view.origin, dist3);
					} else {
						Math_Project (e_orig, v_screen2d);
						VectorSubtract(e_orig, r_refdef.view.origin, dist3);
					}
					dist = VectorLength (dist3);
					// Dist is d(P1,P2) = (x2 x1)2 + (y2 y1)2 + (z2 z1)2
					// Sort
					
					char s_descr[64];
					c_dpsnprintf2 (s_descr, "%08.1f %d", dist, (int)edict_num);
					char *s_this = Z_strdup (s_descr); 
					stringlistappend (&matchedSet, s_this);


				} // edict not free
			} // e
		} // if
	} // for

	// SORT
	stringlistsort (&matchedSet, fs_make_unique_false);
	
	// Reverse so furthest away prints first, closest prints last.
	for (int idx = matchedSet.numstrings - 1; idx >= 0; idx --) {
		char *sxy = matchedSet.strings[idx];
		char *s_edictnum = &sxy[9]; // First 8 characters are 0 filled distance, space, then edict num
		int edict_num = atoi(s_edictnum);
		prvm_edict_t *ed = PRVM_EDICT_NUM(edict_num);

		const char	*s_cls = PRVM_GetString(prog, PRVM_alledictstring(ed, classname));
		const char	*s_mdl = PRVM_GetString(prog, PRVM_alledictstring(ed, model));
		prvm_vec_t *e_orig = PRVM_serveredictvector(ed, origin);

		vec3_t v_screen2d= {0};
		int ismap = s_mdl && s_mdl[0] == '*';
		float dist;
		vec3_t dist3;
		if (ismap) {
			prvm_vec_t *v1 = PRVM_serveredictvector(ed, absmin);
			prvm_vec_t *v2 = PRVM_serveredictvector(ed, absmax);
			vec3_t e_box_center = { (v1[0] + v2[0])/2.0, 
				          (v1[1] + v2[1])/2.0, 
						  (v1[2] + v2[2])/2.0
					};
			Math_Project (e_box_center, v_screen2d);
			VectorSubtract(e_box_center, r_refdef.view.origin, dist3);
		} else {
			Math_Project (e_orig, v_screen2d);
			VectorSubtract(e_orig, r_refdef.view.origin, dist3);
		}
		dist = VectorLength (dist3);
		// Dist is d(P1,P2) = (x2 x1)2 + (y2 y1)2 + (z2 z1)2
		// Sort
#if 1
					// Set tool_inspector 2 for single frame printed information to console
					// it then reverts to tool_inspector 1
					if (tool_inspector.integer > 1)
						Con_PrintLinef ("%4d %-20.20s @ %3.1f %3.1f %3.1f ", edict_num, s_cls, e_orig[0], e_orig[1], e_orig[2]);

					char vuf[64];
					char vuf2[64];
					char vuf3[64];
					
					c_dpsnprintf1 (vuf, "edict %d", edict_num);
					c_dpsnprintf1 (vuf2, "%s", s_cls);
					c_dpsnprintf1 (vuf3, "%s", s_mdl);

					int larger = (int)Largest(strlen(vuf), Largest(strlen(vuf2), strlen(vuf3))) ;
					int width_row	= 8 * larger * vid_conwidth.value/conwidth_2d_phase;
					int height_row	= 8 * 1 * vid_conheight.value/conheight_2d_phase;
					int height_tot	= height_row * 3;
					
					int x	= v_screen2d[0] - width_row / 2.0;
					int y0	= v_screen2d[1] - height_row / 2.0;
					int y1	= y0 + height_row;
					int y2	= y1 + height_row;
					
					DrawQ_Fill		(x,y0, width_row, height_tot, /*rgba:*/ 0, 0, 0, 1.0, DRAWFLAG_NORMAL_0);
					DrawQ_String	(x,y0, vuf,  0, /*scale x y*/ height_row, height_row, /*rgba:*/ 1, 1, 1, 1, DRAWFLAG_NORMAL_0, q_outcolor_null, q_ignore_color_codes_true, FONT_CENTERPRINT);
					DrawQ_String	(x,y1, vuf2, 0, /*scale x y*/ height_row, height_row, /*rgba:*/ 1, 1, 1, 1, DRAWFLAG_NORMAL_0, q_outcolor_null, q_ignore_color_codes_true, FONT_CENTERPRINT);
					DrawQ_String	(x,y2, vuf3, 0, /*scale x y*/ height_row, height_row, /*rgba:*/ 1, 1, 1, 1, DRAWFLAG_NORMAL_0, q_outcolor_null, q_ignore_color_codes_true, FONT_CENTERPRINT);
#endif


	}	

	stringlistfreecontents (&matchedSet);

	if (tool_inspector.integer > 1)
		Cvar_SetValueQuick (&tool_inspector, 1);
	
	SV_UnlockThreadMutex ();
}
#endif

void CL_Tool_Marker (void)
{
	int conwidth_2d_phase = scale_width_360;
	int conheight_2d_phase = scale_height_360;

	vec3_t v_screen2d = {0};
	vec3_t mark_origin = {0};

	Math_atov (tool_marker.string, mark_origin);

	if (!mark_origin[1] && !mark_origin[2]) {
		Con_PrintLinef (CON_RED "Tool marker needs x y z where x and y are non-zero");
		Con_PrintLinef (CON_BRONZE "Make sure you put the origin in quotes");
		Con_PrintLinef (CON_BRONZE "Example: " CON_WHITE "tool_marker " QUOTED_STR("120 -3600 24") );
		Con_PrintLinef (CON_BRONZE "and ensure X and Y are not zero");
		Cvar_SetValueQuick (&tool_marker, 0);
	}

	Math_Project (mark_origin, v_screen2d);

	if (v_screen2d[2] <= 0)
		return; // It's behind us

	int width_row	= (8 * ONE_CHAR_1 * 3) * vid_conwidth.value/conwidth_2d_phase;
	int height_row	= (8 * ONE_CHAR_1 * 1) * vid_conheight.value/conheight_2d_phase;
	int height_tot	= height_row * 3;
	
	int x	= v_screen2d[0] - width_row / 2.0;
	int y0	= v_screen2d[1] - height_row / 2.0;
	int y1	= y0 + height_row;
	//int y2	= y1 + height_row;
	
	DrawQ_Fill		(x, y0, width_row, height_tot, /*rgba:*/ 0, 0, 1.0, 1.0, DRAWFLAG_NORMAL_0);
	DrawQ_String	(x + height_row, y1, "X", 0, /*scale x y*/ height_row, height_row, 
		/*rgba:*/ 1, 1, 1, 1.0, 
		DRAWFLAG_NORMAL_0, q_outcolor_null, q_ignore_color_codes_true, FONT_CENTERPRINT);

}

WARP_X_CALLERS_ (sure)
static void CL_UpdateScreen_SCR_DrawScreen(void)
{
cldraw2d0:
	Draw_Frame(); // Looks like frees unused pics

	// Baker: This does canvas stuff, we must have vid_conwidth / vid_conheight finalized BEFORE
drawstart:
	DrawQ_Start(); // Scissor, viewport, matrixes, etc.

	R_Mesh_Start();

	R_UpdateVariables(); // nearclip, gamma, etc.

	// Quake uses clockwise winding, so these are swapped
	r_refdef.view.cullface_front = GL_BACK;
	r_refdef.view.cullface_back = GL_FRONT;

	if (!scr_loading && cls.signon == SIGNONS_4) {
		float size;

		size = scr_viewsize.value * (1.0 / 100.0);
		size = Smallest(size, 1);

		if (r_stereo_sidebyside.integer)
		{
			r_refdef.view.width = (int)(vid.width * size / 2.5);
			r_refdef.view.height = (int)(vid.height * size / 2.5 * (1 - bound(0, r_letterbox.value, 100) / 100));
			r_refdef.view.depth = 1;
			r_refdef.view.x = (int)((vid.width - r_refdef.view.width * 2.5) * 0.5);
			r_refdef.view.y = (int)((vid.height - r_refdef.view.height) / 2);
			r_refdef.view.z = 0;
			if (r_stereo_side)
				r_refdef.view.x += (int)(r_refdef.view.width * 1.5);
		}
		else if (r_stereo_horizontal.integer)
		{
			r_refdef.view.width = (int)(vid.width * size / 2);
			r_refdef.view.height = (int)(vid.height * size * (1 - bound(0, r_letterbox.value, 100) / 100));
			r_refdef.view.depth = 1;
			r_refdef.view.x = (int)((vid.width - r_refdef.view.width * 2.0) / 2);
			r_refdef.view.y = (int)((vid.height - r_refdef.view.height) / 2);
			r_refdef.view.z = 0;
			if (r_stereo_side)
				r_refdef.view.x += (int)(r_refdef.view.width);
		}
		else if (r_stereo_vertical.integer)
		{
			r_refdef.view.width = (int)(vid.width * size);
			r_refdef.view.height = (int)(vid.height * size * (1 - bound(0, r_letterbox.value, 100) / 100) / 2);
			r_refdef.view.depth = 1;
			r_refdef.view.x = (int)((vid.width - r_refdef.view.width) / 2);
			r_refdef.view.y = (int)((vid.height - r_refdef.view.height * 2.0) / 2);
			r_refdef.view.z = 0;
			if (r_stereo_side)
				r_refdef.view.y += (int)(r_refdef.view.height);
		}
		else
		{
			r_refdef.view.width = (int)(vid.width * size);
			r_refdef.view.height = (int)(vid.height * size * (1 - bound(0, r_letterbox.value, 100) / 100));
			r_refdef.view.depth = 1;
			r_refdef.view.x = (int)((vid.width - r_refdef.view.width) / 2);
			r_refdef.view.y = (int)((vid.height - r_refdef.view.height) / 2);
			r_refdef.view.z = 0;
		}

		// LadyHavoc: viewzoom (zoom in for sniper rifles, etc)
		// LadyHavoc: this is designed to produce widescreen fov values
		// when the screen is wider than 4/3 width/height aspect, to do
		// this it simply assumes the requested fov is the vertical fov
		// for a 4x3 display, if the ratio is not 4x3 this makes the fov
		// higher/lower according to the ratio
		r_refdef.view.useperspective = true;
		r_refdef.view.frustum_y = tan(scr_fov.value * M_PI / 360.0) * (3.0 / 4.0) * cl.viewzoom;
		r_refdef.view.frustum_x = r_refdef.view.frustum_y * (float)r_refdef.view.width / (float)r_refdef.view.height / vid_pixelheight.value;

		r_refdef.view.frustum_x *= r_refdef.frustumscale_x;
		r_refdef.view.frustum_y *= r_refdef.frustumscale_y;
		r_refdef.view.ortho_x = atan(r_refdef.view.frustum_x) * (360.0 / M_PI); // abused as angle by VM_CL_R_SetView
		r_refdef.view.ortho_y = atan(r_refdef.view.frustum_y) * (360.0 / M_PI); // abused as angle by VM_CL_R_SetView

		r_refdef.view.ismain = true;
	csqc:
		// if CSQC is loaded, it is required to provide the CSQC_UpdateView function,
		// and won't render a view if it does not call that.
		if (cl.csqc_loaded) {
			CL_VM_UpdateView(r_stereo_side ? 0.0 : max(0.0, cl.time - cl.oldtime));
		}
		else
		{
			cl.csqc_vidvars.drawworld = r_drawworld.integer != 0; // Baker: r_drawworld sometimes ignored fix.

			// Prepare the scene mesh for rendering - this is lightning beams and other effects rendered as normal surfaces
			CL_MeshEntities_Scene_FinalizeRenderEntity();

			CL_UpdateEntityShading();
			R_RenderView(0, NULL, NULL, r_refdef.view.x, r_refdef.view.y, r_refdef.view.width, r_refdef.view.height);
		}

		if (!cls.demoplayback && tool_inspector.integer) {
			CL_Tool_Inspector ();
		}
		if (tool_marker.integer) {
			CL_Tool_Marker ();
		}
		// END CL_VM_UpdateView / CSQC
	} // cls.signon == SIGNONS_4

	// Don't apply debugging stuff like r_showsurfaces to the UI
	r_refdef.view.showdebug = false;

draw9part1submit:
draw9wrap3d:
	// If CSQC plus wants fullscreen
	// 1. Finish this draw
	// 2. Reset conwidth/conheight
	// 3. Start a new draw
baker_draw_phase_2:
	if (CLVM_prog->loaded && csqc_full_width_height.integer) {
		// CSQC loaded and wants full screen
		DrawQ_Finish();
		R_Mesh_Finish();
		Cvar_SetValueQuick (&vid_conwidth,  scale_width_360);
		Cvar_SetValueQuick (&vid_conheight, scale_height_360);

baker_really_do_this:
		SCR_SetUpToDrawConsole(); // conlines measurement and stuff

		// Start a new draw, this time without fullscreen
		//Draw_Frame();						// Looks like frees unused pics
		r_viewport_t viewport;

		R_Viewport_InitOrtho(&viewport, &identitymatrix, 0, 0, vid.width, vid.height, 0, 0, vid_conwidth.integer, vid_conheight.integer, -10, 100, NULL);
		R_Mesh_SetRenderTargets(0, NULL, NULL, NULL, NULL, NULL);
		R_SetViewport(&viewport);

		R_Mesh_Start();

	} // end csqc plus wants fullscreen


	if (!r_stereo_sidebyside.integer && !r_stereo_horizontal.integer && !r_stereo_vertical.integer)
	{
		r_refdef.view.width = vid.width;
		r_refdef.view.height = vid.height;
		r_refdef.view.depth = 1;
		r_refdef.view.x = 0;
		r_refdef.view.y = 0;
		r_refdef.view.z = 0;
		r_refdef.view.useperspective = false;
	}

	if (cls.timedemo && cls.td_frames > 0 && timedemo_screenshotframelist.string && timedemo_screenshotframelist.string[0])
	{
		const char *t;
		int framenum;
		t = timedemo_screenshotframelist.string;
		while (*t)
		{
			while (*t == ' ')
				t++;
			if (!*t)
				break;
			framenum = atof(t);
			if (framenum == cls.td_frames)
				break;
			while (*t && *t != ' ')
				t++;
		}
		if (*t)
		{
			// we need to take a screenshot of this frame...
			char filename[MAX_QPATH_128];
			unsigned char *buffer1;
			unsigned char *buffer2;
			dpsnprintf(filename, sizeof(filename), "timedemoscreenshots/%s%06d.tga", cls.demoname, cls.td_frames);
			buffer1 = (unsigned char *)Mem_Alloc(tempmempool, vid.width * vid.height * 4);
			buffer2 = (unsigned char *)Mem_Alloc(tempmempool, vid.width * vid.height * 3);
			SCR_ScreenShot(filename, buffer1, buffer2, 0, 0, vid.width, vid.height, false, false, false, false, false, true, false);
			Mem_Free(buffer1);
			Mem_Free(buffer2);
		}
	}

d2go:
	// draw 2D stuff

	if (!scr_con_current && Have_Flag(key_consoleactive, KEY_CONSOLEACTIVE_FORCED_4) == false) {
		if (isin2(key_dest, key_game, key_message) && 
			!r_letterbox.value && 
			!scr_loading)
			Con_DrawNotify();	// only draw notify in game
	} // if

	if (cl.islocalgame && (key_dest != key_game || key_consoleactive))
		host.paused = true;
	else
		host.paused = false;

	if (!scr_loading && cls.signon == SIGNONS_4) {
		SCR_DrawNet();
		SCR_DrawTurtle();
		SCR_DrawPause();
		if (!r_letterbox.value)
			Sbar_Draw();
		SHOWLMP_drawall();
		SCR_CheckDrawCenterString();
	}
	SCR_DrawNetGraph();

menu:
#ifdef CONFIG_MENU
	WARP_X_(M_Draw; MP_Draw /*csqc*/)
		if (!scr_loading)
			MR_Draw();
#endif
	CL_DrawVideo();
	R_Shadow_EditLights_DrawSelectedLightProperties();

	if (scr_loading)
	{
		loadingscreenstack_t connect_status;
		qbool show_connect_status = !loadingscreenstack && (cls.connect_trying || cls.state == ca_connected);
		if (show_connect_status)
		{
			connect_status.absolute_loading_amount_min = 0;
			if (cls.signon > 0)
				dpsnprintf(connect_status.msg, sizeof(connect_status.msg), "Connect: Signon stage %d of %d", cls.signon, SIGNONS_4);
			else if (cls.connect_remainingtries > 0)
				dpsnprintf(connect_status.msg, sizeof(connect_status.msg), "Connect: Trying...  %d", cls.connect_remainingtries);
			else
				dpsnprintf(connect_status.msg, sizeof(connect_status.msg), "Connect: Waiting %d seconds for reply", 10 + cls.connect_remainingtries);
			loadingscreenstack = &connect_status;
		}

		SCR_DrawLoadingScreen();

		if (show_connect_status)
			loadingscreenstack = NULL;
	}

baker_draw_phase_console:
	SCR_DrawConsole();
	SCR_DrawInfobar();

	if (!scr_loading) {
		SCR_DrawBrand();
		SCR_DrawTouchscreenOverlay();
	}

	if (r_timereport_active)
		R_TimeReport("2d");

	R_TimeReport_EndFrame();
	R_TimeReport_BeginFrame();
	if (!scr_loading)
		Sbar_ShowFPS();

draw9finish:
	R_Mesh_Finish();
	DrawQ_Finish();
	R_RenderTarget_FreeUnused(false);
}

extern cvar_t cl_minfps;
extern cvar_t cl_minfps_fade;
extern cvar_t cl_minfps_qualitymax;
extern cvar_t cl_minfps_qualitymin;
extern cvar_t cl_minfps_qualitymultiply;
extern cvar_t cl_minfps_qualityhysteresis;
extern cvar_t cl_minfps_qualitystepmax;
extern cvar_t cl_minfps_force;


void CL_UpdateScreen(void)
{
	static double cl_updatescreen_quality = 1;

	vec3_t vieworigin;
	static double drawscreenstart = 0.0;
	double drawscreendelta;
	r_viewport_t viewport;

	// TODO: Move to a better place.
	cl_punchangle_applied = 0;

	if (drawscreenstart) {
		drawscreendelta = Sys_DirtyTime() - drawscreenstart;
#ifdef CONFIG_VIDEO_CAPTURE
		if (cl_minfps.value > 0 && (cl_minfps_force.integer || !(cls.timedemo || (cls.capturevideo.active && !cls.capturevideo.is_realtime))) && drawscreendelta >= 0 && drawscreendelta < 60)
#else
		if (cl_minfps.value > 0 && (cl_minfps_force.integer || !cls.timedemo) && drawscreendelta >= 0 && drawscreendelta < 60)
#endif // CONFIG_VIDEO_CAPTURE
		{
			// quality adjustment according to render time
			double actualframetime;
			double targetframetime;
			double adjust;
			double f;
			double h;

			// fade lastdrawscreentime
			r_refdef.lastdrawscreentime += (drawscreendelta - r_refdef.lastdrawscreentime) * cl_minfps_fade.value;

			// find actual and target frame times
			actualframetime = r_refdef.lastdrawscreentime;
			targetframetime = (1.0 / cl_minfps.value);

			// we scale hysteresis by quality
			h = cl_updatescreen_quality * cl_minfps_qualityhysteresis.value;

			// calculate adjustment assuming linearity
			f = cl_updatescreen_quality / actualframetime * cl_minfps_qualitymultiply.value;
			adjust = (targetframetime - actualframetime) * f;

			// one sided hysteresis
			if (adjust > 0)
				adjust = max(0, adjust - h);

			// don't adjust too much at once
			adjust = bound(-cl_minfps_qualitystepmax.value, adjust, cl_minfps_qualitystepmax.value);

			// adjust!
			cl_updatescreen_quality += adjust;
			cl_updatescreen_quality = bound(max(0.01, cl_minfps_qualitymin.value), cl_updatescreen_quality, cl_minfps_qualitymax.value);
		}
		else
		{
			cl_updatescreen_quality = 1;
			r_refdef.lastdrawscreentime = 0;
		}
	}

	drawscreenstart = Sys_DirtyTime();

	Sbar_ShowFPS_Update();

	if (!scr_initialized || !con_initialized || !scr_refresh.integer)
		return;				// not initialized yet

	if (IS_NEXUIZ_DERIVED(gamemode)) {
		// play a bit with the palette (experimental)
		palette_rgb_pantscolormap[15][0] = (unsigned char) (128 + 127 * sin(cl.time / exp(1.0f) + 0.0f*M_PI/3.0f));
		palette_rgb_pantscolormap[15][1] = (unsigned char) (128 + 127 * sin(cl.time / exp(1.0f) + 2.0f*M_PI/3.0f));
		palette_rgb_pantscolormap[15][2] = (unsigned char) (128 + 127 * sin(cl.time / exp(1.0f) + 4.0f*M_PI/3.0f));
		palette_rgb_shirtcolormap[15][0] = (unsigned char) (128 + 127 * sin(cl.time /  M_PI  + 5.0f*M_PI/3.0f));
		palette_rgb_shirtcolormap[15][1] = (unsigned char) (128 + 127 * sin(cl.time /  M_PI  + 3.0f*M_PI/3.0f));
		palette_rgb_shirtcolormap[15][2] = (unsigned char) (128 + 127 * sin(cl.time /  M_PI  + 1.0f*M_PI/3.0f));
		memcpy(palette_rgb_pantsscoreboard[15], palette_rgb_pantscolormap[15], sizeof(*palette_rgb_pantscolormap));
		memcpy(palette_rgb_shirtscoreboard[15], palette_rgb_shirtcolormap[15], sizeof(*palette_rgb_shirtcolormap));
	}

#ifdef CONFIG_VIDEO_CAPTURE
	if (vid_hidden && !cls.capturevideo.active && !cl_capturevideo.integer)
#else
	if (vid_hidden)
#endif
	{
		VID_Finish();
		return;
	}

	if (scr_loading)
	{
		if (!loadingscreenstack && !cls.connect_trying && (cls.state != ca_connected || cls.signon == SIGNONS_4))
			SCR_EndLoadingPlaque();
		else if (scr_loadingscreen_maxfps.value)
		{
			static float lastupdate;
			float now = Sys_DirtyTime();
			if (now - lastupdate < 1.0f / scr_loadingscreen_maxfps.value)
				return;
			lastupdate = now;
		}
	}

baker_csqc_draw_start_logic_here:
	CL_UpdateScreen_SCR_UpdateVars(); // Viewsize and stuff, scale_360_calc conwidth

	R_FrameData_NewFrame(); // Nothing with conwidth
	R_BufferData_NewFrame(); // Nothing with conwidth
cldraw2d2:

	Matrix4x4_OriginFromMatrix(&r_refdef.view.matrix, vieworigin); // View matrix is expected to be set
	R_HDR_UpdateIrisAdaptation(vieworigin);

	r_refdef.view.colormask[0] = 1;
	r_refdef.view.colormask[1] = 1;
	r_refdef.view.colormask[2] = 1;

baker_bypass_console_calcs_do_later:
	if (CLVM_prog->loaded && csqc_full_width_height.integer) {
		// Skip for later
	} else {
		SCR_SetUpToDrawConsole(); // conlines measurement and stuff
	}

#ifndef USE_GLES2
	CHECKGLERROR
	qglDrawBuffer(GL_BACK);CHECKGLERROR
#endif

	// The idea here is to give csqc the entirety of the screen

bang:
	R_Viewport_InitOrtho(&viewport, &identitymatrix, 0, 0, vid.width, vid.height, 0, 0, vid_conwidth.integer, vid_conheight.integer, -10, 100, NULL);
	R_Mesh_SetRenderTargets(0, NULL, NULL, NULL, NULL, NULL);
	R_SetViewport(&viewport);
	GL_ScissorTest(false);
	GL_ColorMask(1,1,1,1);
	GL_DepthMask(true);

	R_ClearScreen(false);
	r_refdef.view.clear = false;
	r_refdef.view.isoverlay = false;

	// calculate r_refdef.view.quality
	r_refdef.view.quality = cl_updatescreen_quality;

	if (scr_stipple.integer)
	{
		Con_PrintLinef ("FIXME: scr_stipple not implemented");
		Cvar_SetValueQuick(&scr_stipple, 0);
	}

#ifndef USE_GLES2
	if (R_Stereo_Active())
	{
		r_stereo_side = 0;

		if (r_stereo_redblue.integer || r_stereo_redgreen.integer || r_stereo_redcyan.integer)
		{
			r_refdef.view.colormask[0] = 1;
			r_refdef.view.colormask[1] = 0;
			r_refdef.view.colormask[2] = 0;
		}

		if (vid.stereobuffer)
			qglDrawBuffer(GL_BACK_RIGHT);
cldraw2d4:
		CL_UpdateScreen_SCR_DrawScreen(); // This does not hit

		r_stereo_side = 1;
		r_refdef.view.clear = true;

		if (r_stereo_redblue.integer || r_stereo_redgreen.integer || r_stereo_redcyan.integer)
		{
			r_refdef.view.colormask[0] = 0;
			r_refdef.view.colormask[1] = r_stereo_redcyan.integer || r_stereo_redgreen.integer;
			r_refdef.view.colormask[2] = r_stereo_redcyan.integer || r_stereo_redblue.integer;
		}

		if (vid.stereobuffer)
			qglDrawBuffer(GL_BACK_LEFT);

		CL_UpdateScreen_SCR_DrawScreen(); // Stereo draw - This does not hit
		r_stereo_side = 0;
	}
	else
#endif
	{
		r_stereo_side = 0;
		CL_UpdateScreen_SCR_DrawScreen(); // Stereo draw .. this hits
	}

#ifdef CONFIG_VIDEO_CAPTURE
	SCR_CaptureVideo();
#endif

	qglFlush(); // ensure that the commands are submitted to the GPU before we do other things

	// Baker: So what is relative?  Does this mean clip to window or what?

	if (!vid_activewindow || key_consoleactive) {
		// Not active window or console shown
		VID_SetMouse(q_mouse_relative_false, q_mouse_hidecursor_false);
	}
	else if (key_dest == key_menu || 
			key_dest == key_menu_grabbed || 
			scr_loading || 
			(key_dest == key_game && cls.demoplayback)) { // Baker 8081 mouse cursor shows during demo playback.
#ifdef CONFIG_MENU
		if (menu_is_csqc == false) {
				VID_SetMouse(q_mouse_relative_false, q_mouse_hidecursor_false);
		} else {
			// Baker: Tends to be true in game
			VID_SetMouse(vid_mouse.integer && !in_client_mouse && !vid_touchscreen.integer, !vid_touchscreen.integer);
			// VID_SetMouse(is_relative true, mousegrab true); is typical key_menu behavior here
		}
#endif
	}
	else {
		// Norm for in-game
		// is_relative = vid_mouse.integer 1 && cl.csqc_wantsmousemove == 0 && cl_prydoncursor == 0
		// and (demoplay is 0 or cl_demo_mousegrab_0 > 0 && vid touchscreen is 0
		// is mousegrab since touchscreen is 0 will always be true typically
		int is_relative = vid_mouse.integer && !cl.csqc_wantsmousemove && cl_prydoncursor.integer <= 0
			&& (!cls.demoplayback || cl_demo_mousegrab.integer) && !vid_touchscreen.integer;
		// Tendds to be true in game
		VID_SetMouse(is_relative, !vid_touchscreen.integer);
		// VID_SetMouse(is_relative true, mousegrab true); is typical key_game behavior here
	}

	VID_Finish();

	// Baker r9006: Kleskby ALT-TAB fix for certain international keyboards.
#ifdef WIN32
	if (vid_fullscreen.integer && GetAsyncKeyState(VK_MENU) && GetAsyncKeyState(VK_TAB)) { //KleskBY Alt-Tab Fix 
		ShowWindow(FindWindowA("SDL_app", NULL), SW_MINIMIZE);
	} // if
#endif
}

void CL_Screen_NewMap(void)
{
}

void CL_Screen_Init(void)
{
	int i;
	Cvar_RegisterVariable (&scr_fov);
	Cvar_RegisterVariable (&scr_viewsize);
	Cvar_RegisterVariable (&scr_conalpha);
	Cvar_RegisterVariable (&scr_conalphafactor);
	Cvar_RegisterVariable (&scr_conalpha2factor);
	Cvar_RegisterVariable (&scr_conalpha3factor);
	Cvar_RegisterVariable (&scr_conscroll_x);
	Cvar_RegisterVariable (&scr_conscroll_y);
	Cvar_RegisterVariable (&scr_conscroll2_x);
	Cvar_RegisterVariable (&scr_conscroll2_y);
	Cvar_RegisterVariable (&scr_conscroll3_x);
	Cvar_RegisterVariable (&scr_conscroll3_y);
	Cvar_RegisterVariable (&scr_conbrightness);
	Cvar_RegisterVariable (&scr_conforcewhiledisconnected);
	Cvar_RegisterVariable (&scr_conheight);

	// Baker r7003, r7004
	Cvar_RegisterVariable (&csqc_full_width_height);
	Cvar_RegisterVariable (&csqc_full_width_height_available);

#ifdef CONFIG_MENU
	Cvar_RegisterVariable (&scr_menuforcewhiledisconnected);
#endif
	Cvar_RegisterVariable (&scr_loadingscreen_background);
	Cvar_RegisterVariable (&scr_loadingscreen_scale);
	Cvar_RegisterVariable (&scr_loadingscreen_scale_base);
	Cvar_RegisterVariable (&scr_loadingscreen_scale_limit);
	Cvar_RegisterVariable (&scr_loadingscreen_picture);
	Cvar_RegisterVariable (&scr_loadingscreen_count);
	Cvar_RegisterVariable (&scr_loadingscreen_firstforstartup);
	Cvar_RegisterVariable (&scr_loadingscreen_barcolor);
	Cvar_RegisterVariable (&scr_loadingscreen_barheight);
	Cvar_RegisterVariable (&scr_loadingscreen_maxfps);
	Cvar_RegisterVariable (&scr_infobar_height);
	Cvar_RegisterVariable (&scr_showram);
	Cvar_RegisterVariable (&scr_showturtle);
	Cvar_RegisterVariable (&scr_showpause);
	Cvar_RegisterVariable (&scr_showbrand);
	Cvar_RegisterVariable (&scr_centertime);
	Cvar_RegisterVariable (&scr_printspeed);
	Cvar_RegisterVariable (&vid_conwidth);
	Cvar_RegisterVariable (&vid_conheight);

	Cvar_RegisterVariable (&vid_pixelheight);
	//Cvar_RegisterVariable (&vid_conwidthauto); // Baker r0005
	Cvar_RegisterVariable (&scr_screenshot_jpeg);
	Cvar_RegisterVariable (&scr_screenshot_jpeg_quality);
	Cvar_RegisterVariable (&scr_screenshot_png);
	Cvar_RegisterVariable (&scr_screenshot_gammaboost);
	Cvar_RegisterVariable (&scr_screenshot_name_in_mapdir);
	Cvar_RegisterVariable (&scr_screenshot_alpha);
	Cvar_RegisterVariable (&scr_screenshot_timestamp);
#ifdef CONFIG_VIDEO_CAPTURE
	Cvar_RegisterVariable (&cl_capturevideo);
	Cvar_RegisterVariable (&cl_capturevideo_demo_stop);
	Cvar_RegisterVariable (&cl_capturevideo_printfps);
	Cvar_RegisterVariable (&cl_capturevideo_width);
	Cvar_RegisterVariable (&cl_capturevideo_height);
	Cvar_RegisterVariable (&cl_capturevideo_realtime);
	Cvar_RegisterVariable (&cl_capturevideo_fps);
	Cvar_RegisterVariable (&cl_capturevideo_nameformat);
	Cvar_RegisterVariable (&cl_capturevideo_number);
	Cvar_RegisterVariable (&cl_capturevideo_ogg);
	Cvar_RegisterVariable (&cl_capturevideo_framestep);
#endif
	Cvar_RegisterVariable (&tool_inspector);
	Cvar_RegisterVariable (&tool_marker); // Baker r0109: tool marker

	Cvar_RegisterVariable (&r_letterbox);
	Cvar_RegisterVariable (&r_stereo_separation);
	Cvar_RegisterVariable (&r_stereo_sidebyside);
	Cvar_RegisterVariable (&r_stereo_horizontal);
	Cvar_RegisterVariable (&r_stereo_vertical);
	Cvar_RegisterVariable (&r_stereo_redblue);
	Cvar_RegisterVariable (&r_stereo_redcyan);
	Cvar_RegisterVariable (&r_stereo_redgreen);
	Cvar_RegisterVariable (&r_stereo_angle);
	Cvar_RegisterVariable (&scr_stipple);
	Cvar_RegisterVariable (&scr_refresh);
	Cvar_RegisterVariable (&net_graph);
	Cvar_RegisterVirtual( &net_graph, "shownetgraph");
	Cvar_RegisterVariable (&cl_demo_mousegrab);
	Cvar_RegisterVariable (&timedemo_screenshotframelist);
	Cvar_RegisterVariable (&vid_touchscreen_outlinealpha);
	Cvar_RegisterVariable (&vid_touchscreen_overlayalpha);
	Cvar_RegisterVariable (&r_speeds_graph);
	for (i = 0;i < (int)(sizeof(r_speeds_graph_filter)/sizeof(r_speeds_graph_filter[0]));i++)
		Cvar_RegisterVariable(&r_speeds_graph_filter[i]);
	Cvar_RegisterVariable (&r_speeds_graph_length);
	Cvar_RegisterVariable (&r_speeds_graph_seconds);
	Cvar_RegisterVariable (&r_speeds_graph_x);
	Cvar_RegisterVariable (&r_speeds_graph_y);
	Cvar_RegisterVariable (&r_speeds_graph_width);
	Cvar_RegisterVariable (&r_speeds_graph_height);
	Cvar_RegisterVariable (&r_speeds_graph_maxtimedelta);
	Cvar_RegisterVariable (&r_speeds_graph_maxdefault);

	// if we want no console, turn it off here too
	if (Sys_CheckParm ("-noconsole"))
		Cvar_SetQuick(&scr_conforcewhiledisconnected, "0");

	Cmd_AddCommand(CF_CLIENT, "sizeup",SCR_SizeUp_f, "increase view size (increases viewsize cvar)");
	Cmd_AddCommand(CF_CLIENT, "sizedown",SCR_SizeDown_f, "decrease view size (decreases viewsize cvar)");
	Cmd_AddCommand(CF_CLIENT, "screenshot",SCR_ScreenShot_f, "takes a screenshot of the next rendered frame");
	Cmd_AddCommand(CF_CLIENT, "envmap", R_Envmap_f, "render a cubemap (skybox) of the current scene");
	Cmd_AddCommand(CF_CLIENT, "infobar", SCR_InfoBar_f, "display a text in the infobar (usage: infobar expiretime string)");

#ifdef CONFIG_VIDEO_CAPTURE
	SCR_CaptureVideo_Ogg_Init();
#endif

	scr_initialized = true;
}
