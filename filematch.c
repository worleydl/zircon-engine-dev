
#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif

#include "darkplaces.h"

#ifdef _WIN32
#include "utf8lib.h"
#endif

// LadyHavoc: some portable directory listing code I wrote for lmp2pcx, now used in darkplaces to load id1/*.pak and such...

int matchpattern(const char *in, const char *pattern, int caseinsensitive)
{
	return matchpattern_with_separator(in, pattern, caseinsensitive, "/\\:", false);
}

// wildcard_least_one: if true * matches 1 or more characters
//                     if false * matches 0 or more characters
int matchpattern_with_separator(const char *in, const char *pattern, int caseinsensitive, const char *separators, qbool wildcard_least_one)
{
	int c1, c2;
	while (*pattern)
	{
		switch (*pattern)
		{
		case 0:
			return 1; // end of pattern
		case '?': // match any single character
			if (*in == 0 || strchr(separators, *in))
				return 0; // no match
			in++;
			pattern++;
			break;
		case '*': // match anything until following string
			if (wildcard_least_one)
			{
				if (*in == 0 || strchr(separators, *in))
					return 0; // no match
				in++;
			}
			pattern++;
			while (*in)
			{
				if (strchr(separators, *in))
					break;
				// see if pattern matches at this offset
				if (matchpattern_with_separator(in, pattern, caseinsensitive, separators, wildcard_least_one))
					return 1;
				// nope, advance to next offset
				in++;
			}
			break;
		default:
			if (*in != *pattern)
			{
				if (!caseinsensitive)
					return 0; // no match
				c1 = *in;
				if (c1 >= 'A' && c1 <= 'Z')
					c1 += 'a' - 'A';
				c2 = *pattern;
				if (c2 >= 'A' && c2 <= 'Z')
					c2 += 'a' - 'A';
				if (c1 != c2)
					return 0; // no match
			}
			in++;
			pattern++;
			break;
		}
	}
	if (*in)
		return 0; // reached end of pattern but not end of input
	return 1; // success
}

// a little strings system
void stringlistinit(stringlist_t *list)
{
	memset(list, 0, sizeof(*list));
}

void stringlistfreecontents(stringlist_t *list)
{
	int i;
	for (i = 0;i < list->numstrings;i++)
	{
		if (list->strings[i])
			Z_Free(list->strings[i]);
		list->strings[i] = NULL;
	}
	list->numstrings = 0;
	list->maxstrings = 0;
	if (list->strings)
		Z_Free(list->strings);
	list->strings = NULL;
}

void stringlistprint (stringlist_t *list, const char *title_optional, void (*myprintf)(const char *, ...) )
{
	if (title_optional)
		myprintf ("%s = %d", title_optional, list->numstrings);
	else {
		// Print nothing!
	}

	for (int idx = 0; idx < list->numstrings; idx ++) {
		const char *s = list->strings[idx];
		Con_PrintLinef ("%4d: %s", idx, s);
	} // for idx

}

void stringlistappend (stringlist_t *list, const char *text)
{
	//size_t textlen;
	char **oldstrings;

	if (list->numstrings >= list->maxstrings) {
		oldstrings = list->strings;
		list->maxstrings += 4096;
		list->strings = (char **) Z_Malloc(list->maxstrings * sizeof(*list->strings));
		if (list->numstrings)
			memcpy(list->strings, oldstrings, list->numstrings * sizeof(*list->strings));
		if (oldstrings)
			Z_Free(oldstrings);
	}
	
	list->strings[list->numstrings] = Z_StrDup(text);
	list->numstrings++;
}

void stringlistappend_blob (stringlist_t *list, const byte *blob, size_t blobsize)
{
	char **oldstrings;

	// Baker: This reallocs in batches of 4096
	if (list->numstrings >= list->maxstrings) {
		oldstrings = list->strings;
		list->maxstrings += 4096;
		list->strings = (char **) Z_Malloc(list->maxstrings * sizeof(*list->strings));
		if (list->numstrings)
			memcpy (list->strings, oldstrings, list->numstrings * sizeof(*list->strings));
		if (oldstrings)
			Z_Free(oldstrings);
	}
	//textlen = strlen(text) + 1;
	list->strings[list->numstrings] = (char *) Z_Malloc(blobsize);
	memcpy (list->strings[list->numstrings], blob, blobsize);
	list->numstrings++;
}

