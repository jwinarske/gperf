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
#include "positions.h"
#include "bool-array.h"

class Search
{
public:
                        Search (KeywordExt_List *list);
                        ~Search ();
  void                  optimize ();
private:
  void                  preprepare ();

  /* Initializes each keyword's _selchars array.  */
  void                  init_selchars_tuple (bool use_all_chars, const Positions& positions) const;
  /* Deletes each keyword's _selchars array.  */
  void                  delete_selchars () const;

  /* Count the duplicate keywords that occur with a given set of positions.  */
  unsigned int          count_duplicates_tuple (const Positions& positions) const;

  /* Find good key positions.  */
  void                  find_positions ();

  /* Initializes each keyword's _selchars array.  */
  void                  init_selchars_multiset (bool use_all_chars, const Positions& positions, const unsigned int *alpha_inc) const;

  /* Count the duplicate keywords that occur with the given set of positions
     and a given alpha_inc[] array.  */
  unsigned int          count_duplicates_multiset (const unsigned int *alpha_inc) const;

  /* Find good _alpha_inc[].  */
  void                  find_alpha_inc ();

  void                  prepare ();

  /* Computes the sum of occurrences of the _selchars of a keyword.  */
  int                   compute_occurrence (KeywordExt *ptr) const;

  /* Auxiliary functions used by Search::reorder().  */
  void                  clear_determined ();
  void                  set_determined (KeywordExt *keyword);
  bool                  already_determined (KeywordExt *keyword) const;
  /* Reorders the keyword list so as to minimize search times.  */
  void                  reorder ();

  /* Returns the length of keyword list.  */
  int                   keyword_list_length () const;

  /* Returns the maximum length of keywords.  */
  int                   max_key_length () const;

  /* Returns the number of key positions.  */
  int                   get_max_keysig_size () const;

  /* Initializes the asso_values[] related parameters.  */
  void                  prepare_asso_values ();
  /* Puts a first guess into asso_values[].  */
  void                  init_asso_values ();

  /* Computes a keyword's hash value, relative to the current _asso_values[],
     and stores it in keyword->_hash_value.  */
  int                   compute_hash (KeywordExt *keyword) const;

  /* Computes the frequency of occurrence of a character among the keywords
     up to the given keyword.  */
  unsigned int          compute_occurrence (unsigned int c, KeywordExt *curr) const;

  /* Sorts the given set in increasing frequency of _occurrences[].  */
  void                  sort_by_occurrence (unsigned int *set, unsigned int len) const;
  /* Sorts the given set in increasing frequency of occurrences among the
     keywords up to the given keyword.  */
  void                  sort_by_occurrence (unsigned int *set, unsigned int len, KeywordExt *curr) const;

  bool                  has_collisions (KeywordExt *curr);

  KeywordExt *          collision_prior_to (KeywordExt *curr);

  /* Finds some _asso_values[] that fit.  */
  void                  find_asso_values ();

  /* Finds good _asso_values[].  */
  void                  find_good_asso_values ();

  /* Sorts the keyword list by hash value.  */
  void                  sort ();

public:

  /* Linked list of keywords.  */
  KeywordExt_List *     _head;

  /* Total number of keywords, counting duplicates.  */
  int                   _total_keys;

  /* Maximum length of the longest keyword.  */
  int                   _max_key_len;

  /* Minimum length of the shortest keyword.  */
  int                   _min_key_len;

  /* User-specified or computed key positions.  */
  Positions             _key_positions;

  /* Adjustments to add to bytes add specific key positions.  */
  unsigned int *        _alpha_inc;

  /* Total number of duplicates that have been moved to _duplicate_link lists
     (not counting their representatives which stay on the main list).  */
  int                   _total_duplicates;

  /* Size of alphabet.  */
  unsigned int          _alpha_size;

  /* Counts occurrences of each key set character.
     _occurrences[c] is the number of times that c occurs among the _selchars
     of a keyword.  */
  int *                 _occurrences;
  /* Value associated with each character. */
  int *                 _asso_values;

private:

  /* Length of _head list.  Number of keywords, not counting duplicates.  */
  int                   _list_len;

  /* Vector used during Search::reorder().  */
  bool *                _determined;

  /* Exclusive upper bound for every _asso_values[c].  A power of 2.  */
  unsigned int          _asso_value_max;

  /* Initial value for asso_values table.  -1 means random.  */
  int                   _initial_asso_value;
  /* Jump length when trying alternative values.  0 means random.  */
  int                   _jump;

  /* Maximal possible hash value.  */
  int                   _max_hash_value;

  /* Sparse bit vector for collision detection.  */
  Bool_Array *          _collision_detector;
};

#endif
