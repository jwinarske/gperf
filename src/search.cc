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
#include <limits.h> /* defines INT_MIN, INT_MAX, UINT_MAX */
#include "options.h"
#include "hash-table.h"

/* The most general form of the hash function is

      hash (keyword) = sum (asso_values[keyword[i] + alpha_inc[i]] : i in Pos)
                       + len (keyword)

   where Pos is a set of byte positions,
   each alpha_inc[i] is a nonnegative integer,
   each asso_values[c] is a nonnegative integer,
   len (keyword) is the keyword's length if !option[NOLENGTH], or 0 otherwise.

   Theorem 1: If all keywords are different, there is a set Pos such that
   all tuples (keyword[i] : i in Pos) are different.

   Theorem 2: If all tuples (keyword[i] : i in Pos) are different, there
   are nonnegative integers alpha_inc[i] such that all multisets
   {keyword[i] + alpha_inc[i] : i in Pos} are different.

   Theorem 3: If all multisets selchars[keyword] are different, there are
   nonnegative integers asso_values[c] such that all hash values
   sum (asso_values[c] : c in selchars[keyword]) are different.

   Based on these three facts, we find the hash function in three steps:

   Step 1 (Finding good byte positions):
   Find a set Pos, as small as possible, such that all tuples
   (keyword[i] : i in Pos) are different.

   Step 2 (Finding good alpha increments):
   Find nonnegative integers alpha_inc[i], as many of them as possible being
   zero, and the others being as small as possible, such that all multisets
   {keyword[i] + alpha_inc[i] : i in Pos} are different.

   Step 3 (Finding good asso_values):
   Find asso_values[c] such that all hash (keyword) are different.

   In other words, each step finds a projection that is injective on the
   given finite set:
     proj1 : String --> Map (Pos --> N)
     proj2 : Map (Pos --> N) --> Map (Pos --> N) / S(Pos)
     proj3 : Map (Pos --> N) / S(Pos) --> N
   where N denotes the set of nonnegative integers, and S(Pos) is the
   symmetric group over Pos.

   This was the theory for option[NOLENGTH]; if !option[NOLENGTH], slight
   modifications apply:
     proj1 : String --> Map (Pos --> N) x N
     proj2 : Map (Pos --> N) x N --> Map (Pos --> N) / S(Pos) x N
     proj3 : Map (Pos --> N) / S(Pos) x N --> N
 */

/* ==================== Initialization and Preparation ===================== */

Search::Search (KeywordExt_List *list)
  : _head (list)
{
}

void
Search::preprepare ()
{
  KeywordExt_List *temp;

  /* Compute the total number of keywords.  */
  _total_keys = 0;
  for (temp = _head; temp; temp = temp->rest())
    _total_keys++;

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
}

/* ====================== Finding good byte positions ====================== */

/* Initializes each keyword's _selchars array.  */
void
Search::init_selchars_tuple (bool use_all_chars, const Positions& positions) const
{
  for (KeywordExt_List *temp = _head; temp; temp = temp->rest())
    temp->first()->init_selchars_tuple(use_all_chars, positions);
}

/* Deletes each keyword's _selchars array.  */
void
Search::delete_selchars () const
{
  for (KeywordExt_List *temp = _head; temp; temp = temp->rest())
    temp->first()->delete_selchars();
}

/* Count the duplicate keywords that occur with a given set of positions.
   In other words, it returns the difference
     # K - # proj1 (K)
   where K is the multiset of given keywords.  */
unsigned int
Search::count_duplicates_tuple (const Positions& positions) const
{
  /* Run through the keyword list and count the duplicates incrementally.
     The result does not depend on the order of the keyword list, thanks to
     the formula above.  */
  init_selchars_tuple (option[ALLCHARS], positions);

  unsigned int count = 0;
  {
    Hash_Table representatives (_total_keys, option[NOLENGTH]);
    for (KeywordExt_List *temp = _head; temp; temp = temp->rest())
      {
        KeywordExt *keyword = temp->first();
        if (representatives.insert (keyword))
          count++;
      }
  }

  delete_selchars ();

  return count;
}

