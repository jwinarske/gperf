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

/* Specification. */
#include "search.h"

#include <stdio.h>
#include <stdlib.h> /* declares exit(), rand(), srand() */
#include <string.h> /* declares memset(), memcmp() */
#include <time.h> /* declares time() */
#include <limits.h> /* defines INT_MIN, INT_MAX */
#include "options.h"
#include "hash-table.h"

/* -------------------- Initialization and Preparation --------------------- */

Search::Search (KeywordExt_List *list)
  : _head (list),
    _alpha_size (option[SEVENBIT] ? 128 : 256),
    _occurrences (new int[_alpha_size]),
    _asso_values (new int[_alpha_size]),
    _determined (new bool[_alpha_size])
{
}

void
Search::prepare ()
{
  KeywordExt_List *temp;

  /* Compute the total number of keywords.  */
  _total_keys = 0;
  for (temp = _head; temp; temp = temp->rest())
    _total_keys++;

  /* Initialize each keyword's _selchars array.  */
  for (temp = _head; temp; temp = temp->rest())
    temp->first()->init_selchars();

  /* Compute the minimum and maximum keyword length.  */
  _max_key_len = INT_MIN;
  _min_key_len = INT_MAX;
  for (temp = _head; temp; temp = temp->rest())
    {
      KeywordExt *keyword = temp->first();

      if (_max_key_len < keyword->_allchars_length)
        _max_key_len = keyword->_allchars_length;
      if (_min_key_len > keyword->_allchars_length)
        _min_key_len = keyword->_allchars_length;
    }

  /* Exit program if an empty string is used as key, since the comparison
     expressions don't work correctly for looking up an empty string.  */
  if (_min_key_len == 0)
    {
      fprintf (stderr, "Empty input key is not allowed.\n"
                       "To recognize an empty input key, your code should check for\n"
                       "len == 0 before calling the gperf generated lookup function.\n");
      exit (1);
    }

  /* Check for duplicates, i.e. keywords with the same _selchars array
     (and - if !option[NOLENGTH] - also the same length).
     We deal with these by building an equivalence class, so that only
     1 keyword is representative of the entire collection.  Only this
     representative remains in the keyword list; the others are accessible
     through the _duplicate_link chain, starting at the representative.
     This *greatly* simplifies processing during later stages of the program.
     Set _total_duplicates and _list_len = _total_keys - _total_duplicates.  */
  {
    _list_len = _total_keys;
    _total_duplicates = 0;
    /* Make hash table for efficiency.  */
    Hash_Table representatives (_list_len, option[NOLENGTH]);

    KeywordExt_List *prev = NULL; /* list node before temp */
    for (temp = _head; temp; )
      {
        KeywordExt *keyword = temp->first();
        KeywordExt *other_keyword = representatives.insert (keyword);
        KeywordExt_List *garbage = NULL;

        if (other_keyword)
          {
            _total_duplicates++;
            _list_len--;
            /* Remove keyword from the main list.  */
            prev->rest() = temp->rest();
            garbage = temp;
            /* And insert it on other_keyword's duplicate list.  */
            keyword->_duplicate_link = other_keyword->_duplicate_link;
            other_keyword->_duplicate_link = keyword;

            /* Complain if user hasn't enabled the duplicate option. */
            if (!option[DUP] || option[DEBUG])
              fprintf (stderr, "Key link: \"%.*s\" = \"%.*s\", with key set \"%.*s\".\n",
                               keyword->_allchars_length, keyword->_allchars,
                               other_keyword->_allchars_length, other_keyword->_allchars,
                               keyword->_selchars_length, keyword->_selchars);
          }
        else
          {
            keyword->_duplicate_link = NULL;
            prev = temp;
          }
        temp = temp->rest();
        if (garbage)
          delete garbage;
      }
  }

  /* Exit program if duplicates exists and option[DUP] not set, since we
     don't want to continue in this case.  (We don't want to turn on
     option[DUP] implicitly, because the generated code is usually much
     slower.  */
  if (_total_duplicates)
    {
      if (option[DUP])
        fprintf (stderr, "%d input keys have identical hash values, examine output carefully...\n",
                         _total_duplicates);
      else
        {
          fprintf (stderr, "%d input keys have identical hash values,\ntry different key positions or use option -D.\n",
                           _total_duplicates);
          exit (1);
        }
    }

  /* Compute the occurrences of each character in the alphabet.  */
  memset (_occurrences, 0, _alpha_size * sizeof (_occurrences[0]));
  for (temp = _head; temp; temp = temp->rest())
    {
      KeywordExt *keyword = temp->first();
      const unsigned char *ptr = keyword->_selchars;
      for (int count = keyword->_selchars_length; count > 0; ptr++, count--)
        _occurrences[*ptr]++;
    }
}

