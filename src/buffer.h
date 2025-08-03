#ifndef EMACS_BUFFER_H
#define EMACS_BUFFER_H

#include "lisp.h"
#include "nvim.h"

#define current_buffer (nvim_current_buffer ())

extern struct buffer buffer_defaults;

enum
{
  BEG = 1,
  BEG_BYTE = BEG
};

#define BUF_Z(buf) ZV // TODO: is this correct? (we presume there's no gap)
#define BUF_Z_BYTE(buf) \
  ZV_BYTE // TODO: is this correct? (we presume there's no gap)

#define ZV (nvim_get_field_zv (current_buffer, true))
#define ZV_BYTE (nvim_get_field_zv (current_buffer, false))
#define BEGV (nvim_get_field_begv (current_buffer, true))
#define BEGV_BYTE (nvim_get_field_begv (current_buffer, false))
#define PT (nvim_get_field_pt (current_buffer, true))
#define PT_BYTE (nvim_get_field_pt (current_buffer, false))

#define set_point nvim_set_point

INLINE void
SET_PT (ptrdiff_t position)
{
  set_point (position);
}

INLINE ptrdiff_t
BYTE_TO_CHAR (ptrdiff_t bytepos)
{
#if TODO_NELISP_LATER_AND
  return buf_bytepos_to_charpos (current_buffer, bytepos);
#else
  return bytepos;
#endif
}

INLINE ptrdiff_t
CHAR_TO_BYTE (ptrdiff_t charpos)
{
#if TODO_NELISP_LATER_AND
  return buf_charpos_to_bytepos (current_buffer, charpos);
#else
  return charpos;
#endif
}

extern void set_buffer_if_live (Lisp_Object);

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

#define FOR_EACH_LIVE_BUFFER(list_var, buf_var)                   \
  for ((list_var) = (nvim_buffer_list ());                        \
       (CONSP (list_var) && ((buf_var) = XCAR (list_var), true)); \
       (list_var) = XCDR (list_var))

INLINE void
record_unwind_current_buffer (void)
{
  record_unwind_protect (set_buffer_if_live, Fcurrent_buffer ());
}

#define BVAR(buf, field) nvim_bvar (buf, NVIM_BUFFER_VAR_##field##_)
#define BVAR_(buf, field) ((buf)->field##_)
#define MBVAR_(buf, field) (((struct meta_buffer *) buf)->field##_)
#define BVAR_SET(buf, field, val) \
  nvim_bvar_set (buf, NVIM_BUFFER_VAR_##field##_, val)

INLINE bool
BUFFER_LIVE_P (struct buffer *b)
{
  return !NILP (BVAR (b, name));
}

enum
{
  BUFFER_LISP_SIZE = PSEUDOVECSIZE (struct buffer, last_obj_),
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

INLINE Lisp_Object
per_buffer_value (struct buffer *b, int offset)
{
  if (offset >= sizeof (struct buffer))
    return nvim_mbvar (b, offset);
  return *(Lisp_Object *) (offset + (char *) b);
}
INLINE void
set_per_buffer_value (struct buffer *b, int offset, Lisp_Object value)
{
  if (offset >= sizeof (struct buffer))
    nvim_mbvar_set (b, offset, value);
  else
    *(Lisp_Object *) (offset + (char *) b) = value;
}

INLINE void
bset_local_var_alist (struct buffer *b, Lisp_Object val)
{
  b->local_var_alist_ = val;
}
INLINE void
bset_undo_list (struct buffer *b, Lisp_Object val)
{
  BVAR_SET (b, undo_list, val);
}
INLINE void
bset_enable_multibyte_characters (struct buffer *b, Lisp_Object val)
{
  b->enable_multibyte_characters_ = val;
}

INLINE ptrdiff_t
BUF_PT (struct buffer *buf)
{
  return (buf == current_buffer ? PT : (TODO, 0));
}
INLINE ptrdiff_t
BUF_PT_BYTE (struct buffer *buf)
{
  return (buf == current_buffer ? PT_BYTE : (TODO, 0));
}
INLINE ptrdiff_t
BUF_BEG (struct buffer *buf)
{
  return BEG;
}
INLINE ptrdiff_t
BUF_BEG_BYTE (struct buffer *buf)
{
  return BEG_BYTE;
}
INLINE ptrdiff_t
BUF_ZV (struct buffer *buf)
{
  return (buf == current_buffer ? ZV : (TODO, 0));
}
INLINE ptrdiff_t
BUF_ZV_BYTE (struct buffer *buf)
{
  return (buf == current_buffer ? ZV_BYTE : (TODO, 0));
}

INLINE void
buffer_memcpy (unsigned char *dst, ptrdiff_t beg, ptrdiff_t size)
{
  nvim_buf_memcpy (dst, beg, size);
}

INLINE int
sanitize_char_width (EMACS_INT width)
{
  return 0 <= width && width <= 1000 ? width : 1000;
}

INLINE int
CHARACTER_WIDTH (int c)
{
  return (0x20 <= c && c < 0x7f  ? 1
          : 0x7f < c             ? (sanitize_char_width (
                         XFIXNUM (CHAR_TABLE_REF (Vchar_width_table, c))))
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
