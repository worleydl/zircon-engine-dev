// cl_input_pmove_spectator.c.h

// Print it when we are spectator
// How do we know if we are spectator
// WARP_X_CALLERS_ (CL_ClientMovement_PlayerMove cl.scores[cl.playerentity-1].qw_spectator QW_CL_ProcessUserInfo)

// FTE sends the extensions for Quakeworld in the challenge QW response
//
WARP_X_(QW_TREAT_AS_SPECTATOR)


WARP_X_ (CL_ClientMovement_Physics_Swim, CL_ClientMovement_PlayerMove cl.scores[cl.playerentity-1].qw_spectator)

WARP_X_ (CL_ClientMovement_Move)
static void CL_ClientMovement_Move_PM_SpectatorMove (cl_clientmovement_state_t *s)
{
	int bump;
	double t;
	vec_t f;
	vec3_t neworigin;
	vec3_t primalvelocity;
	trace_t trace;
	CL_ClientMovement_UpdateStatus (s, /*HITT_PLAYERS_1*/ HITT_PLAYERS_1);
	VectorCopy(s->velocity, primalvelocity);

	for (bump = 0, t = s->cmd.clx_frametime; bump < 8 && VectorLength2(s->velocity) > 0; bump++) {
		VectorMA(s->origin, t, s->velocity, neworigin);

		// check if it moved all the way
//		if (trace.fraction == 1)
			break;

		// this is only really needed for nogravityonground combined with gravityunaffectedbyticrate
		// <LadyHavoc> I'm pretty sure I commented it out solely because it seemed redundant
		// this got commented out in a change that supposedly makes the code match QW better
		// so if this is broken, maybe put it in an if (cls.protocol != PROTOCOL_QUAKEWORLD) block
		if (trace.plane.normal[2] > 0.7)
			s->onground = true;

		t -= t * trace.fraction;

		f = DotProduct(s->velocity, trace.plane.normal);
		VectorMA(s->velocity, -f, trace.plane.normal, s->velocity);
	}
	if (s->waterjumptime > 0)
		VectorCopy(primalvelocity, s->velocity);
}

void CL_ClientMovement_Physics_PM_SpectatorMove (cl_clientmovement_state_t *s)
{
	vec_t wishspeed;
//	vec_t f;
	vec3_t wishvel;
	vec3_t wishdir;


	{
		// swim
		vec3_t forward;
		vec3_t right;
		vec3_t up;
		// calculate movement vector
		AngleVectors(s->cmd.viewangles, forward, right, up);
		VectorSet(up, 0, 0, 1);
		VectorMAMAM(s->cmd.clx_forwardmove, forward, s->cmd.clx_sidemove, right, s->cmd.clx_upmove, up, wishvel);
	}

	// split wishvel into wishspeed and wishdir
	VectorCopy(wishvel, wishdir);
	wishspeed = VectorNormalizeLength(wishdir);
	wishspeed = min(wishspeed, cl.movevars_maxspeed) * 0.7;

//	if (s->crouched)
//		wishspeed *= 0.5;


	CL_ClientMovement_Move_PM_SpectatorMove (s);
}


