#include "lisp.h"
#include "buffer.h"
#include "intervals.h"

enum property_set_type
{
  TEXT_PROPERTY_REPLACE,
  TEXT_PROPERTY_PREPEND,
  TEXT_PROPERTY_APPEND
};

static void
CHECK_STRING_OR_BUFFER (Lisp_Object x)
{
  CHECK_TYPE (STRINGP (x) || BUFFERP (x), Qbuffer_or_string_p, x);
}

enum
{
  soft = false,
  hard = true
};

INTERVAL
validate_interval_range (Lisp_Object object, Lisp_Object *begin,
                         Lisp_Object *end, bool force)
{
  INTERVAL i;
  ptrdiff_t searchpos;
  Lisp_Object begin0 = *begin, end0 = *end;

  CHECK_STRING_OR_BUFFER (object);
  CHECK_FIXNUM_COERCE_MARKER (*begin);
  CHECK_FIXNUM_COERCE_MARKER (*end);

  if (EQ (*begin, *end) && begin != end)
    return NULL;

  if (XFIXNUM (*begin) > XFIXNUM (*end))
    {
      Lisp_Object n;
      n = *begin;
      *begin = *end;
      *end = n;
    }

  if (BUFFERP (object))
    TODO;
  else
    {
      ptrdiff_t len = SCHARS (object);

      if (!(0 <= XFIXNUM (*begin) && XFIXNUM (*begin) <= XFIXNUM (*end)
            && XFIXNUM (*end) <= len))
        args_out_of_range (begin0, end0);
      i = string_intervals (object);

      if (len == 0)
        return NULL;

      searchpos = XFIXNUM (*begin);
    }

  if (!i)
    return (force ? create_root_interval (object) : i);

  return find_interval (i, searchpos);
}

static Lisp_Object
validate_plist (Lisp_Object list)
{
  if (NILP (list))
    return Qnil;

  if (CONSP (list))
    {
      Lisp_Object tail = list;
      do
        {
          tail = XCDR (tail);
          if (!CONSP (tail))
            error ("Odd length text property list");
          tail = XCDR (tail);
          maybe_quit ();
        }
      while (CONSP (tail));

      return list;
    }

  return list2 (list, Qnil);
}

static bool
interval_has_all_properties (Lisp_Object plist, INTERVAL i)
{
  Lisp_Object tail1, tail2;

  for (tail1 = plist; CONSP (tail1); tail1 = Fcdr (XCDR (tail1)))
    {
      Lisp_Object sym1 = XCAR (tail1);
      bool found = false;

      for (tail2 = i->plist; CONSP (tail2); tail2 = Fcdr (XCDR (tail2)))
        if (EQ (sym1, XCAR (tail2)))
          {
            if (!EQ (Fcar (XCDR (tail1)), Fcar (XCDR (tail2))))
              return false;

            found = true;
            break;
          }

      if (!found)
        return false;
    }

  return true;
}

static bool
add_properties (Lisp_Object plist, INTERVAL i, Lisp_Object object,
                enum property_set_type set_type, bool destructive)
{
  Lisp_Object tail1, tail2, sym1, val1;
  bool changed = false;

  tail1 = plist;
  sym1 = Qnil;
  val1 = Qnil;

  for (tail1 = plist; CONSP (tail1); tail1 = Fcdr (XCDR (tail1)))
    {
      bool found = false;
      sym1 = XCAR (tail1);
      val1 = Fcar (XCDR (tail1));

      for (tail2 = i->plist; CONSP (tail2); tail2 = Fcdr (XCDR (tail2)))
        if (EQ (sym1, XCAR (tail2)))
          {
            Lisp_Object this_cdr;

            this_cdr = XCDR (tail2);
            found = true;

            if (EQ (val1, Fcar (this_cdr)))
              break;

            if (BUFFERP (object))
              {
                TODO; // record_property_change (i->position, LENGTH (i),
                      // 			sym1, Fcar (this_cdr), object);
              }

            if (set_type == TEXT_PROPERTY_REPLACE)
              Fsetcar (this_cdr, val1);
            else
              {
                if (CONSP (Fcar (this_cdr))
                    && (!EQ (sym1, Qface)
                        || NILP (Fkeywordp (Fcar (Fcar (this_cdr))))))
                  if (set_type == TEXT_PROPERTY_PREPEND)
                    Fsetcar (this_cdr, Fcons (val1, Fcar (this_cdr)));
                  else
                    {
                      if (destructive)
                        nconc2 (Fcar (this_cdr), list1 (val1));
                      else
                        Fsetcar (this_cdr, CALLN (Fappend, Fcar (this_cdr),
                                                  list1 (val1)));
                    }
                else
                  {
                    if (set_type == TEXT_PROPERTY_PREPEND)
                      Fsetcar (this_cdr, list2 (val1, Fcar (this_cdr)));
                    else
                      Fsetcar (this_cdr, list2 (Fcar (this_cdr), val1));
                  }
              }
            changed = true;
            break;
          }

      if (!found)
        {
          if (BUFFERP (object))
            {
              TODO; // record_property_change (i->position, LENGTH (i),
              //    sym1, Qnil, object);
            }
          set_interval_plist (i, Fcons (sym1, Fcons (val1, i->plist)));
          changed = true;
        }
    }