void stringlistappendlist (stringlist_t *plist, const stringlist_t *add_these)
{
	for (int idx = 0; idx < add_these->numstrings; idx++) {
		char *sxy = add_these->strings[idx];
		stringlistappend (plist, sxy);
	}

}

void stringlistappendfssearch (stringlist_t *plist, fssearch_t *t)
{
	if (!t)
		return;
	
	for (int idx = 0; idx < t->numfilenames; idx++) {
		char *sxy = t->filenames[idx];
		stringlistappend (plist, sxy);
	}

}

void stringlistappend_search_pattern (stringlist_t *plist, const char *s_pattern)
{
	fssearch_t	*t = FS_Search (s_pattern, fs_caseless_true, fs_quiet_true, fs_pakfile_null, fs_gamedironly_false);

	if (!t) return;

	// Baker: Entries should be the entire file name like
	// "sound/ambience/thunder.wav" or whatever

	stringlistappendfssearch (plist, t);

	if (t) FS_FreeSearch(t);
}

void stringlist_replace_at_index (stringlist_t *list, int idx, const char *text)
{
	if (in_range_beyond (0, idx, list->numstrings) == false) {
		Con_PrintLinef (CON_ERROR "string list at %d is out of bounds of 0 to %d", idx, list->numstrings);
		return;
	}
	Z_Free (list->strings[idx]);

	list->strings[idx] = Z_StrDup(text);
}

static int stringlistsort_cmp(const void *a, const void *b)
{
	return strcasecmp(*(const char **)a, *(const char **)b);
}

static int sstart = 0;
static int slength = 0;
static int stringlistsort_start_length_cmp(const void *_a, const void *_b)
{
	const char *a = *(const char **)_a;
	const char *b = *(const char **)_b;

	char *sa = Z_StrDup (a);
	char *sb = Z_StrDup (b);
	File_URL_Edit_Remove_Extension (sa);
	File_URL_Edit_Remove_Extension (sb);

	int result = 0;
#if 1 
	// Safety checks
	int slena = strlen (sa);
	int slenb = strlen (sb);

	if (slena < sstart + slength) {
		result = strcasecmp(sa, sb); // This is not right, but repeatably consistent in the sort order
		goto failout; // Too short
	}

	if (slenb < sstart + slength) {
		result = strcasecmp(sa, sb); // This is not right, but repeatably consistent in the sort order 
		goto failout; // Too short
	}
#endif

	memmove (sa, &sa[sstart], slength + 1); // +1 to copy the null too
	memmove (sb, &sb[sstart], slength + 1); // +1 to copy the null too

	result = strcasecmp(sa, sb); 

failout:

	Mem_FreeNull_ (sa);
	Mem_FreeNull_ (sb);

	return result;
}

void stringlistsort_custom(stringlist_t *list, qbool uniq, int myfunc(const void *, const void *) );

void stringlistsort(stringlist_t *list, qbool uniq)
{
	stringlistsort_custom (list, uniq, stringlistsort_cmp);
}

void stringlistsort_substring (stringlist_t *list, qbool uniq, int startpos, int slength)
{
	sstart = startpos;
	slength = slength;

	stringlistsort_custom (list, uniq, stringlistsort_start_length_cmp);
}

void stringlist_condump (stringlist_t *plist)
{
	for (int idx = 0; idx < plist->numstrings; idx++) {
		char *sxy = plist->strings[idx];

		Con_PrintLinef ("%4d: " QUOTED_S, idx, sxy);
	} // for
}

void stringlistsort_custom(stringlist_t *list, qbool uniq, int myfunc(const void *, const void *) )
{
	if (list->numstrings < 1)
		return;

	qsort(&list->strings[0], list->numstrings, sizeof(list->strings[0]), myfunc);

	// If Make Unique ... 
	if (uniq) {
		// i: the item to read
		// j: the item last written
	int i, j;
		for (i = 1, j = 0; i < list->numstrings; ++i)
		{
			char *save;
			if (String_Does_Match_Caseless(list->strings[i], list->strings[j]))
				continue;
			++j;
			save = list->strings[j];
			list->strings[j] = list->strings[i];
			list->strings[i] = save;
		}
		for(i = j + 1; i < list->numstrings; i ++) {
			if (list->strings[i])
				Z_Free(list->strings[i]);
		}
		list->numstrings = j+1;
	}
}


