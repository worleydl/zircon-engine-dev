// menu_lighting.c.h - 100% 

#define		frame_cursor	m_optcursor
#define 	local_count		OPTIONS_GRAPHICS_ITEMS
#define		local_cursor	options_graphics_cursor
#define 	visiblerows 	m_graphics_visiblerows

#define	OPTIONS_GRAPHICS_ITEMS	18

static int options_graphics_cursor;
int m_graphics_visiblerows;

void M_Menu_Options_Graphics_f(cmd_state_t *cmd)
{
	key_dest = key_menu;
	menu_state_set_nova (m_options_graphics);
	m_entersound = true;
}

extern cvar_t r_shadow_gloss;
extern cvar_t r_shadow_realtime_dlight;
extern cvar_t r_shadow_realtime_dlight_shadows;
extern cvar_t r_shadow_realtime_world;
extern cvar_t r_shadow_realtime_world_lightmaps;
extern cvar_t r_shadow_realtime_world_shadows;
extern cvar_t r_bloom;
extern cvar_t r_bloom_colorscale;
extern cvar_t r_bloom_colorsubtract;
extern cvar_t r_bloom_colorexponent;
extern cvar_t r_bloom_blur;
extern cvar_t r_bloom_brighten;
extern cvar_t r_bloom_resolution;
extern cvar_t r_hdr_scenebrightness;
extern cvar_t r_hdr_glowintensity;

static void M_Menu_Options_Graphics_AdjustSliders (cmd_state_t *cmd, int dir)
{
	int optnum;
	S_LocalSound ("sound/misc/menu3.wav");

	optnum = 0;

	     if (local_cursor == optnum++) Cvar_SetValueQuick (&r_coronas, bound(0, r_coronas.value + dir * 0.125, 4));
	else if (local_cursor == optnum++) Cvar_SetValueQuick (&gl_flashblend, !gl_flashblend.integer);
	else if (local_cursor == optnum++) Cvar_SetValueQuick (&r_shadow_gloss,							bound(0, r_shadow_gloss.integer + dir, 2));
	else if (local_cursor == optnum++) Cvar_SetValueQuick (&r_shadow_realtime_dlight,				!r_shadow_realtime_dlight.integer);
	else if (local_cursor == optnum++) Cvar_SetValueQuick (&r_shadow_realtime_dlight_shadows,		!r_shadow_realtime_dlight_shadows.integer);
	else if (local_cursor == optnum++) Cvar_SetValueQuick (&r_shadow_realtime_world,					!r_shadow_realtime_world.integer);
	else if (local_cursor == optnum++) Cvar_SetValueQuick (&r_shadow_realtime_world_lightmaps,		bound(0, r_shadow_realtime_world_lightmaps.value + dir * 0.1, 1));
	else if (local_cursor == optnum++) Cvar_SetValueQuick (&r_shadow_realtime_world_shadows,			!r_shadow_realtime_world_shadows.integer);
	else if (local_cursor == optnum++) Cvar_SetValueQuick (&r_bloom,                                 !r_bloom.integer);
	else if (local_cursor == optnum++) Cvar_SetValueQuick (&r_hdr_scenebrightness,                   bound(0.25, r_hdr_scenebrightness.value + dir * 0.125, 4));
	else if (local_cursor == optnum++) Cvar_SetValueQuick (&r_hdr_glowintensity,                     bound(0, r_hdr_glowintensity.value + dir * 0.25, 4));
	else if (local_cursor == optnum++) Cvar_SetValueQuick (&r_bloom_colorscale,                      bound(0.0625, r_bloom_colorscale.value + dir * 0.0625, 1));
	else if (local_cursor == optnum++) Cvar_SetValueQuick (&r_bloom_colorsubtract,                   bound(0, r_bloom_colorsubtract.value + dir * 0.0625, 1-0.0625));
	else if (local_cursor == optnum++) Cvar_SetValueQuick (&r_bloom_colorexponent,                   bound(1, r_bloom_colorexponent.value * (dir > 0 ? 2.0 : 0.5), 8));
	else if (local_cursor == optnum++) Cvar_SetValueQuick (&r_bloom_brighten,                        bound(1, r_bloom_brighten.value + dir * 0.0625, 4));
	else if (local_cursor == optnum++) Cvar_SetValueQuick (&r_bloom_blur,                            bound(1, r_bloom_blur.value + dir * 1, 16));
	else if (local_cursor == optnum++) Cvar_SetValueQuick (&r_bloom_resolution,                      bound(64, r_bloom_resolution.value + dir * 64, 2048));
	else if (local_cursor == optnum++) Cbuf_AddTextLine (cmd, "r_restart");
}

