/* Handles parsing the Options provided to the user.
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

/* Specification. */
#include "options.h"

#include <stdio.h>
#include <stdlib.h> /* declares atoi(), abs(), exit() */
#include <string.h> /* declares strcmp() */
#include <ctype.h>  /* declares isdigit() */
#include "getopt.h"
#include "version.h"

/* Global option coordinator for the entire program.  */
Options option;

/* Records the program name.  */
const char *program_name;

/* Size to jump on a collision.  */
static const int DEFAULT_JUMP_VALUE = 5;

/* Default name for generated lookup function.  */
static const char *const DEFAULT_NAME = "in_word_set";

/* Default name for the key component.  */
static const char *const DEFAULT_KEY = "name";

/* Default struct initializer suffix.  */
static const char *const DEFAULT_INITIALIZER_SUFFIX = "";

/* Default name for the generated class.  */
static const char *const DEFAULT_CLASS_NAME = "Perfect_Hash";

/* Default name for generated hash function.  */
static const char *const DEFAULT_HASH_NAME = "hash";

/* Default name for generated hash table array.  */
static const char *const DEFAULT_WORDLIST_NAME = "wordlist";

/* Default delimiters that separate keywords from their attributes.  */
static const char *const DEFAULT_DELIMITERS = ",\n";

/* Prints program usage to given stream. */

void
Options::short_usage (FILE * stream) const
{
  fprintf (stream, "Usage: %s [-cCdDef[num]F<initializers>GhH<hashname>i<init>Ij<jump>k<keys>K<keyname>lL<language>m<num>nN<function name>ors<size>S<switches>tTvW<wordlistname>Z<class name>7] [input-file]\n"
           "Try '%s --help' for more information.\n",
           program_name, program_name);
}

