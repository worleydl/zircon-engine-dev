// menu_effects.c.h - 100%

#define 	local_count		OPTIONS_EFFECTS_ITEMS
#define		local_cursor	options_effects_cursor
#define 	visiblerows 	m_effects_visiblerows

#define	OPTIONS_EFFECTS_ITEMS	37

static int options_effects_cursor;
int m_effects_visiblerows;

void M_Menu_Options_Effects_f(cmd_state_t *cmd)
{
	KeyDest_Set (key_menu); // key_dest = key_menu;
	menu_state_set_nova (m_options_effects);
	m_entersound = true;
}


extern cvar_t cl_stainmaps;
extern cvar_t cl_stainmaps_clearonload;
extern cvar_t r_explosionclip;
extern cvar_t r_coronas;
extern cvar_t gl_flashblend;
extern cvar_t cl_beams_polygons;
extern cvar_t cl_beams_quakepositionhack;
extern cvar_t cl_beams_instantaimhack;
extern cvar_t cl_beams_lightatend;
extern cvar_t r_lightningbeam_thickness;
extern cvar_t r_lightningbeam_scroll;
extern cvar_t r_lightningbeam_repeatdistance;
extern cvar_t r_lightningbeam_color_red;
extern cvar_t r_lightningbeam_color_green;
extern cvar_t r_lightningbeam_color_blue;
extern cvar_t r_lightningbeam_qmbtexture;

static void M_Menu_Options_Effects_AdjustSliders (int dir)
{
	S_LocalSound ("sound/misc/menu3.wav");

	switch (local_cursor) {
	case 0: Cvar_SetValueQuick (&cl_particles, !cl_particles.integer); break;
	case 1: Cvar_SetValueQuick (&cl_particles_quake, !cl_particles_quake.integer); break;
	case 2: Cvar_SetValueQuick (&cl_particles_quality, bound(1, cl_particles_quality.value + dir * 0.5, 4)); break;
	case 3: Cvar_SetValueQuick (&cl_particles_explosions_shell, !cl_particles_explosions_shell.integer); break;
	case 4: Cvar_SetValueQuick (&r_explosionclip, !r_explosionclip.integer); break;
	case 5: Cvar_SetValueQuick (&cl_stainmaps, !cl_stainmaps.integer); break;
	case 6: Cvar_SetValueQuick (&cl_stainmaps_clearonload, !cl_stainmaps_clearonload.integer); break;
	case 7: Cvar_SetValueQuick (&cl_decals, !cl_decals.integer); break;
	case 8: Cvar_SetValueQuick (&cl_particles_bulletimpacts, !cl_particles_bulletimpacts.integer); break;
	case 9: Cvar_SetValueQuick (&cl_particles_smoke, !cl_particles_smoke.integer); break;
	case 10: Cvar_SetValueQuick (&cl_particles_sparks, !cl_particles_sparks.integer); break;
	case 11: Cvar_SetValueQuick (&cl_particles_bubbles, !cl_particles_bubbles.integer); break;
	case 12: Cvar_SetValueQuick (&cl_particles_blood, !cl_particles_blood.integer); break;
	case 13: Cvar_SetValueQuick (&cl_particles_blood_alpha, bound(0.2, cl_particles_blood_alpha.value + dir * 0.1, 1)); break;
	case 14: Cvar_SetValueQuick (&cl_particles_blood_bloodhack, !cl_particles_blood_bloodhack.integer); break;
	case 15: Cvar_SetValueQuick (&cl_beams_polygons, !cl_beams_polygons.integer); break;
	case 16: Cvar_SetValueQuick (&cl_beams_instantaimhack, !cl_beams_instantaimhack.integer); break;
	case 17: Cvar_SetValueQuick (&cl_beams_quakepositionhack, !cl_beams_quakepositionhack.integer); break;
	case 18: Cvar_SetValueQuick (&cl_beams_lightatend, !cl_beams_lightatend.integer); break;
	case 19: Cvar_SetValueQuick (&r_lightningbeam_thickness, bound(1, r_lightningbeam_thickness.integer + dir, 10)); break;
	case 20: Cvar_SetValueQuick (&r_lightningbeam_scroll, bound(0, r_lightningbeam_scroll.integer + dir, 10)); break;
	case 21: Cvar_SetValueQuick (&r_lightningbeam_repeatdistance, bound(64, r_lightningbeam_repeatdistance.integer + dir * 64, 1024)); break;
	case 22: Cvar_SetValueQuick (&r_lightningbeam_color_red, bound(0, r_lightningbeam_color_red.value + dir * 0.1, 1)); break;
	case 23: Cvar_SetValueQuick (&r_lightningbeam_color_green, bound(0, r_lightningbeam_color_green.value + dir * 0.1, 1)); break;
	case 24: Cvar_SetValueQuick (&r_lightningbeam_color_blue, bound(0, r_lightningbeam_color_blue.value + dir * 0.1, 1)); break;
	case 25: Cvar_SetValueQuick (&r_lightningbeam_qmbtexture, !r_lightningbeam_qmbtexture.integer); break;
	case 26: Cvar_SetValueQuick (&r_lerpmodels, !r_lerpmodels.integer); break;
	case 27: Cvar_SetValueQuick (&r_lerpsprites, !r_lerpsprites.integer); break;
	case 28: Cvar_SetValueQuick (&r_lerplightstyles, !r_lerplightstyles.integer); break;
	case 29: Cvar_SetValueQuick (&gl_polyblend, bound(0, gl_polyblend.value + dir * 0.1, 1)); break;
	case 30: Cvar_SetValueQuick (&r_skyscroll1, bound(-8, r_skyscroll1.value + dir * 0.1, 8)); break;
	case 31: Cvar_SetValueQuick (&r_skyscroll2, bound(-8, r_skyscroll2.value + dir * 0.1, 8)); break;
	case 32: Cvar_SetValueQuick (&r_waterwarp, bound(0, (int)(r_waterwarp.value + dir) , 2)); break;
	case 33: Cvar_SetValueQuick (&r_wateralpha, bound(0, r_wateralpha.value + dir * 0.1, 1)); break;
	case 34: Cvar_SetValueQuick (&r_waterdeform, bound(0, (int)(r_waterdeform.value + dir) , 2)); break;
	case 35: Cvar_SetValueQuick (&r_waterscroll, bound(0, r_waterscroll.value + dir * 0.5, 10)); break;
	case 36: Cvar_SetValueQuick (&snd_waterfx, !snd_waterfx.integer); break;
	} // sw
}