/* Find good key positions.  */

void
Search::find_positions ()
{
  /* If the user gave the key positions, we use them.  */
  if (option[POSITIONS])
    {
      _key_positions = option.get_key_positions();
      return;
    }

  /* 1. Find positions that must occur in order to distinguish duplicates.  */
  Positions mandatory;

  if (!option[DUP])
    {
      for (KeywordExt_List *l1 = _head; l1 && l1->rest(); l1 = l1->rest())
        {
          KeywordExt *keyword1 = l1->first();
          for (KeywordExt_List *l2 = l1->rest(); l2; l2 = l2->rest())
            {
              KeywordExt *keyword2 = l2->first();

              /* If keyword1 and keyword2 have the same length and differ
                 in just one position, and it is not the last character,
                 this position is mandatory.  */
              if (keyword1->_allchars_length == keyword2->_allchars_length)
                {
                  int n = keyword1->_allchars_length;
                  int i;
                  for (i = 1; i < n; i++)
                    if (keyword1->_allchars[i-1] != keyword2->_allchars[i-1])
                      break;
                  if (i < n
                      && memcmp (&keyword1->_allchars[i],
                                 &keyword2->_allchars[i],
                                 n - i)
                         == 0)
                    {
                      /* Position i is mandatory.  */
                      if (!mandatory.contains (i))
                        mandatory.add (i);
                    }
                }
            }
        }
    }

  /* 2. Add positions, as long as this decreases the duplicates count.  */
  int imax = (_max_key_len < Positions::MAX_KEY_POS
              ? _max_key_len : Positions::MAX_KEY_POS);
  Positions current = mandatory;
  unsigned int current_duplicates_count = count_duplicates_tuple (current);
  for (;;)
    {
      Positions best;
      unsigned int best_duplicates_count = UINT_MAX;

      for (int i = imax; i >= 0; i--)
        if (!current.contains (i))
          {
            Positions tryal = current;
            tryal.add (i);
            unsigned int try_duplicates_count = count_duplicates_tuple (tryal);

            /* We prefer 'try' to 'best' if it produces less duplicates,
               or if it produces the same number of duplicates but with
               a more efficient hash function.  */
            if (try_duplicates_count < best_duplicates_count
                || (try_duplicates_count == best_duplicates_count && i > 0))
              {
                best = tryal;
                best_duplicates_count = try_duplicates_count;
              }
          }

      /* Stop adding positions when it gives no improvement.  */
      if (best_duplicates_count >= current_duplicates_count)
        break;

      current = best;
      current_duplicates_count = best_duplicates_count;
    }

  /* 3. Remove positions, as long as this doesn't increase the duplicates
     count.  */
  for (;;)
    {
      Positions best;
      unsigned int best_duplicates_count = UINT_MAX;

      for (int i = imax; i >= 0; i--)
        if (current.contains (i) && !mandatory.contains (i))
          {
            Positions tryal = current;
            tryal.remove (i);
            unsigned int try_duplicates_count = count_duplicates_tuple (tryal);

            /* We prefer 'try' to 'best' if it produces less duplicates,
               or if it produces the same number of duplicates but with
               a more efficient hash function.  */
            if (try_duplicates_count < best_duplicates_count
                || (try_duplicates_count == best_duplicates_count && i == 0))
              {
                best = tryal;
                best_duplicates_count = try_duplicates_count;
              }
          }

      /* Stop removing positions when it gives no improvement.  */
      if (best_duplicates_count > current_duplicates_count)
        break;

      current = best;
      current_duplicates_count = best_duplicates_count;
    }

  /* 4. Replace two positions by one, as long as this doesn't increase the
     duplicates count.  */
  for (;;)
    {
      Positions best;
      unsigned int best_duplicates_count = UINT_MAX;

      for (int i1 = imax; i1 >= 0; i1--)
        if (current.contains (i1) && !mandatory.contains (i1))
          for (int i2 = imax; i2 >= 0; i2--)
            if (current.contains (i2) && !mandatory.contains (i2) && i2 != i1)
              for (int i3 = imax; i3 >= 0; i3--)
                if (!current.contains (i3))
                  {
                    Positions tryal = current;
                    tryal.remove (i1);
                    tryal.remove (i2);
                    tryal.add (i3);
                    unsigned int try_duplicates_count =
                      count_duplicates_tuple (tryal);

                    /* We prefer 'try' to 'best' if it produces less duplicates,
                       or if it produces the same number of duplicates but with
                       a more efficient hash function.  */
                    if (try_duplicates_count < best_duplicates_count
                        || (try_duplicates_count == best_duplicates_count
                            && (i1 == 0 || i2 == 0 || i3 > 0)))
                      {
                        best = tryal;
                        best_duplicates_count = try_duplicates_count;
                      }
                  }

      /* Stop removing positions when it gives no improvement.  */
      if (best_duplicates_count > current_duplicates_count)
        break;

      current = best;
      current_duplicates_count = best_duplicates_count;
    }

  /* That's it.  Hope it's good enough.  */
  _key_positions = current;

  if (option[DEBUG])
    {
      /* Print the result.  */
      fprintf (stderr, "\nComputed positions: ");
      PositionReverseIterator iter (_key_positions);
      bool seen_lastchar = false;
      bool first = true;
      for (int i; (i = iter.next ()) != PositionReverseIterator::EOS; )
        {
          if (!first)
            fprintf (stderr, ", ");
          if (i == Positions::LASTCHAR)
            seen_lastchar = true;
          else
            {
              fprintf (stderr, "%d", i);
              first = false;
            }
        }
      if (seen_lastchar)
        {
          if (!first)
            fprintf (stderr, ", ");
          fprintf (stderr, "$");
        }
      fprintf (stderr, "\n");
    }
}

