// baker.h -- platform environment. Orgins in 2014 and 2015.

#ifndef __BAKER_H__
#define __BAKER_H__

#define WARP_X_(...)			// For warp without including code  see also: SPECIAL_POS___
#define WARP_X_CALLERS_(...)	// For warp without including code  see also: SPECIAL_POS___

#define NULLFIX2		NULL	// Marking kludge fixes "NULLFIX, "

#define PLUS1(x)							(x+1)
#define UNPLUS1(YOUR_PLUS_1)				((YOUR_PLUS_1) - 1)

#define unconstanting			// Baker: Used to mark a cast specifically to unconst

WARP_X_ (dpsnprintf)
WARP_X_ (Partial_Reset String_Does_Have_Uppercase)
WARP_X_ (String_Does_Have_Uppercase)

///////////////////////////////////////////////////////////////////////////////
//  PLATFORM: Baker
///////////////////////////////////////////////////////////////////////////////

#if defined(_WIN32) || defined(_WIN64)
 	# define PLATFORM_WINDOWS
#else

#endif // !_WIN32 _WIN64

#ifdef __GNUC__ // Clang has GNUC defined, yes or no?  Sounds like it does.  Confirm yes or no.  October 2 2020
	#define	__core_attribute__	__attribute__
#else
	#define	__core_attribute__(x)
#endif



///////////////////////////////////////////////////////////////////////////////
//  DATA TYPES: Baker
///////////////////////////////////////////////////////////////////////////////

typedef unsigned char byte;

typedef struct _crect_t_s {
		int		left, top, width, height;
} crect_s;

#define RECT_HIT_X(rect, x)				in_range_beyond((rect).left, x, (rect).left + (rect).width)
#define RECT_HIT_Y(rect, y)				in_range_beyond((rect).top, y,  (rect).top  + (rect).height)
#define RECT_HIT(rect, x, y)			(RECT_HIT_X(rect, x) && RECT_HIT_Y(rect, y))

#define RECT_SET(rect, _left, _top, _width, _height) \
	(rect).left = _left, (rect).top = _top, (rect).width = _width, (rect).height = _height

///////////////////////////////////////////////////////////////////////////////
//  HUMAN READABLE CONSTANTS: Baker
///////////////////////////////////////////////////////////////////////////////

#define ONE_CHAR_1						1
#define ONE_ENTRY_1						1
#define ONE_SIZEOF_BYTE_1				1
#define ONE_SIZEOF_CHAR_1				1
#define ONE_SIZEOF_NULL_TERM_1			1

#define SYSTEM_STRING_SIZE_1024			1024

#define not_found_neg1					-1				// strstrofs more legible

#define RGBA_4							4				// Used to indicate a 4 x is bytes per pixel
#define BGRA_4							4				// Used to indicate a 4 x is bytes per pixel
#define BGRA_BPP_32						32				// Used to indicate a 4 x is bytes per pixel
#define RGBA_BPP_32						32				// Used to indicate a 4 x is bytes per pixel
#define RGB_BPP_24						24				// Used to indicate a 4 x is bytes per pixel
#define BGR_BPP_24						24				// Used to indicate a 4 x is bytes per pixel
#define RGB_3							3				// Used to indicate a 3 x is bytes per pixel

#define NULL_CHAR_0						0
#define SPACE_CHAR_32					32
#define TAB_CHAR_9						9
#define NEWLINE_CHAR_10					10				// '\n'		//#define NEWLINE_CHAR_10				10
#define CARRIAGE_RETURN_CHAR_13			13				// '\r'		//#define CARRIAGE_RETURN_CHAR_13		13
#define	CHAR_TILDE_126					126
#define	CHAR_BACKQUOTE_96				96
#define	CHAR_BACKSPACE_8				8

#define	CARRIAGE_RETURN					"\r"
#define	NEWLINE							"\n"
#define	TAB_CHARACTER					"\t"
#define SPACE_CHARACTER					" "

///////////////////////////////////////////////////////////////////////////////
//  HUMAN READABLE MACROS: Baker
///////////////////////////////////////////////////////////////////////////////

// Baker
#define Flag_Remove_From(x,flag)				x = (x) - ((x) & (flag))
#define Flag_Toggle(x,flag)						x = ((x) ^ (flag))
#define Flag_Mask_To_Only(x,flag)				x = ((x) & (flag))
#define Flag_Add_To(x,flag)						x |= (flag)
#define Have_Flag(x,flag)						((x) & (flag))
#define Have_Flag_Bool(x,flag)					(((x) & (flag)) != 0)
#define Have_Flag_Strict_Bool(x,flag)			(  ((x) & (flag) ) == (flag) )
#define No_Have_Flag(x,flag)					!((x) & (flag))

