#ifndef EMACS_BUFFER_H
#define EMACS_BUFFER_H

#include "lisp.h"
#include "nvim.h"

#define current_buffer (nvim_current_buffer ())

extern struct buffer buffer_defaults;

#define ZV (nvim_get_field_zv (current_buffer, true))
#define ZV_BYTE (nvim_get_field_zv (current_buffer, false))
#define BEGV (nvim_get_field_begv (current_buffer, true))
#define BEGV_BYTE (nvim_get_field_begv (current_buffer, false))

INLINE bool
BUFFERP (Lisp_Object a)
{
  return PSEUDOVECTORP (a, PVEC_BUFFER);
}

INLINE void
CHECK_BUFFER (Lisp_Object x)
{
  CHECK_TYPE (BUFFERP (x), Qbufferp, x);
}

INLINE struct buffer *
XBUFFER (Lisp_Object a)
{
  eassert (BUFFERP (a));
  return XUNTAG (a, Lisp_Vectorlike, struct buffer);
}

INLINE struct buffer *
decode_buffer (Lisp_Object b)
{
  return NILP (b) ? current_buffer : (CHECK_BUFFER (b), XBUFFER (b));
}

#define BVAR(buf, field) nvim_bvar (buf, NVIM_BUFFER_VAR__##field)

INLINE bool
BUFFER_LIVE_P (struct buffer *b)
{
  return !NILP (BVAR (b, name));
}

enum
{
  BUFFER_LISP_SIZE = PSEUDOVECSIZE (struct buffer, _last_obj),
};

enum
{
  BUFFER_REST_SIZE = VECSIZE (struct buffer) - BUFFER_LISP_SIZE
};

INLINE void
BUFFER_PVEC_INIT (struct buffer *b)
{
  XSETPVECTYPESIZE (b, PVEC_BUFFER, BUFFER_LISP_SIZE, BUFFER_REST_SIZE);
}

extern EMACS_INT fix_position (Lisp_Object);
#define CHECK_FIXNUM_COERCE_MARKER(x) ((x) = make_fixnum (fix_position (x)))

INLINE void
bset_local_var_alist (struct buffer *b, Lisp_Object val)
{
  b->_local_var_alist = val;
}

INLINE void
buffer_memcpy (unsigned char *dst, ptrdiff_t beg, ptrdiff_t size)
{
  nvim_buf_memcpy (dst, beg, size);
}

INLINE int
CHARACTER_WIDTH (int c)
{
  return (0x20 <= c && c < 0x7f  ? 1
          : 0x7f < c             ? (TODO, 0)
          : c == '\t'            ? (TODO, 0)
          : c == '\n'            ? 0
          : !NILP ((TODO, Qnil)) ? 2
                                 : 4);
}

INLINE int
downcase (int c)
{
  Lisp_Object downcase_table = BVAR (current_buffer, downcase_table);
  Lisp_Object down = CHAR_TABLE_REF (downcase_table, c);
  return FIXNATP (down) ? XFIXNAT (down) : c;
}

INLINE int
upcase (int c)
{
  Lisp_Object upcase_table = BVAR (current_buffer, upcase_table);
  Lisp_Object up = CHAR_TABLE_REF (upcase_table, c);
  return FIXNATP (up) ? XFIXNAT (up) : c;
}
#endif
