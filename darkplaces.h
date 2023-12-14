/*
Copyright (C) 2020 Ashley Rose Hale (LadyHavoc)
Copyright (C) 2020 David Knapp (Cloudwalk)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

/* darkplaces.h - Master definitions file for Darkplaces engine */

#ifndef DARKPLACES_H
#define DARKPLACES_H

extern const char *buildstring;
extern const char *buildstringshort; // Baker r8002: Zircon console name
extern char engineversion[128];
extern char engineversionshort[128]; // Baker r8002: Zircon console name



#ifdef __APPLE__
# include <TargetConditionals.h>
#endif

#include <sys/types.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "sys.h"
#include "qtypes.h"
#include "qdefs.h"
#include "zone.h"
#include "thread.h"
#include "com_game.h"
#include "com_infostring.h"
#include "baker.h"

// Baker: Function call clarity ...

#define ALL_FLAGS_ANTIZERO				(~0)

#define fs_caseless_true				true
#define fs_quiet_true					true
#define fs_quiet_FALSE					false

#define fs_size_ptr_null				NULL
#define fs_pakfile_null					NULL
#define fs_package_index_reply_null		NULL

#define fs_nonblocking_false			false	// Baker: I haven't seen a blocking one yet ...

#define fs_all_files_empty_string		""
#define fs_make_unique_true				true
#define fs_make_unique_false			false
#define fs_one_per_line_true			true
#define fs_one_per_line_false			false
#define fs_reply_already_loaded_null	NULL
#define fs_keep_plain_dirs_false		false
#define fs_is_dlcache_false				false

#define fs_gamedironly_false			false
#define fs_gamedironly_true				true

//#define fs_loadinfo_in_null			NULL
//#define fs_loadinfo_out_null			NULL
#define q_is_large_modelindex_true		true
#define q_is_large_modelindex_false		false
#define q_is_large_soundindex_false		false
#define q_is_large_soundindex_true		true
#define q_undo_action_normal_0			0
#define q_undo_action_add_1				1
#define q_undo_action_delete_neg_1		-1
#define q_netchange_zero				0

#define q_is_from_nothing_false			false
#define q_is_from_nothing_true			true
#define q_s_loadgame_NULL				NULL

#define q_vm_wildcard_NULL				NULL
#define q_vm_classname_NULL				NULL
#define q_vm_targetname_NULL			NULL
#define q_vm_printfree_true				true
#define q_vm_printfree_false			false
#define q_is_console_true				true

#define q_is_quakeworld_true			true
#define q_is_quakeworld_false			false

#define q_is_doublewidth_true			true
#define q_is_doublewidth_false			false

#define q_was_a_space_false				false
#define q_was_a_space_true				true

#define q_is_menu_fill_false			false
#define q_is_menu_fill_true				true
#define q_is_zautocomplete_true			true
#define q_is_zautocomplete_false		false
#define q_is_suppress_print_true		true
#define q_is_suppress_print_false		false
#define q_darken_true					true
#define q_darken_false					false
#define q_strip_exten_true				true
#define q_strip_exten_false				false
#define q_reply_buf_NULL				NULL
#define q_reply_size_0					0
#define q_is_fence_model_false			false // Quake .mdl with MF_FENCE 16384 set means color 255 is transparent
#define q_is_sky_load_false				false

#define q_mouse_relative_false			false
#define q_mouse_hidecursor_false		false

#define q_is_kicked_true				true
#define q_is_kicked_false				false
#define q_is_leaving_false				false

#define q_tx_complain_false				false
#define q_tx_complain_true				true
#define q_tx_fallback_notexture_false	false
#define q_tx_fallback_notexture_true	true
#define q_tx_do_external_true			true
#define q_tx_do_external_false			false
#define q_tx_is_sRGB_false				false
#define q_tx_allowfixtrans_false		false
#define q_tx_allowfixtrans_true			true
#define q_tx_convertsrgb_false			false
#define q_tx_miplevel_null				NULL
#define q_tx_warn_missing_true			true
#define q_tx_warn_missing_false			false

#define q_fail_on_missing_false			false
#define q_fail_on_missing_true			true


#define q_write_depth_false				false		// GL_DepthMask
#define q_write_depth_true				true		// GL_DepthMask
#define q_prepass_false					false
#define q_is_ui_fog_ignore_false		false		// fog related, presumably to exclude fog from 
													//   affecting certain elements


#define q_text_maxlen_0					0
#define q_outcolor_null					NULL
#define q_ignore_color_codes_true		true
#define q_ignore_color_codes_false		false


#define qnfo_send_true					true
#define qnfo_send_false					false
#define	qnfo_allowstar_true				true
#define	qnfo_allowstar_false			false
#define qnfo_allowmodel_true			true
#define qnfo_allowmodel_false			false
#define qnfo_quiet_true					true
#define qnfo_quiet_false				false

#define qsv_resetcache_true				true
#define qsv_querydp_true				true
#define qsv_queryqw_true				true
#define qsv_querydp_false				false
#define qsv_queryqw_false				false
#define qsv_consoleoutput_true			true
#define qsv_consoleoutput_false			false

#define q_include_port_true				true
#define q_include_port_false			false
#define q_is_oob_true					true
#define q_is_oob_false					false

#define q_is_reliable_true				true
#define q_is_reliable_false				false


#define q_disconnect_message_NULL		NULL

#define q_fitz_version_none_0			0
#define q_fitz_version_1				1
#define q_fitz_version_2				2

#include "common.h"
#include "filematch.h"
#include "fs.h"

#include "host.h"
#include "cvar.h"
#include "cmd.h"
#include "console.h"
#include "lhnet.h"
#include "mathlib.h"
#include "matrixlib.h"

extern cvar_t developer;
extern cvar_t developer_entityparsing;
extern cvar_t developer_extra;
extern cvar_t developer_insane;
extern cvar_t developer_loadfile;
extern cvar_t developer_loading;
extern cvar_t host_isclient;
extern cvar_t sessionid;

void LOC_LoadFile (void); // AURA 8.0
const char *LOC_GetString (const char *s_dollar_key); // AURA 1.2
#endif