#if 1 // Reason: complex formulas evaluated only once
	#define	Smallest(a, b)						min(a, b)
	#define	Largest(a, b)						max(a, b)
#else
	#define	Smallest(a, b)						(((a) < (b)) ? (a) : (b))
	#define	Largest(a, b)						(((a) > (b)) ? (a) : (b))
#endif

#define in_range( _lo, _v, _hi )				( (_lo) <= (_v) && (_v) <= (_hi) )
#define in_range_beyond(_start, _v, _beyond )	( (_start) <= (_v) && (_v) < (_beyond) )

#define ARRAY_COUNT(_array)						(sizeof(_array) / sizeof(_array[0]))	// Used tons.

#define modulo %

#define isin0(x) (0)					// No, not insane.  You can mark an future list with this.  Beats a comment.
#define isin1(x,a) ((x) == (a))			// No, not insane.  What if you expect more, but can only remember one or only know of 1 at the moment.
#define isin2(x,a,b) ((x) == (a) || (x) == (b))
#define isin3(x,a,b,c) ((x) == (a) || (x) == (b) || (x) == (c))
#define isin4(x,a,b,c,d) ((x) == (a) || (x) == (b) || (x) == (c) || (x) == (d))
#define isin5(x,a,b,c,d,e) ((x) == (a) || (x) == (b) || (x) == (c) || (x) == (d) || (x) == (e))
#define isin6(x,a,b,c,d,e,f) ((x) == (a) || (x) == (b) || (x) == (c) || (x) == (d) || (x) == (e) || (x) == (f))
#define isin7(x,a,b,c,d,e,f,g) ((x) == (a) || (x) == (b) || (x) == (c) || (x) == (d) || (x) == (e) || (x) == (f) || (x) == (g))
#define isin8(x,a,b,c,d,e,f,g,h) ((x) == (a) || (x) == (b) || (x) == (c) || (x) == (d) || (x) == (e) || (x) == (f) || (x) == (g) || (x) == (h))
#define isin9(x,a,b,c,d,e,f,g,h,i) ((x) == (a) || (x) == (b) || (x) == (c) || (x) == (d) || (x) == (e) || (x) == (f) || (x) == (g) || (x) == (h) || (x) == (i))
#define isin10(x,a,b,c,d,e,f,g,h,i,j) ((x) == (a) || (x) == (b) || (x) == (c) || (x) == (d) || (x) == (e) || (x) == (f) || (x) == (g) || (x) == (h) || (x) == (i) || (x) == (j))
#define isin11(x,a,b,c,d,e,f,g,h,i,j,k) ((x) == (a) || (x) == (b) || (x) == (c) || (x) == (d) || (x) == (e) || (x) == (f) || (x) == (g) || (x) == (h) || (x) == (i) || (x) == (j) || (x) == (k))
#define isin12(x,a,b,c,d,e,f,g,h,i,j,k,l) ((x) == (a) || (x) == (b) || (x) == (c) || (x) == (d) || (x) == (e) || (x) == (f) || (x) == (g) || (x) == (h) || (x) == (i) || (x) == (j) || (x) == (k) || (x) == (l))
#define isin13(x,a,b,c,d,e,f,g,h,i,j,k,l,m) ((x) == (a) || (x) == (b) || (x) == (c) || (x) == (d) || (x) == (e) || (x) == (f) || (x) == (g) || (x) == (h) || (x) == (i) || (x) == (j) || (x) == (k) || (x) == (l) || (x) == (m))
#define isin14(x,a,b,c,d,e,f,g,h,i,j,k,l,m,n) ((x) == (a) || (x) == (b) || (x) == (c) || (x) == (d) || (x) == (e) || (x) == (f) || (x) == (g) || (x) == (h) || (x) == (i) || (x) == (j) || (x) == (k) || (x) == (l) || (x) == (m) || (x) == (n))
#define isin15(x,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o) ((x) == (a) || (x) == (b) || (x) == (c) || (x) == (d) || (x) == (e) || (x) == (f) || (x) == (g) || (x) == (h) || (x) == (i) || (x) == (j) || (x) == (k) || (x) == (l) || (x) == (m) || (x) == (n) || (x) == (o))
#define isin16(x,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p) ((x) == (a) || (x) == (b) || (x) == (c) || (x) == (d) || (x) == (e) || (x) == (f) || (x) == (g) || (x) == (h) || (x) == (i) || (x) == (j) || (x) == (k) || (x) == (l) || (x) == (m) || (x) == (n) || (x) == (o) || (x) == (p))
#define isin17(x,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q) ((x) == (a) || (x) == (b) || (x) == (c) || (x) == (d) || (x) == (e) || (x) == (f) || (x) == (g) || (x) == (h) || (x) == (i) || (x) == (j) || (x) == (k) || (x) == (l) || (x) == (m) || (x) == (n) || (x) == (o) || (x) == (p)    || (x) == (q) )
#define isin18(x,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r) ((x) == (a) || (x) == (b) || (x) == (c) || (x) == (d) || (x) == (e) || (x) == (f) || (x) == (g) || (x) == (h) || (x) == (i) || (x) == (j) || (x) == (k) || (x) == (l) || (x) == (m) || (x) == (n) || (x) == (o) || (x) == (p)  || (x) == (q) || (x) == (r) )
#define isin19(x,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s) ((x) == (a) || (x) == (b) || (x) == (c) || (x) == (d) || (x) == (e) || (x) == (f) || (x) == (g) || (x) == (h) || (x) == (i) || (x) == (j) || (x) == (k) || (x) == (l) || (x) == (m) || (x) == (n) || (x) == (o) || (x) == (p)  || (x) == (q) || (x) == (r) || (x) == (s) )
#define isin20(x,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t) ((x) == (a) || (x) == (b) || (x) == (c) || (x) == (d) || (x) == (e) || (x) == (f) || (x) == (g) || (x) == (h) || (x) == (i) || (x) == (j) || (x) == (k) || (x) == (l) || (x) == (m) || (x) == (n) || (x) == (o) || (x) == (p)  || (x) == (q) || (x) == (r) || (x) == (s) || (x) == (t) )
#define isin21(x,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u) ((x) == (a) || (x) == (b) || (x) == (c) || (x) == (d) || (x) == (e) || (x) == (f) || (x) == (g) || (x) == (h) || (x) == (i) || (x) == (j) || (x) == (k) || (x) == (l) || (x) == (m) || (x) == (n) || (x) == (o) || (x) == (p)  || (x) == (q) || (x) == (r) || (x) == (s) || (x) == (t) || (x) == (u) )


