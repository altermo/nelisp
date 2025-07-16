#include "lisp.h"

static Lisp_Object Vccl_program_table;

#define CCL_HEADER_BUF_MAG 0
#define CCL_HEADER_EOF 1
#define CCL_HEADER_MAIN 2

#define ASCENDING_ORDER(lo, med, hi) (((lo) <= (med)) & ((med) <= (hi)))

static Lisp_Object
resolve_symbol_ccl_program (Lisp_Object ccl)
{
  int i, veclen, unresolved = 0;
  Lisp_Object result, contents, val;

  if (!(CCL_HEADER_MAIN < ASIZE (ccl) && ASIZE (ccl) <= INT_MAX))
    return Qnil;
  result = Fcopy_sequence (ccl);
  veclen = ASIZE (result);

  for (i = 0; i < veclen; i++)
    {
      contents = AREF (result, i);
      if (TYPE_RANGED_FIXNUMP (int, contents))
        continue;
      else if (CONSP (contents) && SYMBOLP (XCAR (contents))
               && SYMBOLP (XCDR (contents)))
        {
          val = Fget (XCAR (contents), XCDR (contents));
          if (RANGED_FIXNUMP (0, val, INT_MAX))
            ASET (result, i, val);
          else
            unresolved = 1;
          continue;
        }
      else if (SYMBOLP (contents))
        {
          val = Fget (contents, Qtranslation_table_id);
          if (RANGED_FIXNUMP (0, val, INT_MAX))
            ASET (result, i, val);
          else
            {
              val = Fget (contents, Qcode_conversion_map_id);
              if (RANGED_FIXNUMP (0, val, INT_MAX))
                ASET (result, i, val);
              else
                {
                  val = Fget (contents, Qccl_program_idx);
                  if (RANGED_FIXNUMP (0, val, INT_MAX))
                    ASET (result, i, val);
                  else
                    unresolved = 1;
                }
            }
          continue;
        }
      return Qnil;
    }

  if (!(0 <= XFIXNUM (AREF (result, CCL_HEADER_BUF_MAG))
        && ASCENDING_ORDER (0, XFIXNUM (AREF (result, CCL_HEADER_EOF)),
                            ASIZE (ccl))))
    return Qnil;

  return (unresolved ? Qt : result);
}

DEFUN ("register-ccl-program", Fregister_ccl_program, Sregister_ccl_program,
       2, 2, 0,
       doc: /* Register CCL program CCL-PROG as NAME in `ccl-program-table'.
CCL-PROG should be a compiled CCL program (vector), or nil.
If it is nil, just reserve NAME as a CCL program name.
Return index number of the registered CCL program.  */)
(Lisp_Object name, Lisp_Object ccl_prog)
{
  ptrdiff_t len = ASIZE (Vccl_program_table);
  ptrdiff_t idx;
  Lisp_Object resolved;

  CHECK_SYMBOL (name);
  resolved = Qnil;
  if (!NILP (ccl_prog))
    {
      CHECK_VECTOR (ccl_prog);
      resolved = resolve_symbol_ccl_program (ccl_prog);
      if (NILP (resolved))
        error ("Error in CCL program");
      if (VECTORP (resolved))
        {
          ccl_prog = resolved;
          resolved = Qt;
        }
      else
        resolved = Qnil;
    }

  for (idx = 0; idx < len; idx++)
    {
      Lisp_Object slot;

      slot = AREF (Vccl_program_table, idx);
      if (!VECTORP (slot))
        break;

      if (EQ (name, AREF (slot, 0)))
        {
          ASET (slot, 1, ccl_prog);
          ASET (slot, 2, resolved);
          ASET (slot, 3, Qt);
          return make_fixnum (idx);
        }
    }

  if (idx == len)
    Vccl_program_table = larger_vector (Vccl_program_table, 1, -1);

  ASET (Vccl_program_table, idx, CALLN (Fvector, name, ccl_prog, resolved, Qt));

  Fput (name, Qccl_program_idx, make_fixnum (idx));
  return make_fixnum (idx);
}

void
syms_of_ccl (void)
{
  staticpro (&Vccl_program_table);
  Vccl_program_table = make_nil_vector (32);

  DEFSYM (Qccl, "ccl");

  DEFSYM (Qccl_program_idx, "ccl-program-idx");

  DEFSYM (Qcode_conversion_map_id, "code-conversion-map-id");

  defsubr (&Sregister_ccl_program);
}
