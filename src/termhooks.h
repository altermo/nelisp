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

  Lisp_Object _last_obj;

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

#define FRAME_TERMINAL(f) nvim_frame_terminal (f)

#endif