/* ---------------- Reordering the Keyword list (optional) ----------------- */

/* Computes the sum of occurrences of the _selchars of a keyword.
   This is a kind of correlation measure: Keywords which have many
   selected characters in common with other keywords have a high
   occurrence sum.  Keywords whose selected characters don't occur
   in other keywords have a low occurrence sum.  */

inline int
Search::compute_occurrence (KeywordExt *ptr) const
{
  int value = 0;

  const unsigned char *p = ptr->_selchars;
  unsigned int i = ptr->_selchars_length;
  for (; i > 0; p++, i--)
    value += _occurrences[*p];

  return value;
}

/* Comparison function for sorting by decreasing _occurrence valuation.  */
static bool
greater_by_occurrence (KeywordExt *keyword1, KeywordExt *keyword2)
{
  return keyword1->_occurrence > keyword2->_occurrence;
}

/* Auxiliary function for reorder():
   Sets all alphabet characters as undetermined.  */

inline void
Search::clear_determined ()
{
  memset (_determined, 0, _alpha_size * sizeof (_determined[0]));
}

/* Auxiliary function for reorder():
   Sets all selected characters of the keyword as determined.  */

inline void
Search::set_determined (KeywordExt *keyword)
{
  const unsigned char *p = keyword->_selchars;
  unsigned int i = keyword->_selchars_length;
  for (; i > 0; p++, i--)
    _determined[*p] = true;
}

/* Auxiliary function for reorder():
   Returns true if the keyword's selected characters are all determined.  */

inline bool
Search::already_determined (KeywordExt *keyword) const
{
  const unsigned char *p = keyword->_selchars;
  unsigned int i = keyword->_selchars_length;
  for (; i > 0; p++, i--)
    if (!_determined[*p])
      return false;

  return true;
}

/* Reorders the keyword list so as to minimize search times.
   First the list is reordered so that frequently occuring keys appear first.
   Then the list is reordered so that keys whose values are already determined
   will be placed towards the front of the list.  This helps prune the search
   time by handling inevitable collisions early in the search process.  See
   Cichelli's paper from Jan 1980 JACM for details.... */

void
Search::reorder ()
{
  KeywordExt_List *ptr;

  /* Compute the _occurrence valuation of every keyword on the list.  */
  for (ptr = _head; ptr; ptr = ptr->rest())
    {
      KeywordExt *keyword = ptr->first();

      keyword->_occurrence = compute_occurrence (keyword);
    }

  /* Sort the list by decreasing _occurrence valuation.  */
  _head = mergesort_list (_head, greater_by_occurrence);

  /* Reorder the list to maximize the efficiency of the search.  */

  /* At the beginning, consider that no asso_values[c] is fixed.  */
  clear_determined ();
  for (ptr = _head; ptr != NULL && ptr->rest() != NULL; ptr = ptr->rest())
    {
      KeywordExt *keyword = ptr->first();

      /* Then we'll fix asso_values[c] for all c occurring in this keyword.  */
      set_determined (keyword);

      /* Then we wish to test for hash value collisions the remaining keywords
         whose hash value is completely determined, as quickly as possible.
         For this purpose, move all the completely determined keywords in the
         remaining list immediately past this keyword.  */
      KeywordExt_List *curr_ptr;
      KeywordExt_List *next_ptr; /* = curr_ptr->rest() */
      for (curr_ptr = ptr, next_ptr = curr_ptr->rest();
           next_ptr != NULL;
           next_ptr = curr_ptr->rest())
        {
          KeywordExt *next_keyword = next_ptr->first();

          if (already_determined (next_keyword))
            {
              if (curr_ptr == ptr)
                /* Keep next_ptr where it is.  */
                curr_ptr = next_ptr;
              else
                {
                  /* Remove next_ptr from its current list position... */
                  curr_ptr->rest() = next_ptr->rest();
                  /* ... and insert it right after ptr.  */
                  next_ptr->rest() = ptr->rest();
                  ptr->rest() = next_ptr;
                }

              /* Advance ptr.  */
              ptr = ptr->rest();
            }
          else
            curr_ptr = next_ptr;
        }
    }
}

