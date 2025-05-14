#include "lisp.h"
#include "character.h"

ptrdiff_t
copy_text (const unsigned char *from_addr, unsigned char *to_addr,
           ptrdiff_t nbytes, bool from_multibyte, bool to_multibyte)
{
  if (from_multibyte == to_multibyte)
    {
      memcpy (to_addr, from_addr, nbytes);
      return nbytes;
    }
  else if (from_multibyte)
    {
      ptrdiff_t nchars = 0;
      ptrdiff_t bytes_left = nbytes;

      while (bytes_left > 0)
        {
          int thislen, c = string_char_and_length (from_addr, &thislen);
          if (!ASCII_CHAR_P (c))
            c &= 0xFF;
          *to_addr++ = c;
          from_addr += thislen;
          bytes_left -= thislen;
          nchars++;
        }
      return nchars;
    }
  else
    {
      unsigned char *initial_to_addr = to_addr;

      while (nbytes > 0)
        {
          int c = *from_addr++;

          if (!ASCII_CHAR_P (c))
            {
              c = BYTE8_TO_CHAR (c);
              to_addr += CHAR_STRING (c, to_addr);
              nbytes--;
            }
          else
            *to_addr++ = c, nbytes--;
        }
      return to_addr - initial_to_addr;
    }
}