// operating system specific code
static void adddirentry(stringlist_t *list, const char *path, const char *name)
{
	if (String_Does_NOT_Match(name, ".") && String_Does_NOT_Match(name, "..")) {
		char temp[MAX_OSPATH];
		dpsnprintf( temp, sizeof( temp ), "%s%s", path, name );
		stringlistappend(list, temp);
	}
}

#ifdef _WIN32
// Baker: This concats the results
void listdirectory(stringlist_t *list, const char *basepath, const char *path)
{
	#define BUFSIZE 4096
	char pattern[BUFSIZE] = {0};
	wchar patternw[BUFSIZE] = {0};
	char filename[BUFSIZE] = {0};
	wchar *filenamew;
	int lenw = 0;
	WIN32_FIND_DATAW n_file;
	HANDLE hFile;
	c_strlcpy (pattern, basepath);
	c_strlcat (pattern, path);
	c_strlcat (pattern, "*");
	fromwtf8(pattern, (int)strlen(pattern), patternw, BUFSIZE);
	// ask for the directory listing handle
	hFile = FindFirstFileW(patternw, &n_file);
	if (hFile == INVALID_HANDLE_VALUE)
		return;
	do {
		filenamew = n_file.cFileName;
		lenw = 0;
		while(filenamew[lenw] != 0) ++lenw;
		towtf8(filenamew, lenw, filename, BUFSIZE);
		adddirentry(list, path, filename);
	} while (FindNextFileW(hFile, &n_file) != 0);
	FindClose(hFile);
	#undef BUFSIZE
}
#else
void listdirectory(stringlist_t *list, const char *basepath, const char *path)
{
	char fullpath[MAX_OSPATH];
	DIR *dir;
	struct dirent *ent;
	dpsnprintf(fullpath, sizeof(fullpath), "%s%s", basepath, path);
#ifdef __ANDROID__
	// SDL currently does not support listing assets, so we have to emulate
	// it. We're using relative paths for assets, so that will do.
	if (basepath[0] != '/')
	{
		char listpath[MAX_OSPATH];
		qfile_t *listfile;
		dpsnprintf(listpath, sizeof(listpath), "%sls.txt", fullpath);
		char *buf = (char *) FS_SysLoadFile(listpath, tempmempool, true, NULL);
		if (!buf)
			return;
		char *p = buf;
		for (;;)
		{
			char *q = strchr(p, '\n');
			if (q == NULL)
				break;
			*q = 0;
			adddirentry(list, path, p);
			p = q + 1;
		}
		Mem_Free(buf);
		return;
	}
#endif
	dir = opendir(fullpath);
	if (!dir)
		return;

	while ((ent = readdir(dir)))
		adddirentry(list, path, ent->d_name);
	closedir(dir);
}
#endif

// Baker: Usage:
// stringlist_t matchedSet;	
// stringlistinit	(&matchedSet); // this does not allocate, memset 0
//
// stringlist_from_delim (&matchedSet, mystring);
//// SORT plus unique-ify
//stringlistsort (&matchedSet, fs_make_unique_true);
//stringlistfreecontents( &matchedSet );

// Baker: Takes a stringlist and turns it in a single string, spaces between at moment
// Example ?  No callers at moment.
// This was going to be used, then another idea came up
// But code is not quite finished
void stringlist_to_string (stringlist_t *p_stringlist, char *s_delimiter, char *buf, size_t buflen)
{
	buf[0] = 0;
	strlcpy (buf, "", buflen);
	for (int idx = 0; idx < p_stringlist->numstrings; idx ++) {
		char *sxy = p_stringlist->strings[idx];
		if (idx > 0)
			strlcat (buf, " ", buflen);
		strlcat (buf, sxy, buflen);
	} // for
}