/* ------------------------------------------------------------------------- */

/* Returns the length of keyword list.  */

int
Search::keyword_list_length () const
{
  return _list_len;
}

/* Returns the maximum length of keywords.  */

int
Search::max_key_length () const
{
  return _max_key_len;
}

/* Returns the number of key positions.  */

int
Search::get_max_keysig_size () const
{
  return option[ALLCHARS] ? _max_key_len : option.get_max_keysig_size ();
}

/* ---------------------- Finding good asso_values[] ----------------------- */

/* Initializes the asso_values[] related parameters.  */

void
Search::prepare_asso_values ()
{
  int size_multiple = option.get_size_multiple ();
  int non_linked_length = keyword_list_length ();
  int asso_value_max;

  if (size_multiple == 0)
    asso_value_max = non_linked_length;
  else if (size_multiple > 0)
    asso_value_max = non_linked_length * size_multiple;
  else /* if (size_multiple < 0) */
    asso_value_max = non_linked_length / -size_multiple;
  /* Round up to the next power of two.  This makes it easy to ensure
     an _asso_value[c] is >= 0 and < asso_value_max.  Also, the jump value
     being odd, it guarantees that Search::try_asso_value() will iterate
     through different values for _asso_value[c].  */
  if (asso_value_max == 0)
    asso_value_max = 1;
  asso_value_max |= asso_value_max >> 1;
  asso_value_max |= asso_value_max >> 2;
  asso_value_max |= asso_value_max >> 4;
  asso_value_max |= asso_value_max >> 8;
  asso_value_max |= asso_value_max >> 16;
  asso_value_max++;
  _asso_value_max = asso_value_max;

  /* Given the bound for _asso_values[c], we have a bound for the possible
     hash values, as computed in compute_hash().  */
  _max_hash_value = (option[NOLENGTH] ? 0 : max_key_length ())
                    + (_asso_value_max - 1) * get_max_keysig_size ();
  /* Allocate a sparse bit vector for detection of collisions of hash
     values.  */
  _collision_detector = new Bool_Array (_max_hash_value + 1);

  /* Allocate scratch set.  */
  _union_set = new unsigned char [2 * get_max_keysig_size ()];

  if (option[DEBUG])
    fprintf (stderr, "total non-linked keys = %d\nmaximum associated value is %d"
             "\nmaximum size of generated hash table is %d\n",
             non_linked_length, asso_value_max, _max_hash_value);

  if (option[RANDOM] || option.get_jump () == 0)
    /* We will use rand(), so initialize the random number generator.  */
    srand (reinterpret_cast<long>(time (0)));

  _initial_asso_value = (option[RANDOM] ? -1 : option.get_initial_asso_value ());
  _jump = option.get_jump ();
}

/* Puts a first guess into asso_values[].  */

void
Search::init_asso_values ()
{
  if (_initial_asso_value < 0)
    {
      for (int i = 0; i < _alpha_size; i++)
        _asso_values[i] = rand () & (_asso_value_max - 1);
    }
  else
    {
      int asso_value = _initial_asso_value;

      asso_value = asso_value & (_asso_value_max - 1);
      for (int i = 0; i < _alpha_size; i++)
        _asso_values[i] = asso_value;
    }
}

/* Computes a keyword's hash value, relative to the current _asso_values[],
   and stores it in keyword->_hash_value.
   This is called very frequently, and needs to be fast!  */

inline int
Search::compute_hash (KeywordExt *keyword) const
{
  int sum = option[NOLENGTH] ? 0 : keyword->_allchars_length;

  const unsigned char *p = keyword->_selchars;
  int i = keyword->_selchars_length;
  for (; i > 0; p++, i--)
      sum += _asso_values[*p];

  return keyword->_hash_value = sum;
}

