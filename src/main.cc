/* Driver program for the Gen_Perf hash function generator
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

/* Simple driver program for the Gen_Perf.hash function generator.
   Most of the hard work is done in class Gen_Perf and its class methods. */

#include <stdio.h>
#include "options.h"
#include "gen-perf.h"

int
main (int argc, char *argv[])
{
  /* Sets the Options. */
  option.parse_options (argc, argv);

  /* Initializes the key word list. */
  Gen_Perf generate_table;

  /* Generates and prints the Gen_Perf hash table. */
  int status = generate_table.doit_all ();

  /* Check for write error on stdout. */
  if (fflush (stdout) || ferror (stdout))
    status = 1;

  /* Don't use exit() here, it skips the destructors. */
  return status;
}
