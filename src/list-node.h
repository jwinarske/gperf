/* This may look like C code, but it is really -*- C++ -*- */

/* Data and function members for defining values and operations of a list node.

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

#ifndef list_node_h
#define list_node_h 1

#include "vectors.h"
#include "keyword.h"

struct List_Node : public KeywordExt, private Vectors
{
  List_Node  *next;              /* Points to next element on the list. */

              List_Node (const char *key, int len, const char *rest);
  static void set_sort (char *base, int len);
};

#endif