/* Computes the disjoint union of two multisets of characters, i.e.
   the set of characters that are contained with a different multiplicity
   in set_1 and set_2.  This includes those characters which are contained
   in one of the sets but not both.
   Both sets set_1[0..size_1-1] and set_2[0..size_2-1] are given ordered.
   The result, an ordered set (not multiset!) is stored in set_3[0...].
   Returns the size of the resulting set.  */

inline int
compute_disjoint_union (const unsigned char *set_1, int size_1,
                        const unsigned char *set_2, int size_2,
                        unsigned char *set_3)
{
  unsigned char *base = set_3;

  while (size_1 > 0 && size_2 > 0)
    if (*set_1 == *set_2)
      {
        set_1++, size_1--;
        set_2++, size_2--;
      }
    else
      {
        unsigned char next;
        if (*set_1 < *set_2)
          next = *set_1++, size_1--;
        else
          next = *set_2++, size_2--;
        if (set_3 == base || next != set_3[-1])
          *set_3++ = next;
      }

  while (size_1 > 0)
    {
      unsigned char next;
      next = *set_1++, size_1--;
      if (set_3 == base || next != set_3[-1])
        *set_3++ = next;
    }

  while (size_2 > 0)
    {
      unsigned char next;
      next = *set_2++, size_2--;
      if (set_3 == base || next != set_3[-1])
        *set_3++ = next;
    }
  return set_3 - base;
}

/* Sorts the given set in increasing frequency of _occurrences[].  */

inline void
Search::sort_by_occurrence (unsigned char *set, int len) const
{
  /* Use bubble sort, since the set is typically short.  */
  for (int i = 1; i < len; i++)
    {
      int curr;
      unsigned char tmp;

      for (curr = i, tmp = set[curr];
           curr > 0 && _occurrences[tmp] < _occurrences[set[curr-1]];
           curr--)
        set[curr] = set[curr - 1];

      set[curr] = tmp;
    }
}

/* Tries various other values for _asso_values[c].  A value is successful
   if, with it, the recomputed hash values for the keywords from
   _head->first() to curr - inclusive - give fewer than _fewest_collisions
   collisions.  Up to the given number of iterations are performed.
   If successful, _asso_values[c] is changed, _fewest_collisions is decreased,
   and false is returned.
   If all iterations are unsuccessful, _asso_values[c] is restored and
   true is returned.
   This is called very frequently, and needs to be fast!  */

inline bool
Search::try_asso_value (unsigned char c, KeywordExt *curr, int iterations)
{
  int original_value = _asso_values[c];

  /* Try many valid associated values.  */
  for (int i = iterations - 1; i >= 0; i--)
    {
      int collisions = 0;

      /* Try next value.  Wrap around mod _asso_value_max.  */
      _asso_values[c] =
        (_asso_values[c] + (_jump != 0 ? _jump : rand ()))
        & (_asso_value_max - 1);

      /* Iteration Number array is a win, O(1) intialization time!  */
      _collision_detector->clear ();

      for (KeywordExt_List *ptr = _head; ; ptr = ptr->rest())
        {
          KeywordExt *keyword = ptr->first();

          /* Compute new hash code for the keyword, and see whether it
             collides with another keyword's hash code.  If we have too
             many collisions, we can safely abort the fruitless loop.  */
          if (_collision_detector->set_bit (compute_hash (keyword))
              && ++collisions >= _fewest_collisions)
            break;

          if (keyword == curr)
            {
              _fewest_collisions = collisions;
              if (option[DEBUG])
                fprintf (stderr, "- resolved after %d iterations",
                         iterations - i);
              return false;
            }
        }
    }

  /* Restore original values, no more tries.  */
  _asso_values[c] = original_value;
  return true;
}

/* Attempts to change an _asso_value[], in order to resolve a hash value
   collision between the two given keywords.  */

