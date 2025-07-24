#include "intervals.h"
#include "lisp.h"

static void
set_interval_left (INTERVAL i, INTERVAL left)
{
  i->left = left;
}

static void
set_interval_right (INTERVAL i, INTERVAL right)
{
  i->right = right;
}

static void
copy_interval_parent (INTERVAL d, INTERVAL s)
{
  d->up = s->up;
  d->up_obj = s->up_obj;
}

INTERVAL
create_root_interval (Lisp_Object parent)
{
  INTERVAL new;

  new = make_interval ();

  if (!STRINGP (parent))
    TODO;
  else
    {
      CHECK_IMPURE (parent, XSTRING (parent));
      new->total_length = SCHARS (parent);
      eassert (TOTAL_LENGTH (new) >= 0);
      set_string_intervals (parent, new);
      new->position = 0;
    }
  eassert (LENGTH (new) > 0);

  set_interval_object (new, parent);

  return new;
}

void
traverse_intervals_noorder (INTERVAL tree, void (*function) (INTERVAL, void *),
                            void *arg)
{
  while (tree)
    {
      (*function) (tree, arg);
      if (!tree->right)
        tree = tree->left;
      else
        {
          traverse_intervals_noorder (tree->left, function, arg);
          tree = tree->right;
        }
    }
}

static INTERVAL
rotate_right (INTERVAL A)
{
  INTERVAL B = A->left;
  INTERVAL c = B->right;
  ptrdiff_t old_total = A->total_length;

  eassert (old_total > 0);
  eassert (LENGTH (A) > 0);
  eassert (LENGTH (B) > 0);

  if (!ROOT_INTERVAL_P (A))
    {
      if (AM_LEFT_CHILD (A))
        set_interval_left (INTERVAL_PARENT (A), B);
      else
        set_interval_right (INTERVAL_PARENT (A), B);
    }
  copy_interval_parent (B, A);

  set_interval_right (B, A);
  set_interval_parent (A, B);

  set_interval_left (A, c);
  if (c)
    set_interval_parent (c, A);

  A->total_length -= TOTAL_LENGTH (B) - TOTAL_LENGTH0 (c);
  eassert (TOTAL_LENGTH (A) > 0);
  eassert (LENGTH (A) > 0);

  B->total_length = old_total;
  eassert (LENGTH (B) > 0);

  return B;
}

static INTERVAL
rotate_left (INTERVAL A)
{
  INTERVAL B = A->right;
  INTERVAL c = B->left;
  ptrdiff_t old_total = A->total_length;

  eassert (old_total > 0);
  eassert (LENGTH (A) > 0);
  eassert (LENGTH (B) > 0);

  if (!ROOT_INTERVAL_P (A))
    {
      if (AM_LEFT_CHILD (A))
        set_interval_left (INTERVAL_PARENT (A), B);
      else
        set_interval_right (INTERVAL_PARENT (A), B);
    }
  copy_interval_parent (B, A);

  set_interval_left (B, A);
  set_interval_parent (A, B);

  set_interval_right (A, c);
  if (c)
    set_interval_parent (c, A);

  A->total_length -= TOTAL_LENGTH (B) - TOTAL_LENGTH0 (c);
  eassert (TOTAL_LENGTH (A) > 0);
  eassert (LENGTH (A) > 0);

  B->total_length = old_total;
  eassert (LENGTH (B) > 0);

  return B;
}

static INTERVAL
balance_an_interval (INTERVAL i)
{
  register ptrdiff_t old_diff, new_diff;

  eassert (LENGTH (i) > 0);
  eassert (TOTAL_LENGTH (i) >= LENGTH (i));

  while (1)
    {
      old_diff = LEFT_TOTAL_LENGTH (i) - RIGHT_TOTAL_LENGTH (i);
      if (old_diff > 0)
        {
          new_diff = i->total_length - i->left->total_length
                     + RIGHT_TOTAL_LENGTH (i->left)
                     - LEFT_TOTAL_LENGTH (i->left);
          if (eabs (new_diff) >= old_diff)
            break;
          i = rotate_right (i);
          balance_an_interval (i->right);
        }
      else if (old_diff < 0)
        {
          new_diff = i->total_length - i->right->total_length
                     + LEFT_TOTAL_LENGTH (i->right)
                     - RIGHT_TOTAL_LENGTH (i->right);
          if (eabs (new_diff) >= -old_diff)
            break;
          i = rotate_left (i);
          balance_an_interval (i->left);
        }
      else
        break;
    }
  return i;
}

