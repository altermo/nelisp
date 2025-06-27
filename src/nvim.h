#ifndef NVIM_H
#define NVIM_H

#include "lisp.h"

// --- buffer --

#define Xbuffer_vars    \
  X (_local_var_alist)  \
  X (_downcase_table)   \
  X (_upcase_table)     \
  X (_case_canon_table) \
  X (_case_eqv_table)   \
  X (_syntax_table)

struct buffer
{
  union vectorlike_header header;

#define X(field) Lisp_Object field;
  Xbuffer_vars
#undef X

    Lisp_Object _last_obj;

  long bufid;
};

enum nvim_buffer_var_field
{
  NVIM_BUFFER_VAR__name,
#define X(field) NVIM_BUFFER_VAR_##field,
  Xbuffer_vars
#undef X
};

extern Lisp_Object nvim_name_to_bufobj (Lisp_Object);
extern Lisp_Object nvim_create_buf (Lisp_Object, Lisp_Object);
extern Lisp_Object nvim_bvar (struct buffer *, enum nvim_buffer_var_field);
extern void nvim_set_buffer (struct buffer *);
extern struct buffer *nvim_current_buffer (void);

extern ptrdiff_t nvim_get_field_zv (struct buffer *b, bool chars);
extern ptrdiff_t nvim_get_field_begv (struct buffer *b, bool chars);

extern void nvim_buf_memcpy (unsigned char *dst, ptrdiff_t beg, ptrdiff_t size);

// ---- frame (tabpage) --

extern Lisp_Object nvim_frames_list (void);

#endif