void
Options::long_usage (FILE * stream) const
{
  fprintf (stream,
           "GNU 'gperf' generates perfect hash functions.\n");
  fprintf (stream, "\n");
  fprintf (stream,
           "Usage: %s [OPTION]... [INPUT-FILE]\n",
           program_name);
  fprintf (stream, "\n");
  fprintf (stream,
           "If a long option shows an argument as mandatory, then it is mandatory\n"
           "for the equivalent short option also.\n");
  fprintf (stream, "\n");
  fprintf (stream,
           "Input file interpretation:\n");
  fprintf (stream,
           "  -e, --delimiters=DELIMITER-LIST\n"
           "                         Allow user to provide a string containing delimiters\n"
           "                         used to separate keywords from their attributes.\n"
           "                         Default is \",\\n\".\n");
  fprintf (stream,
           "  -t, --struct-type      Allows the user to include a structured type\n"
           "                         declaration for generated code. Any text before %%%%\n"
           "                         is considered part of the type declaration. Key\n"
           "                         words and additional fields may follow this, one\n"
           "                         group of fields per line.\n");
  fprintf (stream, "\n");
  fprintf (stream,
           "Language for the output code:\n");
  fprintf (stream,
           "  -L, --language=LANGUAGE-NAME\n"
           "                         Generates code in the specified language. Languages\n"
           "                         handled are currently C++, ANSI-C, C, and KR-C. The\n"
           "                         default is C.\n");
  fprintf (stream, "\n");
  fprintf (stream,
           "Details in the output code:\n");
  fprintf (stream,
           "  -K, --slot-name=NAME   Select name of the keyword component in the keyword\n"
           "                         structure.\n");
  fprintf (stream,
           "  -F, --initializer-suffix=INITIALIZERS\n"
           "                         Initializers for additional components in the keyword\n"
           "                         structure.\n");
  fprintf (stream,
           "  -H, --hash-fn-name=NAME\n"
           "                         Specify name of generated hash function. Default is\n"
           "                         'hash'.\n");
  fprintf (stream,
           "  -N, --lookup-fn-name=NAME\n"
           "                         Specify name of generated lookup function. Default\n"
           "                         name is 'in_word_set'.\n");
  fprintf (stream,
           "  -Z, --class-name=NAME  Specify name of generated C++ class. Default name is\n"
           "                         'Perfect_Hash'.\n");
  fprintf (stream,
           "  -7, --seven-bit        Assume 7-bit characters.\n");
  fprintf (stream,
           "  -c, --compare-strncmp  Generate comparison code using strncmp rather than\n"
           "                         strcmp.\n");
  fprintf (stream,
           "  -C, --readonly-tables  Make the contents of generated lookup tables\n"
           "                         constant, i.e., readonly.\n");
  fprintf (stream,
           "  -E, --enum             Define constant values using an enum local to the\n"
           "                         lookup function rather than with defines.\n");
  fprintf (stream,
           "  -I, --includes         Include the necessary system include file <string.h>\n"
           "                         at the beginning of the code.\n");
  fprintf (stream,
           "  -G, --global           Generate the static table of keywords as a static\n"
           "                         global variable, rather than hiding it inside of the\n"
           "                         lookup function (which is the default behavior).\n");
  fprintf (stream,
           "  -W, --word-array-name=NAME\n"
           "                         Specify name of word list array. Default name is\n"
           "                         'wordlist'.\n");
  fprintf (stream,
           "  -S, --switch=COUNT     Causes the generated C code to use a switch\n"
           "                         statement scheme, rather than an array lookup table.\n"
           "                         This can lead to a reduction in both time and space\n"
           "                         requirements for some keyfiles. The COUNT argument\n"
           "                         determines how many switch statements are generated.\n"
           "                         A value of 1 generates 1 switch containing all the\n"
           "                         elements, a value of 2 generates 2 tables with 1/2\n"
           "                         the elements in each table, etc. If COUNT is very\n"
           "                         large, say 1000000, the generated C code does a\n"
           "                         binary search.\n");
  fprintf (stream,
           "  -T, --omit-struct-type\n"
           "                         Prevents the transfer of the type declaration to the\n"
           "                         output file. Use this option if the type is already\n"
           "                         defined elsewhere.\n");
  fprintf (stream, "\n");
  fprintf (stream,
           "Algorithm employed by gperf:\n");
  fprintf (stream,
           "  -k, --key-positions=KEYS\n"
           "                         Select the key positions used in the hash function.\n"
           "                         The allowable choices range between 1-%d, inclusive.\n"
           "                         The positions are separated by commas, ranges may be\n"
           "                         used, and key positions may occur in any order.\n"
           "                         Also, the meta-character '*' causes the generated\n"
           "                         hash function to consider ALL key positions, and $\n"
           "                         indicates the \"final character\" of a key, e.g.,\n"
           "                         $,1,2,4,6-10.\n",
           Positions::MAX_KEY_POS);
  fprintf (stream,
           "  -l, --compare-strlen   Compare key lengths before trying a string\n"
           "                         comparison. This helps cut down on the number of\n"
           "                         string comparisons made during the lookup.\n");
  fprintf (stream,
           "  -D, --duplicates       Handle keywords that hash to duplicate values. This\n"
           "                         is useful for certain highly redundant keyword sets.\n");
  fprintf (stream,
           "  -f, --fast=ITERATIONS  Generate the gen-perf.hash function \"fast\". This\n"
           "                         decreases gperf's running time at the cost of\n"
           "                         minimizing generated table size. The numeric\n"
           "                         argument represents the number of times to iterate\n"
           "                         when resolving a collision. '0' means \"iterate by\n"
           "                         the number of keywords\".\n");
  fprintf (stream,
           "  -m, --multiple-iterations=ITERATIONS\n"
           "                         Perform multiple choices of the -i and -j values,\n"
           "                         and choose the best results. This increases the\n"
           "                         running time by a factor of ITERATIONS but does a\n"
           "                         good job minimizing the generated table size.\n");
  fprintf (stream,
           "  -i, --initial-asso=N   Provide an initial value for the associate values\n"
           "                         array. Default is 0. Setting this value larger helps\n"
           "                         inflate the size of the final table.\n");
  fprintf (stream,
           "  -j, --jump=JUMP-VALUE  Affects the \"jump value\", i.e., how far to advance\n"
           "                         the associated character value upon collisions. Must\n"
           "                         be an odd number, default is %d.\n",
           DEFAULT_JUMP_VALUE);
  fprintf (stream,
           "  -n, --no-strlen        Do not include the length of the keyword when\n"
           "                         computing the hash function.\n");
  fprintf (stream,
           "  -o, --occurrence-sort  Reorders input keys by frequency of occurrence of\n"
           "                         the key sets. This should decrease the search time\n"
           "                         dramatically.\n");
  fprintf (stream,
           "  -r, --random           Utilizes randomness to initialize the associated\n"
           "                         values table.\n");
  fprintf (stream,
           "  -s, --size-multiple=N  Affects the size of the generated hash table. The\n"
           "                         numeric argument N indicates \"how many times larger\n"
           "                         or smaller\" the associated value range should be,\n"
           "                         in relationship to the number of keys, e.g. a value\n"
           "                         of 3 means \"allow the maximum associated value to\n"
           "                         be about 3 times larger than the number of input\n"
           "                         keys\". Conversely, a value of -3 means \"make the\n"
           "                         maximum associated value about 3 times smaller than\n"
           "                         the number of input keys\". A larger table should\n"
           "                         decrease the time required for an unsuccessful\n"
           "                         search, at the expense of extra table space. Default\n"
           "                         value is 1.\n");
  fprintf (stream, "\n");
  fprintf (stream,
           "Informative output:\n"
           "  -h, --help             Print this message.\n"
           "  -v, --version          Print the gperf version number.\n"
           "  -d, --debug            Enables the debugging option (produces verbose\n"
           "                         output to the standard error).\n");
  fprintf (stream, "\n");
  fprintf (stream,
           "Report bugs to <bug-gnu-utils@gnu.org>.\n");
}

