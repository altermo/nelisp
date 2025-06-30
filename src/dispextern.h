#ifndef DISPEXTERN_H_INCLUDED
#define DISPEXTERN_H_INCLUDED

#include "lisp.h"

typedef struct
{
  unsigned long pixel;
  unsigned short red, green, blue;
} Emacs_Color;

#define FACE_ID_BITS 20

enum lface_attribute_index
{
  LFACE_FAMILY_INDEX = 1,
  LFACE_FOUNDRY_INDEX,
  LFACE_SWIDTH_INDEX,
  LFACE_HEIGHT_INDEX,
  LFACE_WEIGHT_INDEX,
  LFACE_SLANT_INDEX,
  LFACE_UNDERLINE_INDEX,
  LFACE_INVERSE_INDEX,
  LFACE_FOREGROUND_INDEX,
  LFACE_BACKGROUND_INDEX,
  LFACE_STIPPLE_INDEX,
  LFACE_OVERLINE_INDEX,
  LFACE_STRIKE_THROUGH_INDEX,
  LFACE_BOX_INDEX,
  LFACE_FONT_INDEX,
  LFACE_INHERIT_INDEX,
  LFACE_FONTSET_INDEX,
  LFACE_DISTANT_FOREGROUND_INDEX,
  LFACE_EXTEND_INDEX,
  LFACE_VECTOR_SIZE
};

enum face_box_type
{
  FACE_NO_BOX,

  FACE_SIMPLE_BOX,

  FACE_RAISED_BOX,
  FACE_SUNKEN_BOX
};

enum face_underline_type
{
  FACE_NO_UNDERLINE = 0,
  FACE_UNDERLINE_SINGLE,
  FACE_UNDERLINE_DOUBLE_LINE,
  FACE_UNDERLINE_WAVE,
  FACE_UNDERLINE_DOTS,
  FACE_UNDERLINE_DASHES,
};

struct face
{
  Lisp_Object lface[LFACE_VECTOR_SIZE];

  int id;

  unsigned long foreground;

  unsigned long background;

  unsigned long underline_color;
  unsigned long overline_color;
  unsigned long strike_through_color;
  unsigned long box_color;

  struct font *font;

  int fontset;

  int box_vertical_line_width;
  int box_horizontal_line_width;

  int underline_pixels_above_descent_line;

  ENUM_BF (face_box_type) box : 2;

  ENUM_BF (face_underline_type) underline : 3;

  bool_bf use_box_color_for_shadows_p : 1;

  bool_bf overline_p : 1;
  bool_bf strike_through_p : 1;

  bool_bf foreground_defaulted_p : 1;
  bool_bf background_defaulted_p : 1;

  bool_bf underline_defaulted_p : 1;
  bool_bf overline_color_defaulted_p : 1;
  bool_bf strike_through_color_defaulted_p : 1;
  bool_bf box_color_defaulted_p : 1;

  bool_bf underline_at_descent_line_p : 1;

  bool_bf tty_bold_p : 1;
  bool_bf tty_italic_p : 1;
  bool_bf tty_reverse_p : 1;
  bool_bf tty_strike_through_p : 1;

  bool_bf colors_copied_bitwise_p : 1;

  bool_bf overstrike : 1;

  uintptr_t hash;

  struct face *next, *prev;

  struct face *ascii_face;
};

#define FACE_TTY_DEFAULT_COLOR ((unsigned long) -1)

#define FACE_TTY_DEFAULT_FG_COLOR ((unsigned long) -2)

#define FACE_TTY_DEFAULT_BG_COLOR ((unsigned long) -3)

enum face_id
{
  DEFAULT_FACE_ID,
  MODE_LINE_ACTIVE_FACE_ID,
  MODE_LINE_INACTIVE_FACE_ID,
  TOOL_BAR_FACE_ID,
  FRINGE_FACE_ID,
  HEADER_LINE_FACE_ID,
  SCROLL_BAR_FACE_ID,
  BORDER_FACE_ID,
  CURSOR_FACE_ID,
  MOUSE_FACE_ID,
  MENU_FACE_ID,
  VERTICAL_BORDER_FACE_ID,
  WINDOW_DIVIDER_FACE_ID,
  WINDOW_DIVIDER_FIRST_PIXEL_FACE_ID,
  WINDOW_DIVIDER_LAST_PIXEL_FACE_ID,
  INTERNAL_BORDER_FACE_ID,
  CHILD_FRAME_BORDER_FACE_ID,
  TAB_BAR_FACE_ID,
  TAB_LINE_FACE_ID,
  BASIC_FACE_ID_SENTINEL
};

#define MAX_FACE_ID ((1 << FACE_ID_BITS) - 1)

struct face_cache
{
  struct face **buckets;

  struct frame *f;

  struct face **faces_by_id;

  ptrdiff_t size;
  int used;

#if TODO_NELISP_LATER_AND
  bool_bf menu_face_changed_p : 1;
#endif
};

void init_frame_faces (struct frame *);
extern char unspecified_fg[], unspecified_bg[];

#define TTY_CAP_INVERSE 0x01
#define TTY_CAP_UNDERLINE 0x02
#define TTY_CAP_BOLD 0x04
#define TTY_CAP_DIM 0x08
#define TTY_CAP_ITALIC 0x10
#define TTY_CAP_STRIKE_THROUGH 0x20
#define TTY_CAP_UNDERLINE_STYLED (0x32 & TTY_CAP_UNDERLINE)

#endif
