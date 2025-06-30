#ifndef EMACS_FRAME_H
#define EMACS_FRAME_H

#include "lisp.h"
#include "nvim.h"

struct frame
{
  union vectorlike_header header;

  Lisp_Object face_hash_table;

  Lisp_Object param_alist;

  Lisp_Object _last_obj;

  long tabpageid;
};

#define XFRAME(p) \
  (eassert (FRAMEP (p)), XUNTAG (p, Lisp_Vectorlike, struct frame))
#define XSETFRAME(a, b) XSETPSEUDOVECTOR (a, b, PVEC_FRAME)

#define FRAME_W32_P(f) false
#define FRAME_MSDOS_P(f) false
#define FRAME_WINDOW_P(f) false

#define FRAME_LIVE_P(f) nvim_frame_is_valid (f)

#define FRAME_COLS(f) nvim_frame_cols (f)
#define FRAME_LINES(f) nvim_frame_lines (f)

#define FRAME_MENU_BAR_LINES(f) nvim_frame_menu_bar_lines (f)
#define FRAME_TAB_BAR_LINES(f) nvim_frame_tab_bar_lines (f)

#define FRAME_WANTS_MODELINE_P(f) nvim_frame_wants_modeline_p (f)
#define FRAME_NO_SPLIT_P(f) nvim_frame_no_split_p (f)

#if TODO_NELISP_LATER_ELSE
# define FRAME_FOREGROUND_PIXEL(f) (int) FACE_TTY_DEFAULT_FG_COLOR
# define FRAME_BACKGROUND_PIXEL(f) (int) FACE_TTY_DEFAULT_BG_COLOR
#endif

#define CHECK_FRAME(x) CHECK_TYPE (FRAMEP (x), Qframep, x)

#define CHECK_LIVE_FRAME(x) \
  CHECK_TYPE (FRAMEP (x) && FRAME_LIVE_P (XFRAME (x)), Qframe_live_p, x)

#define selected_frame nvim_get_current_frame ()

#endif
