/*
   Tests the generated perfect hash function.
   The -v option prints diagnostics as to whether a word is in
   the set or not.  Without -v the program is useful for timing.
*/

#include <stdio.h>

#define MAX_LEN 80

int
main (argc, argv)
     int   argc;
     char *argv[];
{
  int  verbose = argc > 1 ? 1 : 0;
  char buf[2*MAX_LEN];
  int buflen;

  for (;;)
    {
      /* Simulate gets(buf) with 2 bytes per character. */
      char *p = buf;
      while (fread (p, 2, 1, stdin) == 1)
        {
          if ((p[0] << 8) + p[1] == '\n')
            break;
          p += 2;
        }
      buflen = p - buf;

      if (buflen == 0)
        break;

      if (in_word_set (buf, buflen) && verbose)
        printf ("in word set ");
      else if (verbose)
        printf ("NOT in word set ");
      for (p = buf; p < buf + buflen; p++)
        printf ("%02X", (unsigned char) *p);
      printf("\n");
    }

  return 0;
}