extern cvar_t host_timescale;
void CL_UpdateMoveVars(void)
{
	if (cls.protocol == PROTOCOL_QUAKEWORLD)
	{
		cl.moveflags = 0;
	}
	else if (cl.stats[STAT_MOVEVARS_TICRATE])
	{
		cl.moveflags = cl.stats[STAT_MOVEFLAGS]; // Baker: MOVEFLAG_NOGRAVITYONGROUND and such
		cl.movevars_ticrate = cl.statsf[STAT_MOVEVARS_TICRATE];
		cl.movevars_timescale = cl.statsf[STAT_MOVEVARS_TIMESCALE];
		cl.movevars_gravity = cl.statsf[STAT_MOVEVARS_GRAVITY];
		cl.movevars_stopspeed = cl.statsf[STAT_MOVEVARS_STOPSPEED] ;
		cl.movevars_maxspeed = cl.statsf[STAT_MOVEVARS_MAXSPEED];
		cl.movevars_spectatormaxspeed = cl.statsf[STAT_MOVEVARS_SPECTATORMAXSPEED];
		cl.movevars_accelerate = cl.statsf[STAT_MOVEVARS_ACCELERATE];
		cl.movevars_airaccelerate = cl.statsf[STAT_MOVEVARS_AIRACCELERATE];
		cl.movevars_wateraccelerate = cl.statsf[STAT_MOVEVARS_WATERACCELERATE];
		cl.movevars_entgravity = cl.statsf[STAT_MOVEVARS_ENTGRAVITY];
		cl.movevars_jumpvelocity = cl.statsf[STAT_MOVEVARS_JUMPVELOCITY];
		cl.movevars_edgefriction = cl.statsf[STAT_MOVEVARS_EDGEFRICTION];
		cl.movevars_maxairspeed = cl.statsf[STAT_MOVEVARS_MAXAIRSPEED];
		cl.movevars_stepheight = cl.statsf[STAT_MOVEVARS_STEPHEIGHT];
		cl.movevars_airaccel_qw = cl.statsf[STAT_MOVEVARS_AIRACCEL_QW];
		cl.movevars_airaccel_qw_stretchfactor = cl.statsf[STAT_MOVEVARS_AIRACCEL_QW_STRETCHFACTOR];
		cl.movevars_airaccel_sideways_friction = cl.statsf[STAT_MOVEVARS_AIRACCEL_SIDEWAYS_FRICTION];
		cl.movevars_friction = cl.statsf[STAT_MOVEVARS_FRICTION];
		cl.movevars_wallfriction = cl.statsf[STAT_MOVEVARS_WALLFRICTION];
		cl.movevars_waterfriction = cl.statsf[STAT_MOVEVARS_WATERFRICTION];
		cl.movevars_airstopaccelerate = cl.statsf[STAT_MOVEVARS_AIRSTOPACCELERATE];
		cl.movevars_airstrafeaccelerate = cl.statsf[STAT_MOVEVARS_AIRSTRAFEACCELERATE];
		cl.movevars_maxairstrafespeed = cl.statsf[STAT_MOVEVARS_MAXAIRSTRAFESPEED];
		cl.movevars_airstrafeaccel_qw = cl.statsf[STAT_MOVEVARS_AIRSTRAFEACCEL_QW];
		cl.movevars_aircontrol = cl.statsf[STAT_MOVEVARS_AIRCONTROL];
		cl.movevars_aircontrol_power = cl.statsf[STAT_MOVEVARS_AIRCONTROL_POWER];
		cl.movevars_aircontrol_penalty = cl.statsf[STAT_MOVEVARS_AIRCONTROL_PENALTY];
		cl.movevars_warsowbunny_airforwardaccel = cl.statsf[STAT_MOVEVARS_WARSOWBUNNY_AIRFORWARDACCEL];
		cl.movevars_warsowbunny_accel = cl.statsf[STAT_MOVEVARS_WARSOWBUNNY_ACCEL];
		cl.movevars_warsowbunny_topspeed = cl.statsf[STAT_MOVEVARS_WARSOWBUNNY_TOPSPEED];
		cl.movevars_warsowbunny_turnaccel = cl.statsf[STAT_MOVEVARS_WARSOWBUNNY_TURNACCEL];
		cl.movevars_warsowbunny_backtosideratio = cl.statsf[STAT_MOVEVARS_WARSOWBUNNY_BACKTOSIDERATIO];
		cl.movevars_airspeedlimit_nonqw = cl.statsf[STAT_MOVEVARS_AIRSPEEDLIMIT_NONQW];
	}
	else
	{
		cl.moveflags = 0;
		cl.movevars_ticrate = (cls.demoplayback ? 1.0f : host_timescale.value) / bound(10.0f, cl_netfps.value, 1000.0f);
		cl.movevars_timescale = (cls.demoplayback ? 1.0f : host_timescale.value);
		cl.movevars_gravity = sv_gravity.value;
		cl.movevars_stopspeed = cl_movement_stopspeed.value;
		cl.movevars_maxspeed = cl_movement_maxspeed.value;
		cl.movevars_spectatormaxspeed = cl_movement_maxspeed.value;
		cl.movevars_accelerate = cl_movement_accelerate.value;
		cl.movevars_airaccelerate = cl_movement_airaccelerate.value < 0 ? cl_movement_accelerate.value : cl_movement_airaccelerate.value;
		cl.movevars_wateraccelerate = cl_movement_wateraccelerate.value < 0 ? cl_movement_accelerate.value : cl_movement_wateraccelerate.value;
		cl.movevars_friction = cl_movement_friction.value;
		cl.movevars_wallfriction = cl_movement_wallfriction.value;
		cl.movevars_waterfriction = cl_movement_waterfriction.value < 0 ? cl_movement_friction.value : cl_movement_waterfriction.value;
		cl.movevars_entgravity = 1;
		cl.movevars_jumpvelocity = cl_movement_jumpvelocity.value;
		cl.movevars_edgefriction = cl_movement_edgefriction.value;
		cl.movevars_maxairspeed = cl_movement_maxairspeed.value;
		cl.movevars_stepheight = cl_movement_stepheight.value;
		cl.movevars_airaccel_qw = cl_movement_airaccel_qw.value;
		cl.movevars_airaccel_qw_stretchfactor = 0;
		cl.movevars_airaccel_sideways_friction = cl_movement_airaccel_sideways_friction.value;
		cl.movevars_airstopaccelerate = 0;
		cl.movevars_airstrafeaccelerate = 0;
		cl.movevars_maxairstrafespeed = 0;
		cl.movevars_airstrafeaccel_qw = 0;
		cl.movevars_aircontrol = 0;
		cl.movevars_aircontrol_power = 2;
		cl.movevars_aircontrol_penalty = 0;
		cl.movevars_warsowbunny_airforwardaccel = 0;
		cl.movevars_warsowbunny_accel = 0;
		cl.movevars_warsowbunny_topspeed = 0;
		cl.movevars_warsowbunny_turnaccel = 0;
		cl.movevars_warsowbunny_backtosideratio = 0;
		cl.movevars_airspeedlimit_nonqw = 0;
	}

	if (!(cl.moveflags & MOVEFLAG_VALID))
	{
		if (gamemode == GAME_NEXUIZ)  // Legacy hack to work with old servers of Nexuiz.
			cl.moveflags = MOVEFLAG_Q2AIRACCELERATE;
	}

	if (cl.movevars_aircontrol_power <= 0)
		cl.movevars_aircontrol_power = 2; // CPMA default
}

