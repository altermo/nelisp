#include "frame.h"
#include "lisp.h"
#include "dispextern.h"
#include "nvim.h"

struct frame *
decode_any_frame (register Lisp_Object frame)
{
  if (NILP (frame))
    frame = selected_frame;
  CHECK_FRAME (frame);
  return XFRAME (frame);
}

char unspecified_fg[] = "unspecified-fg", unspecified_bg[] = "unspecified-bg";

Lisp_Object
tty_color_name (struct frame *f, int idx)
{
  if (idx >= 0 && !NILP (Ffboundp (Qtty_color_by_index)))
    {
      Lisp_Object frame;
      Lisp_Object coldesc;

      XSETFRAME (frame, f);
      coldesc = call2 (Qtty_color_by_index, make_fixnum (idx), frame);

      if (!NILP (coldesc))
        return XCAR (coldesc);
    }

  if (idx == FACE_TTY_DEFAULT_FG_COLOR)
    return build_string (unspecified_fg);
  if (idx == FACE_TTY_DEFAULT_BG_COLOR)
    return build_string (unspecified_bg);

  return Qunspecified;
}

void
store_in_alist (Lisp_Object *alistptr, Lisp_Object prop, Lisp_Object val)
{
  Lisp_Object tem = Fassq (prop, *alistptr);
  if (NILP (tem))
    *alistptr = Fcons (Fcons (prop, val), *alistptr);
  else
    Fsetcdr (tem, val);
}

DEFUN ("framep", Fframep, Sframep, 1, 1, 0,
       doc: /* Return non-nil if OBJECT is a frame.
Value is:
  t for a termcap frame (a character-only terminal),
 `x' for an Emacs frame that is really an X window,
 `w32' for an Emacs frame that is a window on MS-Windows display,
 `ns' for an Emacs frame on a GNUstep or Macintosh Cocoa display,
 `pc' for a direct-write MS-DOS frame,
 `pgtk' for an Emacs frame running on pure GTK.
 `haiku' for an Emacs frame running in Haiku.
 `android' for an Emacs frame running in Android.
See also `frame-live-p'.  */)
(Lisp_Object object)
{
  TODO_NELISP_LATER;
  if (!FRAMEP (object))
    return Qnil;
  return Qt;
}

DEFUN ("frame-list", Fframe_list, Sframe_list,
       0, 0, 0,
       doc: /* Return a list of all live frames.
The return value does not include any tooltip frame.  */)
(void)
{
  Lisp_Object list = Qnil;
  return nvim_frames_list ();
}

DEFUN ("frame-parameters", Fframe_parameters, Sframe_parameters, 0, 1, 0,
       doc: /* Return the parameters-alist of frame FRAME.
It is a list of elements of the form (PARM . VALUE), where PARM is a symbol.
The meaningful PARMs depend on the kind of frame.
If FRAME is omitted or nil, return information on the currently selected frame.  */)
(Lisp_Object frame)
{
  Lisp_Object alist;
  struct frame *f = decode_any_frame (frame);
  int height, width;

  if (!FRAME_LIVE_P (f))
    return Qnil;

  alist = Fcopy_alist (f->param_alist);

  if (!FRAME_WINDOW_P (f))
    {
      Lisp_Object elt;

      elt = Fassq (Qforeground_color, alist);
      if (CONSP (elt) && STRINGP (XCDR (elt)))
        TODO;
      else
        store_in_alist (&alist, Qforeground_color,
                        tty_color_name (f, FRAME_FOREGROUND_PIXEL (f)));
      elt = Fassq (Qbackground_color, alist);
      if (CONSP (elt) && STRINGP (XCDR (elt)))
        TODO;
      else
        store_in_alist (&alist, Qbackground_color,
                        tty_color_name (f, FRAME_BACKGROUND_PIXEL (f)));
      store_in_alist (&alist, Qfont,
                      build_string (FRAME_MSDOS_P (f) ? "ms-dos"
                                    : FRAME_W32_P (f) ? "w32term"
                                                      : "tty"));
    }

  store_in_alist (&alist, Qname, nvim_frame_name (f));

#if TODO_NELISP_LATER_ELSE
  height = (FRAME_LINES (f));
  store_in_alist (&alist, Qheight, make_fixnum (height));
  width = (FRAME_COLS (f));
  store_in_alist (&alist, Qwidth, make_fixnum (width));
#endif

  store_in_alist (&alist, Qmodeline, FRAME_WANTS_MODELINE_P (f) ? Qt : Qnil);
  store_in_alist (&alist, Qunsplittable, FRAME_NO_SPLIT_P (f) ? Qt : Qnil);
  store_in_alist (&alist, Qbuffer_list, nvim_frame_buffer_list (f));
  store_in_alist (&alist, Qburied_buffer_list,
                  nvim_frame_buried_buffer_list (f));

  {
    Lisp_Object lines;

    XSETFASTINT (lines, FRAME_MENU_BAR_LINES (f));
    store_in_alist (&alist, Qmenu_bar_lines, lines);
    XSETFASTINT (lines, FRAME_TAB_BAR_LINES (f));
    store_in_alist (&alist, Qtab_bar_lines, lines);
  }

  return alist;
}

