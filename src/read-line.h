/* This may look like C code, but it is really -*- C++ -*- */

/* Reads arbitrarily long string from input file, returning it as a
   dynamically allocated buffer.

   Copyright (C) 1989-1998, 2002 Free Software Foundation, Inc.
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

#ifndef read_line_h
#define read_line_h 1

#include <stdio.h>
#include "getline.h"

/* An instance of this class is used for repeatedly reading lines of text
   from an input stream.  */
class Read_Line
{
public:

  /* Initializes the instance with a given input stream.  */
  Read_Line (FILE *stream = stdin) : fp (stream) {}

  /* Reads the next line and returns it, excluding the terminating newline,
     and ignoring lines starting with '#'.  Returns NULL on error or EOF.
     The storage for the string is dynamically allocated and must be freed
     through delete[].  */
  char *read_next_line (void);

private:
  FILE *fp;                       /* FILE pointer to the input stream. */
};

#ifdef __OPTIMIZE__

#define INLINE inline
#include "read-line.icc"
#undef INLINE

#endif

#endif
