// sv_user_zmove.c.h

void SV_PhysicsX_Zircon_Warp_Start (client_t *hcl, const char *s_reason)
{
	if (hcl->movesequence == 0) {
		hcl->zircon_warp_sequence = 0; //host_client->movesequence;
		hcl->zircon_warp_sequence_cleared = 0;
		hcl->last_wanted_is_zircon_free_move = false;
		return;
//		int j = 5;
//		
//		host_client->zircon_warp_sequence = host_client->movesequence;
//		host_client->zircon_warp_sequence_cleared = 0;
	}
	if (Have_Flag (developer_movement.integer, /*SV*/ 2))
		Con_PrintLinef ("SV: Warp start -> reason " QUOTED_S " seq %u", s_reason, host_client->movesequence);
	hcl->last_wanted_is_zircon_free_move = false;
	hcl->zircon_warp_sequence = hcl->movesequence;
	hcl->zircon_warp_sequence_cleared = 0;

}

void Zircon_Warp_Clear (void)
{

}

void SV_ReadClientMessage_Zircon_Warp_Ack_Read (void)
{
	unsigned int zircon_warp_ack_seq = MSG_ReadLong (&sv_message); // ZMOVE RECEIVE END MOVE

	if (zircon_warp_ack_seq == ALL_FLAGS_ANTIZERO) {
		// Client can't clear this one
		return;
	}
	if (zircon_warp_ack_seq == host_client->zircon_warp_sequence) { 
		if (Have_Flag (developer_movement.integer, /*SV*/ 2))
			Con_PrintLinef ("SV: Received Client ack warp OK %u", (int)zircon_warp_ack_seq);
		host_client->zircon_warp_sequence = 0; // ZMOVE
		host_client->zircon_warp_sequence_cleared = 3;
		//SV_ReadClientMessage_ReadClientMove ();
	} else {
		//Con_PrintLinef ("Invalid zircon_warp ack received %u expected %u", zircon_warp_ack_seq, host_client->zircon_warp_sequence);

	}

}

WARP_X_ (SV_UpdateToReliableMessages ZMOVE_SV_DENIED)
void SV_UpdateToReliableMessages_Zircon_Warp_Think (void)
{
	if (false == Have_Zircon_Ext_Flag_SV_HCL (ZIRCON_EXT_FREEMOVE_4)) {
		if (Have_Flag (developer_movement.integer, /*SV*/ 2))
			Con_PrintLinef ("SV: SV_UpdateToReliableMessages_Zircon_Warp_Think: CL no ZMOVE");
		return;
	}

	if (host_client->zircon_warp_sequence) {
		if (Have_Flag (developer_movement.integer, /*SV*/ 2))
			Con_PrintLinef ("SV: svc_zircon_warp START %u", host_client->zircon_warp_sequence);

		MSG_WriteByte (&host_client->netconnection->message, svc_zircon_warp); // ZMOVE
		MSG_WriteLong (&host_client->netconnection->message, host_client->zircon_warp_sequence);
	}

	if (host_client->zircon_warp_sequence_cleared) { // ZMOVE_WARP: SV TO CL
		if (host_client->zircon_warp_sequence) {
			int j = 5; // Not supposed to ever happen
		}
		host_client->zircon_warp_sequence_cleared --;
		if (host_client->zircon_warp_sequence_cleared == 0) {
			if (Have_Flag (developer_movement.integer, /*SV*/ 2))
				Con_PrintLinef ("SV: svc_zircon_warp CLEAR %u:count %d", host_client->zircon_warp_sequence, host_client->zircon_warp_sequence_cleared);
			MSG_WriteByte (&host_client->netconnection->message, svc_zircon_warp); // ZMOVE
			MSG_WriteLong (&host_client->netconnection->message, host_client->zircon_warp_sequence);
			if (Have_Flag (developer_movement.integer, /*SV*/ 2))
				Con_PrintLinef ("SV: Warp Cleared");
		} // Cleared
	} //

}


WARP_X_ (SV_SpawnServer)
// This occurs when a client connects (or reconnects)
void SV_Begin_f_Zircon_Warp_Initialize (void)
{
	Con_PrintLinef ("SV_Begin_f_Zircon_Warp_Initialize");
	host_client->zircon_warp_sequence = 0;
	host_client->zircon_warp_sequence_cleared = 0;
}
