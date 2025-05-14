#ifndef EMACS_DISPTAB_H
#define EMACS_DISPTAB_H

#define DISP_TABLE_P(obj)                                                \
  (CHAR_TABLE_P (obj) && EQ (XCHAR_TABLE (obj)->purpose, Qdisplay_table) \
   && CHAR_TABLE_EXTRA_SLOTS (XCHAR_TABLE (obj)) == DISP_TABLE_EXTRA_SLOTS)

#define DISP_TABLE_EXTRA_SLOTS 6

#endif
