/* This may look like C code, but it is really -*- C++ -*- */

/* Keyword list.

   Copyright (C) 1989-1998, 2000, 2002 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>.

This file is part of GNU GPERF.

GNU GPERF is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

GNU GPERF is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU GPERF; see the file COPYING.  If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA 02111, USA.  */

#ifndef keyword_list_h
#define keyword_list_h 1

#include "keyword.h"

/* List node of a linear list.  */
class KeywordExt_List : private KeywordExt {
public:
  /* Constructor.  */
  KeywordExt_List (const char *allchars, int allchars_length, const char *rest);

  /* Access to first element of list.  */
  KeywordExt* first () { return this; }
  /* Access to next element of list.  */
  KeywordExt_List *& rest () { return cdr; }

private:
  KeywordExt_List * cdr;
};

#endif
