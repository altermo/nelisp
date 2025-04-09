#ifndef EMACS_TERMHOOKS_H
#define EMACS_TERMHOOKS_H

#include "lisp.h"

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

#endif
