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

/* An instance of this class is a keyword, as specified in the input file.  */
struct Keyword
{
  /* Constructor.  */
  Keyword (const char *allchars, int allchars_length, const char *rest);

  /* Data members.  */
  /* The keyword as a string, possibly containing NUL bytes.  */
  const char *const allchars;
  const int allchars_length;
  /* Additional stuff seen on the same line of the input file.  */
  const char *const rest;
};

/* A factory for creating Keyword instances.  */
class Keyword_Factory
{
public:
  Keyword_Factory ();
  virtual ~Keyword_Factory ();
  /* Creates a new Keyword.  */
  virtual Keyword create_keyword (const char *allchars, int allchars_length,
                                  const char *rest) = 0;
};

#endif
