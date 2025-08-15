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

#define Xmeta_buffer_vars \
  X (read_only_)          \
  X (undo_list_)

struct buffer
{
  union vectorlike_header header;

#define X(field) Lisp_Object field;
  Xbuffer_vars
#undef X

    Lisp_Object last_obj_;

  long bufid;
};

struct meta_buffer
{
  struct buffer b;

#define X(field) Lisp_Object field;
  Xmeta_buffer_vars
#undef X
};

enum nvim_buffer_var_field
{
  NVIM_BUFFER_VAR_name_,
  NVIM_BUFFER_VAR_filename_,
#define X(field) NVIM_BUFFER_VAR_##field,
  Xbuffer_vars
#undef X
#define X(field) NVIM_BUFFER_VAR_##field,
    Xmeta_buffer_vars
#undef X
};

extern Lisp_Object nvim_name_to_bufobj (Lisp_Object);
extern Lisp_Object nvim_create_buf (Lisp_Object, Lisp_Object);
extern void nvim_set_buffer (struct buffer *);
extern struct buffer *nvim_current_buffer (void);
extern Lisp_Object nvim_buffer_name (struct buffer *);
extern Lisp_Object nvim_buffer_list (void);
extern Lisp_Object nvim_buffer_filename (struct buffer *);
extern bool nvim_buffer_kill (struct buffer *);

extern ptrdiff_t nvim_get_field_zv (struct buffer *b, bool chars);
extern ptrdiff_t nvim_get_field_begv (struct buffer *b, bool chars);
extern ptrdiff_t nvim_get_field_pt (struct buffer *b, bool chars);

extern void nvim_set_point (ptrdiff_t);

extern bool nvim_buffer_option_is_true (struct buffer *b, const char[]);
extern void nvim_buffer_set_bool_option (struct buffer *b, const char[],
                                         bool value);

extern void nvim_buffer_set_undo_list (struct buffer *b, Lisp_Object value);
extern Lisp_Object nvim_buffer_undo_list (struct buffer *b);

extern void nvim_buf_memcpy (unsigned char *dst, ptrdiff_t beg, ptrdiff_t size);
extern void nvim_buf_insert (const char *string, ptrdiff_t nbytes);

static inline Lisp_Object __attribute__ ((always_inline))
nvim_bvar (struct buffer *b, enum nvim_buffer_var_field field)
{
  switch (field)
    {
    case NVIM_BUFFER_VAR_name_:
      return nvim_buffer_name (b);
    case NVIM_BUFFER_VAR_read_only_:
      return nvim_buffer_option_is_true (b, "modifiable") ? Qnil : Qt;
    case NVIM_BUFFER_VAR_filename_:
      return nvim_buffer_filename (b);
    case NVIM_BUFFER_VAR_undo_list_:
      return nvim_buffer_undo_list (b);
#define X(field)              \
case NVIM_BUFFER_VAR_##field: \
  return b->field;
      Xbuffer_vars
#undef X
        default : emacs_abort ();
    }
}

static inline Lisp_Object __attribute__ ((always_inline))
nvim_mbvar (struct buffer *b, int offset)
{
  eassert (offset >= sizeof (struct buffer));
  eassert (offset < sizeof (struct meta_buffer));
  switch (offset)
    {
#define X(field)                           \
case offsetof (struct meta_buffer, field): \
  return nvim_bvar (b, NVIM_BUFFER_VAR_##field);
      Xmeta_buffer_vars;
#undef X
    default:
      emacs_abort ();
    }
}

static inline void __attribute__ ((always_inline))
nvim_bvar_set (struct buffer *b, enum nvim_buffer_var_field field,
               Lisp_Object value)
{
  switch (field)
    {
    case NVIM_BUFFER_VAR_undo_list_:
      nvim_buffer_set_undo_list (b, value);
      break;
    default:
      emacs_abort ();
    }
}

static inline void __attribute__ ((always_inline))
nvim_mbvar_set (struct buffer *b, int offset, Lisp_Object value)
{
  eassert (offset >= sizeof (struct buffer));
  eassert (offset < sizeof (struct meta_buffer));
  switch (offset)
    {
#define X(field)                                     \
case offsetof (struct meta_buffer, field):           \
  nvim_bvar_set (b, NVIM_BUFFER_VAR_##field, value); \
  break;
      Xmeta_buffer_vars;
#undef X
    default:
      emacs_abort ();
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
extern struct terminal *nvim_frame_terminal (struct frame *);

// ---- window --

extern Lisp_Object nvim_get_current_window (void);
extern Lisp_Object nvim_window_content (struct window *);

// ---- marker (extmark) --
extern void nvim_mark_set_all (struct Lisp_Marker *, struct buffer *, ptrdiff_t,
                               ptrdiff_t, bool);

#endif