// Returns not_found_neg1 if not found
static int _stringlist_find_index (stringlist_t *p_stringlist, char *s_find_this, int caseinsensitive)
{
	typedef int (*comparemethod_fn_t) (const char *s1, const char *s2);
	comparemethod_fn_t compare_fn = caseinsensitive ? strcasecmp : strcmp;
	for (int idx = 0; idx < p_stringlist->numstrings; idx ++) {
		char *sxy = p_stringlist->strings[idx];
		if (compare_fn (sxy, s_find_this) == 0)
			return idx;
	} // for
	return not_found_neg1;
}

int stringlist_find_index_caseless (stringlist_t *p_stringlist, char *s_find_this)
{
	return _stringlist_find_index (p_stringlist, s_find_this, fs_caseless_true);
}

int stringlist_find_index (stringlist_t *p_stringlist, char *s_find_this)
{
	return _stringlist_find_index (p_stringlist, s_find_this, fs_caseless_false);
}

void stringlist_from_delim (stringlist_t *p_stringlist, const char *s_space_delimited)
{
	// This process depends on this s_space_delimited having items.
	if (s_space_delimited[0] == NULL_CHAR_0)
		return;

	const char	*s_space_delim		= " ";
	int			s_len			= (int)strlen(s_space_delimited);
	int			s_delim_len		= (int)strlen(s_space_delim);

	// Baker: This works the searchpos against s_space_delimited
	// finding the delimiter (space) and adding a list item until there are no more spaces
	// (an iteration with no space adds the rest of the string.

	// Baker: have we tested this against a single item without a space to see what happens?
	// It looks like it can handle that.

	// BUILD LIST

	int			searchpos		= 0;
	while (1) {
		char s_this_copy[MAX_INPUTLINE_16384];
		const char	*space_pos	= strstr (&s_space_delimited[searchpos], s_space_delim); // string_find_pos_start_at(s, s_delim, searchpos);
		int			endpos		= (space_pos == NULL) ? (s_len - 1) : ( (space_pos - s_space_delimited) - 1); // (commapos == not_found_neg1) ? (s_len -1) : (commapos -1);
		int			this_w		= (endpos - searchpos + 1); // string_range_width (searchpos, endpos); (endpos - startpos + 1)

		memcpy (s_this_copy, &s_space_delimited[searchpos], this_w);
		s_this_copy[this_w] = NULL_CHAR_0; // term

		stringlistappend (p_stringlist, s_this_copy);

		// If no space found, we added the rest of the string as an item, so get out!
		if (space_pos == NULL)
			break;

		searchpos = (space_pos - s_space_delimited) + s_delim_len;
	} // while
}

// Supply null or zero length string for optionals
// Returns num matches
int stringlist_from_dir_pattern (stringlist_t *p_stringlist, const char *s_optional_dir_no_slash, const char *s_optional_dot_extension, int wants_strip_extension)
{
	fssearch_t	*t;
	char		s_pattern[1024];
	int			num_matches = 0;
	int			j;
	//int		is_dir = s_optional_dir_no_slash && s_optional_dir_no_slash[0];
	int			is_ext = s_optional_dot_extension && s_optional_dot_extension[0];

	const char	*s_ext = is_ext ? s_optional_dot_extension : "";

	if (s_optional_dir_no_slash) {
			// "%s/*%s"
			c_strlcpy (s_pattern, s_optional_dir_no_slash);
			c_strlcat (s_pattern, "/");
			c_strlcat (s_pattern, "*");
			c_strlcat (s_pattern, s_ext);
	}
	else	c_dpsnprintf1 (s_pattern, "*%s", s_ext);
	
	t = FS_Search(s_pattern, fs_caseless_true, fs_quiet_true, fs_pakfile_null, fs_gamedironly_false);

	if (t && t->numfilenames > 0) {
		for (j = 0; j < t->numfilenames; j ++) {
			char *sxy = t->filenames[j];
			if (wants_strip_extension)
				File_URL_Edit_Remove_Extension (sxy);
			stringlistappend (p_stringlist, sxy);
			num_matches ++;
		} // for
	} // if

	if (t) FS_FreeSearch(t);

	return num_matches;
}



