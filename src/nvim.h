#ifndef NVIM_H
#define NVIM_H

#include "lisp.h"

// --- buffer --

#define Xbuffer_vars               \
  X (local_var_alist_)             \
  X (downcase_table_)              \
  X (upcase_table_)                \
  X (case_canon_table_)            \
  X (case_eqv_table_)              \
  X (syntax_table_)                \
  X (enable_multibyte_characters_) \
  X (category_table_)

struct buffer
{
  union vectorlike_header header;

#define X(field) Lisp_Object field;
  Xbuffer_vars
#undef X

    Lisp_Object last_obj_;

  long bufid;
};

enum nvim_buffer_var_field
{
  NVIM_BUFFER_VAR_name_,
#define X(field) NVIM_BUFFER_VAR_##field,
  Xbuffer_vars
#undef X
};

extern Lisp_Object nvim_name_to_bufobj (Lisp_Object);
extern Lisp_Object nvim_create_buf (Lisp_Object, Lisp_Object);
extern void nvim_set_buffer (struct buffer *);
extern struct buffer *nvim_current_buffer (void);
extern Lisp_Object nvim_buffer_name (struct buffer *);
extern Lisp_Object nvim_buffer_list (void);

extern ptrdiff_t nvim_get_field_zv (struct buffer *b, bool chars);
extern ptrdiff_t nvim_get_field_begv (struct buffer *b, bool chars);
extern ptrdiff_t nvim_get_field_pt (struct buffer *b);

extern void nvim_buf_memcpy (unsigned char *dst, ptrdiff_t beg, ptrdiff_t size);

static inline Lisp_Object __attribute__ ((always_inline))
nvim_bvar (struct buffer *b, enum nvim_buffer_var_field field)
{
  switch (field)
    {
    case NVIM_BUFFER_VAR_name_:
      return nvim_buffer_name (b);
#define X(field)              \
case NVIM_BUFFER_VAR_##field: \
  return b->field;
      Xbuffer_vars
#undef X
        default : emacs_abort ();
    }
}

// ---- frame (tabpage) --

extern Lisp_Object nvim_frame_list (void);
bool nvim_frame_is_valid (struct frame *);
extern Lisp_Object nvim_get_current_frame (void);
extern Lisp_Object nvim_frame_name (struct frame *);
extern int nvim_frame_lines (struct frame *);
extern int nvim_frame_cols (struct frame *);
extern bool nvim_frame_wants_modeline_p (struct frame *);
extern bool nvim_frame_no_split_p (struct frame *);
extern Lisp_Object nvim_frame_buffer_list (struct frame *);
extern Lisp_Object nvim_frame_buried_buffer_list (struct frame *);
extern int nvim_frame_menu_bar_lines (struct frame *);
extern int nvim_frame_tab_bar_lines (struct frame *);

#endif