static void M_Options_Graphics_Draw (void)
{
	cachepic_t	*p0;

	PPX_Start (local_cursor, frame_cursor); // PPX FRAME

	M_Background(320, bound(200, 32 + local_count * 8, vid_conheight.integer), q_darken_true);

	M_DrawPic(16, 4, "gfx/qplaque", NO_HOTSPOTS_0, NA0, NA0);
	p0 = Draw_CachePic ("gfx/p_option");
	M_DrawPic((320-Draw_GetPicWidth(p0))/2, 4, "gfx/p_option", NO_HOTSPOTS_0, NA0, NA0);

	visiblerows = (int)((menu_height - 32) / 8);
	drawcur_y = 32 - bound(0, local_cursor - (visiblerows / 2), 
		max(0, local_count - visiblerows)) * 8;

	M_Options_PrintSlider(  "      Corona Intensity", true, r_coronas.value, 0, 4);
	M_Options_PrintCheckbox("      Use Only Coronas", true, gl_flashblend.integer);
	M_Options_PrintSlider(  "            Gloss Mode", true, r_shadow_gloss.integer, 0, 2);
	M_Options_PrintCheckbox("            RT DLights", !gl_flashblend.integer, r_shadow_realtime_dlight.integer);
	M_Options_PrintCheckbox("     RT DLight Shadows", !gl_flashblend.integer, r_shadow_realtime_dlight_shadows.integer);
	M_Options_PrintCheckbox("              RT World", true, r_shadow_realtime_world.integer);
	M_Options_PrintSlider(  "    RT World Lightmaps", true, r_shadow_realtime_world_lightmaps.value, 0, 1);
	M_Options_PrintCheckbox("       RT World Shadow", true, r_shadow_realtime_world_shadows.integer);
	M_Options_PrintCheckbox("          Bloom Effect", true, r_bloom.integer);
	M_Options_PrintSlider(  "      Scene Brightness", true, r_hdr_scenebrightness.value, 0.25, 4);
	M_Options_PrintSlider(  "       Glow Brightness", true, r_hdr_glowintensity.value, 0, 4);
	M_Options_PrintSlider(  "     Bloom Color Scale", r_bloom.integer, r_bloom_colorscale.value, 0.0625, 1);
	M_Options_PrintSlider(  "  Bloom Color Subtract", r_bloom.integer, r_bloom_colorsubtract.value, 0, 1-0.0625);
	M_Options_PrintSlider(  "  Bloom Color Exponent", r_bloom.integer, r_bloom_colorexponent.value, 1, 8);
	M_Options_PrintSlider(  "       Bloom Intensity", r_bloom.integer, r_bloom_brighten.value, 1, 4);
	M_Options_PrintSlider(  "            Bloom Blur", r_bloom.integer, r_bloom_blur.value, 1, 16);
	M_Options_PrintSlider(  "      Bloom Resolution", r_bloom.integer, r_bloom_resolution.value, 64, 2048);
	M_Options_PrintCommand( "      Restart Renderer", true);

	PPX_DrawSel_End ();
}


static void M_Options_Graphics_Key (cmd_state_t *cmd, int k, int ascii)
{
	switch (k) {
	case K_MOUSE2: if (Hotspots_DidHit_Slider()) { local_cursor = hotspotx_hover; goto leftus; } // PPX Key2 fall thru
	case K_ESCAPE:
		M_Menu_Options_Classic_f(cmd);
		break;

	case K_MOUSE1: 
		if (!Hotspots_DidHit () ) { return;	}
		local_cursor = hotspotx_hover;
		// fall thru

	case K_ENTER:
		M_Menu_Options_Graphics_AdjustSliders (cmd, 1);
		break;

		case K_HOME: 
		local_cursor = 0;
		break;

	case K_END:
		local_cursor = local_count - 1;
		break;

	case K_PGUP:
		local_cursor -= visiblerows / 2;
		if (local_cursor < 0) 
			local_cursor = 0;
		break;

	case K_MWHEELUP:
		local_cursor -= visiblerows / 4;
		if (local_cursor < 0) 
			local_cursor = 0;
		break;

	case K_PGDN:
		local_cursor += visiblerows / 2;
		if (local_cursor >= local_count) 
			local_cursor = local_count - 1;
		break;

	case K_MWHEELDOWN:
		local_cursor += visiblerows / 4;
		if (local_cursor >= local_count) 
			local_cursor = local_count - 1;
		break;

	case K_UPARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		local_cursor--;
		if (local_cursor < 0)
			local_cursor = 0;
		break;

	case K_DOWNARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		local_cursor++;
		if (local_cursor >= local_count)
			local_cursor = local_count - 1;
		break;

leftus:
	case K_LEFTARROW:
		M_Menu_Options_Graphics_AdjustSliders(cmd, -1);
		break;

	case K_RIGHTARROW:
		M_Menu_Options_Graphics_AdjustSliders(cmd, 1);
		break;
	} // sw
}

#undef frame_cursor
#undef local_count
#undef local_cursor
#undef visiblerows
