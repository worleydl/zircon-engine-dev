#ifndef QSTATS_H
#define QSTATS_H

#define QW_TREAT_AS_CL_MOVEMENT ((cl_movement.integer && !sv.active) || cl_movement.integer > 1)
#define QW_TREAT_AS_SPECTATOR	(cls.netcon && cls.protocol == PROTOCOL_QUAKEWORLD && \
			(cl.scores[cl.playerentity-1].qw_spectator || cl.stats[STAT_HEALTH] <= 0 || cl.stats[STAT_HEALTH] >= 1000) )
//
// stats are integers communicated to the client by the server
//
#define	MAX_CL_STATS		256
#define	STAT_HEALTH			0
//#define	STAT_FRAGS			1
#define	STAT_WEAPON			2
#define	STAT_AMMO			3
#define	STAT_ARMOR			4
#define	STAT_WEAPONFRAME	5
#define	STAT_SHELLS			6
#define	STAT_NAILS			7
#define	STAT_ROCKETS		8
#define	STAT_CELLS			9
#define	STAT_ACTIVEWEAPON	10
#define	STAT_TOTALSECRETS	11
#define	STAT_TOTALMONSTERS	12
#define	STAT_SECRETS		13		///< bumped on client side by svc_foundsecret
#define	STAT_MONSTERS		14		///< bumped by svc_killedmonster
#define STAT_ITEMS			15 ///< FTE, DP
#define STAT_VIEWHEIGHT		16 ///< FTE, DP
//#define STAT_TIME			17 ///< FTE
//#define STAT_VIEW2		20 ///< FTE
#define STAT_VIEWZOOM_21	21 ///< DP
#define MIN_VM_STAT_32      32 ///< stat range available to VM_SV_AddStat
#define MAX_VM_STAT_220     220 ///< stat range available to VM_SV_AddStat
#define STAT_MOVEVARS_AIRACCEL_QW_STRETCHFACTOR 220 ///< DP
#define STAT_MOVEVARS_AIRCONTROL_PENALTY					221 ///< DP
#define STAT_MOVEVARS_AIRSPEEDLIMIT_NONQW 222 ///< DP
#define STAT_MOVEVARS_AIRSTRAFEACCEL_QW 223 ///< DP
#define STAT_MOVEVARS_AIRCONTROL_POWER					224 ///< DP
#define STAT_MOVEFLAGS                              225 ///< DP
#define STAT_MOVEVARS_WARSOWBUNNY_AIRFORWARDACCEL	226 ///< DP
#define STAT_MOVEVARS_WARSOWBUNNY_ACCEL				227 ///< DP
#define STAT_MOVEVARS_WARSOWBUNNY_TOPSPEED			228 ///< DP
#define STAT_MOVEVARS_WARSOWBUNNY_TURNACCEL			229 ///< DP
#define STAT_MOVEVARS_WARSOWBUNNY_BACKTOSIDERATIO	230 ///< DP
#define STAT_MOVEVARS_AIRSTOPACCELERATE				231 ///< DP
#define STAT_MOVEVARS_AIRSTRAFEACCELERATE			232 ///< DP
#define STAT_MOVEVARS_MAXAIRSTRAFESPEED				233 ///< DP
#define STAT_MOVEVARS_AIRCONTROL					234 ///< DP
#define STAT_FRAGLIMIT								235 ///< DP
#define STAT_TIMELIMIT								236 ///< DP
#define STAT_MOVEVARS_WALLFRICTION					237 ///< DP
#define STAT_MOVEVARS_FRICTION						238 ///< DP
#define STAT_MOVEVARS_WATERFRICTION					239 ///< DP
#define STAT_MOVEVARS_TICRATE						240 ///< DP
#define STAT_MOVEVARS_TIMESCALE						241 ///< DP
#define STAT_MOVEVARS_GRAVITY						242 ///< DP
#define STAT_MOVEVARS_STOPSPEED						243 ///< DP
#define STAT_MOVEVARS_MAXSPEED						244 ///< DP
#define STAT_MOVEVARS_SPECTATORMAXSPEED				245 ///< DP
#define STAT_MOVEVARS_ACCELERATE					246 ///< DP
#define STAT_MOVEVARS_AIRACCELERATE					247 ///< DP
#define STAT_MOVEVARS_WATERACCELERATE				248 ///< DP
#define STAT_MOVEVARS_ENTGRAVITY					249 ///< DP
#define STAT_MOVEVARS_JUMPVELOCITY					250 ///< DP
#define STAT_MOVEVARS_EDGEFRICTION					251 ///< DP
#define STAT_MOVEVARS_MAXAIRSPEED					252 ///< DP
#define STAT_MOVEVARS_STEPHEIGHT					253 ///< DP
#define STAT_MOVEVARS_AIRACCEL_QW					254 ///< DP
#define STAT_MOVEVARS_AIRACCEL_SIDEWAYS_FRICTION	255 ///< DP

#endif
