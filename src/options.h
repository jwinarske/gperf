/* This may look like C code, but it is really -*- C++ -*- */

/* Handles parsing the Options provided to the user.

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

/* This module provides a uniform interface to the various options available
   to a user of the gperf hash function generator.  */

#ifndef options_h
#define options_h 1

#include <stdio.h>

/* Enumeration of the possible boolean options. */

enum Option_Type
{
  /* Enable debugging (prints diagnostics to stderr).  */
  DEBUG        = 1 << 0,

  /* Apply ordering heuristic to speed-up search time.  */
  ORDER        = 1 << 1,

  /* Use all characters in hash function.  */
  ALLCHARS     = 1 << 2,

  /* Handle user-defined type structured keyword input.  */
  TYPE         = 1 << 3,

  /* Randomly initialize the associated values table.  */
  RANDOM       = 1 << 4,

  /* Make default char positions be 1,$ (end of keyword).  */
  DEFAULTCHARS = 1 << 5,

  /* Generate switch output to save space.  */
  SWITCH       = 1 << 6,

  /* Don't include keyword length in hash computations.  */
  NOLENGTH     = 1 << 7,

  /* Generate a length table for string comparison.  */
  LENTABLE     = 1 << 8,

  /* Handle duplicate hash values for keywords.  */
  DUP          = 1 << 9,

  /* Generate the hash function "fast".  */
  FAST         = 1 << 10,

  /* Don't include user-defined type definition in output -- it's already
     defined elsewhere.  */
  NOTYPE       = 1 << 11,

  /* Generate strncmp rather than strcmp.  */
  COMP         = 1 << 12,

  /* Make the keyword table a global variable.  */
  GLOBAL       = 1 << 13,

  /* Make the generated tables readonly (const).  */
  CONST        = 1 << 14,

  /* Generate K&R C code: no prototypes, no const.  */
  KRC          = 1 << 15,

  /* Generate C code: no prototypes, but const (user can #define it away).  */
  C            = 1 << 16,

  /* Generate ISO/ANSI C code: prototypes and const, but no class.  */
  ANSIC        = 1 << 17,

  /* Generate C++ code: prototypes, const, class, inline, enum.  */
  CPLUSPLUS    = 1 << 18,

  /* Use enum for constants.  */
  ENUM         = 1 << 19,

  /* Generate #include statements.  */
  INCLUDE      = 1 << 20,

  /* Assume 7-bit, not 8-bit, characters.  */
  SEVENBIT     = 1 << 21
};

/* This class denotes a set of key positions.  */

class Positions
{
  friend class PositionIterator;
public:
  /* Denotes the last char of a keyword, depending on the keyword's length.  */
  static const int LASTCHAR = 0;

  /* Maximum size of the set.  */
  static const int MAX_KEY_POS = 127;

  /* Constructors.  */
  Positions ();
  Positions (int key1);
  Positions (int key1, int key2);

  /* Accessors.  */
  int operator[] (unsigned int index) const;
  unsigned int get_size () const;

  /* Write access.  */
  unsigned char * pointer ();
  void set_size (unsigned int size);

  /* Sorts the array in reverse order.
     Returns 1 if there are no duplicates, 0 otherwise.  */
  int sort ();

private:
  /* Number of positions, excluding the terminating PositionIterator::EOS.  */
  unsigned int _size;
  /* Array of positions.  1 for the first char, 2 for the second char etc.,
     LASTCHAR for the last char.  PositionIterator::EOS past the end.  */
  unsigned char _positions[MAX_KEY_POS];
};

/* This class denotes an iterator through a set of key positions.  */

class PositionIterator
{
public:
  /* Initializes an iterator through POSITIONS.  */
  PositionIterator (Positions const& positions);

  /* End of iteration marker.  */
  static const int EOS = Positions::MAX_KEY_POS;

  /* Retrieves the next position, or EOS past the end.  */
  int next ();

private:
  const Positions& _set;
  int _index;
};

/* Class manager for gperf program Options. */

class Options
{
public:
  /* Constructor.  */
  Options ();

  /* Destructor.  */
  ~Options ();

  /* Parses the options given in the command-line arguments.  */
  void parse_options (int argc, char *argv[]);

  /* Prints the given options.  */
  void print_options () const;

  /* Accessors.  */

  /* Tests a given boolean option.  Returns 1 if set, 0 otherwise.  */
  int operator[] (Option_Type option) const;

  /* Returns the iterations value.  */
  int get_iterations () const;

  /* Returns the jump value.  */
  int get_jump () const;

  /* Returns the initial associated character value.  */
  int get_initial_asso_value () const;

  /* Returns the total number of switch statements to generate.  */
  int get_total_switches () const;

  /* Returns the factor by which to multiply the generated table's size.  */
  int get_size_multiple () const;

  /* Returns the generated function name.  */
  const char * get_function_name () const;

  /* Returns the keyword key name.  */
  const char * get_key_name () const;

  /* Returns the struct initializer suffix.  */
  const char * get_initializer_suffix () const;

  /* Returns the generated class name.  */
  const char * get_class_name () const;

  /* Returns the hash function name.  */
  const char * get_hash_name () const;

  /* Returns the hash table array name.  */
  const char * get_wordlist_name () const;

  /* Returns the string used to delimit keywords from other attributes.  */
  const char * get_delimiter () const;

  /* Returns key positions.  */
  const Positions& get_key_positions () const;

  /* Returns total distinct key positions. */
  int get_max_keysig_size () const;

private:
  /* Prints program usage to given stream. */
  void short_usage (FILE * stream) const;

  /* Prints program usage to given stream. */
  void long_usage (FILE * stream) const;

  /* Records count of command-line arguments.  */
  int _argument_count;

  /* Stores a pointer to command-line argument vector.  */
  char **_argument_vector;

  /* Holds the boolean options.  */
  int _option_word;

  /* Amount to iterate when a collision occurs.  */
  int _iterations;

  /* Jump length when trying alternative values.  */
  int _jump;

  /* Initial value for asso_values table.  */
  int _initial_asso_value;

  /* Number of switch statements to generate.  */
  int _total_switches;

  /* Factor by which to multiply the generated table's size.  */
  int _size_multiple;

  /* Names used for generated lookup function.  */
  const char *_function_name;

  /* Name used for keyword key.  */
  const char *_key_name;

  /* Suffix for empty struct initializers.  */
  const char *_initializer_suffix;

  /* Name used for generated C++ class.  */
  const char *_class_name;

  /* Name used for generated hash function.  */
  const char *_hash_name;

  /* Name used for hash table array.  */
  const char *_wordlist_name;

  /* Separates keywords from other attributes.  */
  const char *_delimiters;

  /* Contains user-specified key choices.  */
  Positions _key_positions;
};

/* Global option coordinator for the entire program.  */
extern Options option;

#ifdef __OPTIMIZE__

#define INLINE inline
#include "options.icc"
#undef INLINE

#endif

#endif
