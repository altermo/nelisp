#ifndef EMACS_KEYBOARD_H
#define EMACS_KEYBOARD_H

#define EVENT_HAS_PARAMETERS(event) CONSP (event)

#define EVENT_HEAD(event) \
  (EVENT_HAS_PARAMETERS (event) ? XCAR (event) : (event))

#endif
