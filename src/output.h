/* This may look like C code, but it is really -*- C++ -*- */

/* Output routines.

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

#ifndef output_h
#define output_h 1

#include "keyword-list.h"
#include "vectors.h"

/* OSF/1 cxx needs these forward declarations. */
struct Output_Constants;
struct Output_Compare;

class Output
{
public:
  Output (KeywordExt_List *head, const char *array_type, const char *return_type, const char *struct_tag, int additional_code, const char *include_src, int total_keys, int total_duplicates, int max_key_len, int min_key_len, Vectors *v);
  void        output (void);
private:
  void        compute_min_max (void);
  int         num_hash_values (void);
  void        output_constants (struct Output_Constants&);
  void        output_hash_function (void);
  void        output_keylength_table (void);
  void        output_keyword_table (void);
  void        output_lookup_array (void);
  void        output_lookup_tables (void);
  void        output_lookup_function_body (const struct Output_Compare&);
  void        output_lookup_function (void);

  /* Linked list of keywords.  */
  KeywordExt_List *head;

  /* Pointer to the type for word list. */
  const char *array_type;
  /* Pointer to return type for lookup function. */
  const char *return_type;
  /* Shorthand for user-defined struct tag type. */
  const char *struct_tag;
  /* True if any additional C code is included. */
  int         additional_code;
  /* C source code to be included verbatim. */
  const char *include_src;
  /* Total number of keys, counting duplicates. */
  int         total_keys;
  /* Total number of duplicate hash values. */
  int         total_duplicates;
  /* Maximum length of the longest keyword. */
  int         max_key_len;
  /* Minimum length of the shortest keyword. */
  int         min_key_len;
  /* Minimum hash value for all keywords. */
  int         min_hash_value;
  /* Maximum hash value for all keywords. */
  int         max_hash_value;
  Vectors * v;
};

#endif