static void M_Options_Effects_Draw (void)
{
	cachepic_t	*p0;

	PPX_Start (local_cursor);

	M_Background(320, bound(200, 32 + local_count * 8, vid_conheight.integer), q_darken_true);

	M_DrawPic(16, 4, "gfx/qplaque", NO_HOTSPOTS_0, NA0, NA0);
	p0 = Draw_CachePic ("gfx/p_option");
	M_DrawPic((320-Draw_GetPicWidth(p0))/2, 4, "gfx/p_option", NO_HOTSPOTS_0, NA0, NA0);

	visiblerows = (int)((menu_height - 32) / 8);
	drawcur_y = 32 - bound(0, local_cursor - (visiblerows / 2), 
		max(0, local_count - visiblerows)) * 8;

	M_Options_PrintCheckbox("             Particles", true, cl_particles.integer);
	M_Options_PrintCheckbox(" Quake-style Particles", true, cl_particles_quake.integer);
	M_Options_PrintSlider(  "     Particles Quality", true, cl_particles_quality.value, 1, 4);
	M_Options_PrintCheckbox("       Explosion Shell", true, cl_particles_explosions_shell.integer);
	M_Options_PrintCheckbox("  Explosion Shell Clip", true, r_explosionclip.integer);
	M_Options_PrintCheckbox("             Stainmaps", true, cl_stainmaps.integer);
	M_Options_PrintCheckbox("Onload Clear Stainmaps", true, cl_stainmaps_clearonload.integer);
	M_Options_PrintCheckbox("                Decals", true, cl_decals.integer);
	M_Options_PrintCheckbox("        Bullet Impacts", true, cl_particles_bulletimpacts.integer);
	M_Options_PrintCheckbox("                 Smoke", true, cl_particles_smoke.integer);
	M_Options_PrintCheckbox("                Sparks", true, cl_particles_sparks.integer);
	M_Options_PrintCheckbox("               Bubbles", true, cl_particles_bubbles.integer);
	M_Options_PrintCheckbox("                 Blood", true, cl_particles_blood.integer);
	M_Options_PrintSlider(  "         Blood Opacity", true, cl_particles_blood_alpha.value, 0.2, 1);
	M_Options_PrintCheckbox("Force New Blood Effect", true, cl_particles_blood_bloodhack.integer);
	M_Options_PrintCheckbox("     Polygon Lightning", true, cl_beams_polygons.integer);
	M_Options_PrintCheckbox("Smooth Sweep Lightning", true, cl_beams_instantaimhack.integer);
	M_Options_PrintCheckbox(" Waist-level Lightning", true, cl_beams_quakepositionhack.integer);
	M_Options_PrintCheckbox("   Lightning End Light", true, cl_beams_lightatend.integer);
	M_Options_PrintSlider(  "   Lightning Thickness", cl_beams_polygons.integer, r_lightningbeam_thickness.integer, 1, 10);
	M_Options_PrintSlider(  "      Lightning Scroll", cl_beams_polygons.integer, r_lightningbeam_scroll.integer, 0, 10);
	M_Options_PrintSlider(  " Lightning Repeat Dist", cl_beams_polygons.integer, r_lightningbeam_repeatdistance.integer, 64, 1024);
	M_Options_PrintSlider(  "   Lightning Color Red", cl_beams_polygons.integer, r_lightningbeam_color_red.value, 0, 1);
	M_Options_PrintSlider(  " Lightning Color Green", cl_beams_polygons.integer, r_lightningbeam_color_green.value, 0, 1);
	M_Options_PrintSlider(  "  Lightning Color Blue", cl_beams_polygons.integer, r_lightningbeam_color_blue.value, 0, 1);
	M_Options_PrintCheckbox(" Lightning QMB Texture", cl_beams_polygons.integer, r_lightningbeam_qmbtexture.integer);
	M_Options_PrintCheckbox("   Model Interpolation", true, r_lerpmodels.integer);
	M_Options_PrintCheckbox("  Sprite Interpolation", true, r_lerpsprites.integer);
	M_Options_PrintCheckbox(" Flicker Interpolation", true, r_lerplightstyles.integer);
	M_Options_PrintSlider(  "            View Blend", true, gl_polyblend.value, 0, 1);
	M_Options_PrintSlider(  "Upper Sky Scroll Speed", true, r_skyscroll1.value, -8, 8);
	M_Options_PrintSlider(  "Lower Sky Scroll Speed", true, r_skyscroll2.value, -8, 8);
	M_Options_PrintSS(      "  Underwater View Warp", true, get_waterwarp_text(get_waterwarp_rot()) );
	M_Options_PrintSlider(  " Water Alpha (opacity)", true, r_wateralpha.value, 0, 1);
	M_Options_PrintSS(      "          Water Deform", true, get_waterdeform_text(get_deform_rot()) );
	M_Options_PrintSlider(  "          Water Scroll", true, r_waterscroll.value, 0, 10);
	M_Options_PrintCheckbox(" Underwater Snd Muffle", true, snd_waterfx.integer);

	PPX_DrawSel_End ();
}


