/*  Copyright (C) 1995, 2000-2002 Free Software Foundation, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef GETLINE_H_
# define GETLINE_H_ 1

# include <stddef.h>
# include <stdio.h>

/* Like the glibc functions get_line and get_delim, except that the result
   must be freed using delete[], not free().  */

extern int get_line (char **lineptr, size_t *n, FILE *stream);

extern int get_delim (char **lineptr, size_t *n, int delimiter, FILE *stream);

#endif /* not GETLINE_H_ */
