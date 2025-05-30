#ifndef EMACS_INTERVALS_H
#define EMACS_INTERVALS_H

#include "lisp.h"
#include "buffer.h"

struct interval
{
  ptrdiff_t total_length;
  ptrdiff_t position;
  struct interval *left;
  struct interval *right;

  union
  {
    struct interval *interval;
    Lisp_Object obj;
  } up;
  bool_bf up_obj : 1;

  bool_bf gcmarkbit : 1;

  bool_bf write_protect : 1;
  bool_bf visible : 1;
  bool_bf front_sticky : 1;
  bool_bf rear_sticky : 1;
  Lisp_Object plist;
};

#define NULL_RIGHT_CHILD(i) ((i)->right == NULL)

#define NULL_LEFT_CHILD(i) ((i)->left == NULL)

#define NULL_PARENT(i) ((i)->up_obj || (i)->up.interval == 0)

#define AM_LEFT_CHILD(i) (!NULL_PARENT (i) && INTERVAL_PARENT (i)->left == (i))

#define ROOT_INTERVAL_P(i) NULL_PARENT (i)

#define TOTAL_LENGTH(i) ((i)->total_length)

#define TOTAL_LENGTH0(i) ((i) ? TOTAL_LENGTH (i) : 0)

#define LENGTH(i) \
  (TOTAL_LENGTH (i) - RIGHT_TOTAL_LENGTH (i) - LEFT_TOTAL_LENGTH (i))

#define LEFT_TOTAL_LENGTH(i) TOTAL_LENGTH0 ((i)->left)

#define RIGHT_TOTAL_LENGTH(i) TOTAL_LENGTH0 ((i)->right)

#define DEFAULT_INTERVAL_P(i) (!i || NILP ((i)->plist))

#define INTERVAL_HAS_PARENT(i) (!(i)->up_obj && (i)->up.interval != 0)
#define INTERVAL_HAS_OBJECT(i) ((i)->up_obj)

INLINE void
set_interval_object (INTERVAL i, Lisp_Object obj)
{
  eassert (BUFFERP (obj) || STRINGP (obj));
  i->up_obj = 1;
  i->up.obj = obj;
}

INLINE void
set_interval_parent (INTERVAL i, INTERVAL parent)
{
  i->up_obj = false;
  i->up.interval = parent;
}

INLINE void
set_interval_plist (INTERVAL i, Lisp_Object plist)
{
  i->plist = plist;
}

#define INTERVAL_PARENT(i) \
  (eassert ((i) != 0 && !(i)->up_obj), (i)->up.interval)

#define GET_INTERVAL_OBJECT(d, s) (eassert ((s)->up_obj), (d) = (s)->up.obj)

#define RESET_INTERVAL(i)                           \
  do                                                \
    {                                               \
      (i)->total_length = (i)->position = 0;        \
      (i)->left = (i)->right = NULL;                \
      set_interval_parent (i, NULL);                \
      (i)->write_protect = false;                   \
      (i)->visible = false;                         \
      (i)->front_sticky = (i)->rear_sticky = false; \
      set_interval_plist (i, Qnil);                 \
    }                                               \
  while (false)

#define COPY_INTERVAL_CACHE(from, to)              \
  do                                               \
    {                                              \
      (to)->write_protect = (from)->write_protect; \
      (to)->visible = (from)->visible;             \
      (to)->front_sticky = (from)->front_sticky;   \
      (to)->rear_sticky = (from)->rear_sticky;     \
    }                                              \
  while (false)

extern INTERVAL next_interval (INTERVAL interval);
extern INTERVAL split_interval_right (INTERVAL interval, ptrdiff_t offset);
extern INTERVAL split_interval_left (INTERVAL interval, ptrdiff_t offset);
extern void copy_properties (INTERVAL source, INTERVAL target);
extern INTERVAL create_root_interval (Lisp_Object parent);
extern INTERVAL find_interval (INTERVAL tree, ptrdiff_t position);

#endif
