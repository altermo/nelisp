#ifndef EMACS_KEYBOARD_H
#define EMACS_KEYBOARD_H

#include "lisp.h"

#define EVENT_HAS_PARAMETERS(event) CONSP (event)

#define EVENT_HEAD(event) \
  (EVENT_HAS_PARAMETERS (event) ? XCAR (event) : (event))

extern bool lucid_event_type_list_p (Lisp_Object);

#endif
