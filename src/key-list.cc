/* Routines for building, ordering, and printing the keyword list.
   Copyright (C) 1989-1998, 2000, 2002 Free Software Foundation, Inc.
   written by Douglas C. Schmidt (schmidt@ics.uci.edu)

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

#include <stdio.h>
#include <stdlib.h> /* declares exit() */
#include <limits.h> /* defines INT_MIN, INT_MAX */
#include "options.h"
#include "key-list.h"
#include "input.h"
#include "hash-table.h"

/* Make the hash table 8 times larger than the number of keyword entries. */
static const int TABLE_MULTIPLE     = 10;

/* Efficiently returns the least power of two greater than or equal to X! */
#define POW(X) ((!X)?1:(X-=1,X|=X>>1,X|=X>>2,X|=X>>4,X|=X>>8,X|=X>>16,(++X)))

bool Key_List::_determined[MAX_ALPHA_SIZE];

/* Destructor dumps diagnostics during debugging. */

Key_List::~Key_List ()
{
  if (option[DEBUG])
    {
      fprintf (stderr, "\nDumping key list information:\ntotal non-static linked keywords = %d"
               "\ntotal keywords = %d\ntotal duplicates = %d\nmaximum key length = %d\n",
               _list_len, _total_keys, _total_duplicates, _max_key_len);
      dump ();
      fprintf (stderr, "End dumping list.\n\n");
    }
}

/* Reads in all keys from standard input and creates a linked list pointed
   to by _head.  This list is then quickly checked for "links", i.e.,
   unhashable elements possessing identical key sets and lengths. */

void
Key_List::read_keys ()
{
  Input inputter;
  inputter.read_keys ();
  _array_type      = inputter._array_type;
  _return_type     = inputter._return_type;
  _struct_tag      = inputter._struct_tag;
  _include_src     = inputter._include_src;
  _additional_code = inputter._additional_code;
  _head            = inputter._head;

  KeywordExt_List *temp;
  KeywordExt_List *trail = NULL;

  for (temp = _head; temp; temp = temp->rest())
    {
      temp->first()->init_selchars(this);
      _total_keys++;
    }
       
  /* Hash table this number of times larger than keyword number. */
  int table_size = (_list_len = _total_keys) * TABLE_MULTIPLE;
  /* Table must be a power of 2 for the hash function scheme to work. */
  KeywordExt **table = new KeywordExt*[POW (table_size)];

  /* Make large hash table for efficiency. */
  Hash_Table found_link (table, table_size, option[NOLENGTH]);

  /* Test whether there are any links and also set the maximum length of
     an identifier in the keyword list. */

  for (temp = _head; temp; temp = temp->rest())
    {
      KeywordExt *keyword = temp->first();
      KeywordExt *other_keyword = found_link.insert (keyword);

      /* Check for links.  We deal with these by building an equivalence class
         of all duplicate values (i.e., links) so that only 1 keyword is
         representative of the entire collection.  This *greatly* simplifies
         processing during later stages of the program. */

      if (other_keyword)
        {
          _total_duplicates++;
          _list_len--;
          trail->rest() = temp->rest();
          temp->first()->_duplicate_link = other_keyword->_duplicate_link;
          other_keyword->_duplicate_link = temp->first();

          /* Complain if user hasn't enabled the duplicate option. */
          if (!option[DUP] || option[DEBUG])
            fprintf (stderr, "Key link: \"%.*s\" = \"%.*s\", with key set \"%.*s\".\n",
                             keyword->_allchars_length, keyword->_allchars,
                             other_keyword->_allchars_length, other_keyword->_allchars,
                             keyword->_selchars_length, keyword->_selchars);
        }
      else
        trail = temp;

      /* Update minimum and maximum keyword length, if needed. */
      if (_max_key_len < keyword->_allchars_length)
        _max_key_len = keyword->_allchars_length;
      if (_min_key_len > keyword->_allchars_length)
        _min_key_len = keyword->_allchars_length;
    }

  delete[] table;

  /* Exit program if links exists and option[DUP] not set, since we can't continue */
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
  /* Exit program if an empty string is used as key, since the comparison
     expressions don't work correctly for looking up an empty string. */
  if (_min_key_len == 0)
    {
      fprintf (stderr, "Empty input key is not allowed.\nTo recognize an empty input key, your code should check for\nlen == 0 before calling the gperf generated lookup function.\n");
      exit (1);
    }
}

/* Recursively merges two sorted lists together to form one sorted list. The
   ordering criteria is by frequency of occurrence of elements in the key set
   or by the hash value.  This is a kludge, but permits nice sharing of
   almost identical code without incurring the overhead of a function
   call comparison. */

KeywordExt_List *
Key_List::merge (KeywordExt_List *list1, KeywordExt_List *list2)
{
  KeywordExt_List *result;
  KeywordExt_List **resultp = &result;
  for (;;)
    {
      if (!list1)
        {
          *resultp = list2;
          break;
        }
      if (!list2)
        {
          *resultp = list1;
          break;
        }
      if (_occurrence_sort && list1->first()->_occurrence < list2->first()->_occurrence
          || _hash_sort && list1->first()->_hash_value > list2->first()->_hash_value)
        {
          *resultp = list2;
          resultp = &list2->rest(); list2 = list1; list1 = *resultp;
        }
      else
        {
          *resultp = list1;
          resultp = &list1->rest(); list1 = *resultp;
        }
    }
  return result;
}

