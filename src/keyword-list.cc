/* Keyword list.

   Copyright (C) 2002 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>.

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

/* Specification. */
#include "keyword-list.h"

#include <stddef.h>

/* -------------------------- Keyword_List class --------------------------- */

/* Constructor.  */
Keyword_List::Keyword_List (Keyword *car)
  : _cdr (NULL), _car (car)
{
}

/* ------------------------- KeywordExt_List class ------------------------- */

/* Unused constructor.  */
KeywordExt_List::KeywordExt_List (KeywordExt *car)
  : Keyword_List (car)
{
}

/* ------------------------ Keyword_List functions ------------------------- */

/* Copies a linear list, sharing the list elements.  */
Keyword_List *
copy_list (Keyword_List *list)
{
  Keyword_List *result;
  Keyword_List **lastp = &result;
  while (list != NULL)
    {
      Keyword_List *new_cons = new Keyword_List (list->first());
      *lastp = new_cons;
      lastp = &new_cons->rest();
      list = list->rest();
    }
  *lastp = NULL;
  return result;
}

/* Copies a linear list, sharing the list elements.  */
KeywordExt_List *
copy_list (KeywordExt_List *list)
{
  return static_cast<KeywordExt_List *> (copy_list (static_cast<Keyword_List *> (list)));
}

/* Deletes a linear list, keeping the list elements in memory.  */
void
delete_list (Keyword_List *list)
{
  while (list != NULL)
    {
      Keyword_List *rest = list->rest();
      delete list;
      list = rest;
    }
}


#ifndef __OPTIMIZE__

#define INLINE /* not inline */
#include "keyword-list.icc"
#undef INLINE

#endif /* not defined __OPTIMIZE__ */