DEFUN ("frame-parameter", Fframe_parameter, Sframe_parameter, 2, 2, 0,
       doc: /* Return FRAME's value for parameter PARAMETER.
If FRAME is nil, describe the currently selected frame.  */)
(Lisp_Object frame, Lisp_Object parameter)
{
  struct frame *f = decode_any_frame (frame);
  Lisp_Object value = Qnil;

  CHECK_SYMBOL (parameter);

  XSETFRAME (frame, f);

  if (FRAME_LIVE_P (f))
    {
      if (EQ (parameter, Qname))
        return nvim_frame_name (f);
      else if (EQ (parameter, Qbackground_color)
               || EQ (parameter, Qforeground_color))
        {
          value = Fassq (parameter, f->param_alist);
          if (CONSP (value))
            {
              value = XCDR (value);

              if (STRINGP (value) && !FRAME_WINDOW_P (f))
                {
                  TODO; // Lisp_Object tem = frame_unspecified_color (f, value);

                  // if (!NILP (tem))
                  //   value = tem;
                }
            }
          else
            value = Fcdr (Fassq (parameter, Fframe_parameters (frame)));
        }
      else if (EQ (parameter, Qdisplay_type)
               || EQ (parameter, Qbackground_mode))
        value = Fcdr (Fassq (parameter, f->param_alist));
      else
        value = Fcdr (Fassq (parameter, Fframe_parameters (frame)));
    }

  return value;
}

