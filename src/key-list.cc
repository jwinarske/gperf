/* Routines for building, ordering, and printing the keyword list.
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

#include <stdio.h>
#include <string.h> /* declares strncpy(), strchr() */
#include <stdlib.h> /* declares malloc(), free(), abs(), exit(), abort() */
#include <limits.h> /* defines UCHAR_MAX etc. */
#include "options.h"
#include "read-line.h"
#include "hash-table.h"
#include "key-list.h"

/* Make the hash table 8 times larger than the number of keyword entries. */
static const int TABLE_MULTIPLE     = 10;

/* Efficiently returns the least power of two greater than or equal to X! */
#define POW(X) ((!X)?1:(X-=1,X|=X>>1,X|=X>>2,X|=X>>4,X|=X>>8,X|=X>>16,(++X)))

int Key_List::_determined[MAX_ALPHA_SIZE];

/* Destructor dumps diagnostics during debugging. */

Key_List::~Key_List ()
{
  if (option[DEBUG])
    {
      fprintf (stderr, "\nDumping key list information:\ntotal non-static linked keywords = %d"
               "\ntotal keywords = %d\ntotal duplicates = %d\nmaximum key length = %d\n",
               _list_len, _total_keys, _total_duplicates, _max_key_len);
      dump ();
      fprintf (stderr, "End dumping list.\n\n");
    }
}

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
Key_List::get_special_input (char delimiter)
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
Key_List::save_include_src ()
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
Key_List::get_array_type ()
{
  return get_special_input ('%');
}

/* strcspn - find length of initial segment of S consisting entirely
   of characters not from REJECT (borrowed from Henry Spencer's
   ANSI string package, when GNU libc comes out I'll replace this...). */

#ifndef strcspn
inline int
Key_List::strcspn (const char *s, const char *reject)
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
Key_List::set_output_types ()
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

/* Reads in all keys from standard input and creates a linked list pointed
   to by Head.  This list is then quickly checked for ``links,'' i.e.,
   unhashable elements possessing identical key sets and lengths. */

void
Key_List::read_keys ()
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
  else
    {
      const char *delimiter = option.get_delimiter ();
      KeywordExt_List *temp;
      KeywordExt_List *trail = NULL;

      _head = parse_line (ptr, delimiter);
      _head->first()->init_selchars(this);

      for (temp = _head;
           (ptr = Read_Line::read_next_line ()) && strcmp (ptr, "%%");
           temp = temp->rest())
        {
          temp->rest() = parse_line (ptr, delimiter);
          temp->rest()->first()->init_selchars(this);
          _total_keys++;
        }

      /* See if any additional C code is included at end of this file. */
      if (ptr)
        _additional_code = 1;

      /* Hash table this number of times larger than keyword number. */
      int table_size = (_list_len = _total_keys) * TABLE_MULTIPLE;
      /* Table must be a power of 2 for the hash function scheme to work. */
      KeywordExt **table = new KeywordExt*[POW (table_size)];

      /* Make large hash table for efficiency. */
      Hash_Table found_link (table, table_size, option[NOLENGTH]);

      /* Test whether there are any links and also set the maximum length of
        an identifier in the keyword list. */

      for (temp = _head; temp; temp = temp->rest())
        {
          KeywordExt *keyword = temp->first();
          KeywordExt *other_keyword = found_link.insert (keyword);

          /* Check for links.  We deal with these by building an equivalence class
             of all duplicate values (i.e., links) so that only 1 keyword is
             representative of the entire collection.  This *greatly* simplifies
             processing during later stages of the program. */

          if (other_keyword)
            {
              _total_duplicates++;
              _list_len--;
              trail->rest() = temp->rest();
              temp->first()->_duplicate_link = other_keyword->_duplicate_link;
              other_keyword->_duplicate_link = temp->first();

              /* Complain if user hasn't enabled the duplicate option. */
              if (!option[DUP] || option[DEBUG])
                fprintf (stderr, "Key link: \"%.*s\" = \"%.*s\", with key set \"%.*s\".\n",
                                 keyword->_allchars_length, keyword->_allchars,
                                 other_keyword->_allchars_length, other_keyword->_allchars,
                                 keyword->_selchars_length, keyword->_selchars);
            }
          else
            trail = temp;

          /* Update minimum and maximum keyword length, if needed. */
          if (_max_key_len < keyword->_allchars_length)
            _max_key_len = keyword->_allchars_length;
          if (_min_key_len > keyword->_allchars_length)
            _min_key_len = keyword->_allchars_length;
        }

      delete[] table;

      /* Exit program if links exists and option[DUP] not set, since we can't continue */
      if (_total_duplicates)
        {
          if (option[DUP])
            fprintf (stderr, "%d input keys have identical hash values, examine output carefully...\n",
                             _total_duplicates);
          else
            {
              fprintf (stderr, "%d input keys have identical hash values,\ntry different key positions or use option -D.\n",
                               _total_duplicates);
              exit (1);
            }
        }
      /* Exit program if an empty string is used as key, since the comparison
         expressions don't work correctly for looking up an empty string. */
      if (_min_key_len == 0)
        {
          fprintf (stderr, "Empty input key is not allowed.\nTo recognize an empty input key, your code should check for\nlen == 0 before calling the gperf generated lookup function.\n");
          exit (1);
        }
      if (option[ALLCHARS])
        option.set_keysig_size (_max_key_len);
    }
}

