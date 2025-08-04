#ifndef EMACS_TERMHOOKS_H
#define EMACS_TERMHOOKS_H

#include "lisp.h"
#include "nvim.h"

enum
{
  up_modifier = 1,
  down_modifier = 2,
  drag_modifier = 4,
  click_modifier = 8,
  double_modifier = 16,
  triple_modifier = 32,
  alt_modifier = CHAR_ALT,
  super_modifier = CHAR_SUPER,
  hyper_modifier = CHAR_HYPER,
  shift_modifier = CHAR_SHIFT,
  ctrl_modifier = CHAR_CTL,
  meta_modifier = CHAR_META
};

struct terminal
{
  union vectorlike_header header;

  Lisp_Object charset_list;

  Lisp_Object param_alist;

  Lisp_Object _last_obj;

  struct coding_system *terminal_coding;
  char *name;
} GCALIGNED_STRUCT;

INLINE bool
TERMINALP (Lisp_Object a)
{
  return PSEUDOVECTORP (a, PVEC_TERMINAL);
}

INLINE struct terminal *
XTERMINAL (Lisp_Object a)
{
  eassert (TERMINALP (a));
  return XUNTAG (a, Lisp_Vectorlike, struct terminal);
}

INLINE void
tset_charset_list (struct terminal *t, Lisp_Object val)
{
  t->charset_list = val;
}

#define TERMINAL_TERMINAL_CODING(d) ((d)->terminal_coding)

#define FRAME_TERMINAL(f) nvim_frame_terminal (f)

extern struct terminal *decode_live_terminal (Lisp_Object);

#endif
