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

  /* Merges two sorted lists together to form one sorted list.  */
  KeywordExt_List *     merge (KeywordExt_List *list1, KeywordExt_List *list2);
  /* Sorts a list using the recursive merge sort algorithm.  */
  KeywordExt_List *     merge_sort (KeywordExt_List *head);

  int                   compute_occurrence (KeywordExt *ptr);
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

  /* Linked list of keywords.  */
  KeywordExt_List *     _head;

  /* Total number of keywords, counting duplicates.  */
  int                   _total_keys;

  /* Total number of duplicates that have been moved to _duplicate_link lists
     (not counting their representatives which stay on the main list).  */
  int                   _total_duplicates;

  /* Maximum length of the longest keyword.  */
  int                   _max_key_len;

  /* Minimum length of the shortest keyword.  */
  int                   _min_key_len;

  /* Size of alphabet.  */
  int const             _alpha_size;

  /* Counts occurrences of each key set character.
     _occurrences[c] is the number of times that c occurs among the _selchars
     of a keyword.  */
  int * const           _occurrences;
  /* Value associated with each character. */
  int * const           _asso_values;

private:

  /* Length of _head list.  Number of keywords, not counting duplicates.  */
  int                   _list_len;

  /* Choice of sorting criterion during Search::merge_sort.  */
  /* True if sorting by occurrence.  */
  bool                  _occurrence_sort;
  /* True if sorting by hash value.  */
  bool                  _hash_sort;

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