/* Prints the given options.  */

void
Options::print_options () const
{
  int i;

  printf ("/* Command-line: ");

  for (i = 0; i < _argument_count; i++)
    {
      const char *arg = _argument_vector[i];

      /* Escape arg if it contains shell metacharacters.  */
      if (*arg == '-')
        {
          putchar (*arg);
          arg++;
          if (*arg >= 'A' && *arg <= 'Z' || *arg >= 'a' && *arg <= 'z')
            {
              putchar (*arg);
              arg++;
            }
        }
      if (strpbrk (arg, "\t\n !\"#$&'()*;<>?[\\]`{|}~") != NULL)
        {
          if (strchr (arg, '\'') != NULL)
            {
              putchar ('"');
              for (; *arg; arg++)
                {
                  if (*arg == '\"' || *arg == '\\' || *arg == '$' || *arg == '`')
                    putchar ('\\');
                  putchar (*arg);
                }
              putchar ('"');
            }
          else
            {
              putchar ('\'');
              for (; *arg; arg++)
                {
                  if (*arg == '\\')
                    putchar ('\\');
                  putchar (*arg);
                }
              putchar ('\'');
            }
        }
      else
        printf ("%s", arg);

      printf (" ");
    }

  printf (" */");
}

/* ------------------------------------------------------------------------- */

/* Parses a string denoting key positions.  */

class PositionStringParser
{
public:
  /* Initializes a key position string parser for string STR.  */
                        PositionStringParser (const char *str,
                                              int low_bound, int high_bound,
                                              int end_word_marker, int error_value, int end_marker);
  /* Returns the next key position from the given string.  */
  int                   nextPosition ();
private:
  /* A pointer to the string provided by the user.  */
  const char *          _str;
  /* Smallest possible value, inclusive.  */
  int const             _low_bound;
  /* Greatest possible value, inclusive.  */
  int const             _high_bound;
  /* A value marking the abstract "end of word" ( usually '$').  */
  int const             _end_word_marker;
  /* Error value returned when input is syntactically erroneous.  */
  int const             _error_value;
  /* Value returned after last key is processed.  */
  int const             _end_marker;
  /* Intermediate state for producing a range of positions.  */
  bool                  _in_range;           /* True while producing a range of positions.  */
  int                   _range_upper_bound;  /* Upper bound (inclusive) of the range.  */
  int                   _range_curr_value;   /* Last value returned.  */
};

/* Initializes a key position strng parser for string STR.  */
PositionStringParser::PositionStringParser (const char *str,
                                            int low_bound, int high_bound,
                                            int end_word_marker, int error_value, int end_marker)
  : _str (str),
    _low_bound (low_bound),
    _high_bound (high_bound),
    _end_word_marker (end_word_marker),
    _error_value (error_value),
    _end_marker (end_marker),
    _in_range (false)
{
}

