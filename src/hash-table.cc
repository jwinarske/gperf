/* Hash table for checking keyword links.  Implemented using double hashing.
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

#include "hash-table.h"

#include <stdio.h>
#include <string.h> /* declares memset(), strcmp() */
#include <hash.h>
#include "options.h"

/* The size of the hash table is always the smallest power of 2 >= the size
   indicated by the user.  This allows several optimizations, including
   the use of double hashing and elimination of the mod instruction.
   Note that the size had better be larger than the number of items
   in the hash table, else there's trouble!!!  Note that the memory
   for the hash table is allocated *outside* the intialization routine.
   This compromises information hiding somewhat, but greatly reduces
   memory fragmentation, since we can now use alloca! */

Hash_Table::Hash_Table (KeywordExt **table_ptr, int s, int ignore_len):
     _table (table_ptr), _size (s), _collisions (0), _ignore_length (ignore_len)
{
  memset ((char *) _table, 0, _size * sizeof (*_table));
}

Hash_Table::~Hash_Table ()
{
  if (option[DEBUG])
    {
      int field_width;

      if (option[ALLCHARS])
        {
          field_width = 0;
          for (int i = _size - 1; i >= 0; i--)
            if (_table[i])
              if (field_width < _table[i]->_selchars_length)
                field_width = _table[i]->_selchars_length;
        }
      else
        field_width = option.get_max_keysig_size ();

      fprintf (stderr,
               "\ndumping the hash table\n"
               "total available table slots = %d, total bytes = %d, total collisions = %d\n"
               "location, %*s, keyword\n",
               _size, _size * (int) sizeof (*_table), _collisions,
               field_width, "keysig");

      for (int i = _size - 1; i >= 0; i--)
        if (_table[i])
          fprintf (stderr, "%8d, %*.*s, %.*s\n",
                   i,
                   field_width, _table[i]->_selchars_length, _table[i]->_selchars,
                   _table[i]->_allchars_length, _table[i]->_allchars);

      fprintf (stderr, "\nend dumping hash table\n\n");
    }
}

/* If the ITEM is already in the hash table return the item found
   in the table.  Otherwise inserts the ITEM, and returns FALSE.
   Uses double hashing. */

KeywordExt *
Hash_Table::insert (KeywordExt *item)
{
  unsigned hash_val  = hashpjw (item->_selchars, item->_selchars_length);
  int      probe     = hash_val & (_size - 1);
  int      increment = ((hash_val ^ item->_allchars_length) | 1) & (_size - 1);

  while (_table[probe])
    {
      if (_table[probe]->_selchars_length == item->_selchars_length
          && memcmp (_table[probe]->_selchars, item->_selchars, item->_selchars_length) == 0
          && (_ignore_length || _table[probe]->_allchars_length == item->_allchars_length))
        return _table[probe];

      _collisions++;
      probe = (probe + increment) & (_size - 1);
    }

  _table[probe] = item;
  return NULL;
}
