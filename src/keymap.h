#ifndef KEYMAP_H
#define KEYMAP_H

#include "lisp.h"

#define KEYMAPP(m) (!NILP (get_keymap (m, false, false)))

#endif
