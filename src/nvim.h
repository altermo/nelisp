#ifndef NVIM_H
#define NVIM_H

#include "lisp.h"

enum nvim_buffer_var_field
{
  NVIM_BUFFER_VAR__name,
};

extern Lisp_Object nvim_name_to_bufobj (Lisp_Object);
extern Lisp_Object nvim_create_buf (Lisp_Object, Lisp_Object);
extern Lisp_Object nvim_bvar (struct buffer *, enum nvim_buffer_var_field);
extern void nvim_set_buffer (struct buffer *);
extern struct buffer *nvim_current_buffer (void);

extern ptrdiff_t nvim_get_field_zv (struct buffer *);

#endif
