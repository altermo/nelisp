#include "lisp.h"

#define BYTE_CODE_SAFE (TODO_NELISP_LATER, true)

#define BYTE_CODES                             \
  DEFINE (Bstack_ref, 0)                       \
  DEFINE (Bstack_ref1, 1)                      \
  DEFINE (Bstack_ref2, 2)                      \
  DEFINE (Bstack_ref3, 3)                      \
  DEFINE (Bstack_ref4, 4)                      \
  DEFINE (Bstack_ref5, 5)                      \
  DEFINE (Bstack_ref6, 6)                      \
  DEFINE (Bstack_ref7, 7)                      \
  DEFINE (Bvarref, 010)                        \
  DEFINE (Bvarref1, 011)                       \
  DEFINE (Bvarref2, 012)                       \
  DEFINE (Bvarref3, 013)                       \
  DEFINE (Bvarref4, 014)                       \
  DEFINE (Bvarref5, 015)                       \
  DEFINE (Bvarref6, 016)                       \
  DEFINE (Bvarref7, 017)                       \
  DEFINE (Bvarset, 020)                        \
  DEFINE (Bvarset1, 021)                       \
  DEFINE (Bvarset2, 022)                       \
  DEFINE (Bvarset3, 023)                       \
  DEFINE (Bvarset4, 024)                       \
  DEFINE (Bvarset5, 025)                       \
  DEFINE (Bvarset6, 026)                       \
  DEFINE (Bvarset7, 027)                       \
  DEFINE (Bvarbind, 030)                       \
  DEFINE (Bvarbind1, 031)                      \
  DEFINE (Bvarbind2, 032)                      \
  DEFINE (Bvarbind3, 033)                      \
  DEFINE (Bvarbind4, 034)                      \
  DEFINE (Bvarbind5, 035)                      \
  DEFINE (Bvarbind6, 036)                      \
  DEFINE (Bvarbind7, 037)                      \
  DEFINE (Bcall, 040)                          \
  DEFINE (Bcall1, 041)                         \
  DEFINE (Bcall2, 042)                         \
  DEFINE (Bcall3, 043)                         \
  DEFINE (Bcall4, 044)                         \
  DEFINE (Bcall5, 045)                         \
  DEFINE (Bcall6, 046)                         \
  DEFINE (Bcall7, 047)                         \
  DEFINE (Bunbind, 050)                        \
  DEFINE (Bunbind1, 051)                       \
  DEFINE (Bunbind2, 052)                       \
  DEFINE (Bunbind3, 053)                       \
  DEFINE (Bunbind4, 054)                       \
  DEFINE (Bunbind5, 055)                       \
  DEFINE (Bunbind6, 056)                       \
  DEFINE (Bunbind7, 057)                       \
                                               \
  DEFINE (Bpophandler, 060)                    \
  DEFINE (Bpushconditioncase, 061)             \
  DEFINE (Bpushcatch, 062)                     \
                                               \
  DEFINE (Bnth, 070)                           \
  DEFINE (Bsymbolp, 071)                       \
  DEFINE (Bconsp, 072)                         \
  DEFINE (Bstringp, 073)                       \
  DEFINE (Blistp, 074)                         \
  DEFINE (Beq, 075)                            \
  DEFINE (Bmemq, 076)                          \
  DEFINE (Bnot, 077)                           \
  DEFINE (Bcar, 0100)                          \
  DEFINE (Bcdr, 0101)                          \
  DEFINE (Bcons, 0102)                         \
  DEFINE (Blist1, 0103)                        \
  DEFINE (Blist2, 0104)                        \
  DEFINE (Blist3, 0105)                        \
  DEFINE (Blist4, 0106)                        \
  DEFINE (Blength, 0107)                       \
  DEFINE (Baref, 0110)                         \
  DEFINE (Baset, 0111)                         \
  DEFINE (Bsymbol_value, 0112)                 \
  DEFINE (Bsymbol_function, 0113)              \
  DEFINE (Bset, 0114)                          \
  DEFINE (Bfset, 0115)                         \
  DEFINE (Bget, 0116)                          \
  DEFINE (Bsubstring, 0117)                    \
  DEFINE (Bconcat2, 0120)                      \
  DEFINE (Bconcat3, 0121)                      \
  DEFINE (Bconcat4, 0122)                      \
  DEFINE (Bsub1, 0123)                         \
  DEFINE (Badd1, 0124)                         \
  DEFINE (Beqlsign, 0125)                      \
  DEFINE (Bgtr, 0126)                          \
  DEFINE (Blss, 0127)                          \
  DEFINE (Bleq, 0130)                          \
  DEFINE (Bgeq, 0131)                          \
  DEFINE (Bdiff, 0132)                         \
  DEFINE (Bnegate, 0133)                       \
  DEFINE (Bplus, 0134)                         \
  DEFINE (Bmax, 0135)                          \
  DEFINE (Bmin, 0136)                          \
  DEFINE (Bmult, 0137)                         \
                                               \
  DEFINE (Bpoint, 0140)                        \
                                               \
  DEFINE (Bsave_current_buffer_OBSOLETE, 0141) \
  DEFINE (Bgoto_char, 0142)                    \
  DEFINE (Binsert, 0143)                       \
  DEFINE (Bpoint_max, 0144)                    \
  DEFINE (Bpoint_min, 0145)                    \
  DEFINE (Bchar_after, 0146)                   \
  DEFINE (Bfollowing_char, 0147)               \
  DEFINE (Bpreceding_char, 0150)               \
  DEFINE (Bcurrent_column, 0151)               \
  DEFINE (Bindent_to, 0152)                    \
                                               \
  DEFINE (Beolp, 0154)                         \
  DEFINE (Beobp, 0155)                         \
  DEFINE (Bbolp, 0156)                         \
  DEFINE (Bbobp, 0157)                         \
  DEFINE (Bcurrent_buffer, 0160)               \
  DEFINE (Bset_buffer, 0161)                   \
  DEFINE (Bsave_current_buffer, 0162)          \
                                               \
  DEFINE (Binteractive_p, 0164)                \
                                               \
  DEFINE (Bforward_char, 0165)                 \
  DEFINE (Bforward_word, 0166)                 \
  DEFINE (Bskip_chars_forward, 0167)           \
  DEFINE (Bskip_chars_backward, 0170)          \
  DEFINE (Bforward_line, 0171)                 \
  DEFINE (Bchar_syntax, 0172)                  \
  DEFINE (Bbuffer_substring, 0173)             \
  DEFINE (Bdelete_region, 0174)                \
  DEFINE (Bnarrow_to_region, 0175)             \
  DEFINE (Bwiden, 0176)                        \
  DEFINE (Bend_of_line, 0177)                  \
                                               \
  DEFINE (Bconstant2, 0201)                    \
  DEFINE (Bgoto, 0202)                         \
  DEFINE (Bgotoifnil, 0203)                    \
  DEFINE (Bgotoifnonnil, 0204)                 \
  DEFINE (Bgotoifnilelsepop, 0205)             \
  DEFINE (Bgotoifnonnilelsepop, 0206)          \
  DEFINE (Breturn, 0207)                       \
  DEFINE (Bdiscard, 0210)                      \
  DEFINE (Bdup, 0211)                          \
                                               \
  DEFINE (Bsave_excursion, 0212)               \
  DEFINE (Bsave_window_excursion, 0213)        \
  DEFINE (Bsave_restriction, 0214)             \
  DEFINE (Bcatch, 0215)                        \
                                               \
  DEFINE (Bunwind_protect, 0216)               \
  DEFINE (Bcondition_case, 0217)               \
  DEFINE (Btemp_output_buffer_setup, 0220)     \
  DEFINE (Btemp_output_buffer_show, 0221)      \
                                               \
  DEFINE (Bset_marker, 0223)                   \
  DEFINE (Bmatch_beginning, 0224)              \
  DEFINE (Bmatch_end, 0225)                    \
  DEFINE (Bupcase, 0226)                       \
  DEFINE (Bdowncase, 0227)                     \
                                               \
  DEFINE (Bstringeqlsign, 0230)                \
  DEFINE (Bstringlss, 0231)                    \
  DEFINE (Bequal, 0232)                        \
  DEFINE (Bnthcdr, 0233)                       \
  DEFINE (Belt, 0234)                          \
  DEFINE (Bmember, 0235)                       \
  DEFINE (Bassq, 0236)                         \
  DEFINE (Bnreverse, 0237)                     \
  DEFINE (Bsetcar, 0240)                       \
  DEFINE (Bsetcdr, 0241)                       \
  DEFINE (Bcar_safe, 0242)                     \
  DEFINE (Bcdr_safe, 0243)                     \
  DEFINE (Bnconc, 0244)                        \
  DEFINE (Bquo, 0245)                          \
  DEFINE (Brem, 0246)                          \
  DEFINE (Bnumberp, 0247)                      \
  DEFINE (Bintegerp, 0250)                     \
                                               \
  DEFINE (BlistN, 0257)                        \
  DEFINE (BconcatN, 0260)                      \
  DEFINE (BinsertN, 0261)                      \
                                               \
  DEFINE (Bstack_set, 0262)                    \
  DEFINE (Bstack_set2, 0263)                   \
  DEFINE (BdiscardN, 0266)                     \
                                               \
  DEFINE (Bswitch, 0267)                       \
                                               \
  DEFINE (Bconstant, 0300)

