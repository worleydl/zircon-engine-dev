// menu_color.c.h - 100%

#define 	local_count		OPTIONS_COLORCONTROL_ITEMS
#define		local_cursor	options_colorcontrol_cursor
#define 	visiblerows 	m_color_visiblerows

#define	OPTIONS_COLORCONTROL_ITEMS	17 // hardware gamma control is gone

static int		options_colorcontrol_cursor;
int m_color_visiblerows;


// intensity value to match up to 50% dither to 'correct' quake

void M_Menu_Options_ColorControl_f(cmd_state_t *cmd)
{
	KeyDest_Set (key_menu); // key_dest = key_menu;
	menu_state_set_nova (m_options_colorcontrol);
	m_entersound = true;
}


static void M_Menu_Options_ColorControl_AdjustSliders (int dir)
{
	float f;
	int optnum;
	S_LocalSound ("sound/misc/menu3.wav");

	optnum = 1;
	if (local_cursor == optnum++) {
		Cvar_SetValueQuick (&v_color_enable, 0);
		Cvar_SetValueQuick (&v_gamma, bound(1, v_gamma.value + dir * 0.125, 5));
	} else if (local_cursor == optnum++) {
		Cvar_SetValueQuick (&v_color_enable, 0);
		Cvar_SetValueQuick (&v_contrast, bound(1, v_contrast.value + dir * 0.125, 5));
	} else if (local_cursor == optnum++) {
		Cvar_SetValueQuick (&v_color_enable, 0);
		Cvar_SetValueQuick (&v_brightness, bound(0, v_brightness.value + dir * 0.05, 0.8));
	} else if (local_cursor == optnum++) {
		Cvar_SetValueQuick (&v_color_enable, !v_color_enable.integer);
	} else if (local_cursor == optnum++) {
		Cvar_SetValueQuick (&v_color_enable, 1);
		Cvar_SetValueQuick (&v_color_black_r, bound(0, v_color_black_r.value + dir * 0.0125, 0.8));
	} else if (local_cursor == optnum++) {
		Cvar_SetValueQuick (&v_color_enable, 1);
		Cvar_SetValueQuick (&v_color_black_g, bound(0, v_color_black_g.value + dir * 0.0125, 0.8));
	} else if (local_cursor == optnum++) {
		Cvar_SetValueQuick (&v_color_enable, 1);
		Cvar_SetValueQuick (&v_color_black_b, bound(0, v_color_black_b.value + dir * 0.0125, 0.8));
	} else if (local_cursor == optnum++) {
		Cvar_SetValueQuick (&v_color_enable, 1);
		f = bound(0, (v_color_black_r.value + v_color_black_g.value + v_color_black_b.value) / 3 + dir * 0.0125, 0.8);
		Cvar_SetValueQuick (&v_color_black_r, f);
		Cvar_SetValueQuick (&v_color_black_g, f);
		Cvar_SetValueQuick (&v_color_black_b, f);
	} else if (local_cursor == optnum++) {
		Cvar_SetValueQuick (&v_color_enable, 1);
		Cvar_SetValueQuick (&v_color_grey_r, bound(0, v_color_grey_r.value + dir * 0.0125, 0.95));
	} else if (local_cursor == optnum++) {
		Cvar_SetValueQuick (&v_color_enable, 1);
		Cvar_SetValueQuick (&v_color_grey_g, bound(0, v_color_grey_g.value + dir * 0.0125, 0.95));
	} else if (local_cursor == optnum++) {
		Cvar_SetValueQuick (&v_color_enable, 1);
		Cvar_SetValueQuick (&v_color_grey_b, bound(0, v_color_grey_b.value + dir * 0.0125, 0.95));
	} else if (local_cursor == optnum++) {
		Cvar_SetValueQuick (&v_color_enable, 1);
		f = bound(0, (v_color_grey_r.value + v_color_grey_g.value + v_color_grey_b.value) / 3 + dir * 0.0125, 0.95);
		Cvar_SetValueQuick (&v_color_grey_r, f);
		Cvar_SetValueQuick (&v_color_grey_g, f);
		Cvar_SetValueQuick (&v_color_grey_b, f);
	} else if (local_cursor == optnum++) {
		Cvar_SetValueQuick (&v_color_enable, 1);
		Cvar_SetValueQuick (&v_color_white_r, bound(1, v_color_white_r.value + dir * 0.125, 5));
	} else if (local_cursor == optnum++) {
		Cvar_SetValueQuick (&v_color_enable, 1);
		Cvar_SetValueQuick (&v_color_white_g, bound(1, v_color_white_g.value + dir * 0.125, 5));
	} else if (local_cursor == optnum++) {
		Cvar_SetValueQuick (&v_color_enable, 1);
		Cvar_SetValueQuick (&v_color_white_b, bound(1, v_color_white_b.value + dir * 0.125, 5));
	} else if (local_cursor == optnum++) {
		Cvar_SetValueQuick (&v_color_enable, 1);
		f = bound(1, (v_color_white_r.value + v_color_white_g.value + v_color_white_b.value) / 3 + dir * 0.125, 5);
		Cvar_SetValueQuick (&v_color_white_r, f);
		Cvar_SetValueQuick (&v_color_white_g, f);
		Cvar_SetValueQuick (&v_color_white_b, f);
	}
}

