/* This may look like C code, but it is really -*- C++ -*- */

/* Provides high-level routines to manipulate the keyword list
   structures the code generation output.

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
Software Foundation, 59 Temple Place - Suite 330, Boston, MA  02111, USA. */

#ifndef gen_perf_h
#define gen_perf_h 1

#include "key-list.h"
#include "bool-array.h"

class Gen_Perf : private Key_List
{
private:
  int                   _max_hash_value;    /* Maximum possible hash value. */
  int                   _fewest_collisions; /* Records fewest # of collisions for asso value. */
  int                   _num_done;          /* Number of keywords processed without a collision. */
  Bool_Array *          _collision_detector;

  void                  change (KeywordExt *prior, KeywordExt *curr);
  bool                  affects_prev (char c, KeywordExt *curr);
  static int            hash (KeywordExt *key_node);
  static int            compute_disjoint_union (const char *set_1, int size_1, const char *set_2, int size_2, char *set_3);
  static void           sort_set (char *union_set, int len);

public:
                        Gen_Perf ();
                        ~Gen_Perf ();
  int                   doit_all ();
};

#endif