void
syms_of_frame (void)
{
  DEFSYM (Qframep, "framep");
  DEFSYM (Qframe_live_p, "frame-live-p");
  DEFSYM (Qframe_windows_min_size, "frame-windows-min-size");
  DEFSYM (Qframe_monitor_attributes, "frame-monitor-attributes");
  DEFSYM (Qwindow__pixel_to_total, "window--pixel-to-total");
  DEFSYM (Qmake_initial_minibuffer_frame, "make-initial-minibuffer-frame");
  DEFSYM (Qexplicit_name, "explicit-name");
  DEFSYM (Qheight, "height");
  DEFSYM (Qicon, "icon");
  DEFSYM (Qminibuffer, "minibuffer");
  DEFSYM (Qundecorated, "undecorated");
  DEFSYM (Qno_special_glyphs, "no-special-glyphs");
  DEFSYM (Qparent_frame, "parent-frame");
  DEFSYM (Qskip_taskbar, "skip-taskbar");
  DEFSYM (Qno_focus_on_map, "no-focus-on-map");
  DEFSYM (Qno_accept_focus, "no-accept-focus");
  DEFSYM (Qz_group, "z-group");
  DEFSYM (Qoverride_redirect, "override-redirect");
  DEFSYM (Qdelete_before, "delete-before");
  DEFSYM (Qmodeline, "modeline");
  DEFSYM (Qonly, "only");
  DEFSYM (Qnone, "none");
  DEFSYM (Qwidth, "width");
  DEFSYM (Qtext_pixels, "text-pixels");
  DEFSYM (Qgeometry, "geometry");
  DEFSYM (Qicon_left, "icon-left");
  DEFSYM (Qicon_top, "icon-top");
  DEFSYM (Qtooltip, "tooltip");
  DEFSYM (Quser_position, "user-position");
  DEFSYM (Quser_size, "user-size");
  DEFSYM (Qwindow_id, "window-id");

  DEFSYM (Qparent_id, "parent-id");
  DEFSYM (Qx, "x");
  DEFSYM (Qw32, "w32");
  DEFSYM (Qpc, "pc");
  DEFSYM (Qns, "ns");
  DEFSYM (Qpgtk, "pgtk");
  DEFSYM (Qhaiku, "haiku");
  DEFSYM (Qandroid, "android");
  DEFSYM (Qvisible, "visible");
  DEFSYM (Qbuffer_predicate, "buffer-predicate");
  DEFSYM (Qbuffer_list, "buffer-list");
  DEFSYM (Qburied_buffer_list, "buried-buffer-list");
  DEFSYM (Qdisplay_type, "display-type");
  DEFSYM (Qbackground_mode, "background-mode");
  DEFSYM (Qnoelisp, "noelisp");
  DEFSYM (Qtty_color_mode, "tty-color-mode");
  DEFSYM (Qtty, "tty");
  DEFSYM (Qtty_type, "tty-type");

  DEFSYM (Qface_set_after_frame_default, "face-set-after-frame-default");

  DEFSYM (Qfullwidth, "fullwidth");
  DEFSYM (Qfullheight, "fullheight");
  DEFSYM (Qfullboth, "fullboth");
  DEFSYM (Qmaximized, "maximized");
  DEFSYM (Qshaded, "shaded");
  DEFSYM (Qx_resource_name, "x-resource-name");
  DEFSYM (Qx_frame_parameter, "x-frame-parameter");

  DEFSYM (Qworkarea, "workarea");
  DEFSYM (Qmm_size, "mm-size");

  DEFSYM (Qframes, "frames");
  DEFSYM (Qsource, "source");

  DEFSYM (Qframe_edges, "frame-edges");
  DEFSYM (Qouter_edges, "outer-edges");
  DEFSYM (Qouter_position, "outer-position");
  DEFSYM (Qouter_size, "outer-size");
  DEFSYM (Qnative_edges, "native-edges");
  DEFSYM (Qinner_edges, "inner-edges");
  DEFSYM (Qexternal_border_size, "external-border-size");
  DEFSYM (Qtitle_bar_size, "title-bar-size");
  DEFSYM (Qmenu_bar_external, "menu-bar-external");
  DEFSYM (Qmenu_bar_size, "menu-bar-size");
  DEFSYM (Qtab_bar_size, "tab-bar-size");
  DEFSYM (Qtool_bar_external, "tool-bar-external");
  DEFSYM (Qtool_bar_size, "tool-bar-size");

  DEFSYM (Qx_set_menu_bar_lines, "x_set_menu_bar_lines");
  DEFSYM (Qchange_frame_size, "change_frame_size");
  DEFSYM (Qxg_frame_set_char_size, "xg_frame_set_char_size");
  DEFSYM (Qx_set_window_size_1, "x_set_window_size_1");
  DEFSYM (Qset_window_configuration, "set_window_configuration");
  DEFSYM (Qx_create_frame_1, "x_create_frame_1");
  DEFSYM (Qx_create_frame_2, "x_create_frame_2");
  DEFSYM (Qgui_figure_window_size, "gui_figure_window_size");
  DEFSYM (Qtip_frame, "tip_frame");
  DEFSYM (Qterminal_frame, "terminal_frame");

  DEFSYM (Qalpha, "alpha");
  DEFSYM (Qalpha_background, "alpha-background");
  DEFSYM (Qauto_lower, "auto-lower");
  DEFSYM (Qauto_raise, "auto-raise");
  DEFSYM (Qborder_color, "border-color");
  DEFSYM (Qborder_width, "border-width");
  DEFSYM (Qouter_border_width, "outer-border-width");
  DEFSYM (Qbottom_divider_width, "bottom-divider-width");
  DEFSYM (Qcursor_color, "cursor-color");
  DEFSYM (Qcursor_type, "cursor-type");
  DEFSYM (Qfont_backend, "font-backend");
  DEFSYM (Qfullscreen, "fullscreen");
  DEFSYM (Qhorizontal_scroll_bars, "horizontal-scroll-bars");
  DEFSYM (Qicon_name, "icon-name");
  DEFSYM (Qicon_type, "icon-type");
  DEFSYM (Qchild_frame_border_width, "child-frame-border-width");
  DEFSYM (Qinternal_border_width, "internal-border-width");
  DEFSYM (Qleft_fringe, "left-fringe");
  DEFSYM (Qleft_fringe_help, "left-fringe-help");
  DEFSYM (Qline_spacing, "line-spacing");
  DEFSYM (Qmenu_bar_lines, "menu-bar-lines");
  DEFSYM (Qtab_bar_lines, "tab-bar-lines");
  DEFSYM (Qmouse_color, "mouse-color");
  DEFSYM (Qname, "name");
  DEFSYM (Qright_divider_width, "right-divider-width");
  DEFSYM (Qright_fringe, "right-fringe");
  DEFSYM (Qright_fringe_help, "right-fringe-help");
  DEFSYM (Qscreen_gamma, "screen-gamma");
  DEFSYM (Qscroll_bar_background, "scroll-bar-background");
  DEFSYM (Qscroll_bar_foreground, "scroll-bar-foreground");
  DEFSYM (Qscroll_bar_height, "scroll-bar-height");
  DEFSYM (Qscroll_bar_width, "scroll-bar-width");
  DEFSYM (Qsticky, "sticky");
  DEFSYM (Qtitle, "title");
  DEFSYM (Qtool_bar_lines, "tool-bar-lines");
  DEFSYM (Qtool_bar_position, "tool-bar-position");
  DEFSYM (Qunsplittable, "unsplittable");
  DEFSYM (Qvertical_scroll_bars, "vertical-scroll-bars");
  DEFSYM (Qvisibility, "visibility");
  DEFSYM (Qwait_for_wm, "wait-for-wm");
  DEFSYM (Qinhibit_double_buffering, "inhibit-double-buffering");
  DEFSYM (Qno_other_frame, "no-other-frame");
  DEFSYM (Qbelow, "below");
  DEFSYM (Qabove_suspended, "above-suspended");
  DEFSYM (Qmin_width, "min-width");
  DEFSYM (Qmin_height, "min-height");
  DEFSYM (Qmouse_wheel_frame, "mouse-wheel-frame");
  DEFSYM (Qkeep_ratio, "keep-ratio");
  DEFSYM (Qwidth_only, "width-only");
  DEFSYM (Qheight_only, "height-only");
  DEFSYM (Qleft_only, "left-only");
  DEFSYM (Qtop_only, "top-only");
  DEFSYM (Qiconify_top_level, "iconify-top-level");
  DEFSYM (Qmake_invisible, "make-invisible");
  DEFSYM (Quse_frame_synchronization, "use-frame-synchronization");
  DEFSYM (Qfont_parameter, "font-parameter");

  defsubr (&Sframep);
  defsubr (&Sframe_list);
  defsubr (&Sframe_parameters);
  defsubr (&Sframe_parameter);
}