/* Applies the merge sort algorithm to recursively sort the key list by
   frequency of occurrence of elements in the key set. */

KeywordExt_List *
Key_List::merge_sort (KeywordExt_List *head)
{
  if (!head || !head->rest())
    return head;
  else
    {
      KeywordExt_List *middle = head;
      KeywordExt_List *temp   = head->rest()->rest();

      while (temp)
        {
          temp   = temp->rest();
          middle = middle->rest();
          if (temp)
            temp = temp->rest();
        }

      temp         = middle->rest();
      middle->rest() = 0;
      return merge (merge_sort (head), merge_sort (temp));
    }
}

/* Returns the frequency of occurrence of elements in the key set. */

inline int
Key_List::get_occurrence (KeywordExt *ptr)
{
  int value = 0;

  const char *p = ptr->_selchars;
  unsigned int i = ptr->_selchars_length;
  for (; i > 0; p++, i--)
    value += _occurrences[static_cast<unsigned char>(*p)];

  return value;
}

/* Enables the index location of all key set elements that are now
   determined. */

inline void
Key_List::set_determined (KeywordExt *ptr)
{
  const char *p = ptr->_selchars;
  unsigned int i = ptr->_selchars_length;
  for (; i > 0; p++, i--)
    _determined[static_cast<unsigned char>(*p)] = true;
}

/* Returns TRUE if PTR's key set is already completely determined. */

inline bool
Key_List::already_determined (KeywordExt *ptr)
{
  bool is_determined = true;

  const char *p = ptr->_selchars;
  unsigned int i = ptr->_selchars_length;
  for (; is_determined && i > 0; p++, i--)
    is_determined = _determined[static_cast<unsigned char>(*p)];

  return is_determined;
}

/* Reorders the table by first sorting the list so that frequently occuring
   keys appear first, and then the list is reordered so that keys whose values
   are already determined will be placed towards the front of the list.  This
   helps prune the search time by handling inevitable collisions early in the
   search process.  See Cichelli's paper from Jan 1980 JACM for details.... */

void
Key_List::reorder ()
{
  KeywordExt_List *ptr;
  for (ptr = _head; ptr; ptr = ptr->rest())
    {
      KeywordExt *keyword = ptr->first();

      keyword->_occurrence = get_occurrence (keyword);
    }

  _hash_sort = false;
  _occurrence_sort = true;

  _head = merge_sort (_head);

  for (ptr = _head; ptr->rest(); ptr = ptr->rest())
    {
      set_determined (ptr->first());

      if (!already_determined (ptr->rest()->first()))
        {
          KeywordExt_List *trail_ptr = ptr->rest();
          KeywordExt_List *run_ptr   = trail_ptr->rest();

          for (; run_ptr; run_ptr = trail_ptr->rest())
            {

              if (already_determined (run_ptr->first()))
                {
                  trail_ptr->rest() = run_ptr->rest();
                  run_ptr->rest()   = ptr->rest();
                  ptr = ptr->rest() = run_ptr;
                }
              else
                trail_ptr = run_ptr;
            }
        }
    }
}

/* Sorts the keys by hash value. */

void
Key_List::sort ()
{
  _hash_sort       = true;
  _occurrence_sort = false;

  _head = merge_sort (_head);
}

/* Dumps the key list to stderr stream. */

void
Key_List::dump ()
{
  int field_width = get_max_keysig_size ();

  fprintf (stderr, "\nList contents are:\n(hash value, key length, index, %*s, keyword):\n",
           field_width, "selchars");

  for (KeywordExt_List *ptr = _head; ptr; ptr = ptr->rest())
    fprintf (stderr, "%11d,%11d,%6d, %*.*s, %.*s\n",
             ptr->first()->_hash_value, ptr->first()->_allchars_length, ptr->first()->_final_index,
             field_width, ptr->first()->_selchars_length, ptr->first()->_selchars,
             ptr->first()->_allchars_length, ptr->first()->_allchars);
}

/* Simple-minded constructor action here... */

Key_List::Key_List ()
{
  _total_keys       = 0;
  _max_key_len      = INT_MIN;
  _min_key_len      = INT_MAX;
  _array_type       = 0;
  _return_type      = 0;
  _struct_tag       = 0;
  _head             = 0;
  _total_duplicates = 0;
  _additional_code  = false;
}

/* Returns the length of entire key list. */

int
Key_List::keyword_list_length ()
{
  return _list_len;
}

/* Returns length of longest key read. */

int
Key_List::max_key_length ()
{
  return _max_key_len;
}

/* Returns number of key positions.  */

int
Key_List::get_max_keysig_size ()
{
  return option[ALLCHARS] ? _max_key_len : option.get_max_keysig_size ();
}