/* ===================== Finding good alpha increments ===================== */

/* Initializes each keyword's _selchars array.  */
void
Search::init_selchars_multiset (bool use_all_chars, const Positions& positions, const unsigned int *alpha_inc) const
{
  for (KeywordExt_List *temp = _head; temp; temp = temp->rest())
    temp->first()->init_selchars_multiset(use_all_chars, positions, alpha_inc);
}

/* Count the duplicate keywords that occur with the given set of positions
   and a given alpha_inc[] array.
   In other words, it returns the difference
     # K - # proj2 (proj1 (K))
   where K is the multiset of given keywords.  */
unsigned int
Search::count_duplicates_multiset (const unsigned int *alpha_inc) const
{
  /* Run through the keyword list and count the duplicates incrementally.
     The result does not depend on the order of the keyword list, thanks to
     the formula above.  */
  init_selchars_multiset (option[ALLCHARS], _key_positions, alpha_inc);

  unsigned int count = 0;
  {
    Hash_Table representatives (_total_keys, option[NOLENGTH]);
    for (KeywordExt_List *temp = _head; temp; temp = temp->rest())
      {
        KeywordExt *keyword = temp->first();
        if (representatives.insert (keyword))
          count++;
      }
  }

  delete_selchars ();

  return count;
}

/* Find good _alpha_inc[].  */

