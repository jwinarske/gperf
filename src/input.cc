/* Input routines.
   Copyright (C) 1989-1998, 2002 Free Software Foundation, Inc.
   Written by Douglas C. Schmidt <schmidt@ics.uci.edu>
   and Bruno Haible <bruno@clisp.org>.

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

#include <stdio.h>
#include <stdlib.h> /* declares exit() */
#include <string.h> /* declares strncpy(), strchr() */
#include <limits.h> /* defines UCHAR_MAX etc. */
#include "options.h"
#include "input.h"

/* Gathers the input stream into a buffer until one of two things occur:

   1. We read a '%' followed by a '%'
   2. We read a '%' followed by a '}'

   The first symbolizes the beginning of the keyword list proper,
   The second symbolizes the end of the C source code to be generated
   verbatim in the output file.

   I assume that the keys are separated from the optional preceding struct
   declaration by a consecutive % followed by either % or } starting in
   the first column. The code below uses an expandible buffer to scan off
   and return a pointer to all the code (if any) appearing before the delimiter. */

const char *
Input::get_special_input (char delimiter)
{
  int size  = 80;
  char *buf = new char[size];
  int c, i;

  for (i = 0; (c = getchar ()) != EOF; i++)
    {
      if (c == '%')
        {
          if ((c = getchar ()) == delimiter)
            {

              while ((c = getchar ()) != '\n')
                ; /* discard newline */

              if (i == 0)
                return "";
              else
                {
                  buf[delimiter == '%' && buf[i - 2] == ';' ? i - 2 : i - 1] = '\0';
                  return buf;
                }
            }
          else
            buf[i++] = '%';
        }
      else if (i >= size) /* Yikes, time to grow the buffer! */
        {
          char *temp = new char[size *= 2];
          int j;

          for (j = 0; j < i; j++)
            temp[j] = buf[j];

          buf = temp;
        }
      buf[i] = c;
    }

  return 0;        /* Problem here. */
}

/* Stores any C text that must be included verbatim into the
   generated code output. */

const char *
Input::save_include_src ()
{
  int c;

  if ((c = getchar ()) != '%')
    ungetc (c, stdin);
  else if ((c = getchar ()) != '{')
    {
      fprintf (stderr, "internal error, %c != '{' on line %d in file %s", c, __LINE__, __FILE__);
      exit (1);
    }
  else
    return get_special_input ('}');
  return "";
}

/* Determines from the input file whether the user wants to build a table
   from a user-defined struct, or whether the user is content to simply
   use the default array of keys. */

const char *
Input::get_array_type ()
{
  return get_special_input ('%');
}

/* strcspn - find length of initial segment of S consisting entirely
   of characters not from REJECT (borrowed from Henry Spencer's
   ANSI string package, when GNU libc comes out I'll replace this...). */

#ifndef strcspn
inline int
Input::strcspn (const char *s, const char *reject)
{
  const char *scan;
  const char *rej_scan;
  int   count = 0;

  for (scan = s; *scan; scan++)
    {

      for (rej_scan = reject; *rej_scan; rej_scan++)
        if (*scan == *rej_scan)
          return count;

      count++;
    }

  return count;
}
#endif

/* Sets up the Return_Type, the Struct_Tag type and the Array_Type
   based upon various user Options. */

void
Input::set_output_types ()
{
  if (option[TYPE])
    {
      _array_type = get_array_type ();
      if (!_array_type)
        /* Something's wrong, but we'll catch it later on, in read_keys()... */
        return;
      /* Yow, we've got a user-defined type... */
      int i = strcspn (_array_type, "{\n\0");
      /* Remove trailing whitespace. */
      while (i > 0 && strchr (" \t", _array_type[i-1]))
        i--;
      int struct_tag_length = i;

      /* Set `struct_tag' to a naked "struct something". */
      char *structtag = new char[struct_tag_length + 1];
      strncpy (structtag, _array_type, struct_tag_length);
      structtag[struct_tag_length] = '\0';
      _struct_tag = structtag;

      /* The return type of the lookup function is "struct something *".
         No "const" here, because if !option[CONST], some user code might want
         to modify the structure. */
      char *rettype = new char[struct_tag_length + 3];
      strncpy (rettype, _array_type, struct_tag_length);
      rettype[struct_tag_length] = ' ';
      rettype[struct_tag_length + 1] = '*';
      rettype[struct_tag_length + 2] = '\0';
      _return_type = rettype;
    }
}