enum byte_code_op
{
#define DEFINE(name, value) name = value,
  BYTE_CODES
#undef DEFINE
};
#define FETCH (*pc++)
#define FETCH2 (op = FETCH, op | (FETCH << 8))
#define PUSH(x) (*++top = (x))
#define POP (*top--)
#define DISCARD(n) (top -= (n))
#define TOP (*top)

struct bc_frame
{
  struct bc_frame *saved_fp;

  Lisp_Object *saved_top;
  const unsigned char *saved_pc;

  Lisp_Object fun;

  Lisp_Object next_stack[];
};

#define BC_STACK_SIZE (512 * 1024 * sizeof (Lisp_Object))

static bool
valid_sp (struct bc_thread_state *bc, Lisp_Object *sp)
{
  struct bc_frame *fp = bc->fp;
  return sp < (Lisp_Object *) fp && sp + 1 >= fp->saved_fp->next_stack;
}

Lisp_Object
exec_byte_code (Lisp_Object fun, ptrdiff_t args_template, ptrdiff_t nargs,
                Lisp_Object *args)
{
  TODO_NELISP_LATER;
  unsigned char quitcounter = 1;
  struct bc_thread_state *bc = &bc_;
  Lisp_Object *top = NULL;
  unsigned char const *pc = NULL;
  Lisp_Object bytestr = AREF (fun, CLOSURE_CODE);
setup_frame:;
  eassert (!STRING_MULTIBYTE (bytestr));
  eassert (string_immovable_p (bytestr));
  Lisp_Object vector = AREF (fun, CLOSURE_CONSTANTS);
  Lisp_Object maxdepth = AREF (fun, CLOSURE_STACK_DEPTH);
  ptrdiff_t const_length = ASIZE (vector);
  ptrdiff_t bytestr_length = SCHARS (bytestr);
  UNUSED (bytestr_length);
  Lisp_Object *vectorp = XVECTOR (vector)->contents;
  EMACS_INT max_stack = XFIXNAT (maxdepth);
  Lisp_Object *frame_base = bc->fp->next_stack;
  struct bc_frame *fp = (struct bc_frame *) (frame_base + max_stack);
  if ((char *) fp->next_stack > bc->stack_end)
    error ("Bytecode stack overflow");
  fp->fun = fun;
  fp->saved_top = top;
  fp->saved_pc = pc;
  fp->saved_fp = bc->fp;
  bc->fp = fp;
  top = frame_base - 1;
  unsigned char const *bytestr_data = SDATA (bytestr);
  pc = bytestr_data;
  bool rest = (args_template & 128) != 0;
  int mandatory = args_template & 127;
  ptrdiff_t nonrest = args_template >> 8;
  if (!(mandatory <= nargs && (rest || nargs <= nonrest)))
    Fsignal (Qwrong_number_of_arguments,
             list2 (Fcons (make_fixnum (mandatory), make_fixnum (nonrest)),
                    make_fixnum (nargs)));
  ptrdiff_t pushedargs = min (nonrest, nargs);
  for (ptrdiff_t i = 0; i < pushedargs; i++, args++)
    PUSH (*args);
  if (nonrest < nargs)
    TODO; // PUSH (Flist (nargs - nonrest, args));
  else
    for (ptrdiff_t i = nargs - rest; i < nonrest; i++)
      PUSH (Qnil);

  while (true)
    {
      int op;
      // enum handlertype type;

      if (BYTE_CODE_SAFE && !valid_sp (bc, top))
        emacs_abort ();

      op = FETCH;

#define CASE(OP) case OP
#define NEXT break
#define FIRST switch (op)
#define CASE_DEFAULT \
  case 255:          \
  default:
#define CASE_ABORT case 0
      FIRST
        {
        CASE (Bvarref7):
          op = FETCH2;
          goto varref;

        CASE (Bvarref):
        CASE (Bvarref1):
        CASE (Bvarref2):
        CASE (Bvarref3):
        CASE (Bvarref4):
        CASE (Bvarref5):
          op -= Bvarref;
          goto varref;

        CASE (Bvarref6):
          op = FETCH;
        varref:
          {
            Lisp_Object v1 = vectorp[op], v2;
            if (XBARE_SYMBOL (v1)->u.s.redirect != SYMBOL_PLAINVAL
                || (v2 = XBARE_SYMBOL (v1)->u.s.val.value,
                    BASE_EQ (v2, Qunbound)))
              v2 = Fsymbol_value (v1);
            PUSH (v2);
            NEXT;
          }

        CASE (Bgotoifnil):
          {
            Lisp_Object v1 = POP;
            op = FETCH2;
            if (NILP (v1))
              goto op_branch;
            NEXT;
          }
        CASE (Beq):
          {
            Lisp_Object v1 = POP;
            TOP = EQ (v1, TOP) ? Qt : Qnil;
            NEXT;
          }

        CASE (Bmemq):
          {
            Lisp_Object v1 = POP;
            TOP = Fmemq (TOP, v1);
            NEXT;
          }

        CASE (Bdup):
          {
            Lisp_Object v1 = TOP;
            PUSH (v1);
            NEXT;
          }

        CASE (Breturn):
          {
            Lisp_Object *saved_top = bc->fp->saved_top;
            if (saved_top)
              {
                Lisp_Object val = TOP;

                lisp_eval_depth--;
#if TODO_NELISP_LATER_AND
                if (backtrace_debug_on_exit (specpdl_ptr - 1))
                  val = call_debugger (list2 (Qexit, val));
#endif
                specpdl_ptr--;

                top = saved_top;
                pc = bc->fp->saved_pc;
                struct bc_frame *fp = bc->fp->saved_fp;
                bc->fp = fp;

                Lisp_Object fun = fp->fun;
                Lisp_Object bytestr = AREF (fun, CLOSURE_CODE);
                Lisp_Object vector = AREF (fun, CLOSURE_CONSTANTS);
                bytestr_data = SDATA (bytestr);
                vectorp = XVECTOR (vector)->contents;
                if (BYTE_CODE_SAFE)
                  {
                    const_length = ASIZE (vector);
                    bytestr_length = SCHARS (bytestr);
                  }

                TOP = val;
                NEXT;
              }
            else
              goto exit;
          }

        CASE (Bdiscard):
          DISCARD (1);
          NEXT;

        CASE (Bcons):
          {
            Lisp_Object v1 = POP;
            TOP = Fcons (TOP, v1);
            NEXT;
          }

        CASE (Blist1):
          TOP = list1 (TOP);
          NEXT;

        CASE (Blist2):
          {
            Lisp_Object v1 = POP;
            TOP = list2 (TOP, v1);
            NEXT;
          }

        CASE (Blist3):
          DISCARD (2);
          TOP = list3 (TOP, top[1], top[2]);
          NEXT;

        CASE (Blist4):
          DISCARD (3);
          TOP = list4 (TOP, top[1], top[2], top[3]);
          NEXT;

        CASE (BlistN):
          op = FETCH;
          DISCARD (op - 1);
          TOP = Flist (op, &TOP);
          NEXT;

        CASE (Baref):
          {
            Lisp_Object idxval = POP;
            Lisp_Object arrayval = TOP;
            if (!FIXNUMP (idxval))
              {
                record_in_backtrace (Qaref, &TOP, 2);
                wrong_type_argument (Qfixnump, idxval);
              }
            ptrdiff_t size;
            if (((VECTORP (arrayval) && (size = ASIZE (arrayval), true))
                 || (RECORDP (arrayval) && (size = PVSIZE (arrayval), true))))
              {
                ptrdiff_t idx = XFIXNUM (idxval);
                if (idx >= 0 && idx < size)
                  TOP = AREF (arrayval, idx);
                else
                  {
                    record_in_backtrace (Qaref, &TOP, 2);
                    args_out_of_range (arrayval, idxval);
                  }
              }
            else
              TOP = Faref (arrayval, idxval);
            NEXT;
          }

        CASE (Baset):
          {
            Lisp_Object newelt = POP;
            Lisp_Object idxval = POP;
            Lisp_Object arrayval = TOP;
            if (!FIXNUMP (idxval))
              {
                record_in_backtrace (Qaset, &TOP, 3);
                wrong_type_argument (Qfixnump, idxval);
              }
            ptrdiff_t size;
            if (((VECTORP (arrayval) && (size = ASIZE (arrayval), true))
                 || (RECORDP (arrayval) && (size = PVSIZE (arrayval), true))))
              {
                ptrdiff_t idx = XFIXNUM (idxval);
                if (idx >= 0 && idx < size)
                  {
                    ASET (arrayval, idx, newelt);
                    TOP = newelt;
                  }
                else
                  {
                    record_in_backtrace (Qaset, &TOP, 3);
                    args_out_of_range (arrayval, idxval);
                  }
              }
            else
              TOP = Faset (arrayval, idxval, newelt);
            NEXT;
          }

        CASE (Bsymbol_function):
          TOP = Fsymbol_function (TOP);
          NEXT;

        CASE (Bfset):
          {
            Lisp_Object v1 = POP;
            TOP = Ffset (TOP, v1);
            NEXT;
          }

        CASE (Bsub1):
          TOP = (FIXNUMP (TOP) && XFIXNUM (TOP) != MOST_NEGATIVE_FIXNUM
                   ? make_fixnum (XFIXNUM (TOP) - 1)
                   : Fsub1 (TOP));
          NEXT;

        CASE (Badd1):
          TOP = (FIXNUMP (TOP) && XFIXNUM (TOP) != MOST_POSITIVE_FIXNUM
                   ? make_fixnum (XFIXNUM (TOP) + 1)
                   : Fadd1 (TOP));
          NEXT;

        CASE (Beqlsign):
          {
            Lisp_Object v2 = POP;
            Lisp_Object v1 = TOP;
            if (FIXNUMP (v1) && FIXNUMP (v2))
              TOP = BASE_EQ (v1, v2) ? Qt : Qnil;
            else
              TOP = arithcompare (v1, v2, ARITH_EQUAL);
            NEXT;
          }

        CASE (Bgtr):
          {
            Lisp_Object v2 = POP;
            Lisp_Object v1 = TOP;
            if (FIXNUMP (v1) && FIXNUMP (v2))
              TOP = XFIXNUM (v1) > XFIXNUM (v2) ? Qt : Qnil;
            else
              TOP = arithcompare (v1, v2, ARITH_GRTR);
            NEXT;
          }

        CASE (Blss):
          {
            Lisp_Object v2 = POP;
            Lisp_Object v1 = TOP;
            if (FIXNUMP (v1) && FIXNUMP (v2))
              TOP = XFIXNUM (v1) < XFIXNUM (v2) ? Qt : Qnil;
            else
              TOP = arithcompare (v1, v2, ARITH_LESS);
            NEXT;
          }

        CASE (Bleq):
          {
            Lisp_Object v2 = POP;
            Lisp_Object v1 = TOP;
            if (FIXNUMP (v1) && FIXNUMP (v2))
              TOP = XFIXNUM (v1) <= XFIXNUM (v2) ? Qt : Qnil;
            else
              TOP = arithcompare (v1, v2, ARITH_LESS_OR_EQUAL);
            NEXT;
          }

        CASE (Bgeq):
          {
            Lisp_Object v2 = POP;
            Lisp_Object v1 = TOP;
            if (FIXNUMP (v1) && FIXNUMP (v2))
              TOP = XFIXNUM (v1) >= XFIXNUM (v2) ? Qt : Qnil;
            else
              TOP = arithcompare (v1, v2, ARITH_GRTR_OR_EQUAL);
            NEXT;
          }

        CASE (Bcall6):
          op = FETCH;
          goto docall;

        CASE (Bcall7):
          op = FETCH2;
          goto docall;

        CASE_ABORT:
          error ("Invalid byte opcode: op=%d, ptr=%" pD "d", op,
                 pc - 1 - bytestr_data);

        CASE (Bstack_ref1):
        CASE (Bstack_ref2):
        CASE (Bstack_ref3):
        CASE (Bstack_ref4):
        CASE (Bstack_ref5):
          {
            Lisp_Object v1 = top[Bstack_ref - op];
            PUSH (v1);
            NEXT;
          }
        CASE (Bstack_ref6):
          {
            Lisp_Object v1 = top[-FETCH];
            PUSH (v1);
            NEXT;
          }
        CASE (Bstack_ref7):
          {
            Lisp_Object v1 = top[-FETCH2];
            PUSH (v1);
            NEXT;
          }

        CASE (Bstack_set):
          {
            Lisp_Object *ptr = top - FETCH;
            *ptr = POP;
            NEXT;
          }
        CASE (Bstack_set2):
          {
            Lisp_Object *ptr = top - FETCH2;
            *ptr = POP;
            NEXT;
          }

        CASE (BdiscardN):
          op = FETCH;
          if (op & 0x80)
            {
              op &= 0x7F;
              top[-op] = TOP;
            }
          DISCARD (op);
          NEXT;

        CASE (Bcall):
        CASE (Bcall1):
        CASE (Bcall2):
        CASE (Bcall3):
        CASE (Bcall4):
        CASE (Bcall5):
          op -= Bcall;
        docall:
          {
            DISCARD (op);
            maybe_quit ();

            if (++lisp_eval_depth > max_lisp_eval_depth)
              {
                if (max_lisp_eval_depth < 100)
                  max_lisp_eval_depth = 100;
                if (lisp_eval_depth > max_lisp_eval_depth)
                  error ("Lisp nesting exceeds `max-lisp-eval-depth'");
              }

            ptrdiff_t call_nargs = op;
            Lisp_Object call_fun = TOP;
            Lisp_Object *call_args = &TOP + 1;

            specpdl_ref count1
              = record_in_backtrace (call_fun, call_args, call_nargs);
            maybe_gc ();
#if TODO_NELISP_LATER_AND
            if (debug_on_next_call)
              do_debug_on_call (Qlambda, count1);
#endif

            Lisp_Object original_fun = call_fun;
            UNUSED (original_fun);
            UNUSED (count1);
            if (BARE_SYMBOL_P (call_fun))
              call_fun = XBARE_SYMBOL (call_fun)->u.s.function;
            if (CLOSUREP (call_fun))
              {
                Lisp_Object template = AREF (call_fun, CLOSURE_ARGLIST);
                if (FIXNUMP (template))
                  {
                    fun = call_fun;
                    bytestr = AREF (call_fun, CLOSURE_CODE),
                    args_template = XFIXNUM (template);
                    nargs = call_nargs;
                    args = call_args;
                    goto setup_frame;
                  }
              }

            Lisp_Object val;
            if (SUBRP (call_fun) && !NATIVE_COMP_FUNCTION_DYNP (call_fun))
              val = funcall_subr (XSUBR (call_fun), call_nargs, call_args);
            else
              val = funcall_general (original_fun, call_nargs, call_args);

            lisp_eval_depth--;
#if TODO_NELISP_LATER_AND
            if (backtrace_debug_on_exit (specpdl_ptr - 1))
              val = call_debugger (list2 (Qexit, val));
#endif
            specpdl_ptr--;

            TOP = val;
            NEXT;
          }

        CASE (Bgoto):
          op = FETCH2;
        op_branch:
          op -= pc - bytestr_data;
          if (BYTE_CODE_SAFE
              && !(bytestr_data - pc <= op
                   && op < bytestr_data + bytestr_length - pc))
            emacs_abort ();
          quitcounter += op < 0;
          if (!quitcounter)
            {
              quitcounter = 1;
              maybe_gc ();
              maybe_quit ();
            }
          pc += op;
          NEXT;

        CASE (Bgotoifnonnil):
          op = FETCH2;
          if (!NILP (POP))
            goto op_branch;
          NEXT;

        CASE (Bgotoifnilelsepop):
          op = FETCH2;
          if (NILP (TOP))
            goto op_branch;
          DISCARD (1);
          NEXT;

        CASE (Bgotoifnonnilelsepop):
          op = FETCH2;
          if (!NILP (TOP))
            goto op_branch;
          DISCARD (1);
          NEXT;

        CASE_DEFAULT
        CASE (Bconstant):
          if (BYTE_CODE_SAFE
              && !(Bconstant <= op && op < Bconstant + const_length))
            {
              char msg[] = "Not-implemented bytecode: op=0oxxx";
              msg[sizeof (msg) - 2] = '0' + (op % 010);
              msg[sizeof (msg) - 3] = '0' + (op % 0100 / 010);
              msg[sizeof (msg) - 4] = '0' + (op / 0100);
              TODO_msg (__FILE__, __LINE__, msg); // emacs_abort ();
            }
          PUSH (vectorp[op - Bconstant]);
          NEXT;
        }
    }
exit:

  bc->fp = bc->fp->saved_fp;

  Lisp_Object result = TOP;
  return result;
}