/* Returns the next key position from the given string.  */
int
PositionStringParser::nextPosition ()
{
  if (_in_range)
    {
      /* We are inside a range.  Return the next value from the range.  */
      if (++_range_curr_value >= _range_upper_bound)
        _in_range = false;
      return _range_curr_value;
    }
  else
    {
      /* Continue parsing the given string.  */
      while (*_str)
        switch (*_str)
          {
          case ',':
            /* Skip the comma.  */
            _str++;
            break;
          case '$':
            /* Valid key position.  */
            _str++;
            return _end_word_marker;
          case '0': case '1': case '2': case '3': case '4':
          case '5': case '6': case '7': case '8': case '9':
            /* Valid key position.  */
            {
              int curr_value;
              for (curr_value = 0; isdigit (static_cast<unsigned char>(*_str)); _str++)
                curr_value = curr_value * 10 + (*_str - '0');

              if (*_str == '-')
                {
                  _str++;
                  /* Starting a range of key positions.  */
                  _in_range = true;

                  for (_range_upper_bound = 0;
                       isdigit (static_cast<unsigned char>(*_str));
                       _str++)
                    _range_upper_bound = _range_upper_bound * 10 + (*_str - '0');

                  /* Verify range's upper bound.  */
                  if (!(_range_upper_bound > curr_value && _range_upper_bound <= _high_bound))
                    return _error_value;
                  _range_curr_value = curr_value;
                }

              /* Verify range's lower bound.  */
              if (!(curr_value >= _low_bound && curr_value <= _high_bound))
                return _error_value;
              return curr_value;
            }
          default:
            /* Invalid syntax.  */
            return _error_value;
          }

      return _end_marker;
    }
}

/* ------------------------------------------------------------------------- */

/* Sets the default Options. */

Options::Options ()
  : _option_word (C),
    _input_file_name (NULL),
    _iterations (0),
    _jump (DEFAULT_JUMP_VALUE),
    _initial_asso_value (0),
    _asso_iterations (0),
    _total_switches (1),
    _size_multiple (1),
    _function_name (DEFAULT_NAME),
    _key_name (DEFAULT_KEY),
    _initializer_suffix (DEFAULT_INITIALIZER_SUFFIX),
    _class_name (DEFAULT_CLASS_NAME),
    _hash_name (DEFAULT_HASH_NAME),
    _wordlist_name (DEFAULT_WORDLIST_NAME),
    _delimiters (DEFAULT_DELIMITERS),
    _key_positions (1, Positions::LASTCHAR)
{
}

/* Dumps option status when debug is set. */

Options::~Options ()
{
  if (_option_word & DEBUG)
    {
      fprintf (stderr, "\ndumping Options:"
               "\nDEBUG is.......: %s"
               "\nORDER is.......: %s"
               "\nTYPE is........: %s"
               "\nRANDOM is......: %s"
               "\nSWITCH is......: %s"
               "\nNOLENGTH is....: %s"
               "\nLENTABLE is....: %s"
               "\nDUP is.........: %s"
               "\nFAST is........: %s"
               "\nCOMP is........: %s"
               "\nNOTYPE is......: %s"
               "\nGLOBAL is......: %s"
               "\nCONST is.......: %s"
               "\nKRC is.........: %s"
               "\nC is...........: %s"
               "\nANSIC is.......: %s"
               "\nCPLUSPLUS is...: %s"
               "\nENUM is........: %s"
               "\nINCLUDE is.....: %s"
               "\nSEVENBIT is....: %s"
               "\niterations = %d"
               "\nlookup function name = %s"
               "\nhash function name = %s"
               "\nword list name = %s"
               "\nkey name = %s"
               "\ninitializer suffix = %s"
               "\nasso_values iterations = %d"
               "\njump value = %d"
               "\nhash table size multiplier = %d"
               "\ninitial associated value = %d"
               "\ndelimiters = %s"
               "\nnumber of switch statements = %d\n",
               _option_word & DEBUG ? "enabled" : "disabled",
               _option_word & ORDER ? "enabled" : "disabled",
               _option_word & TYPE ? "enabled" : "disabled",
               _option_word & RANDOM ? "enabled" : "disabled",
               _option_word & SWITCH ? "enabled" : "disabled",
               _option_word & NOLENGTH ? "enabled" : "disabled",
               _option_word & LENTABLE ? "enabled" : "disabled",
               _option_word & DUP ? "enabled" : "disabled",
               _option_word & FAST ? "enabled" : "disabled",
               _option_word & COMP ? "enabled" : "disabled",
               _option_word & NOTYPE ? "enabled" : "disabled",
               _option_word & GLOBAL ? "enabled" : "disabled",
               _option_word & CONST ? "enabled" : "disabled",
               _option_word & KRC ? "enabled" : "disabled",
               _option_word & C ? "enabled" : "disabled",
               _option_word & ANSIC ? "enabled" : "disabled",
               _option_word & CPLUSPLUS ? "enabled" : "disabled",
               _option_word & ENUM ? "enabled" : "disabled",
               _option_word & INCLUDE ? "enabled" : "disabled",
               _option_word & SEVENBIT ? "enabled" : "disabled",
               _iterations,
               _function_name, _hash_name, _wordlist_name, _key_name,
               _initializer_suffix, _asso_iterations, _jump, _size_multiple,
               _initial_asso_value, _delimiters, _total_switches);
      if (_option_word & ALLCHARS)
        fprintf (stderr, "all characters are used in the hash function\n");
      else
        {
          fprintf (stderr, "maximum keysig size = %d\nkey positions are: \n",
                   _key_positions.get_size());

          PositionIterator iter (_key_positions);
          for (int pos; (pos = iter.next()) != PositionIterator::EOS; )
            if (pos == Positions::LASTCHAR)
              fprintf (stderr, "$\n");
            else
              fprintf (stderr, "%d\n", pos);
        }

      fprintf (stderr, "finished dumping Options\n");
    }
}