/* Extracts a key from an input line and creates a new KeywordExt_List for
   it. */

static KeywordExt_List *
parse_line (const char *line, const char *delimiters)
{
  if (*line == '"')
    {
      /* Parse a string in ANSI C syntax. */
      char *key = new char[strlen(line)];
      char *kp = key;
      const char *lp = line + 1;

      for (; *lp;)
        {
          char c = *lp;

          if (c == '\0')
            {
              fprintf (stderr, "unterminated string: %s\n", line);
              exit (1);
            }
          else if (c == '\\')
            {
              c = *++lp;
              switch (c)
                {
                case '0': case '1': case '2': case '3':
                case '4': case '5': case '6': case '7':
                  {
                    int code = 0;
                    int count = 0;
                    while (count < 3 && *lp >= '0' && *lp <= '7')
                      {
                        code = (code << 3) + (*lp - '0');
                        lp++;
                        count++;
                      }
                    if (code > UCHAR_MAX)
                      fprintf (stderr, "octal escape out of range: %s\n", line);
                    *kp = (char) code;
                    break;
                  }
                case 'x':
                  {
                    int code = 0;
                    int count = 0;
                    lp++;
                    while ((*lp >= '0' && *lp <= '9')
                           || (*lp >= 'A' && *lp <= 'F')
                           || (*lp >= 'a' && *lp <= 'f'))
                      {
                        code = (code << 4)
                               + (*lp >= 'A' && *lp <= 'F' ? *lp - 'A' + 10 :
                                  *lp >= 'a' && *lp <= 'f' ? *lp - 'a' + 10 :
                                  *lp - '0');
                        lp++;
                        count++;
                      }
                    if (count == 0)
                      fprintf (stderr, "hexadecimal escape without any hex digits: %s\n", line);
                    if (code > UCHAR_MAX)
                      fprintf (stderr, "hexadecimal escape out of range: %s\n", line);
                    *kp = (char) code;
                    break;
                  }
                case '\\': case '\'': case '"':
                  *kp = c;
                  lp++;
                  break;
                case 'n':
                  *kp = '\n';
                  lp++;
                  break;
                case 't':
                  *kp = '\t';
                  lp++;
                  break;
                case 'r':
                  *kp = '\r';
                  lp++;
                  break;
                case 'f':
                  *kp = '\f';
                  lp++;
                  break;
                case 'b':
                  *kp = '\b';
                  lp++;
                  break;
                case 'a':
                  *kp = '\a';
                  lp++;
                  break;
                case 'v':
                  *kp = '\v';
                  lp++;
                  break;
                default:
                  fprintf (stderr, "invalid escape sequence in string: %s\n", line);
                  exit (1);
                }
            }
          else if (c == '"')
            break;
          else
            {
              *kp = c;
              lp++;
            }
          kp++;
        }
      lp++;
      if (*lp != '\0')
        {
          if (strchr (delimiters, *lp) == NULL)
            {
              fprintf (stderr, "string not followed by delimiter: %s\n", line);
              exit (1);
            }
          lp++;
        }
      return new KeywordExt_List (key, kp - key, option[TYPE] ? lp : "");
    }
  else
    {
      /* Not a string. Look for the delimiter. */
      int len = strcspn (line, delimiters);
      const char *rest;

      if (line[len] == '\0')
        rest = "";
      else
        /* Skip the first delimiter. */
        rest = &line[len + 1];
      return new KeywordExt_List (line, len, option[TYPE] ? rest : "");
    }
}

void
Input::read_keys ()
{
  char *ptr;

  _include_src = save_include_src ();
  set_output_types ();

  /* Oops, problem with the input file. */
  if (! (ptr = Read_Line::read_next_line ()))
    {
      fprintf (stderr, "No words in input file, did you forget to prepend %s or use -t accidentally?\n", "%%");
      exit (1);
    }

  /* Read in all the keywords from the input file. */
  const char *delimiter = option.get_delimiter ();

  _head = parse_line (ptr, delimiter);

  for (KeywordExt_List *temp = _head;
       (ptr = Read_Line::read_next_line ()) && strcmp (ptr, "%%");
       temp = temp->rest())
    temp->rest() = parse_line (ptr, delimiter);

  /* See if any additional C code is included at end of this file. */
  if (ptr)
    _additional_code = true;
}