void
Search::find_alpha_inc ()
{
  /* The goal is to choose _alpha_inc[] such that it doesn't introduce
     artificial duplicates.
     In other words, the goal is  # proj2 (proj1 (K)) = # proj1 (K).  */
  unsigned int duplicates_goal = count_duplicates_tuple (_key_positions);

  /* Start with zero increments.  This is sufficient in most cases.  */
  unsigned int *current = new unsigned int [_max_key_len];
  for (int i = 0; i < _max_key_len; i++)
    current[i] = 0;
  unsigned int current_duplicates_count = count_duplicates_multiset (current);

  if (current_duplicates_count > duplicates_goal)
    {
      /* Look which _alpha_inc[i] we are free to increment.  */
      unsigned int nindices;
      if (option[ALLCHARS])
        nindices = _max_key_len;
      else
        {
          /* Ignore Positions::LASTCHAR.  Remember that since Positions are
             sorted in decreasing order, Positions::LASTCHAR comes last.  */
          nindices = (_key_positions.get_size() == 0
                      || _key_positions[_key_positions.get_size() - 1]
                         != Positions::LASTCHAR
                      ? _key_positions.get_size()
                      : _key_positions.get_size() - 1);
        }

      unsigned int indices[nindices];
      if (option[ALLCHARS])
        for (unsigned int j = 0; j < nindices; j++)
          indices[j] = j;
      else
        {
          PositionIterator iter (_key_positions);
          for (unsigned int j = 0; j < nindices; j++)
            {
              int key_pos = iter.next ();
              if (key_pos == PositionIterator::EOS
                  || key_pos == Positions::LASTCHAR)
                abort ();
              indices[j] = key_pos - 1;
            }
        }

      /* Perform several rounds of searching for a good alpha increment.
         Each round reduces the number of artificial collisions by adding
         an increment in a single key position.  */
      unsigned int best[_max_key_len];
      unsigned int tryal[_max_key_len];
      do
        {
          /* An increment of 1 is not always enough.  Try higher increments
             also.  */
          for (unsigned int inc = 1; ; inc++)
            {
              unsigned int best_duplicates_count = UINT_MAX;

              for (unsigned int j = 0; j < nindices; j++)
                {
                  memcpy (tryal, current, _max_key_len * sizeof (unsigned int));
                  tryal[indices[j]] += inc;
                  unsigned int try_duplicates_count =
                    count_duplicates_multiset (tryal);

                  /* We prefer 'try' to 'best' if it produces less
                     duplicates.  */
                  if (try_duplicates_count < best_duplicates_count)
                    {
                      memcpy (best, tryal, _max_key_len * sizeof (unsigned int));
                      best_duplicates_count = try_duplicates_count;
                    }
                }

              /* Stop this round when we got an improvement.  */
              if (best_duplicates_count < current_duplicates_count)
                {
                  memcpy (current, best, _max_key_len * sizeof (unsigned int));
                  current_duplicates_count = best_duplicates_count;
                  break;
                }
            }
        }
      while (current_duplicates_count > duplicates_goal);

      if (option[DEBUG])
        {
          /* Print the result.  */
          fprintf (stderr, "\nComputed alpha increments: ");
          if (option[ALLCHARS])
            {
              bool first = true;
              for (unsigned int j = 0; j < nindices; j++)
                if (current[indices[j]] != 0)
                  {
                    if (!first)
                      fprintf (stderr, ", ");
                    fprintf (stderr, "%u:+%u",
                             indices[j] + 1, current[indices[j]]);
                    first = false;
                  }
            }
          else
            {
              bool first = true;
              for (unsigned int j = nindices; j-- > 0; )
                if (current[indices[j]] != 0)
                  {
                    if (!first)
                      fprintf (stderr, ", ");
                    fprintf (stderr, "%u:+%u",
                             indices[j] + 1, current[indices[j]]);
                    first = false;
                  }
            }
          fprintf (stderr, "\n");
        }
    }

  _alpha_inc = current;
}

/* ======================= Finding good asso_values ======================== */