/* Parses the command line Options and sets appropriate flags in option_word. */

static const struct option long_options[] =
{
  { "delimiters", required_argument, NULL, 'e' },
  { "struct-type", no_argument, NULL, 't' },
  { "language", required_argument, NULL, 'L' },
  { "slot-name", required_argument, NULL, 'K' },
  { "initializer-suffix", required_argument, NULL, 'F' },
  { "hash-fn-name", required_argument, NULL, 'H' },
  { "lookup-fn-name", required_argument, NULL, 'N' },
  { "class-name", required_argument, NULL, 'Z' },
  { "seven-bit", no_argument, NULL, '7' },
  { "compare-strncmp", no_argument, NULL, 'c' },
  { "readonly-tables", no_argument, NULL, 'C' },
  { "enum", no_argument, NULL, 'E' },
  { "includes", no_argument, NULL, 'I' },
  { "global", no_argument, NULL, 'G' },
  { "word-array-name", required_argument, NULL, 'W' },
  { "switch", required_argument, NULL, 'S' },
  { "omit-struct-type", no_argument, NULL, 'T' },
  { "key-positions", required_argument, NULL, 'k' },
  { "compare-strlen", no_argument, NULL, 'l' },
  { "duplicates", no_argument, NULL, 'D' },
  { "fast", required_argument, NULL, 'f' },
  { "initial-asso", required_argument, NULL, 'i' },
  { "jump", required_argument, NULL, 'j' },
  { "multiple-iterations", required_argument, NULL, 'm' },
  { "no-strlen", no_argument, NULL, 'n' },
  { "occurrence-sort", no_argument, NULL, 'o' },
  { "random", no_argument, NULL, 'r' },
  { "size-multiple", required_argument, NULL, 's' },
  { "help", no_argument, NULL, 'h' },
  { "version", no_argument, NULL, 'v' },
  { "debug", no_argument, NULL, 'd' },
  { NULL, no_argument, NULL, 0 }
};

