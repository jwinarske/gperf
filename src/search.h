/* This may look like C code, but it is really -*- C++ -*- */

/* Search algorithm.

   Copyright (C) 1989-1998, 2000, 2002 Free Software Foundation, Inc.
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

#ifndef search_h
#define search_h 1

#include "keyword-list.h"
#include "bool-array.h"

class Search
{
public:
                        Search (KeywordExt_List *list);
                        ~Search ();
  void                  optimize ();
private:
  void                  prepare ();
  KeywordExt_List *     merge (KeywordExt_List *list1, KeywordExt_List *list2);
  KeywordExt_List *     merge_sort (KeywordExt_List *head);
  int                   get_occurrence (KeywordExt *ptr);
  void                  set_determined (KeywordExt *ptr);
  bool                  already_determined (KeywordExt *ptr);
  void                  reorder ();
  int                   keyword_list_length ();
  int                   max_key_length ();
  int                   get_max_keysig_size ();
  int                   hash (KeywordExt *key_node);
  static int            compute_disjoint_union (const unsigned char *set_1, int size_1, const unsigned char *set_2, int size_2, unsigned char *set_3);
  void                  sort_set (unsigned char *union_set, int len);
  bool                  affects_prev (unsigned char c, KeywordExt *curr);
  void                  change (KeywordExt *prior, KeywordExt *curr);
  void                  sort ();
public:
  KeywordExt_List *     _head;                            /* Points to the head of the linked list. */
  int                   _total_keys;                           /* Total number of keys, counting duplicates. */
  int                   _total_duplicates;                     /* Total number of duplicate hash values. */
  int                   _max_key_len;                          /* Maximum length of the longest keyword. */
  int                   _min_key_len;                          /* Minimum length of the shortest keyword. */
  /* Size of alphabet. */
  int const             _alpha_size;
  /* Counts occurrences of each key set character. */
  int * const           _occurrences;
  /* Value associated with each character. */
  int * const           _asso_values;
private:
  int                   _list_len;                             /* Length of head's Key_List, not counting duplicates. */
  bool                  _occurrence_sort;                      /* True if sorting by occurrence. */
  bool                  _hash_sort;                            /* True if sorting by hash value. */
  bool * const          _determined;                           /* Used in function reorder, below. */
  int                   _num_done;          /* Number of keywords processed without a collision. */
  int                   _fewest_collisions; /* Records fewest # of collisions for asso value. */
  int                   _max_hash_value;    /* Maximum possible hash value. */
  Bool_Array *          _collision_detector;
  int                   _size;                                 /* Range of the hash table. */
  void                  set_asso_max (int r) { _size = r; }
  int                   get_asso_max () { return _size; }
};

#endif
