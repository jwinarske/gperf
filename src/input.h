/* This may look like C code, but it is really -*- C++ -*- */

/* Input routines.

   Copyright (C) 1989-1998, 2002 Free Software Foundation, Inc.
   Written by Douglas C. Schmidt <schmidt@ics.uci.edu>
   and Bruno Haible <bruno@clisp.org>.

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

#ifndef input_h
#define input_h 1

#include "read-line.h"
#include "keyword-list.h"

class Input : private Read_Line
{
public:
  void                  read_keys ();
private:
#ifndef strcspn
  static int            strcspn (const char *s, const char *reject);
#endif
  void                  set_output_types ();
  const char *          get_array_type ();
  const char *          save_include_src ();
  const char *          get_special_input (char delimiter);
public:
  const char *          _array_type;                           /* Pointer to the type for word list. */
  const char *          _return_type;                          /* Pointer to return type for lookup function. */
  const char *          _struct_tag;                           /* Shorthand for user-defined struct tag type. */
  const char *          _include_src;                          /* C source code to be included verbatim. */
  bool                  _additional_code;                      /* True if any additional C code is included. */
  KeywordExt_List *     _head;                            /* Points to the head of the linked list. */
};

#endif