/* Recursively merges two sorted lists together to form one sorted list. The
   ordering criteria is by frequency of occurrence of elements in the key set
   or by the hash value.  This is a kludge, but permits nice sharing of
   almost identical code without incurring the overhead of a function
   call comparison. */

KeywordExt_List *
Key_List::merge (KeywordExt_List *list1, KeywordExt_List *list2)
{
  KeywordExt_List *result;
  KeywordExt_List **resultp = &result;
  for (;;)
    {
      if (!list1)
        {
          *resultp = list2;
          break;
        }
      if (!list2)
        {
          *resultp = list1;
          break;
        }
      if (_occurrence_sort && list1->first()->_occurrence < list2->first()->_occurrence
          || _hash_sort && list1->first()->_hash_value > list2->first()->_hash_value)
        {
          *resultp = list2;
          resultp = &list2->rest(); list2 = list1; list1 = *resultp;
        }
      else
        {
          *resultp = list1;
          resultp = &list1->rest(); list1 = *resultp;
        }
    }
  return result;
}

/* Applies the merge sort algorithm to recursively sort the key list by
   frequency of occurrence of elements in the key set. */

KeywordExt_List *
Key_List::merge_sort (KeywordExt_List *head)
{
  if (!head || !head->rest())
    return head;
  else
    {
      KeywordExt_List *middle = head;
      KeywordExt_List *temp   = head->rest()->rest();

      while (temp)
        {
          temp   = temp->rest();
          middle = middle->rest();
          if (temp)
            temp = temp->rest();
        }

      temp         = middle->rest();
      middle->rest() = 0;
      return merge (merge_sort (head), merge_sort (temp));
    }
}

/* Returns the frequency of occurrence of elements in the key set. */

inline int
Key_List::get_occurrence (KeywordExt *ptr)
{
  int value = 0;

  const char *p = ptr->_selchars;
  unsigned int i = ptr->_selchars_length;
  for (; i > 0; p++, i--)
    value += _occurrences[(unsigned char)(*p)];

  return value;
}

/* Enables the index location of all key set elements that are now
   determined. */

inline void
Key_List::set_determined (KeywordExt *ptr)
{
  const char *p = ptr->_selchars;
  unsigned int i = ptr->_selchars_length;
  for (; i > 0; p++, i--)
    _determined[(unsigned char)(*p)] = 1;
}

/* Returns TRUE if PTR's key set is already completely determined. */

inline int
Key_List::already_determined (KeywordExt *ptr)
{
  int is_determined = 1;

  const char *p = ptr->_selchars;
  unsigned int i = ptr->_selchars_length;
  for (; is_determined && i > 0; p++, i--)
    is_determined = _determined[(unsigned char)(*p)];

  return is_determined;
}

/* Reorders the table by first sorting the list so that frequently occuring
   keys appear first, and then the list is reordered so that keys whose values
   are already determined will be placed towards the front of the list.  This
   helps prune the search time by handling inevitable collisions early in the
   search process.  See Cichelli's paper from Jan 1980 JACM for details.... */

void
Key_List::reorder ()
{
  KeywordExt_List *ptr;
  for (ptr = _head; ptr; ptr = ptr->rest())
    {
      KeywordExt *keyword = ptr->first();

      keyword->_occurrence = get_occurrence (keyword);
    }

  _hash_sort = 0;
  _occurrence_sort = 1;

  _head = merge_sort (_head);

  for (ptr = _head; ptr->rest(); ptr = ptr->rest())
    {
      set_determined (ptr->first());

      if (!already_determined (ptr->rest()->first()))
        {
          KeywordExt_List *trail_ptr = ptr->rest();
          KeywordExt_List *run_ptr   = trail_ptr->rest();

          for (; run_ptr; run_ptr = trail_ptr->rest())
            {

              if (already_determined (run_ptr->first()))
                {
                  trail_ptr->rest() = run_ptr->rest();
                  run_ptr->rest()   = ptr->rest();
                  ptr = ptr->rest() = run_ptr;
                }
              else
                trail_ptr = run_ptr;
            }
        }
    }
}

/* Sorts the keys by hash value. */

void
Key_List::sort ()
{
  _hash_sort       = 1;
  _occurrence_sort = 0;

  _head = merge_sort (_head);
}

/* Dumps the key list to stderr stream. */

void
Key_List::dump ()
{
  int field_width = option.get_max_keysig_size ();

  fprintf (stderr, "\nList contents are:\n(hash value, key length, index, %*s, keyword):\n",
           field_width, "selchars");

  for (KeywordExt_List *ptr = _head; ptr; ptr = ptr->rest())
    fprintf (stderr, "%11d,%11d,%6d, %*.*s, %.*s\n",
             ptr->first()->_hash_value, ptr->first()->_allchars_length, ptr->first()->_final_index,
             field_width, ptr->first()->_selchars_length, ptr->first()->_selchars,
             ptr->first()->_allchars_length, ptr->first()->_allchars);
}

/* Simple-minded constructor action here... */

Key_List::Key_List ()
{
  _total_keys       = 1;
  _max_key_len      = INT_MIN;
  _min_key_len      = INT_MAX;
  _array_type       = 0;
  _return_type      = 0;
  _struct_tag       = 0;
  _head             = 0;
  _total_duplicates = 0;
  _additional_code  = 0;
}

/* Returns the length of entire key list. */

int
Key_List::keyword_list_length ()
{
  return _list_len;
}

/* Returns length of longest key read. */

int
Key_List::max_key_length ()
{
  return _max_key_len;
}

