/* This may look like C code, but it is really -*- C++ -*- */

/* A set of byte positions.

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

#ifndef positions_h
#define positions_h 1

/* This class denotes a set of byte positions, used to access a keyword.  */

class Positions
{
  friend class PositionIterator;
public:
  /* Denotes the last char of a keyword, depending on the keyword's length.  */
  static const int      LASTCHAR = 0;

  /* Maximum key position specifiable by the user.
     Note that this must fit into the element type of _positions[], below.  */
  static const int      MAX_KEY_POS = 255;

  /* Constructors.  */
                        Positions ();
                        Positions (int pos1);
                        Positions (int pos1, int pos2);

  /* Copy constructor.  */
                        Positions (const Positions& src);

  /* Assignment operator.  */
  Positions&            operator= (const Positions& src);

  /* Accessors.  */
  int                   operator[] (unsigned int index) const;
  unsigned int          get_size () const;

  /* Write access.  */
  unsigned char *       pointer ();
  void                  set_size (unsigned int size);

  /* Sorts the array in reverse order.
     Returns true if there are no duplicates, false otherwise.  */
  bool                  sort ();

  /* Set operations.  Assumes the array is in reverse order.  */
  bool                  contains (int pos) const;
  void                  add (int pos);
  void                  remove (int pos);

  /* Output in external syntax.  */
  void                  print () const;

private:
  /* Number of positions.  */
  unsigned int          _size;
  /* Array of positions.  1 for the first char, 2 for the second char etc.,
     LASTCHAR for the last char.
     Note that since duplicates are eliminated, the maximum possible size
     is MAX_KEY_POS + 1.  */
  unsigned char         _positions[MAX_KEY_POS + 1];
};

/* This class denotes an iterator through a set of byte positions.  */

class PositionIterator
{
public:
  /* Initializes an iterator through POSITIONS.  */
                        PositionIterator (Positions const& positions);

  /* End of iteration marker.  */
  static const int      EOS = -1;

  /* Retrieves the next position, or EOS past the end.  */
  int                   next ();

private:
  const Positions&      _set;
  unsigned int          _index;
};

#ifdef __OPTIMIZE__

#include <string.h>
#define INLINE inline
#include "positions.icc"
#undef INLINE

#endif

#endif
