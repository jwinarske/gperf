/* This may look like C code, but it is really -*- C++ -*- */

/* Simple lookup table abstraction implemented as an Iteration Number Array.

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
Software Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

#ifndef bool_array_h
#define bool_array_h 1

/* A Bool_Array instance is a bit array of fixed size, optimized for being
   filled sparsely and cleared frequently.  For example, when processing
   tests/chill.gperf, the array will be:
     - of size 15391,
     - clear will be called 3509 times,
     - set_bit will be called 300394 times.
   With a conventional bit array implementation, clear would be too slow.
   With a tree/hash based bit array implementation, set_bit would be slower. */
class Bool_Array
{
public:
  /* Initializes the bit array with room for s bits, numbered from 0 to s-1. */
  Bool_Array (unsigned int s);

  /* Frees this object.  */
  ~Bool_Array ();

  /* Resets all bits to zero.  */
  void clear ();

  /* Sets the specified bit to one.  Returns its previous value (0 or 1).  */
  int set_bit (unsigned int index);

private:
  unsigned int _size;             /* Size of array.  */
  unsigned int _iteration_number; /* Number of times clear() was called + 1. */
  /* For each index, we store in storage_array[index] the iteration_number at
     the time set_bit(index) was last called.  */
  unsigned int *_storage_array;  
};

#ifdef __OPTIMIZE__  /* efficiency hack! */

#include <stdio.h>
#include <string.h>
#include "options.h"
#define INLINE inline
#include "bool-array.icc"
#undef INLINE

#endif

#endif
