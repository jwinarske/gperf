/* Driver program for the hash function generator
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

#include <stdio.h>
#include "options.h"
#include "input.h"
#include "search.h"
#include "output.h"


/* This Keyword factory produces KeywordExt instances.  */

class KeywordExt_Factory : public Keyword_Factory
{
virtual Keyword *       create_keyword (const char *allchars, int allchars_length,
                                        const char *rest);
};

Keyword *
KeywordExt_Factory::create_keyword (const char *allchars, int allchars_length, const char *rest)
{
  return new KeywordExt (allchars, allchars_length, rest);
}


int
main (int argc, char *argv[])
{
  /* Set the Options. */
  option.parse_options (argc, argv);

  /* Initialize the key word list. */
  KeywordExt_Factory factory;
  Input inputter (&factory);
  Vectors::ALPHA_SIZE = (option[SEVENBIT] ? 128 : 256);
  inputter.read_keys ();
  /* We can cast the keyword list to KeywordExt_List* because its list
     elements were created by KeywordExt_Factory. */
  KeywordExt_List* list = static_cast<KeywordExt_List*>(inputter._head);

  /* Search for a good hash function.  */
  Search searcher (list);
  searcher.optimize ();

  /* Output the hash function code.  */
  Output outputter (searcher._head,
                    inputter._array_type,
                    inputter._return_type,
                    inputter._struct_tag,
                    inputter._additional_code,
                    inputter._include_src,
                    searcher._total_keys,
                    searcher._total_duplicates,
                    searcher._max_key_len,
                    searcher._min_key_len,
                    &searcher);
  outputter.output ();

  /* Check for write error on stdout. */
  int status = 0;
  if (fflush (stdout) || ferror (stdout))
    status = 1;

  /* Don't use exit() here, it skips the destructors. */
  return status;
}