static void M_Options_ColorControl_Draw (void)
{
	float x, s, t, u, v;
	float c[3];
	cachepic_t	*p0, *dither;


	M_Background(320, 256, q_darken_true);
	PPX_Start (local_cursor);

	dither = Draw_CachePic_Flags ("gfx/colorcontrol/ditherpattern", CACHEPICFLAG_NOCLAMP);

	M_DrawPic(16, 4, "gfx/qplaque", NO_HOTSPOTS_0, NA0, NA0);
	p0 = Draw_CachePic ("gfx/p_option");
	M_DrawPic((320-Draw_GetPicWidth(p0))/2, 4, "gfx/p_option", NO_HOTSPOTS_0, NA0, NA0);

	visiblerows = (int)((menu_height - 32) / 8);
	drawcur_y = 32 - bound(0, local_cursor - (visiblerows / 2), 
		max(0, local_count - visiblerows)) * 8;

	M_Options_PrintCommand( "     Reset to defaults", true);
	M_Options_PrintSlider(  "                 Gamma", !v_color_enable.integer, v_gamma.value, 1, 5);
	M_Options_PrintSlider(  "              Contrast", !v_color_enable.integer, v_contrast.value, 1, 5);
	M_Options_PrintSlider(  "            Brightness", !v_color_enable.integer, v_brightness.value, 0, 0.8);
	M_Options_PrintCheckbox("  Color Level Controls", true, v_color_enable.integer);
	M_Options_PrintSlider(  "          Black: Red  ", v_color_enable.integer, v_color_black_r.value, 0, 0.8);
	M_Options_PrintSlider(  "          Black: Green", v_color_enable.integer, v_color_black_g.value, 0, 0.8);
	M_Options_PrintSlider(  "          Black: Blue ", v_color_enable.integer, v_color_black_b.value, 0, 0.8);
	M_Options_PrintSlider(  "          Black: Grey ", v_color_enable.integer, (v_color_black_r.value + v_color_black_g.value + v_color_black_b.value) / 3, 0, 0.8);
	M_Options_PrintSlider(  "           Grey: Red  ", v_color_enable.integer, v_color_grey_r.value, 0, 0.95);
	M_Options_PrintSlider(  "           Grey: Green", v_color_enable.integer, v_color_grey_g.value, 0, 0.95);
	M_Options_PrintSlider(  "           Grey: Blue ", v_color_enable.integer, v_color_grey_b.value, 0, 0.95);
	M_Options_PrintSlider(  "           Grey: Grey ", v_color_enable.integer, (v_color_grey_r.value + v_color_grey_g.value + v_color_grey_b.value) / 3, 0, 0.95);
	M_Options_PrintSlider(  "          White: Red  ", v_color_enable.integer, v_color_white_r.value, 1, 5);
	M_Options_PrintSlider(  "          White: Green", v_color_enable.integer, v_color_white_g.value, 1, 5);
	M_Options_PrintSlider(  "          White: Blue ", v_color_enable.integer, v_color_white_b.value, 1, 5);
	M_Options_PrintSlider(  "          White: Grey ", v_color_enable.integer, (v_color_white_r.value + v_color_white_g.value + v_color_white_b.value) / 3, 1, 5);

	drawcur_y += 4;
	DrawQ_Fill(menu_x, menu_y + drawcur_y, 320, 4 + 64 + 8 + 64 + 4, 0, 0, 0, 1, 0);drawcur_y += 4;
	s = (float) 312 / 2 * vid.width / vid_conwidth.integer;
	t = (float) 4 / 2 * vid.height / vid_conheight.integer;
	DrawQ_SuperPic(menu_x + 4, menu_y + drawcur_y, dither, 312, 4, 0,0, 1,0,0,1, s,0, 1,0,0,1, 0,t, 1,0,0,1, s,t, 1,0,0,1, 0);drawcur_y += 4;
	DrawQ_SuperPic(menu_x + 4, menu_y + drawcur_y, NULL  , 312, 4, 0,0, 0,0,0,1, 1,0, 1,0,0,1, 0,1, 0,0,0,1, 1,1, 1,0,0,1, 0);drawcur_y += 4;
	DrawQ_SuperPic(menu_x + 4, menu_y + drawcur_y, dither, 312, 4, 0,0, 0,1,0,1, s,0, 0,1,0,1, 0,t, 0,1,0,1, s,t, 0,1,0,1, 0);drawcur_y += 4;
	DrawQ_SuperPic(menu_x + 4, menu_y + drawcur_y, NULL  , 312, 4, 0,0, 0,0,0,1, 1,0, 0,1,0,1, 0,1, 0,0,0,1, 1,1, 0,1,0,1, 0);drawcur_y += 4;
	DrawQ_SuperPic(menu_x + 4, menu_y + drawcur_y, dither, 312, 4, 0,0, 0,0,1,1, s,0, 0,0,1,1, 0,t, 0,0,1,1, s,t, 0,0,1,1, 0);drawcur_y += 4;
	DrawQ_SuperPic(menu_x + 4, menu_y + drawcur_y, NULL  , 312, 4, 0,0, 0,0,0,1, 1,0, 0,0,1,1, 0,1, 0,0,0,1, 1,1, 0,0,1,1, 0);drawcur_y += 4;
	DrawQ_SuperPic(menu_x + 4, menu_y + drawcur_y, dither, 312, 4, 0,0, 1,1,1,1, s,0, 1,1,1,1, 0,t, 1,1,1,1, s,t, 1,1,1,1, 0);drawcur_y += 4;
	DrawQ_SuperPic(menu_x + 4, menu_y + drawcur_y, NULL  , 312, 4, 0,0, 0,0,0,1, 1,0, 1,1,1,1, 0,1, 0,0,0,1, 1,1, 1,1,1,1, 0);drawcur_y += 4;

	c[0] = menu_options_colorcontrol_correctionvalue.value; // intensity value that should be matched up to a 50% dither to 'correct' quake
	c[1] = c[0];
	c[2] = c[1];
	VID_ApplyGammaToColor(c, c);
	s = (float) 48 / 2 * vid.width / vid_conwidth.integer;
	t = (float) 48 / 2 * vid.height / vid_conheight.integer;
	u = s * 0.5;
	v = t * 0.5;
	drawcur_y += 8;
	x = 4;
	DrawQ_Fill(menu_x + x, menu_y + drawcur_y, 64, 48, c[0], 0, 0, 1, 0);
	DrawQ_SuperPic(menu_x + x + 16, menu_y + drawcur_y + 16, dither, 16, 16, 0,0, 1,0,0,1, s,0, 1,0,0,1, 0,t, 1,0,0,1, s,t, 1,0,0,1, 0);
	DrawQ_SuperPic(menu_x + x + 32, menu_y + drawcur_y + 16, dither, 16, 16, 0,0, 1,0,0,1, u,0, 1,0,0,1, 0,v, 1,0,0,1, u,v, 1,0,0,1, 0);
	x += 80;
	DrawQ_Fill(menu_x + x, menu_y + drawcur_y, 64, 48, 0, c[1], 0, 1, 0);
	DrawQ_SuperPic(menu_x + x + 16, menu_y + drawcur_y + 16, dither, 16, 16, 0,0, 0,1,0,1, s,0, 0,1,0,1, 0,t, 0,1,0,1, s,t, 0,1,0,1, 0);
	DrawQ_SuperPic(menu_x + x + 32, menu_y + drawcur_y + 16, dither, 16, 16, 0,0, 0,1,0,1, u,0, 0,1,0,1, 0,v, 0,1,0,1, u,v, 0,1,0,1, 0);
	x += 80;
	DrawQ_Fill(menu_x + x, menu_y + drawcur_y, 64, 48, 0, 0, c[2], 1, 0);
	DrawQ_SuperPic(menu_x + x + 16, menu_y + drawcur_y + 16, dither, 16, 16, 0,0, 0,0,1,1, s,0, 0,0,1,1, 0,t, 0,0,1,1, s,t, 0,0,1,1, 0);
	DrawQ_SuperPic(menu_x + x + 32, menu_y + drawcur_y + 16, dither, 16, 16, 0,0, 0,0,1,1, u,0, 0,0,1,1, 0,v, 0,0,1,1, u,v, 0,0,1,1, 0);
	x += 80;
	DrawQ_Fill(menu_x + x, menu_y + drawcur_y, 64, 48, c[0], c[1], c[2], 1, 0);
	DrawQ_SuperPic(menu_x + x + 16, menu_y + drawcur_y + 16, dither, 16, 16, 0,0, 1,1,1,1, s,0, 1,1,1,1, 0,t, 1,1,1,1, s,t, 1,1,1,1, 0);
	DrawQ_SuperPic(menu_x + x + 32, menu_y + drawcur_y + 16, dither, 16, 16, 0,0, 1,1,1,1, u,0, 1,1,1,1, 0,v, 1,1,1,1, u,v, 1,1,1,1, 0);

	PPX_DrawSel_End ();
}

