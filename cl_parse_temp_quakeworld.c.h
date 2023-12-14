// cl_parse_temp_quakeworld.c.h
	switch (type) {
		case QW_TE_WIZSPIKE:
			// spike hitting wall
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			CL_FindNonSolidLocation(pos, pos, 4);
			CL_ParticleEffect(EFFECT_TE_WIZSPIKE, 1, pos, pos, vec3_origin, vec3_origin, NULL, 0);
			S_StartSound(-1, 0, cl.sfx_wizhit, pos, 1, 1);
			break;

		case QW_TE_KNIGHTSPIKE:
			// spike hitting wall
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			CL_FindNonSolidLocation(pos, pos, 4);
			CL_ParticleEffect(EFFECT_TE_KNIGHTSPIKE, 1, pos, pos, vec3_origin, vec3_origin, NULL, 0);
			S_StartSound(-1, 0, cl.sfx_knighthit, pos, 1, 1);
			break;

		case QW_TE_SPIKE:
			// spike hitting wall
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			CL_FindNonSolidLocation(pos, pos, 4);
			CL_ParticleEffect(EFFECT_TE_SPIKE, 1, pos, pos, vec3_origin, vec3_origin, NULL, 0);
			if (rand() % 5)
				S_StartSound(-1, 0, cl.sfx_tink1, pos, 1, 1);
			else
			{
				rnd = rand() & 3;
				if (rnd == 1)
					S_StartSound(-1, 0, cl.sfx_ric1, pos, 1, 1);
				else if (rnd == 2)
					S_StartSound(-1, 0, cl.sfx_ric2, pos, 1, 1);
				else
					S_StartSound(-1, 0, cl.sfx_ric3, pos, 1, 1);
			}
			break;
		case QW_TE_SUPERSPIKE:
			// super spike hitting wall
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			CL_FindNonSolidLocation(pos, pos, 4);
			CL_ParticleEffect(EFFECT_TE_SUPERSPIKE, 1, pos, pos, vec3_origin, vec3_origin, NULL, 0);
			if (rand() % 5)
				S_StartSound(-1, 0, cl.sfx_tink1, pos, 1, 1);
			else
			{
				rnd = rand() & 3;
				if (rnd == 1)
					S_StartSound(-1, 0, cl.sfx_ric1, pos, 1, 1);
				else if (rnd == 2)
					S_StartSound(-1, 0, cl.sfx_ric2, pos, 1, 1);
				else
					S_StartSound(-1, 0, cl.sfx_ric3, pos, 1, 1);
			}
			break;

		case QW_TE_EXPLOSION:
			// rocket explosion
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			CL_FindNonSolidLocation(pos, pos, 10);
			CL_ParticleEffect(EFFECT_TE_EXPLOSION, 1, pos, pos, vec3_origin, vec3_origin, NULL, 0);
			S_StartSound(-1, 0, cl.sfx_r_exp3, pos, 1, 1);
			CL_Effect(pos, CL_GetModelByIndex(cl.qw_modelindex_s_explod), 0, 6, 10);
			break;

		case QW_TE_TAREXPLOSION:
			// tarbaby explosion
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			CL_FindNonSolidLocation(pos, pos, 10);
			CL_ParticleEffect(EFFECT_TE_TAREXPLOSION, 1, pos, pos, vec3_origin, vec3_origin, NULL, 0);
			S_StartSound(-1, 0, cl.sfx_r_exp3, pos, 1, 1);
			break;

		case QW_TE_LIGHTNING1:
			// lightning bolts
			CL_ParseBeam(cl.model_bolt, true);
			break;

		case QW_TE_LIGHTNING2:
			// lightning bolts
			CL_ParseBeam(cl.model_bolt2, true);
			break;

		case QW_TE_LIGHTNING3:
			// lightning bolts
			CL_ParseBeam(cl.model_bolt3, false);
			break;

		case QW_TE_LAVASPLASH:
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			CL_ParticleEffect(EFFECT_TE_LAVASPLASH, 1, pos, pos, vec3_origin, vec3_origin, NULL, 0);
			break;

		case QW_TE_TELEPORT:
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			CL_ParticleEffect(EFFECT_TE_TELEPORT, 1, pos, pos, vec3_origin, vec3_origin, NULL, 0);
			break;

		case QW_TE_GUNSHOT:
			// bullet hitting wall
			radius = MSG_ReadByte(&cl_message);
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			CL_FindNonSolidLocation(pos, pos, 4);
			VectorSet(pos2, pos[0] + radius, pos[1] + radius, pos[2] + radius);
			VectorSet(pos, pos[0] - radius, pos[1] - radius, pos[2] - radius);
			CL_ParticleEffect(EFFECT_TE_GUNSHOT, radius, pos, pos2, vec3_origin, vec3_origin, NULL, 0);
			if (cl_sound_ric_gunshot.integer & RIC_GUNSHOT)
			{
				if (rand() % 5)
					S_StartSound(-1, 0, cl.sfx_tink1, pos, 1, 1);
				else
				{
					rnd = rand() & 3;
					if (rnd == 1)
						S_StartSound(-1, 0, cl.sfx_ric1, pos, 1, 1);
					else if (rnd == 2)
						S_StartSound(-1, 0, cl.sfx_ric2, pos, 1, 1);
					else
						S_StartSound(-1, 0, cl.sfx_ric3, pos, 1, 1);
				}
			}
			break;

		case QW_TE_BLOOD:
			count = MSG_ReadByte(&cl_message);
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			CL_FindNonSolidLocation(pos, pos, 4);
			CL_ParticleEffect(EFFECT_TE_BLOOD, count, pos, pos, vec3_origin, vec3_origin, NULL, 0);
			break;

		case QW_TE_LIGHTNINGBLOOD:
			MSG_ReadVector(&cl_message, pos, cls.protocol);
			CL_FindNonSolidLocation(pos, pos, 4);
			CL_ParticleEffect(EFFECT_TE_BLOOD, 2.5, pos, pos, vec3_origin, vec3_origin, NULL, 0);
			break;

		default:
			Host_Error_Line ("CL_ParseTempEntity: bad type %d (hex %02X)", type, type);
		}