void
Search::change_some_asso_value (KeywordExt *prior, KeywordExt *curr)
{
  if (option[DEBUG])
    {
      fprintf (stderr, "collision on keyword #%d, prior = \"%.*s\", curr = \"%.*s\" hash = %d\n",
               _num_done,
               prior->_allchars_length, prior->_allchars,
               curr->_allchars_length, curr->_allchars,
               curr->_hash_value);
      fflush (stderr);
    }

  /* To achieve that the two hash values become different, we have to
     change an _asso_values[c] for a character c that contributes to the
     hash functions of prior and curr with different multiplicity.
     So we compute the set of such c.  */
  unsigned char *union_set = _union_set;
  int union_set_length =
    compute_disjoint_union (prior->_selchars, prior->_selchars_length,
                            curr->_selchars, curr->_selchars_length,
                            union_set);

  /* Sort by decreasing occurrence: Try least-used characters c first.
     The idea is that this reduces the number of freshly introduced
     collisions.  */
  sort_by_occurrence (union_set, union_set_length);

  int iterations =
    !option[FAST]
    ? _asso_value_max    /* Try all possible values of _asso_values[c].  */
    : option.get_iterations ()
      ? option.get_iterations ()
      : keyword_list_length ();

  const unsigned char *p = union_set;
  int i = union_set_length;
  for (; i > 0; p++, i--)
    if (!try_asso_value (*p, curr, iterations))
      {
        /* Good, this _asso_values[] modification reduces the number of
           collisions so far.
           All keyword->_hash_value up to curr - inclusive - and
           _fewest_collisions have been updated.  */
        if (option[DEBUG])
          {
            fprintf (stderr, " by changing asso_value['%c'] (char #%d) to %d\n",
                     *p, p - union_set + 1, _asso_values[*p]);
            fflush (stderr);
          }
        return;
      }

  /* Failed to resolve a collision.  */

  /* Recompute all keyword->_hash_value up to curr - inclusive -.  */
  for (KeywordExt_List *ptr = _head; ; ptr = ptr->rest())
    {
      KeywordExt* keyword = ptr->first();
      compute_hash (keyword);
      if (keyword == curr)
        break;
    }

  if (option[DEBUG])
    {
      fprintf (stderr, "** collision not resolved after %d iterations, %d duplicates remain, continuing...\n",
               iterations, _fewest_collisions + _total_duplicates);
      fflush (stderr);
    }
}

/* Finds good _asso_values[].  */

void
Search::find_asso_values ()
{
  _fewest_collisions = 0;
  init_asso_values ();

  /* Add one keyword after the other and see whether its hash value collides
     with one of the previous hash values.  */
  _num_done = 1;
  for (KeywordExt_List *curr_ptr = _head;
       curr_ptr != NULL;
       curr_ptr = curr_ptr->rest(), _num_done++)
    {
      KeywordExt *curr = curr_ptr->first();

      /* Compute this keyword's hash value.  */
      compute_hash (curr);

      /* See if it collides with a prior keyword.  */
      for (KeywordExt_List *prior_ptr = _head;
           prior_ptr != curr_ptr;
           prior_ptr = prior_ptr->rest())
        {
          KeywordExt *prior = prior_ptr->first();

          if (prior->_hash_value == curr->_hash_value)
            {
              _fewest_collisions++;
              /* Handle collision.  */
              change_some_asso_value (prior, curr);
              break;
            }
        }
    }
}

/* ------------------------------------------------------------------------- */

/* Comparison function for sorting by increasing _hash_value.  */
static bool
less_by_hash_value (KeywordExt *keyword1, KeywordExt *keyword2)
{
  return keyword1->_hash_value < keyword2->_hash_value;
}

/* Sorts the keyword list by hash value.  */

void
Search::sort ()
{
  _head = mergesort_list (_head, less_by_hash_value);
}

