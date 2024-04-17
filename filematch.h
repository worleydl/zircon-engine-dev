/*
Copyright (C) 2006-2021 DarkPlaces contributors

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

#ifndef FILEMATCH_H
#define FILEMATCH_H

#include "qtypes.h"

typedef struct stringlist_s
{
	/// maxstrings changes as needed, causing reallocation of strings[] array
	int maxstrings;
	int numstrings;
	char **strings;
} stringlist_t;

int matchpattern(const char *in, const char *pattern, int caseinsensitive);
int matchpattern_with_separator(const char *in, const char *pattern, int caseinsensitive, const char *separators, qbool wildcard_least_one);
void stringlistinit(stringlist_t *list);
void stringlistfreecontents(stringlist_t *list);
void stringlistappend(stringlist_t *list, const char *text);
// Moved to darkplaces.h so fssearch_t is defined
//void stringlistappendfssearch (stringlist_t *plist, struct fssearch_s *t); // t == NULL is ok and handled
void stringlistappendlist (stringlist_t *plist, const stringlist_t *add_these); // Baker: Add a list to a list
void stringlistprint (stringlist_t *list, const char *title_optional, void (*myprintf)(const char *, ...) );
void stringlist_replace_at_index (stringlist_t *list, int idx, const char *text);
void stringlistsort(stringlist_t *list, qbool uniq);
void stringlist_condump (stringlist_t *plist);
void stringlistsort_custom(stringlist_t *list, qbool uniq, int myfunc(const void *, const void *) );

void stringlistappend_blob (stringlist_t *list, const byte *blob, size_t blobsize); // Put binary data in a stringlist

void stringlistsort_substring(stringlist_t *list, qbool uniq, int startpos, int slength); // Substring sort

void listdirectory(stringlist_t *list, const char *basepath, const char *path);

void stringlist_to_string (stringlist_t *p_stringlist, char *s_delimiter, char *buf, size_t buflen);
void stringlist_from_delim (stringlist_t *p_stringlist, const char *s_space_delimited);
int stringlist_find_index (stringlist_t *p_stringlist, char *s_find_this);
int stringlist_from_dir_pattern (stringlist_t *p_stringlist, const char *s_optional_dir_no_slash, const char *s_optional_dot_extension, int wants_strip_extension);

#endif
