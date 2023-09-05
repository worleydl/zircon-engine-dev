// baker.h -- platform environment. Orgins in 2014 and 2015.


#ifndef __BAKER_H__
#define __BAKER_H__

#define WARP_X_(...)						// For warp without including code  see also: SPECIAL_POS___
#define WARP_X_CALLERS_(...)	// For warp without including code  see also: SPECIAL_POS___

WARP_X_ (dpsnprintf)
WARP_X_ (Partial_Reset String_Does_Have_Uppercase)
WARP_X_ (String_Does_Have_Uppercase)

///////////////////////////////////////////////////////////////////////////////
//  PLATFORM: Baker
///////////////////////////////////////////////////////////////////////////////

#if defined(_WIN32) || defined(WIN64)
 	# define PLATFORM_WINDOWS
#else
	
#endif // !_WIN32 WIN64

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
#define RGB_3							3				// Used to indicate a 3 x is bytes per pixel 

#define SPACE_CHAR_32					32
#define TAB_CHAR_9						9
#define NEWLINE_CHAR_10					10				// '\n'		//#define NEWLINE_CHAR_10				10
#define CARRIAGE_RETURN_CHAR_13			13				// '\r'		//#define CARRIAGE_RETURN_CHAR_13		13
#define	CHAR_TILDE_126					126

#define	NEWLINE							"\n"

///////////////////////////////////////////////////////////////////////////////
//  HUMAN READABLE MACROS: Baker
///////////////////////////////////////////////////////////////////////////////

// Baker
#define Flag_Remove_From(x,flag)				x = (x) - ((x) & (flag))
#define Flag_Mask_To_Only(x,flag)				x = ((x) & (flag))
#define Flag_Add_To(x,flag)						x |= (flag)
#define Have_Flag(x,flag)						((x) & (flag))
#define Have_Flag_Bool(x,flag)					(((x) & (flag)) != 0)
#define Have_Flag_Strict_Bool(x,flag)			(  ((x) & (flag) ) == (flag) )
#define No_Have_Flag(x,flag)					!((x) & (flag))

#define	Smallest(a, b)							(((a) < (b)) ? (a) : (b))
#define	Largest(a, b)							(((a) > (b)) ? (a) : (b))

#define in_range( _lo, _v, _hi )				( (_lo) <= (_v) && (_v) <= (_hi) )
#define in_range_beyond(_start, _v, _beyond )	( (_start) <= (_v) && (_v) < (_beyond) )

#define ARRAY_COUNT(_array)						(sizeof(_array) / sizeof(_array[0]))	// Used tons.

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

#define	QUOTEDSTR(s_literal)		"\"" s_literal "\""		// QUOTEDSTR("maxplayers") -- LITERAL STRING (Unused but keep, move along ...)

#define  QUOTED_F					"\"%f\""		//       quoted  Endangered?
#define  QUOTED_G					"\"%g\""		//       quoted
#define  QUOTED_D					"\"%d\""		//       quoted

#define  QUOTED_S					"\"%s\""		// quoted - NOT SAME AS QUOTEDSTR
#define  SINGLE_QUOTED_S			"'%s'"			// single quoted

	// %WIDTH.TRUNCs <--- yes the .20 truncates.
#define S_FMT_LEFT_PAD_20			"%-20.20s"		// Negative means left pad or right pad?  LEFT  The .20 truncates at 20, right?
#define S_FMT_RIGHT_PAD_20			"%20.20s"		// Negative means left pad or right pad?  LEFT  The 20 pads the .20 truncs

#define S_FMT_LEFT_PAD_40			"%-40.40s"		// Negative means left pad or right pad?  LEFT  The .20 truncates at 20, right?
#define S_FMT_RIGHT_PAD_40			"%40.40s"		// Negative means left pad or right pad?  LEFT  The 20 pads the .20 truncs

///////////////////////////////////////////////////////////////////////////////
//  INTERNAL MACROS: Baker
///////////////////////////////////////////////////////////////////////////////

#define case_break						break; { } case							// MVP.  Used tons.  default: much come first in switch. 

#define STRINGIFY(x)					#x		

#define freenull3_(x)					if (x) { free (x); x = NULL; }
#define setstr(x,y)						freenull3_ (x) x = strdup(y);

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

SBUF___ const char *Sys_Getcwd_SBuf (void);


///////////////////////////////////////////////////////////////////////////////
//  AUTOCOMPLETION: Baker
///////////////////////////////////////////////////////////////////////////////

extern char *spartial_a;
extern char *spartial_curcmd_a;

extern char *spartial_start_pos;
extern char *spartial_beyond_pos;
extern char *spartial_beyond2_pos;

extern char *spartial_best_before_a;	// For 2nd hit SHIFT TAB
extern char *spartial_best_after_a;		// For 2nd hit TAB
extern char *spartial_alphatop_a;
extern char *spartial_alphalast_a;

#define SPARTIAL_EVAL_ \
		if (1) {\
			int cmpres0 = spartial_alphatop_a ? strcasecmp(sxy, spartial_alphatop_a) : -1;\
			if (cmpres0 < 0) {				\
				setstr (spartial_alphatop_a, sxy);\
			}\
		}\
		if (spartial_curcmd_a) {\
			int cmpres0 = strcasecmp(sxy, spartial_curcmd_a);\
			if (cmpres0 < 0) {\
				int cmpres1 = spartial_best_before_a ? strcasecmp(sxy, spartial_best_before_a) : 0; \
				if (cmpres1 == 0 || cmpres1 > 0) {\
					setstr (spartial_best_before_a, sxy);\
				}				\
			}\
		} \
		if (spartial_curcmd_a) {\
			int cmpres0 = strcasecmp(sxy, spartial_curcmd_a); \
			if (cmpres0 > 0) {\
				int cmpres1 = spartial_best_after_a ? strcasecmp(sxy, spartial_best_after_a) : 0; \
				if (cmpres1 == 0 || cmpres1 < 0) {\
					setstr (spartial_best_after_a, sxy);\
				}\
			}\
		} \
		if (1) {\
			int cmpres0 = spartial_alphalast_a ? strcasecmp(sxy, spartial_alphalast_a) : 1;\
			if (cmpres0 > 0) {\
				setstr (spartial_alphalast_a, sxy);\
			}\
		} // Ender



WARP_X_ (Partial_Reset String_Does_Have_Uppercase)
#endif // ! __BAKER_H__