void
Search::optimize ()
{
  /* Preparations.  */
  prepare ();
  if (option[ORDER])
    reorder ();
  prepare_asso_values ();

  /* Search for good _asso_values[].  */
  int asso_iteration;
  if ((asso_iteration = option.get_asso_iterations ()) == 0)
    /* Try only the given _initial_asso_value and _jump.  */
    find_asso_values ();
  else
    {
      /* Try different pairs of _initial_asso_value and _jump, in the
         following order:
           (0, 1)
           (1, 1)
           (2, 1) (0, 3)
           (3, 1) (1, 3)
           (4, 1) (2, 3) (0, 5)
           (5, 1) (3, 3) (1, 5)
           ..... */
      KeywordExt_List *saved_head = _head;
      int best_initial_asso_value = 0;
      int best_jump = 1;
      int *best_asso_values = new int[_alpha_size];
      int best_collisions = INT_MAX;
      int best_max_hash_value = INT_MAX;

      _initial_asso_value = 0; _jump = 1;
      for (;;)
        {
          /* Restore the keyword list in its original order.  */
          _head = copy_list (saved_head);
          /* Find good _asso_values[].  */
          find_asso_values ();
          /* Test whether it is the best solution so far.  */
          int collisions = 0;
          int max_hash_value = INT_MIN;
          _collision_detector->clear ();
          for (KeywordExt_List *ptr = _head; ptr; ptr = ptr->rest())
            {
              KeywordExt *keyword = ptr->first();
              int hashcode = compute_hash (keyword);
              if (max_hash_value < hashcode)
                max_hash_value = hashcode;
              if (_collision_detector->set_bit (hashcode))
                collisions++;
            }
          if (collisions < best_collisions
              || (collisions == best_collisions
                  && max_hash_value < best_max_hash_value))
            {
              memcpy (best_asso_values, _asso_values,
                      _alpha_size * sizeof (_asso_values[0]));
              best_collisions = collisions;
              best_max_hash_value = max_hash_value;
            }
          /* Delete the copied keyword list.  */
          delete_list (_head);

          if (--asso_iteration == 0)
            break;
          /* Prepare for next iteration.  */
          if (_initial_asso_value >= 2)
            _initial_asso_value -= 2, _jump += 2;
          else
            _initial_asso_value += _jump, _jump = 1;
        }
      _head = saved_head;
      /* Install the best found asso_values.  */
      _initial_asso_value = best_initial_asso_value;
      _jump = best_jump;
      memcpy (_asso_values, best_asso_values,
              _alpha_size * sizeof (_asso_values[0]));
      delete[] best_asso_values;
      /* The keywords' _hash_value fields are recomputed below.  */
    }

  /* Make one final check, just to make sure nothing weird happened.... */
  _collision_detector->clear ();
  for (KeywordExt_List *curr_ptr = _head; curr_ptr; curr_ptr = curr_ptr->rest())
    {
      KeywordExt *curr = curr_ptr->first();
      unsigned int hashcode = compute_hash (curr);
      if (_collision_detector->set_bit (hashcode))
        {
          if (option[DUP]) /* Keep track of this number... */
            _total_duplicates++;
          else /* Yow, big problems.  we're outta here! */
            {
              fprintf (stderr,
                       "\nInternal error, duplicate value %d:\n"
                       "try options -D or -m or -r, or use new key positions.\n\n",
                       hashcode);
              exit (1);
            }
        }
    }

  /* Sorts the keyword list by hash value.  */
  sort ();
}

/* Prints out some diagnostics upon completion.  */

Search::~Search ()
{
  delete[] _union_set;
  delete _collision_detector;
  delete[] _determined;
  if (option[DEBUG])
    {
      fprintf (stderr, "\ndumping occurrence and associated values tables\n");

      for (int i = 0; i < _alpha_size; i++)
        if (_occurrences[i])
          fprintf (stderr, "asso_values[%c] = %6d, occurrences[%c] = %6d\n",
                   i, _asso_values[i], i, _occurrences[i]);

      fprintf (stderr, "end table dumping\n");

      fprintf (stderr, "\nDumping key list information:\ntotal non-static linked keywords = %d"
               "\ntotal keywords = %d\ntotal duplicates = %d\nmaximum key length = %d\n",
               _list_len, _total_keys, _total_duplicates, _max_key_len);

      int field_width = get_max_keysig_size ();
      fprintf (stderr, "\nList contents are:\n(hash value, key length, index, %*s, keyword):\n",
               field_width, "selchars");
      for (KeywordExt_List *ptr = _head; ptr; ptr = ptr->rest())
        fprintf (stderr, "%11d,%11d,%6d, %*.*s, %.*s\n",
                 ptr->first()->_hash_value, ptr->first()->_allchars_length, ptr->first()->_final_index,
                 field_width, ptr->first()->_selchars_length, ptr->first()->_selchars,
                 ptr->first()->_allchars_length, ptr->first()->_allchars);

      fprintf (stderr, "End dumping list.\n\n");
    }
  delete[] _asso_values;
  delete[] _occurrences;
}
