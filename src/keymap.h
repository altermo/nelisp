#ifndef KEYMAP_H
#define KEYMAP_H

#include "lisp.h"

#define KEY_DESCRIPTION_SIZE ((2 * 6) + 1 + (CHARACTERBITS / 3) + 1 + 1)

#define KEYMAPP(m) (!NILP (get_keymap (m, false, false)))

typedef void (*map_keymap_function_t) (Lisp_Object key, Lisp_Object val,
                                       Lisp_Object args, void *data);

void initial_define_lispy_key (Lisp_Object keymap, const char *keyname,
                               const char *defname);

#endif