static void M_Options_ColorControl_Key(cmd_state_t *cmd, int key, int ascii)
{
	switch (key) {
	case K_MOUSE2: 
		if (Hotspots_DidHit_Slider()) { 
			local_cursor = hotspotx_hover; 
			goto leftus; 
		}
		// fall thru

	case K_ESCAPE:
		M_Menu_Options_Classic_f(cmd);
		break;

	case K_MOUSE1:						
		if (!Hotspots_DidHit () ) { return;	}
		local_cursor = hotspotx_hover;
		// fall thru

	case K_ENTER:
		m_entersound = true;
		switch (local_cursor) {
		case 0:
			Cvar_SetValueQuick(&v_gamma, 1);
			Cvar_SetValueQuick(&v_contrast, 1);
			Cvar_SetValueQuick(&v_brightness, 0);
			Cvar_SetValueQuick(&v_color_enable, 0);
			Cvar_SetValueQuick(&v_color_black_r, 0);
			Cvar_SetValueQuick(&v_color_black_g, 0);
			Cvar_SetValueQuick(&v_color_black_b, 0);
			Cvar_SetValueQuick(&v_color_grey_r, 0);
			Cvar_SetValueQuick(&v_color_grey_g, 0);
			Cvar_SetValueQuick(&v_color_grey_b, 0);
			Cvar_SetValueQuick(&v_color_white_r, 1);
			Cvar_SetValueQuick(&v_color_white_g, 1);
			Cvar_SetValueQuick(&v_color_white_b, 1);
			break;
		default:
			M_Menu_Options_ColorControl_AdjustSliders (1);
			break;
		} // sw
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
		M_Menu_Options_ColorControl_AdjustSliders (-1);
		break;

	case K_RIGHTARROW: 
		M_Menu_Options_ColorControl_AdjustSliders (1);
		break;
	} // sw
}

#undef local_count
#undef local_cursor
#undef visiblerows