///////////////////////////////////////////////////////////////////////////////
//  DEV MARKINGS: Baker
///////////////////////////////////////////////////////////////////////////////

#define replyx
#define requiredx

#define RETURNING_ALLOC___		// An alloc function returns it.
#define RETURNING_FREE___		// An alloc function returns it.
#define RETURNING___			// Mark the return of a return_alloc // Marking shift of burden, proof of tracing.

#define AUTO_ALLOC___			// Within the scope of a function, exit should destroy like automatic variables.
#define AUTO_FREE___			// Within the scope of a function, exit should destroy like automatic variables.

#define NOT_MISSING_ASSIGN(pvar,val)		if (pvar) (*(pvar)) = (val)		// Used much.  Simplifies a reply return
#define REQUIRED_ASSIGN(pvar,val)			(*(pvar)) = (val)				// Used much.  Simplifies a reply return

#define SBUF___		// Used to mark functions that are static hence multi-threading sensitive.
					// There are a great many unmarked, but we especially want to mark ones depending on C functions that are not thread safe
					// because those can hit us especially by surprise when we begin implementing multi-threading safety.

///////////////////////////////////////////////////////////////////////////////
//  DEV STRING: Baker
///////////////////////////////////////////////////////////////////////////////

WARP_X_ (PRINTF_INT64)

#define	 QUOTED_STR(s_literal)		"\"" s_literal "\""		// QUOTED_STR("maxplayers") -- LITERAL STRING (Unused but keep, move along ...)

#define  QUOTED_F					"\"%f\""		//       quoted  Endangered?
#define  QUOTED_G					"\"%g\""		//       quoted
#define  QUOTED_D					"\"%d\""		//       quoted

#define  QUOTED_S					"\"%s\""		// quoted - NOT SAME AS QUOTEDSTR
#define  SINGLE_QUOTED_S			"'%s'"			// single quoted

	// %WIDTH.TRUNCs <--- yes the .20 truncates.
#define S_FMT_LEFT_PAD_16			"%-16.16s"		// Negative means left pad or right pad?  LEFT  The .20 truncates at 20, right?
#define S_FMT_RIGHT_PAD_16			"%16.16s"		// Negative means left pad or right pad?  LEFT  The 20 pads the .20 truncs

#define S_FMT_LEFT_PAD_20			"%-20.20s"		// Negative means left pad or right pad?  LEFT  The .20 truncates at 20, right?
#define S_FMT_RIGHT_PAD_20			"%20.20s"		// Negative means left pad or right pad?  LEFT  The 20 pads the .20 truncs

#define S_FMT_LEFT_PAD_40			"%-40.40s"		// Negative means left pad or right pad?  LEFT  The .20 truncates at 20, right?
#define S_FMT_RIGHT_PAD_40			"%40.40s"		// Negative means left pad or right pad?  LEFT  The 20 pads the .20 truncs