void
Options::parse_options (int argc, char *argv[])
{
  int option_char;

  program_name = argv[0];
  _argument_count  = argc;
  _argument_vector = argv;

  while ((option_char =
            getopt_long (_argument_count, _argument_vector,
                         "acCdDe:Ef:F:gGhH:i:Ij:k:K:lL:m:nN:oprs:S:tTvW:Z:7",
                         long_options, NULL))
         != -1)
    {
      switch (option_char)
        {
        case 'a':               /* Generated code uses the ANSI prototype format. */
          break;                /* This is now the default. */
        case 'c':               /* Generate strncmp rather than strcmp. */
          {
            _option_word |= COMP;
            break;
          }
        case 'C':               /* Make the generated tables readonly (const). */
          {
            _option_word |= CONST;
            break;
          }
        case 'd':               /* Enable debugging option. */
          {
            _option_word |= DEBUG;
            fprintf (stderr, "Starting program %s, version %s, with debugging on.\n",
                             program_name, version_string);
            break;
          }
        case 'D':               /* Enable duplicate option. */
          {
            _option_word |= DUP;
            break;
          }
        case 'e':               /* Specify keyword/attribute separator */
          {
            _delimiters = /*getopt*/optarg;
            break;
          }
        case 'E':
          {
            _option_word |= ENUM;
            break;
          }
        case 'f':               /* Generate the hash table "fast". */
          {
            _option_word |= FAST;
            if ((_iterations = atoi (/*getopt*/optarg)) < 0)
              {
                fprintf (stderr, "iterations value must not be negative, assuming 0\n");
                _iterations = 0;
              }
            break;
          }
        case 'F':
          {
            _initializer_suffix = /*getopt*/optarg;
            break;
          }
        case 'g':               /* Use the 'inline' keyword for generated sub-routines, ifdef __GNUC__. */
          break;                /* This is now the default. */
        case 'G':               /* Make the keyword table a global variable. */
          {
            _option_word |= GLOBAL;
            break;
          }
        case 'h':               /* Displays a list of helpful Options to the user. */
          {
            long_usage (stdout);
            exit (0);
          }
        case 'H':               /* Sets the name for the hash function */
          {
            _hash_name = /*getopt*/optarg;
            break;
          }
        case 'i':               /* Sets the initial value for the associated values array. */
          {
            if ((_initial_asso_value = atoi (/*getopt*/optarg)) < 0)
              fprintf (stderr, "Initial value %d should be non-zero, ignoring and continuing.\n", _initial_asso_value);
            if (option[RANDOM])
              fprintf (stderr, "warning, -r option superceeds -i, ignoring -i option and continuing\n");
            break;
          }
        case 'I':               /* Enable #include statements. */
          {
            _option_word |= INCLUDE;
            break;
          }
        case 'j':               /* Sets the jump value, must be odd for later algorithms. */
          {
            if ((_jump = atoi (/*getopt*/optarg)) < 0)
              {
                fprintf (stderr, "Jump value %d must be a positive number.\n", _jump);
                short_usage (stderr);
                exit (1);
              }
            else if (_jump && ((_jump % 2) == 0))
              fprintf (stderr, "Jump value %d should be odd, adding 1 and continuing...\n", _jump++);
            break;
          }
        case 'k':               /* Sets key positions used for hash function. */
          {
            const int BAD_VALUE = -2;
            const int EOS = PositionIterator::EOS;
            int       value;
            PositionStringParser sparser (/*getopt*/optarg, 1, Positions::MAX_KEY_POS, Positions::LASTCHAR, BAD_VALUE, EOS);

            if (/*getopt*/optarg [0] == '*') /* Use all the characters for hashing!!!! */
              _option_word |= ALLCHARS;
            else
              {
                unsigned char *key_positions = _key_positions.pointer();
                unsigned char *key_pos;

                for (key_pos = key_positions; (value = sparser.nextPosition()) != EOS; key_pos++)
                  {
                    if (value == BAD_VALUE)
                      {
                        fprintf (stderr, "Invalid key value or range, use 1,2,3-%d,'$' or '*'.\n",
                                         Positions::MAX_KEY_POS);
                        short_usage (stderr);
                        exit (1);
                      }
                    if (key_pos - key_positions == Positions::MAX_KEY_POS + 1)
                      {
                        /* More than Positions::MAX_KEY_POS + 1 key positions.
                           Since all key positions are in the range
                           1..Positions::MAX_KEY_POS or == Positions::LASTCHAR,
                           there must be duplicates.  */
                        fprintf (stderr, "Duplicate keys selected\n");
                        short_usage (stderr);
                        exit (1);
                      }
                    *key_pos = value;
                  }

                unsigned int total_keysig_size = key_pos - key_positions;
                if (total_keysig_size == 0)
                  {
                    fprintf (stderr, "No keys selected.\n");
                    short_usage (stderr);
                    exit (1);
                  }
                _key_positions.set_size (total_keysig_size);

                /* Sorts the key positions *IN REVERSE ORDER!!*
                   This makes further routines more efficient.  Especially
                   when generating code.  */
                if (! _key_positions.sort())
                  {
                    fprintf (stderr, "Duplicate keys selected\n");
                    short_usage (stderr);
                    exit (1);
                  }
              }
            break;
          }
        case 'K':               /* Make this the keyname for the keyword component field. */
          {
            _key_name = /*getopt*/optarg;
            break;
          }
        case 'l':               /* Create length table to avoid extra string compares. */
          {
            _option_word |= LENTABLE;
            break;
          }
        case 'L':               /* Deal with different generated languages. */
          {
            _option_word &= ~(KRC | C | ANSIC | CPLUSPLUS);
            if (!strcmp (/*getopt*/optarg, "KR-C"))
              _option_word |= KRC;
            else if (!strcmp (/*getopt*/optarg, "C"))
              _option_word |= C;
            else if (!strcmp (/*getopt*/optarg, "ANSI-C"))
              _option_word |= ANSIC;
            else if (!strcmp (/*getopt*/optarg, "C++"))
              _option_word |= CPLUSPLUS;
            else
              {
                fprintf (stderr, "unsupported language option %s, defaulting to C\n", /*getopt*/optarg);
                _option_word |= C;
              }
            break;
          }
        case 'm':               /* Multiple iterations for finding good asso_values.  */
          {
            if ((_asso_iterations = atoi (/*getopt*/optarg)) < 0)
              {
                fprintf (stderr, "asso_iterations value must not be negative, assuming 0\n");
                _asso_iterations = 0;
              }
            break;
          }
        case 'n':               /* Don't include the length when computing hash function. */
          {
            _option_word |= NOLENGTH;
            break;
          }
        case 'N':               /* Make generated lookup function name be optarg */
          {
            _function_name = /*getopt*/optarg;
            break;
          }
        case 'o':               /* Order input by frequency of key set occurrence. */
          {
            _option_word |= ORDER;
            break;
          }
        case 'p':               /* Generated lookup function a pointer instead of int. */
          break;                /* This is now the default. */
        case 'r':               /* Utilize randomness to initialize the associated values table. */
          {
            _option_word |= RANDOM;
            if (_initial_asso_value != 0)
              fprintf (stderr, "warning, -r option superceeds -i, disabling -i option and continuing\n");
            break;
          }
        case 's':               /* Range of associated values, determines size of final table. */
          {
            if (abs (_size_multiple = atoi (/*getopt*/optarg)) > 50)
              fprintf (stderr, "%d is excessive, did you really mean this?! (try `%s --help' for help)\n", _size_multiple, program_name);
            break;
          }
        case 'S':               /* Generate switch statement output, rather than lookup table. */
          {
            _option_word |= SWITCH;
            if ((_total_switches = atoi (/*getopt*/optarg)) <= 0)
              {
                fprintf (stderr, "number of switches %s must be a positive number\n", /*getopt*/optarg);
                short_usage (stderr);
                exit (1);
              }
            break;
          }
        case 't':               /* Enable the TYPE mode, allowing arbitrary user structures. */
          {
            _option_word |= TYPE;
            break;
          }
        case 'T':   /* Don't print structure definition. */
          {
            _option_word |= NOTYPE;
            break;
          }
        case 'v':               /* Print out the version and quit. */
          fprintf (stdout, "GNU gperf %s\n", version_string);
          exit (0);
        case 'W':               /* Sets the name for the hash table array */
          {
            _wordlist_name = /*getopt*/optarg;
            break;
          }
        case 'Z':               /* Set the class name. */
          {
            _class_name = /*getopt*/optarg;
            break;
          }
        case '7':               /* Assume 7-bit characters. */
          {
            _option_word |= SEVENBIT;
            break;
          }
        default:
          short_usage (stderr);
          exit (1);
        }

    }

  if (/*getopt*/optind < argc)
    _input_file_name = argv[/*getopt*/optind++];

  if (/*getopt*/optind < argc)
    {
      fprintf (stderr, "Extra trailing arguments to %s.\n", program_name);
      short_usage (stderr);
      exit (1);
    }
}

#ifndef __OPTIMIZE__

#define INLINE /* not inline */
#include "options.icc"
#undef INLINE

#endif /* not defined __OPTIMIZE__ */
