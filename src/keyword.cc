/* Keyword data.
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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "keyword.h"
#include "options.h"


/* Keyword class.  */

/* Constructor.  */
Keyword::Keyword (const char *s, int s_len, const char *r)
  : allchars (s), allchars_length (s_len), rest (r)
{
}


/* KeywordExt class.  */

/* Constructor.  */
KeywordExt::KeywordExt (const char *s, int s_len, const char *r)
  : Keyword (s, s_len, r), duplicate_link (NULL), final_index (0)
{
}

/* Sort a small set of 'char', base[0..len-1], in place.  */
static inline void sort_char_set (char *base, int len)
{
  /* Bubble sort is sufficient here.  */
  for (int i = 1; i < len; i++)
    {
      int j;
      char tmp;

      for (j = i, tmp = base[j]; j > 0 && tmp < base[j - 1]; j--)
        base[j] = base[j - 1];

      base[j] = tmp;
    }
}

/* Initialize selchars and selchars_length, and update v->occurrences.  */
void KeywordExt::init_selchars (Vectors *v)
{
  const char *k = allchars;
  char *key_set =
    new char[(option[ALLCHARS] ? allchars_length : option.get_max_keysig_size ())];
  char *ptr = key_set;
  int i;

  if (option[ALLCHARS])
    /* Use all the character positions in the KEY. */
    for (i = allchars_length; i > 0; k++, ptr++, i--)
      v->occurrences[(unsigned char)(*ptr = *k)]++;
  else
    /* Only use those character positions specified by the user. */
    {
      /* Iterate through the list of key_positions, initializing occurrences
         table and selchars (via char * pointer ptr). */

      for (option.reset (); (i = option.get ()) != EOS; )
        {
          if (i == WORD_END)
            /* Special notation for last KEY position, i.e. '$'. */
            *ptr = allchars[allchars_length - 1];
          else if (i <= allchars_length)
            /* Within range of KEY length, so we'll keep it. */
            *ptr = allchars[i - 1];
          else
            /* Out of range of KEY length, so we'll just skip it. */
            continue;
          v->occurrences[(unsigned char)*ptr]++;
          ptr++;
        }

      /* Didn't get any hits and user doesn't want to consider the
        keylength, so there are essentially no usable hash positions! */
      if (ptr == selchars && option[NOLENGTH])
        {
          fprintf (stderr, "Can't hash keyword %.*s with chosen key positions.\n",
                   allchars_length, allchars);
          exit (1);
        }
    }

  /* Sort the KEY_SET items alphabetically. */
  sort_char_set (key_set, ptr - key_set);

  selchars = key_set;
  selchars_length = ptr - key_set;
}


/* Keyword_Factory class.  */

Keyword_Factory::Keyword_Factory () {}

Keyword_Factory::~Keyword_Factory () {}