void
Search::prepare ()
{
  KeywordExt_List *temp;

  /* Initialize each keyword's _selchars array.  */
  init_selchars_multiset(option[ALLCHARS], _key_positions, _alpha_inc);

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
              {
                fprintf (stderr, "Key link: \"%.*s\" = \"%.*s\", with key set \"",
                         keyword->_allchars_length, keyword->_allchars,
                         other_keyword->_allchars_length, other_keyword->_allchars);
                for (int j = 0; j < keyword->_selchars_length; j++)
                  putc (keyword->_selchars[j], stderr);
                fprintf (stderr, "\".\n");
              }
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
    if (option[DEBUG])
      representatives.dump();
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
          fprintf (stderr, "%d input keys have identical hash values,\n",
                           _total_duplicates);
          if (option[POSITIONS])
            fprintf (stderr, "try different key positions or use option -D.\n");
          else
            fprintf (stderr, "use option -D.\n");
          exit (1);
        }
    }

  /* Compute _alpha_size, the upper bound on the indices passed to
     asso_values[].  */
  unsigned int max_alpha_inc = 0;
  for (int i = 0; i < _max_key_len; i++)
    if (max_alpha_inc < _alpha_inc[i])
      max_alpha_inc = _alpha_inc[i];
  _alpha_size = (option[SEVENBIT] ? 128 : 256) + max_alpha_inc;

  /* Compute the occurrences of each character in the alphabet.  */
  _occurrences = new int[_alpha_size];
  memset (_occurrences, 0, _alpha_size * sizeof (_occurrences[0]));
  for (temp = _head; temp; temp = temp->rest())
    {
      KeywordExt *keyword = temp->first();
      const unsigned int *ptr = keyword->_selchars;
      for (int count = keyword->_selchars_length; count > 0; ptr++, count--)
        _occurrences[*ptr]++;
    }

  /* Memory allocation.  */
  _asso_values = new int[_alpha_size];
  _determined = new bool[_alpha_size];
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

  const unsigned int *p = ptr->_selchars;
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
  const unsigned int *p = keyword->_selchars;
  unsigned int i = keyword->_selchars_length;
  for (; i > 0; p++, i--)
    _determined[*p] = true;
}

/* Auxiliary function for reorder():
   Returns true if the keyword's selected characters are all determined.  */

inline bool
Search::already_determined (KeywordExt *keyword) const
{
  const unsigned int *p = keyword->_selchars;
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
  return option[ALLCHARS] ? _max_key_len : _key_positions.get_size();
}

/* ---------------------- Finding good asso_values[] ----------------------- */

/* Initializes the asso_values[] related parameters.  */

