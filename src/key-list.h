/* This may look like C code, but it is really -*- C++ -*- */

/* Data and function member declarations for the keyword list class.

   Copyright (C) 1989-1998, 2002 Free Software Foundation, Inc.
   Written by Douglas C. Schmidt <schmidt@ics.uci.edu>
   and Bruno Haible <bruno@clisp.org>.

   This file is part of GNU GPERF.

   GNU GPERF is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU GPERF is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.
   If not, write to the Free Software Foundation, Inc.,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* The key word list is a useful abstraction that keeps track of
   various pieces of information that enable that fast generation
   of the Gen_Perf.hash function.  A Key_List is a singly-linked
   list of List_Nodes. */

#ifndef key_list_h
#define key_list_h 1

#include "keyword-list.h"
#include "vectors.h"
#include "read-line.h"

class Key_List : public Vectors
{
protected:
  const char *          _array_type;                           /* Pointer to the type for word list. */
  const char *          _return_type;                          /* Pointer to return type for lookup function. */
  const char *          _struct_tag;                           /* Shorthand for user-defined struct tag type. */
  const char *          _include_src;                          /* C source code to be included verbatim. */
  int                   _max_key_len;                          /* Maximum length of the longest keyword. */
  int                   _min_key_len;                          /* Minimum length of the shortest keyword. */
private:
  bool                  _occurrence_sort;                      /* True if sorting by occurrence. */
  bool                  _hash_sort;                            /* True if sorting by hash value. */
protected:
  bool                  _additional_code;                      /* True if any additional C code is included. */
private:
  int                   _list_len;                             /* Length of head's Key_List, not counting duplicates. */
protected:
  int                   _total_keys;                           /* Total number of keys, counting duplicates. */
  int                   _size;                                 /* Range of the hash table. */
private:
  static bool           _determined[MAX_ALPHA_SIZE];           /* Used in function reorder, below. */
  static int            get_occurrence (KeywordExt *ptr);
  static bool           already_determined (KeywordExt *ptr);
  static void           set_determined (KeywordExt *ptr);
  void                  dump ();
  KeywordExt_List *     merge (KeywordExt_List *list1, KeywordExt_List *list2);
  KeywordExt_List *     merge_sort (KeywordExt_List *head);

protected:
  KeywordExt_List *     _head;                            /* Points to the head of the linked list. */
  int                   _total_duplicates;                     /* Total number of duplicate hash values. */

public:
                        Key_List ();
                        ~Key_List ();
  int                   keyword_list_length ();
  int                   max_key_length ();
  void                  reorder ();
  void                  sort ();
  void                  read_keys ();
  int                   get_max_keysig_size ();
  void                  set_asso_max (int r) { _size = r; }
  int                   get_asso_max () { return _size; }
};

#endif