DEFUN ("byte-code", Fbyte_code, Sbyte_code, 3, 3, 0,
       doc: /* Function used internally in byte-compiled code.
The first argument, BYTESTR, is a string of byte code;
the second, VECTOR, a vector of constants;
the third, MAXDEPTH, the maximum stack depth used in this function.
If the third argument is incorrect, Emacs may crash.  */)
(Lisp_Object bytestr, Lisp_Object vector, Lisp_Object maxdepth)
{
  if (!(STRINGP (bytestr) && VECTORP (vector) && FIXNATP (maxdepth)))
    error ("Invalid byte-code");

  if (STRING_MULTIBYTE (bytestr))
    {
      TODO; // bytestr = Fstring_as_unibyte (bytestr);
    }
  Lisp_Object fun = CALLN (Fmake_byte_code, Qnil, bytestr, vector, maxdepth);
  return exec_byte_code (fun, 0, 0, NULL);
}

DEFUN ("make-byte-code", Fmake_byte_code, Smake_byte_code, 4, MANY, 0,
       doc: /* Create a byte-code object with specified arguments as elements.
The arguments should be the ARGLIST, bytecode-string BYTE-CODE, constant
vector CONSTANTS, maximum stack size DEPTH, (optional) DOCSTRING,
and (optional) INTERACTIVE-SPEC.
The first four arguments are required; at most six have any
significance.
The ARGLIST can be either like the one of `lambda', in which case the arguments
will be dynamically bound before executing the byte code, or it can be an
integer of the form NNNNNNNRMMMMMMM where the 7bit MMMMMMM specifies the
minimum number of arguments, the 7-bit NNNNNNN specifies the maximum number
of arguments (ignoring &rest) and the R bit specifies whether there is a &rest
argument to catch the left-over arguments.  If such an integer is used, the
arguments will not be dynamically bound but will be instead pushed on the
stack before executing the byte-code.
usage: (make-byte-code ARGLIST BYTE-CODE CONSTANTS DEPTH &optional DOCSTRING INTERACTIVE-SPEC &rest ELEMENTS)  */)
(ptrdiff_t nargs, Lisp_Object *args)
{
  if (!((FIXNUMP (args[CLOSURE_ARGLIST]) || CONSP (args[CLOSURE_ARGLIST])
         || NILP (args[CLOSURE_ARGLIST]))
        && STRINGP (args[CLOSURE_CODE])
        && !STRING_MULTIBYTE (args[CLOSURE_CODE])
        && VECTORP (args[CLOSURE_CONSTANTS])
        && FIXNATP (args[CLOSURE_STACK_DEPTH])))
    error ("Invalid byte-code object");

  pin_string (args[CLOSURE_CODE]);

  Lisp_Object val = Fvector (nargs, args);
  XSETPVECTYPE (XVECTOR (val), PVEC_CLOSURE);
  return val;
}

void
init_bc_thread (struct bc_thread_state *bc)
{
  bc->stack = xmalloc (BC_STACK_SIZE);
  bc->stack_end = bc->stack + BC_STACK_SIZE;
  bc->fp = (struct bc_frame *) bc->stack;
  memset (bc->fp, 0, sizeof *bc->fp);
}

void
init_bytecode (void)
{
  TODO_NELISP_LATER;
  init_bc_thread (&bc_);
}

void
syms_of_bytecode (void)
{
  defsubr (&Sbyte_code);
  defsubr (&Smake_byte_code);
}
