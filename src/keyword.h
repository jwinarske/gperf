/* This may look like C code, but it is really -*- C++ -*- */

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

#ifndef keyword_h
#define keyword_h 1

#include "vectors.h"

/* An instance of this class is a keyword, as specified in the input file.  */
struct Keyword
{
  /* Constructor.  */
  Keyword (const char *allchars, int allchars_length, const char *rest);

  /* Data members defined immediately by the input file.  */
  /* The keyword as a string, possibly containing NUL bytes.  */
  const char *const allchars;
  const int allchars_length;
  /* Additional stuff seen on the same line of the input file.  */
  const char *const rest;
};

/* A keyword, in the context of a given keyposition list.  */
struct KeywordExt : public Keyword
{
  /* Constructor.  */
  KeywordExt (const char *allchars, int allchars_length, const char *rest);

  /* Data members depending on the keyposition list.  */
  /* The selected characters that participate for the hash function,
     reordered according to the keyposition list.  */
  const char * selchars;
  int selchars_length;
  /* Chained list of keywords having the same selchars.  */
  KeywordExt * duplicate_link;

  /* Methods depending on the keyposition list.  */
  /* Initialize selchars and selchars_length, and update v->occurrences.  */
  void init_selchars (Vectors *v);

  /* Data members used by the algorithm.  */
  int occurrence; /* A metric for frequency of key set occurrences. */
  int hash_value; /* Hash value for the key. */

  /* Data members used by the output routines.  */
  int final_index;
};

/* A factory for creating Keyword instances.  */
class Keyword_Factory
{
public:
  Keyword_Factory ();
  virtual ~Keyword_Factory ();
  /* Creates a new Keyword.  */
  virtual Keyword * create_keyword (const char *allchars, int allchars_length,
                                    const char *rest) = 0;
};

#endif
