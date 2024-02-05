#include "quakedef.h"
#include "protocol.h"

WARP_X_ (EntityFrame5_CL_ReadFrame)
void CL_ParseUpdate_Fitz (entity_t *ent, entity_state_t *s, int bits)
{
	// Baker: Everything reset to baseline already
//	int		i;
	int		modnum = -1;
//	int		skin;
	int		modread = false;

	if (bits & U_MODEL) {
		modnum = MSG_ReadByte (&cl_message);
		if (modnum >= QUAKESPASM_MAXMODELS_2048)
			Host_Error_Line ("CL_ParseModel: bad modnum");
		modread = true;
	}

	if (bits & U_FRAME)					s->frame = MSG_ReadByte (&cl_message);
	if (bits & U_COLORMAP)				s->colormap = MSG_ReadByte(&cl_message);
	if (bits & U_SKIN)					s->skin = MSG_ReadByte(&cl_message);
	if (bits & U_EFFECTS)				s->effects = MSG_ReadByte(&cl_message);
	if (bits & U_ORIGIN1)				s->origin[0] = MSG_ReadCoord(&cl_message, cls.protocol);
	if (bits & U_ANGLE1)				s->angles[0] = MSG_ReadAngle(&cl_message, cls.protocol);
	if (bits & U_ORIGIN2)				s->origin[1] = MSG_ReadCoord(&cl_message, cls.protocol);
	if (bits & U_ANGLE2)				s->angles[1] = MSG_ReadAngle(&cl_message, cls.protocol);
	if (bits & U_ORIGIN3)				s->origin[2] = MSG_ReadCoord(&cl_message, cls.protocol);
	if (bits & U_ANGLE3)				s->angles[2] = MSG_ReadAngle(&cl_message, cls.protocol);

	//johnfitz -- lerping for movetype_step entities
	//if (bits & U_STEP)
	//{
	//	ent->lerpflags |= LERP_MOVESTEP;
	//	ent->forcelink = true;
	//}
	//else
	//	ent->lerpflags &= ~LERP_MOVESTEP;
	//johnfitz

	//johnfitz -- PROTOCOL_FITZQUAKE and PROTOCOL_NEHAHRA
	if (bits & U_FITZALPHA_S16) {
		s->alpha = MSG_ReadByte(&cl_message);
		if (s->alpha == 0) s->alpha = 255;
	}
	if (bits & U_RMQ_SCALE_S20) {
			s->scale = MSG_ReadByte(&cl_message);
			if (s->scale == 0) s->scale = 16;
	}
	if (bits & U_FITZFRAME2_S17)
		s->frame = (s->frame & 0x00FF) | (MSG_ReadByte(&cl_message) << 8);
	if (bits & U_FITZMODEL2_S18) {
		modnum = (modnum & 0x00FF) | (MSG_ReadByte(&cl_message) << 8);
		modread = true;
	}
	if (modread)
		s->modelindex = modnum;
	if (Have_Flag (bits, U_FITZLERPFINISH_S19)) {
#if 0
		int lerpfinish = ((float)(MSG_ReadByte(&cl_message)) / 255);
#else
		MSG_ReadByte(&cl_message); // Discard, we don't use it
#endif
		//ent->lerpflags |= LERP_FINISH;
	}	
	//johnfitz

	////johnfitz -- moved here from above
	//model = cl.model_precache[modnum];
	//if (model != ent->model)
	//{
	//	ent->model = model;
	//// automatic animation (torches, etc) can be either all together
	//// or randomized
	//	if (model)
	//	{
	//		if (model->synctype == ST_RAND)
	//			ent->syncbase = (float)(rand()&0x7fff) / 0x7fff;
	//		else
	//			ent->syncbase = 0.0;
	//	}
	//	else
	//		forcelink = true;	// hack to make null model players work
	//	if (num > 0 && num <= cl.maxclients)
	//		R_TranslateNewPlayerSkin (num - 1); //johnfitz -- was R_TranslatePlayerSkin

	//	ent->lerpflags |= LERP_RESETANIM; //johnfitz -- don't lerp animation across model changes
	//}
	////johnfitz

	//if ( forcelink )
	//{	// didn't have an update last message
	//	VectorCopy (ent->msg_origins[0], ent->msg_origins[1]);
	//	VectorCopy (ent->msg_origins[0], ent->origin);
	//	VectorCopy (ent->msg_angles[0], ent->msg_angles[1]);
	//	VectorCopy (ent->msg_angles[0], ent->angles);
	//	ent->forcelink = true;
	//}

}

