#include "lisp.h"

void
syms_of_xfaces (void)
{
  DEFSYM (Qface, "face");

  DEFSYM (Qface_no_inherit, "face-no-inherit");

  DEFSYM (Qbitmap_spec_p, "bitmap-spec-p");

  DEFSYM (Qframe_set_background_mode, "frame-set-background-mode");

  DEFSYM (QCfamily, ":family");
  DEFSYM (QCheight, ":height");
  DEFSYM (QCweight, ":weight");
  DEFSYM (QCslant, ":slant");
  DEFSYM (QCunderline, ":underline");
  DEFSYM (QCinverse_video, ":inverse-video");
  DEFSYM (QCreverse_video, ":reverse-video");
  DEFSYM (QCforeground, ":foreground");
  DEFSYM (QCbackground, ":background");
  DEFSYM (QCstipple, ":stipple");
  DEFSYM (QCwidth, ":width");
  DEFSYM (QCfont, ":font");
  DEFSYM (QCfontset, ":fontset");
  DEFSYM (QCdistant_foreground, ":distant-foreground");
  DEFSYM (QCbold, ":bold");
  DEFSYM (QCitalic, ":italic");
  DEFSYM (QCoverline, ":overline");
  DEFSYM (QCstrike_through, ":strike-through");
  DEFSYM (QCbox, ":box");
  DEFSYM (QCinherit, ":inherit");
  DEFSYM (QCextend, ":extend");

  DEFSYM (QCcolor, ":color");
  DEFSYM (QCline_width, ":line-width");
  DEFSYM (QCstyle, ":style");
  DEFSYM (QCposition, ":position");
  DEFSYM (Qline, "line");
  DEFSYM (Qwave, "wave");
  DEFSYM (Qdouble_line, "double-line");
  DEFSYM (Qdots, "dots");
  DEFSYM (Qdashes, "dashes");
  DEFSYM (Qreleased_button, "released-button");
  DEFSYM (Qpressed_button, "pressed-button");
  DEFSYM (Qflat_button, "flat-button");
  DEFSYM (Qnormal, "normal");
  DEFSYM (Qthin, "thin");
  DEFSYM (Qextra_light, "extra-light");
  DEFSYM (Qultra_light, "ultra-light");
  DEFSYM (Qlight, "light");
  DEFSYM (Qsemi_light, "semi-light");
  DEFSYM (Qmedium, "medium");
  DEFSYM (Qsemi_bold, "semi-bold");
  DEFSYM (Qbook, "book");
  DEFSYM (Qbold, "bold");
  DEFSYM (Qextra_bold, "extra-bold");
  DEFSYM (Qultra_bold, "ultra-bold");
  DEFSYM (Qheavy, "heavy");
  DEFSYM (Qultra_heavy, "ultra-heavy");
  DEFSYM (Qblack, "black");
  DEFSYM (Qoblique, "oblique");
  DEFSYM (Qitalic, "italic");
  DEFSYM (Qreset, "reset");

  DEFSYM (Qbackground_color, "background-color");
  DEFSYM (Qforeground_color, "foreground-color");

  DEFSYM (Qunspecified, "unspecified");
  DEFSYM (QCignore_defface, ":ignore-defface");

  DEFSYM (QCwindow, ":window");
  DEFSYM (QCfiltered, ":filtered");

  DEFSYM (Qface_alias, "face-alias");

  DEFSYM (Qdefault, "default");
  DEFSYM (Qtool_bar, "tool-bar");
  DEFSYM (Qtab_bar, "tab-bar");
  DEFSYM (Qfringe, "fringe");
  DEFSYM (Qtab_line, "tab-line");
  DEFSYM (Qheader_line, "header-line");
  DEFSYM (Qscroll_bar, "scroll-bar");
  DEFSYM (Qmenu, "menu");
  DEFSYM (Qcursor, "cursor");
  DEFSYM (Qborder, "border");
  DEFSYM (Qmouse, "mouse");
  DEFSYM (Qmode_line_inactive, "mode-line-inactive");
  DEFSYM (Qmode_line_active, "mode-line-active");
  DEFSYM (Qvertical_border, "vertical-border");
  DEFSYM (Qwindow_divider, "window-divider");
  DEFSYM (Qwindow_divider_first_pixel, "window-divider-first-pixel");
  DEFSYM (Qwindow_divider_last_pixel, "window-divider-last-pixel");
  DEFSYM (Qinternal_border, "internal-border");
  DEFSYM (Qchild_frame_border, "child-frame-border");

  DEFSYM (Qtty_color_desc, "tty-color-desc");
  DEFSYM (Qtty_color_standard_values, "tty-color-standard-values");
  DEFSYM (Qtty_color_by_index, "tty-color-by-index");

  DEFSYM (Qtty_color_alist, "tty-color-alist");
  DEFSYM (Qtty_defined_color_alist, "tty-defined-color-alist");
}