static void M_Options_Effects_Key(cmd_state_t *cmd, int key, int ascii)
{
	switch (key) {
	case K_MOUSE2: 
		if (Hotspots_DidHit_Slider()) { 
			local_cursor = hotspotx_hover; 
			goto leftus; 
		} 
		// Fall thru
	case K_ESCAPE:
		M_Menu_Options_Classic_f(cmd);
		break;

	case K_MOUSE1: 
		if (!Hotspots_DidHit() ) 
			return;
		local_cursor = hotspotx_hover; 
		// fall thru

	case K_ENTER:
		M_Menu_Options_Effects_AdjustSliders (1);
		break;

	case K_HOME: 
		local_cursor = 0;
		break;

	case K_END:
		local_cursor = local_count - 1;
		break;

	case K_PGUP:
		local_cursor -= visiblerows / 2;
		if (local_cursor < 0) // PGUP does not wrap, stops at start
			local_cursor = 0;
		break;

	case K_MWHEELUP:
		local_cursor -= visiblerows / 4;
		if (local_cursor < 0) // K_MWHEELUP does not wrap, stops at start
			local_cursor = 0;
		break;

	case K_PGDN:
		local_cursor += visiblerows / 2;
		if (local_cursor >= local_count) // PGDN does not wrap, stops at end
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
		if (local_cursor < 0) // K_UPARROW wraps around to end
			local_cursor = local_count - 1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("sound/misc/menu1.wav");
		local_cursor++;
		if (local_cursor >= local_count) // K_DOWNARROW wraps around to start
			local_cursor = 0;
		break;

leftus:
	case K_LEFTARROW:	
		M_Menu_Options_Effects_AdjustSliders (-1);
		break;

	case K_RIGHTARROW:	
		M_Menu_Options_Effects_AdjustSliders (1);
		break;
	} // sw
}

#undef local_count
#undef local_cursor
#undef visiblerows