// FITZQUAKE EQUIVALENT: CL_ParseUpdate ?
WARP_X_ (EntityFrameQuake_WriteFrame)
void EntityFrameQuake_ReadEntity(int bits)
{
	int num;
	entity_t *ent;
	entity_state_t s;

	if (Have_Flag (bits, U_MOREBITS_1))
		bits |= (MSG_ReadByte(&cl_message)<<8);


	// FITZQUAKE LOOKS EQUIVALENT
	if (isin2 (cls.protocol, PROTOCOL_FITZQUAKE666, PROTOCOL_FITZQUAKE999)) {
		// Baker: These might be equivalent but the logic is not the same.
		if (bits & U_EXTEND1_S15)
			bits |= MSG_ReadByte(&cl_message) << 16;

		if (bits & U_EXTEND2_S23)
			bits |= MSG_ReadByte(&cl_message) << 24;
	} 
	else 
	if (Have_Flag (bits, U_EXTEND1_S15) && cls.protocol != PROTOCOL_NEHAHRAMOVIE) {
		bits |= MSG_ReadByte(&cl_message) << 16;
		if (bits & U_EXTEND2_S23)
			bits |= MSG_ReadByte(&cl_message) << 24;
	}

	if (Have_Flag (bits, U_LONGENTITY))
		num = (unsigned short) MSG_ReadShort(&cl_message);
	else
		num = MSG_ReadByte(&cl_message);

	if (num >= MAX_EDICTS_32768)
		Host_Error_Line ("EntityFrameQuake_ReadEntity: entity number (%d) >= MAX_EDICTS_32768 (%d)", num, MAX_EDICTS_32768);
	if (num < 1)
		Host_Error_Line ("EntityFrameQuake_ReadEntity: invalid entity number (%d)", num);

	if (cl.num_entities <= num) {
		cl.num_entities = num + 1;
		if (num >= cl.max_entities)
			CL_ExpandEntities(num);
	}

	ent = cl.entities + num;

	// note: this inherits the 'active' state of the baseline chosen
	// (state_baseline is always active, state_current may not be active if
	// the entity was missing in the last frame)

	// Baker: FitzQuake bits here can collide with U_DELTA
	if (Have_Flag (bits, U_DELTA_S16) && 
			 false == isin2(cls.protocol, PROTOCOL_FITZQUAKE666, PROTOCOL_FITZQUAKE999)) {
		s = ent->state_current;
	}
	else
	{
		s = ent->state_baseline;
		s.active = ACTIVE_NETWORK;
	}

	cl.isquakeentity[num] = true;
	if (cl.lastquakeentity < num)
		cl.lastquakeentity = num;
	s.number = num;
	s.time = cl.mtime[0];
	s.sflags = 0;

	// FITZQUAKE has lerp flag here
	// if (ent->msgtime + 0.2 < cl.mtime[0]) //more than 0.2 seconds since the last message (most entities think every 0.1 sec)
	//	ent->lerpflags |= LERP_RESETANIM; //if we missed a think, we'd be lerping from the wrong frame

	if (isin2 (cls.protocol, PROTOCOL_FITZQUAKE666, PROTOCOL_FITZQUAKE999)) {
		CL_ParseUpdate_Fitz (ent, &s, bits);
		goto fitzquake_bypass;
	}

	// FitzQuake does not come here ...
	if (Have_Flag (bits, U_MODEL)) {
		if (isin3(cls.protocol, PROTOCOL_NEHAHRABJP, PROTOCOL_NEHAHRABJP2, PROTOCOL_NEHAHRABJP3))
							s.modelindex = (unsigned short) MSG_ReadShort(&cl_message);
		else
							s.modelindex = (s.modelindex & 0xFF00) | MSG_ReadByte(&cl_message);
	}
	if (bits & U_FRAME)		s.frame = (s.frame & 0xFF00) | MSG_ReadByte(&cl_message);
	if (bits & U_COLORMAP)	s.colormap = MSG_ReadByte(&cl_message);
	if (bits & U_SKIN)		s.skin = MSG_ReadByte(&cl_message);
	if (bits & U_EFFECTS)	s.effects = (s.effects & 0xFF00) | MSG_ReadByte(&cl_message);
	if (bits & U_ORIGIN1)	s.origin[0] = MSG_ReadCoord(&cl_message, cls.protocol);
	if (bits & U_ANGLE1)	s.angles[0] = MSG_ReadAngle(&cl_message, cls.protocol);
	if (bits & U_ORIGIN2)	s.origin[1] = MSG_ReadCoord(&cl_message, cls.protocol);
	if (bits & U_ANGLE2)	s.angles[1] = MSG_ReadAngle(&cl_message, cls.protocol);
	if (bits & U_ORIGIN3)	s.origin[2] = MSG_ReadCoord(&cl_message, cls.protocol);
	if (bits & U_ANGLE3)	s.angles[2] = MSG_ReadAngle(&cl_message, cls.protocol);

	if (bits & U_STEP)		s.sflags |= RENDER_STEP;

	// Baker: FitzQuake collides with U_DELTA_S16, U_ALPHA_S17, U_SCALE_S18, U_EFFECTS2_S19
	if (bits & U_ALPHA_S17)		s.alpha = MSG_ReadByte(&cl_message);
	if (bits & U_SCALE_S18)		s.scale = MSG_ReadByte(&cl_message);
	if (bits & U_EFFECTS2_S19)	s.effects = (s.effects & 0x00FF) | (MSG_ReadByte(&cl_message) << 8);

	if (bits & U_GLOWSIZE_S20)	s.glowsize = MSG_ReadByte(&cl_message);
	if (bits & U_GLOWCOLOR)		s.glowcolor = MSG_ReadByte(&cl_message);
	if (bits & U_COLORMOD)		{int c = MSG_ReadByte(&cl_message);s.colormod[0] = (unsigned char)(((c >> 5) & 7) * (32.0f / 7.0f));s.colormod[1] = (unsigned char)(((c >> 2) & 7) * (32.0f / 7.0f));s.colormod[2] = (unsigned char)((c & 3) * (32.0f / 3.0f));}
	if (bits & U_GLOWTRAIL)		s.sflags |= RENDER_GLOWTRAIL;
	if (bits & U_FRAME2_S26)	s.frame = (s.frame & 0x00FF) | (MSG_ReadByte(&cl_message) << 8);
	if (bits & U_MODEL2_S27)	s.modelindex = (s.modelindex & 0x00FF) | (MSG_ReadByte(&cl_message) << 8);
	if (bits & U_VIEWMODEL)		s.sflags |= RENDER_VIEWMODEL;
	if (bits & U_EXTERIORMODEL)	s.sflags |= RENDER_EXTERIORMODEL;

	// LadyHavoc: to allow playback of the Nehahra movie
	if (cls.protocol == PROTOCOL_NEHAHRAMOVIE && Have_Flag(bits, U_EXTEND1_S15)) {
		// LadyHavoc: evil format
		int i = (int)MSG_ReadFloat(&cl_message);
		int j = (int)(MSG_ReadFloat(&cl_message) * 255.0f);
		if (i == 2)
		{
			i = (int)MSG_ReadFloat(&cl_message);
			if (i)
				s.effects |= EF_FULLBRIGHT;
		}
		if (j < 0)
			s.alpha = 0;
		else if (j == 0 || j >= 255)
			s.alpha = 255;
		else
			s.alpha = j;
	}
fitzquake_bypass:
	ent->state_previous = ent->state_current;
	ent->state_current = s;
	if (ent->state_current.active == ACTIVE_NETWORK)
	{
		CL_MoveLerpEntityStates(ent);
		cl.entities_active[ent->state_current.number] = true;
	}

	if (cl_message.badread)
		Host_Error_Line ("EntityFrameQuake_ReadEntity: read error");
}

void EntityFrameQuake_ISeeDeadEntities(void)
{
	int num, lastentity;
	if (cl.lastquakeentity == 0)
		return;
	lastentity = cl.lastquakeentity;
	cl.lastquakeentity = 0;
	for (num = 0;num <= lastentity;num++)
	{
		if (cl.isquakeentity[num])
		{
			if (cl.entities_active[num] && cl.entities[num].state_current.time == cl.mtime[0])
			{
				cl.isquakeentity[num] = true;
				cl.lastquakeentity = num;
			}
			else
			{
				cl.isquakeentity[num] = false;
				cl.entities_active[num] = ACTIVE_NOT;
				cl.entities[num].state_current = defaultstate;
				cl.entities[num].state_current.number = num;
			}
		}
	}
}