  return changed;
}

static Lisp_Object
add_text_properties_1 (Lisp_Object start, Lisp_Object end,
                       Lisp_Object properties, Lisp_Object object,
                       enum property_set_type set_type, bool destructive)
{
  if (BUFFERP (object) && XBUFFER (object) != current_buffer)
    TODO;

  INTERVAL i, unchanged;
  ptrdiff_t s, len;
  bool modified = false;
  bool first_time = true;

  properties = validate_plist (properties);
  if (NILP (properties))
    return Qnil;

  if (NILP (object))
    XSETBUFFER (object, current_buffer);

  i = validate_interval_range (object, &start, &end, hard);
  if (!i)
    return Qnil;

  s = XFIXNUM (start);
  len = XFIXNUM (end) - s;

  if (interval_has_all_properties (properties, i))
    {
      ptrdiff_t got = LENGTH (i) - (s - i->position);

      do
        {
          if (got >= len)
            return Qnil;
          len -= got;
          i = next_interval (i);
          got = LENGTH (i);
        }
      while (interval_has_all_properties (properties, i));
    }
  else if (i->position != s)
    {
      unchanged = i;
      i = split_interval_right (unchanged, s - unchanged->position);
      copy_properties (unchanged, i);
    }

  if (BUFFERP (object) && first_time)
    TODO;

  for (;;)
    {
      eassert (i != 0);

      if (LENGTH (i) >= len)
        {
          if (interval_has_all_properties (properties, i))
            {
              if (BUFFERP (object))
                TODO; // signal_after_change (XFIXNUM (start),
              //                      XFIXNUM (end) - XFIXNUM (start),
              //                      XFIXNUM (end) - XFIXNUM (start));

              eassert (modified);
              return Qt;
            }

          if (LENGTH (i) == len)
            {
              add_properties (properties, i, object, set_type, destructive);
              if (BUFFERP (object))
                TODO; // signal_after_change (XFIXNUM (start),
              //                      XFIXNUM (end) - XFIXNUM (start),
              //                      XFIXNUM (end) - XFIXNUM (start));
              return Qt;
            }

          unchanged = i;
          i = split_interval_left (unchanged, len);
          copy_properties (unchanged, i);
          add_properties (properties, i, object, set_type, destructive);
          if (BUFFERP (object))
            TODO; // signal_after_change (XFIXNUM (start),
                  //  XFIXNUM (end) - XFIXNUM (start),
                  //  XFIXNUM (end) - XFIXNUM (start));
          return Qt;
        }

      len -= LENGTH (i);
      modified |= add_properties (properties, i, object, set_type, destructive);
      i = next_interval (i);
    }
}

DEFUN ("add-text-properties", Fadd_text_properties,
       Sadd_text_properties, 3, 4, 0,
       doc: /* Add properties to the text from START to END.
The third argument PROPERTIES is a property list
specifying the property values to add.  If the optional fourth argument
OBJECT is a buffer (or nil, which means the current buffer),
START and END are buffer positions (integers or markers).
If OBJECT is a string, START and END are 0-based indices into it.
Return t if any property value actually changed, nil otherwise.  */)
(Lisp_Object start, Lisp_Object end, Lisp_Object properties, Lisp_Object object)
{
  return add_text_properties_1 (start, end, properties, object,
                                TEXT_PROPERTY_REPLACE, true);
}

void
syms_of_textprop (void)
{
  defsubr (&Sadd_text_properties);
}