static INTERVAL
balance_possible_root_interval (INTERVAL interval)
{
  Lisp_Object parent;
  bool have_parent = false;

  if (INTERVAL_HAS_OBJECT (interval))
    {
      have_parent = true;
      GET_INTERVAL_OBJECT (parent, interval);
    }
  else if (!INTERVAL_HAS_PARENT (interval))
    return interval;

  interval = balance_an_interval (interval);

  if (have_parent)
    {
      if (BUFFERP (parent))
        TODO; // set_buffer_intervals (XBUFFER (parent), interval);
      else if (STRINGP (parent))
        set_string_intervals (parent, interval);
    }

  return interval;
}

static INTERVAL
balance_intervals_internal (register INTERVAL tree)
{
  if (tree->left)
    balance_intervals_internal (tree->left);
  if (tree->right)
    balance_intervals_internal (tree->right);
  return balance_an_interval (tree);
}

INTERVAL
balance_intervals (INTERVAL tree)
{
  return tree ? balance_intervals_internal (tree) : NULL;
}

INTERVAL
find_interval (register INTERVAL tree, register ptrdiff_t position)
{
  register ptrdiff_t relative_position;

  if (!tree)
    return NULL;

  relative_position = position;
  if (INTERVAL_HAS_OBJECT (tree))
    {
      Lisp_Object parent;
      GET_INTERVAL_OBJECT (parent, tree);
      if (BUFFERP (parent))
        TODO; // relative_position -= BUF_BEG (XBUFFER (parent));
    }

  eassert (relative_position <= TOTAL_LENGTH (tree));

  tree = balance_possible_root_interval (tree);

  while (1)
    {
      eassert (tree);
      if (relative_position < LEFT_TOTAL_LENGTH (tree))
        {
          tree = tree->left;
        }
      else if (!NULL_RIGHT_CHILD (tree)
               && relative_position
                    >= (TOTAL_LENGTH (tree) - RIGHT_TOTAL_LENGTH (tree)))
        {
          relative_position
            -= (TOTAL_LENGTH (tree) - RIGHT_TOTAL_LENGTH (tree));
          tree = tree->right;
        }
      else
        {
          tree->position
            = (position - relative_position + LEFT_TOTAL_LENGTH (tree));

          return tree;
        }
    }
}

INTERVAL
next_interval (register INTERVAL interval)
{
  register INTERVAL i = interval;
  register ptrdiff_t next_position;

  if (!i)
    return NULL;
  next_position = interval->position + LENGTH (interval);

  if (!NULL_RIGHT_CHILD (i))
    {
      i = i->right;
      while (!NULL_LEFT_CHILD (i))
        i = i->left;

      i->position = next_position;
      return i;
    }

  while (!NULL_PARENT (i))
    {
      if (AM_LEFT_CHILD (i))
        {
          i = INTERVAL_PARENT (i);
          i->position = next_position;
          return i;
        }

      i = INTERVAL_PARENT (i);
    }

  return NULL;
}

INTERVAL
split_interval_right (INTERVAL interval, ptrdiff_t offset)
{
  INTERVAL new = make_interval ();
  ptrdiff_t position = interval->position;
  ptrdiff_t new_length = LENGTH (interval) - offset;

  new->position = position + offset;
  set_interval_parent (new, interval);

  if (NULL_RIGHT_CHILD (interval))
    {
      set_interval_right (interval, new);
      new->total_length = new_length;
      eassert (LENGTH (new) > 0);
    }
  else
    {
      set_interval_right (new, interval->right);
      set_interval_parent (interval->right, new);
      set_interval_right (interval, new);
      new->total_length = new_length + new->right->total_length;
      balance_an_interval (new);
    }

  balance_possible_root_interval (interval);

  return new;
}

