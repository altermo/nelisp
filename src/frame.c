#include "lisp.h"
#include "nvim.h"

DEFUN ("frame-list", Fframe_list, Sframe_list,
       0, 0, 0,
       doc: /* Return a list of all live frames.
The return value does not include any tooltip frame.  */)
(void)
{
  Lisp_Object list = Qnil;
  return nvim_frames_list ();
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

  defsubr (&Sframe_list);
}