// See also: PRIuPTR PRIx64	"llX" PRIx32 "lX" PRINTF_INT64 "%I64d"
// Baker: We are going to cast size_t to int64_t .. move along ...
WARP_X_ (PRINTF_INT64)
//#if defined(_MSC_VER) || defined(__MINGW32__) //__MINGW32__ should goes before __GNUC__
//  #define SIZE_T_ZU_F				"%Iu"	size_t
//  #define SSIZE_T_ZD_F				"%Id"	ssize_t
//  #define PTRDIFF_T_ZD_F			"%Id"	
////  #define PRIu64							uint64_t
//#elif defined(__GNUC__)
//  #define SIZE_T_ZU_F				"%zu"
//  #define SSIZE_T_ZD_F				"%zd"
//  #define PTRDIFF_T_ZD_F			"%zd"
//#else
//	FIX ME -- Mac?
//#endif

#define VECTOR3_5d1F				"%5.1f %5.1f %5.1f"
#define VECTOR3_SEND(v)				(v)[0], (v)[1], (v)[2]

///////////////////////////////////////////////////////////////////////////////
//  INTERNAL MACROS: Baker
///////////////////////////////////////////////////////////////////////////////

#define case_break						break; { } case							// MVP.  Used tons.  default: much come first in switch.

#define STRINGIFY(x)					#x

#define freenull_(x)					if (x) { free (x); x = NULL; }
#define setstr(x,y)						freenull_ (x) x = strdup(y);

#define SET___
#define CHANGE___

#ifndef c_strlcpy
	#define c_strlcpy(_dest, _source) \
		strlcpy (_dest, _source, sizeof(_dest)) // Ender
#endif

#ifndef c_strlcat
	#define c_strlcat(_dest, _source) \
		strlcat (_dest, _source, sizeof(_dest)) // Ender
#endif

// Checks for existing file before checking file length, if file does not exist returns 0
/*internalish*/ size_t File_Length2 (const char *path_to_file, requiredx int *p_is_existing);

///////////////////////////////////////////////////////////////////////////////
//  SYSTEM STUFF: Baker - why here, though?
///////////////////////////////////////////////////////////////////////////////

int Sys_Clipboard_Set_Image (unsigned *rgba, int width, int height);
int Sys_Clipboard_Set_Text (const char *text_to_clipboard);
int Sys_Folder_Open_Folder_Must_Exist (const char *path_to_file);
SBUF___ const char *Sys_Binary_URL_SBuf (void); //

SBUF___ char *Sys_Getcwd_SBuf (void);

#define roundup_16(n) (((n) + 15) & ~15)

///////////////////////////////////////////////////////////////////////////////
//  AUTOCOMPLETION: Baker
///////////////////////////////////////////////////////////////////////////////

#define SPARTIAL_EVAL_ \
		if (1) {\
			int cmpres0 = _g_autocomplete.s_match_alphatop_a ? strcasecmp(sxy, _g_autocomplete.s_match_alphatop_a) : -1;\
			if (cmpres0 < 0) {				\
				setstr (_g_autocomplete.s_match_alphatop_a, sxy);\
			}\
		}\
		if (_g_autocomplete.s_completion_a) {\
			int cmpres0 = strcasecmp(sxy, _g_autocomplete.s_completion_a);\
			if (cmpres0 < 0) {\
				int cmpres1 = _g_autocomplete.s_match_before_a ? strcasecmp(sxy, _g_autocomplete.s_match_before_a) : 0; \
				if (cmpres1 == 0 || cmpres1 > 0) {\
					setstr (_g_autocomplete.s_match_before_a, sxy);\
				}				\
			}\
		} \
		if (_g_autocomplete.s_completion_a) {\
			int cmpres0 = strcasecmp(sxy, _g_autocomplete.s_completion_a); \
			if (cmpres0 > 0) {\
				int cmpres1 = _g_autocomplete.s_match_after_a ? strcasecmp(sxy, _g_autocomplete.s_match_after_a) : 0; \
				if (cmpres1 == 0 || cmpres1 < 0) {\
					setstr (_g_autocomplete.s_match_after_a, sxy);\
				}\
			}\
		} \
		if (1) {\
			int cmpres0 = _g_autocomplete.s_match_alphalast_a ? strcasecmp(sxy, _g_autocomplete.s_match_alphalast_a) : 1;\
			if (cmpres0 > 0) {\
				setstr (_g_autocomplete.s_match_alphalast_a, sxy);\
			}\
		} // Ender


#define iif(_cond, trueval, falseval)	((_cond) ? (trueval) : (falseval))
WARP_X_ (Partial_Reset String_Does_Have_Uppercase)

#define Math_IsOdd(_yourint) ((_yourint) & 1 ) // IS_EVEN IS_ODD ODD EVEN

#ifdef _DEBUG
	#define c_assert(x) assert(x)
#else
	#define c_assert(x)
#endif

#endif // ! __BAKER_H__