INTERVAL
split_interval_left (INTERVAL interval, ptrdiff_t offset)
{
  INTERVAL new = make_interval ();
  ptrdiff_t new_length = offset;

  new->position = interval->position;
  interval->position = interval->position + offset;
  set_interval_parent (new, interval);

  if (NULL_LEFT_CHILD (interval))
    {
      set_interval_left (interval, new);
      new->total_length = new_length;
      eassert (LENGTH (new) > 0);
    }
  else
    {
      set_interval_left (new, interval->left);
      set_interval_parent (new->left, new);
      set_interval_left (interval, new);
      new->total_length = new_length + new->left->total_length;
      balance_an_interval (new);
    }

  balance_possible_root_interval (interval);

  return new;
}

void
copy_properties (INTERVAL source, INTERVAL target)
{
  if (DEFAULT_INTERVAL_P (source) && DEFAULT_INTERVAL_P (target))
    return;

  COPY_INTERVAL_CACHE (source, target);
  set_interval_plist (target, Fcopy_sequence (source->plist));
}

static INTERVAL
delete_node (register INTERVAL i)
{
  register INTERVAL migrate, this;
  register ptrdiff_t migrate_amt;

  if (!i->left)
    return i->right;
  if (!i->right)
    return i->left;

  migrate = i->left;
  migrate_amt = i->left->total_length;
  this = i->right;
  this->total_length += migrate_amt;
  while (this->left)
    {
      this = this->left;
      this->total_length += migrate_amt;
    }
  set_interval_left (this, migrate);
  set_interval_parent (migrate, this);
  eassert (LENGTH (this) > 0);
  eassert (LENGTH (i->right) > 0);

  return i->right;
}

static void
delete_interval (register INTERVAL i)
{
  register INTERVAL parent;
  ptrdiff_t amt = LENGTH (i);

  eassert (amt <= 0);

  if (ROOT_INTERVAL_P (i))
    {
      Lisp_Object owner;
      GET_INTERVAL_OBJECT (owner, i);
      parent = delete_node (i);
      if (parent)
        set_interval_object (parent, owner);

      if (BUFFERP (owner))
        TODO; // set_buffer_intervals (XBUFFER (owner), parent);
      else if (STRINGP (owner))
        set_string_intervals (owner, parent);
      else
        emacs_abort ();

      return;
    }

  parent = INTERVAL_PARENT (i);
  if (AM_LEFT_CHILD (i))
    {
      set_interval_left (parent, delete_node (i));
      if (parent->left)
        set_interval_parent (parent->left, parent);
    }
  else
    {
      set_interval_right (parent, delete_node (i));
      if (parent->right)
        set_interval_parent (parent->right, parent);
    }
}

INTERVAL
merge_interval_left (register INTERVAL i)
{
  register ptrdiff_t absorb = LENGTH (i);
  register INTERVAL predecessor;

  if (!NULL_LEFT_CHILD (i))
    {
      predecessor = i->left;
      while (!NULL_RIGHT_CHILD (predecessor))
        {
          predecessor->total_length += absorb;
          eassert (LENGTH (predecessor) > 0);
          predecessor = predecessor->right;
        }

      predecessor->total_length += absorb;
      eassert (LENGTH (predecessor) > 0);
      delete_interval (i);
      return predecessor;
    }

  i->total_length -= absorb;
  eassert (TOTAL_LENGTH (i) >= 0);

  predecessor = i;
  while (!NULL_PARENT (predecessor))
    {
      if (AM_RIGHT_CHILD (predecessor))
        {
          predecessor = INTERVAL_PARENT (predecessor);
          delete_interval (i);
          return predecessor;
        }

      predecessor = INTERVAL_PARENT (predecessor);
      predecessor->total_length -= absorb;
      eassert (LENGTH (predecessor) > 0);
    }

  emacs_abort ();
}