void
Search::prepare_asso_values ()
{
  int non_linked_length = keyword_list_length ();
  int asso_value_max;

  asso_value_max =
    static_cast<int>(non_linked_length * option.get_size_multiple());
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

  if (option[DEBUG])
    fprintf (stderr, "total non-linked keys = %d\nmaximum associated value is %d"
             "\nmaximum size of generated hash table is %d\n",
             non_linked_length, asso_value_max, _max_hash_value);

  if (option[RANDOM] || option.get_jump () == 0)
    /* We will use rand(), so initialize the random number generator.  */
    srand (static_cast<long>(time (0)));

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

  const unsigned int *p = keyword->_selchars;
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
compute_disjoint_union (const unsigned int *set_1, int size_1,
                        const unsigned int *set_2, int size_2,
                        unsigned int *set_3)
{
  unsigned int *base = set_3;

  while (size_1 > 0 && size_2 > 0)
    if (*set_1 == *set_2)
      {
        set_1++, size_1--;
        set_2++, size_2--;
      }
    else
      {
        unsigned int next;
        if (*set_1 < *set_2)
          next = *set_1++, size_1--;
        else
          next = *set_2++, size_2--;
        if (set_3 == base || next != set_3[-1])
          *set_3++ = next;
      }

  while (size_1 > 0)
    {
      unsigned int next;
      next = *set_1++, size_1--;
      if (set_3 == base || next != set_3[-1])
        *set_3++ = next;
    }

  while (size_2 > 0)
    {
      unsigned int next;
      next = *set_2++, size_2--;
      if (set_3 == base || next != set_3[-1])
        *set_3++ = next;
    }
  return set_3 - base;
}

/* Sorts the given set in increasing frequency of _occurrences[].  */

inline void
Search::sort_by_occurrence (unsigned int *set, int len) const
{
  /* Use bubble sort, since the set is typically short.  */
  for (int i = 1; i < len; i++)
    {
      int curr;
      unsigned int tmp;

      for (curr = i, tmp = set[curr];
           curr > 0 && _occurrences[tmp] < _occurrences[set[curr-1]];
           curr--)
        set[curr] = set[curr - 1];

      set[curr] = tmp;
    }
}

/* Returns true if the recomputed hash values for the keywords from
   _head->first() to curr - inclusive - give at least one collision.
   This is called very frequently, and needs to be fast!  */
bool
Search::has_collisions (KeywordExt *curr)
{
  /* Iteration Number array is a win, O(1) initialization time!  */
  _collision_detector->clear ();

  for (KeywordExt_List *ptr = _head; ; ptr = ptr->rest())
    {
      KeywordExt *keyword = ptr->first();

      /* Compute new hash code for the keyword, and see whether it
         collides with another keyword's hash code.  If we have too
         many collisions, we can safely abort the fruitless loop.  */
      if (_collision_detector->set_bit (compute_hash (keyword)))
        return true;

      if (keyword == curr)
        return false;
    }
}

/* Tests whether the given keyword has the same hash value as another one
   earlier in the list.  If yes, this earlier keyword is returned (more
   precisely, the first one of them, but it doesn't really matter which one).
   If no collision is present, NULL is returned.  */
KeywordExt *
Search::collision_prior_to (KeywordExt *curr)
{
  for (KeywordExt_List *prior_ptr = _head;
       prior_ptr->first() != curr;
       prior_ptr = prior_ptr->rest())
    {
      KeywordExt *prior = prior_ptr->first();

      if (prior->_hash_value == curr->_hash_value)
        return prior;
    }
  return NULL;
}

/* Finding good asso_values is normally straightforwards, but needs
   backtracking in some cases.  The recurse/backtrack depth can be at most
   _list_len.  Since we cannot assume that the C stack is large enough,
   we perform the processing without recursion, and simulate the stack.  */
struct StackEntry
{
  /* The current keyword.  */
  KeywordExt *          _curr;

  /* The prior keyword, with which curr collides.  */
  KeywordExt *          _prior;

  /* Scratch set.  */
  unsigned int *        _union_set;
  unsigned int          _union_set_length;

  /* Current index into the scratch set.  */
  unsigned int          _union_index;

  /* Trying a different value for _asso_values[_c].  */
  unsigned int          _c;

  /* The original value of _asso_values[_c].  */
  unsigned int          _original_asso_value;

  /* Remaining number of iterations.  */
  int                   _iter;
};

/* Finds some _asso_values[] that fit.  */

void
Search::find_asso_values ()
{
  /* Add one keyword after the other and see whether its hash value collides
     with one of the previous hash values.  If so, change some asso_values[]
     entry until the number of collisions so far is reduced.  Then continue
     with the next keyword.  */

  init_asso_values ();

  int iterations =
    !option[FAST]
    ? _asso_value_max    /* Try all possible values of _asso_values[c].  */
    : option.get_iterations ()
      ? option.get_iterations ()
      : keyword_list_length ();

  /* Allocate stack.  */
  StackEntry *stack = new StackEntry[_list_len];
  {
    KeywordExt_List *ptr = _head;
    for (int i = 0; i < _list_len; i++, ptr = ptr->rest())
      {
        stack[i]._curr = ptr->first();
        stack[i]._union_set = new unsigned int [2 * get_max_keysig_size ()];
      }
  }

  /* Backtracking according to the standard Prolog call pattern:

                          +------------------+
        -------CALL------>|                  |-------RETURN------>
                          |                  |
        <------FAIL-------|                  |<------REDO---------
                          +------------------+

     A CALL and RETURN increase the stack pointer, FAIL and REDO decrease it.
   */
  {
    /* Current stack pointer.  */
    StackEntry *sp = &stack[0];

    /* Local variables corresponding to *sp.  */
    /* The current keyword.  */
    KeywordExt *curr;
    /* The prior keyword, with which curr collides.  */
    KeywordExt *prior;
    /* Scratch set.  */
    unsigned int *union_set;
    unsigned int union_set_length;
    /* Current index into the scratch set.  */
    unsigned int union_index;
    /* Trying a different value for _asso_values[c].  */
    unsigned int c;
    /* The original value of _asso_values[c].  */
    unsigned int original_asso_value;
    /* Remaining number of iterations.  */
    int iter;

    /* ==== CALL ==== */
   CALL:

    /* Next keyword from the list.  */
    curr = sp->_curr;

    /* Compute this keyword's hash value.  */
    compute_hash (curr);

    /* See if it collides with a prior keyword.  */
    prior = collision_prior_to (curr);

    if (prior != NULL)
      {
        /* Handle collision: Attempt to change an _asso_value[], in order to
           resolve a hash value collision between the two given keywords.  */

        if (option[DEBUG])
          {
            fprintf (stderr, "collision on keyword #%d, prior = \"%.*s\", curr = \"%.*s\" hash = %d\n",
                     sp - stack + 1,
                     prior->_allchars_length, prior->_allchars,
                     curr->_allchars_length, curr->_allchars,
                     curr->_hash_value);
            fflush (stderr);
          }

        /* To achieve that the two hash values become different, we have to
           change an _asso_values[c] for a character c that contributes to the
           hash functions of prior and curr with different multiplicity.
           So we compute the set of such c.  */
        union_set = sp->_union_set;
        union_set_length =
          compute_disjoint_union (prior->_selchars, prior->_selchars_length,
                                  curr->_selchars, curr->_selchars_length,
                                  union_set);

        /* Sort by decreasing occurrence: Try least-used characters c first.
           The idea is that this reduces the number of freshly introduced
           collisions.  */
        sort_by_occurrence (union_set, union_set_length);

        for (union_index = 0; union_index < union_set_length; union_index++)
          {
            c = union_set[union_index];

            /* Try various other values for _asso_values[c].  A value is
               successful if, with it, the recomputed hash values for the
               keywords from _head->first() to curr - inclusive - give no
               collisions.  Up to the given number of iterations are performed.
               If successful, _asso_values[c] is changed, and the recursion
               continues. If all iterations are unsuccessful, _asso_values[c]
               is restored and we backtrack, trying the next union_index.  */

            original_asso_value = _asso_values[c];

            /* Try many valid associated values.  */
            for (iter = iterations; iter > 0; iter--)
              {
                /* Try next value.  Wrap around mod _asso_value_max.  */
                _asso_values[c] =
                  (_asso_values[c] + (_jump != 0 ? _jump : rand ()))
                  & (_asso_value_max - 1);

                if (!has_collisions (curr))
                  {
                    /* Good, this _asso_values[] modification reduces the
                       number of collisions so far.
                       All keyword->_hash_value up to curr - inclusive -
                       have been updated.  */
                    if (option[DEBUG])
                      {
                        fprintf (stderr, "- resolved after %d iterations by "
                                 "changing asso_value['%c'] (char #%d) to %d\n",
                                 iterations - iter + 1, c,
                                 union_index + 1, _asso_values[c]);
                        fflush (stderr);
                      }
                    goto RECURSE_COLLISION;
                   BACKTRACK_COLLISION:
                    if (option[DEBUG])
                      {
                        fprintf (stderr, "back to collision on keyword #%d, prior = \"%.*s\", curr = \"%.*s\" hash = %d\n",
                                 sp - stack + 1,
                                 prior->_allchars_length, prior->_allchars,
                                 curr->_allchars_length, curr->_allchars,
                                 curr->_hash_value);
                        fflush (stderr);
                      }
                  }
              }

            /* Restore original values, no more tries.  */
            _asso_values[c] = original_asso_value;
          }

        /* Failed to resolve a collision.  */

        /* Recompute all keyword->_hash_value up to curr - exclusive -.  */
        for (KeywordExt_List *ptr = _head; ; ptr = ptr->rest())
          {
            KeywordExt* keyword = ptr->first();
            if (keyword == curr)
              break;
            compute_hash (keyword);
          }

        if (option[DEBUG])
          {
            fprintf (stderr, "** collision not resolved after %d iterations, backtracking...\n",
                     iterations);
            fflush (stderr);
          }
      }
    else
      {
        /* Nothing to do, just recurse.  */
        goto RECURSE_NO_COLLISION;
       BACKTRACK_NO_COLLISION: ;
      }

    /* ==== FAIL ==== */
    if (sp != stack)
      {
        sp--;
        /* ==== REDO ==== */
        curr = sp->_curr;
        prior = sp->_prior;
        if (prior == NULL)
          goto BACKTRACK_NO_COLLISION;
        union_set = sp->_union_set;
        union_set_length = sp->_union_set_length;
        union_index = sp->_union_index;
        c = sp->_c;
        original_asso_value = sp->_original_asso_value;
        iter = sp->_iter;
        goto BACKTRACK_COLLISION;
      }

    /* No solution found after an exhaustive search!
       We should ideally turn off option[FAST] and, if that doesn't help,
       multiply _asso_value_max by 2.  */
    fprintf (stderr,
             "\nBig failure, always got duplicate hash code values.\n");
    if (option[POSITIONS])
      fprintf (stderr, "try options -m or -r, or use new key positions.\n\n");
    else
      fprintf (stderr, "try options -m or -r.\n\n");
    exit (1);

   RECURSE_COLLISION:
    /*sp->_union_set = union_set;*/ // redundant
    sp->_union_set_length = union_set_length;
    sp->_union_index = union_index;
    sp->_c = c;
    sp->_original_asso_value = original_asso_value;
    sp->_iter = iter;
   RECURSE_NO_COLLISION:
    /*sp->_curr = curr;*/ // redundant
    sp->_prior = prior;
    /* ==== RETURN ==== */
    sp++;
    if (sp - stack < _list_len)
      goto CALL;
  }

  /* Deallocate stack.  */
  {
    for (int i = 0; i < _list_len; i++)
      delete[] stack[i]._union_set;
  }
  delete[] stack;
}

/* Finds good _asso_values[].  */

void
Search::find_good_asso_values ()
{
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
}

/* ========================================================================= */

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
  preprepare ();

  /* Step 1: Finding good byte positions.  */
  find_positions ();

  /* Step 2: Finding good alpha increments.  */
  find_alpha_inc ();

  /* Step 3: Finding good asso_values.  */
  find_good_asso_values ();

  /* Make one final check, just to make sure nothing weird happened.... */
  _collision_detector->clear ();
  for (KeywordExt_List *curr_ptr = _head; curr_ptr; curr_ptr = curr_ptr->rest())
    {
      KeywordExt *curr = curr_ptr->first();
      unsigned int hashcode = compute_hash (curr);
      if (_collision_detector->set_bit (hashcode))
        {
          /* This shouldn't happen.  proj1, proj2, proj3 must have been
             computed to be injective on the given keyword set.  */
          fprintf (stderr,
                   "\nInternal error, unexpected duplicate hash code\n");
          if (option[POSITIONS])
            fprintf (stderr, "try options -m or -r, or use new key positions.\n\n");
          else
            fprintf (stderr, "try options -m or -r.\n\n");
          exit (1);
        }
    }

  /* Sorts the keyword list by hash value.  */
  sort ();
}

/* Prints out some diagnostics upon completion.  */

Search::~Search ()
{
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
        {
          fprintf (stderr, "%11d,%11d,%6d, ",
                   ptr->first()->_hash_value, ptr->first()->_allchars_length, ptr->first()->_final_index);
          if (field_width > ptr->first()->_selchars_length)
            fprintf (stderr, "%*s", field_width - ptr->first()->_selchars_length, "");
          for (int j = 0; j < ptr->first()->_selchars_length; j++)
            putc (ptr->first()->_selchars[j], stderr);
          fprintf (stderr, ", %.*s\n",
                   ptr->first()->_allchars_length, ptr->first()->_allchars);
        }

      fprintf (stderr, "End dumping list.\n\n");
    }
  delete[] _asso_values;
  delete[] _occurrences;
  delete[] _alpha_inc;
}
