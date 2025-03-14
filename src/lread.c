#include "lisp.h"
#include "character.h"
#include <stdlib.h>
#include <math.h>

struct Lisp_Symbol lispsym[];

static Lisp_Object initial_obarray;
static size_t oblookup_last_bucket_number;

Lisp_Object
check_obarray_slow (Lisp_Object obarray)
{
    UNUSED(obarray);
    TODO;
    return NULL;
}

INLINE Lisp_Object
check_obarray (Lisp_Object obarray)
{
    return OBARRAYP (obarray) ? obarray : check_obarray_slow (obarray);
}

static ptrdiff_t
obarray_index (struct Lisp_Obarray *oa, const char *str, ptrdiff_t size_byte)
{
    EMACS_UINT hash = hash_string (str, size_byte);
    return knuth_hash (reduce_emacs_uint_to_hash_hash (hash), oa->size_bits);
}

Lisp_Object
oblookup (Lisp_Object obarray, register const char *ptr, ptrdiff_t size, ptrdiff_t size_byte)
{
    struct Lisp_Obarray *o = XOBARRAY (obarray);
    ptrdiff_t idx = obarray_index (o, ptr, size_byte);
    Lisp_Object bucket = o->buckets[idx];

    oblookup_last_bucket_number = idx;
    if (!BASE_EQ (bucket, make_fixnum (0)))
    {
        Lisp_Object sym = bucket;
        while (1)
        {
            struct Lisp_Symbol *s = XBARE_SYMBOL (sym);
            Lisp_Object name = s->u.s.name;
            if (SBYTES (name) == size_byte && SCHARS (name) == size
                && memcmp (SDATA (name), ptr, size_byte) == 0)
                return sym;
            if (s->u.s.next == NULL)
                break;
            sym = make_lisp_symbol(s->u.s.next);
        }
    }
    return make_fixnum (idx);
}

enum {
    obarray_default_bits = 3,
    word_size_log2 = word_size < 8 ? 5 : 6,
    obarray_max_bits = min (8 * sizeof (int),
                            8 * sizeof (ptrdiff_t) - word_size_log2) - 1,
};

static void
grow_obarray (struct Lisp_Obarray *o)
{
    ptrdiff_t old_size = obarray_size (o);
    eassert (o->count > old_size);
    Lisp_Object *old_buckets = o->buckets;

    int new_bits = o->size_bits + 1;
    if (new_bits > obarray_max_bits)
        TODO; //error ("Obarray too big");
    ptrdiff_t new_size = (ptrdiff_t)1 << new_bits;
    o->buckets = hash_table_alloc_bytes (new_size * sizeof *o->buckets);
    for (ptrdiff_t i = 0; i < new_size; i++)
        o->buckets[i] = make_fixnum (0);
    o->size_bits = new_bits;

    for (ptrdiff_t i = 0; i < old_size; i++)
    {
        Lisp_Object obj = old_buckets[i];
        if (BARE_SYMBOL_P (obj))
        {
            struct Lisp_Symbol *s = XBARE_SYMBOL (obj);
            while (1)
            {
                Lisp_Object name = s->u.s.name;
                ptrdiff_t idx = obarray_index (o, SSDATA (name), SBYTES (name));
                Lisp_Object *loc = o->buckets + idx;
                struct Lisp_Symbol *next = s->u.s.next;
                s->u.s.next = BARE_SYMBOL_P (*loc) ? XBARE_SYMBOL (*loc) : NULL;
                *loc = make_lisp_symbol (s);
                if (next == NULL)
                    break;
                s = next;
            }
        }
    }

    hash_table_free_bytes (old_buckets, old_size * sizeof *old_buckets);
}

static Lisp_Object
intern_sym (Lisp_Object sym, Lisp_Object obarray, Lisp_Object index)
{
    eassert (BARE_SYMBOL_P (sym) && OBARRAYP (obarray) && FIXNUMP (index));
    struct Lisp_Symbol *s = XBARE_SYMBOL (sym);
    s->u.s.interned = (BASE_EQ (obarray, initial_obarray)
        ? SYMBOL_INTERNED_IN_INITIAL_OBARRAY
        : SYMBOL_INTERNED);

    if (SREF (s->u.s.name, 0) == ':' && BASE_EQ (obarray, initial_obarray))
    {
        s->u.s.trapped_write = SYMBOL_NOWRITE;
        s->u.s.redirect = SYMBOL_PLAINVAL;
        s->u.s.declared_special = true;
        SET_SYMBOL_VAL (s, sym);
    }

    struct Lisp_Obarray *o = XOBARRAY (obarray);
    Lisp_Object *ptr = o->buckets + XFIXNUM (index);
    s->u.s.next = BARE_SYMBOL_P (*ptr) ? XBARE_SYMBOL (*ptr) : NULL;
    *ptr = sym;
    o->count++;
    if (o->count > obarray_size (o))
        grow_obarray (o);
    return sym;
}

static struct Lisp_Obarray *
allocate_obarray (void)
{
    return ALLOCATE_PLAIN_PSEUDOVECTOR (struct Lisp_Obarray, PVEC_OBARRAY);
}

static Lisp_Object
make_obarray (unsigned bits)
{
    struct Lisp_Obarray *o = allocate_obarray ();
    o->count = 0;
    o->size_bits = bits;
    ptrdiff_t size = (ptrdiff_t)1 << bits;
    o->buckets = hash_table_alloc_bytes (size * sizeof *o->buckets);
    for (ptrdiff_t i = 0; i < size; i++)
        o->buckets[i] = make_fixnum (0);
    return make_lisp_obarray (o);
}

static void
define_symbol (Lisp_Object sym, char const *str)
{
    ptrdiff_t len = strlen (str);
    Lisp_Object string = make_pure_c_string (str, len);
    init_symbol (sym, string);

    if (! BASE_EQ (sym, Qunbound))
    {
        Lisp_Object bucket = oblookup (initial_obarray, str, len, len);
        eassert (FIXNUMP (bucket));
        intern_sym (sym, initial_obarray, bucket);
    }
}

void
init_obarray_once (void)
{
    Vobarray = make_obarray (15);
    initial_obarray = Vobarray;
    staticpro (&initial_obarray);

    for (unsigned long i = 0; i < ARRAYELTS (lispsym); i++)
        define_symbol (builtin_lisp_symbol (i), defsym_name[i]);

    DEFSYM (Qunbound, "unbound");

    DEFSYM (Qnil, "nil");
    SET_SYMBOL_VAL (XBARE_SYMBOL (Qnil), Qnil);
    make_symbol_constant (Qnil);
    XBARE_SYMBOL (Qnil)->u.s.declared_special = true;

    DEFSYM (Qt, "t");
    SET_SYMBOL_VAL (XBARE_SYMBOL (Qt), Qt);
    make_symbol_constant (Qt);
    XBARE_SYMBOL (Qt)->u.s.declared_special = true;

#if TODO_NELISP_LATER_AND
    Vpurify_flag = Qt;
#endif

    DEFSYM (Qvariable_documentation, "variable-documentation");
}

Lisp_Object
intern_driver (Lisp_Object string, Lisp_Object obarray, Lisp_Object index)
{
    SET_SYMBOL_VAL (XBARE_SYMBOL (Qobarray_cache), Qnil);
    return intern_sym (Fmake_symbol (string), obarray, index);
}

Lisp_Object
intern_c_string_1 (const char *str, ptrdiff_t len)
{
    Lisp_Object obarray = check_obarray (Vobarray);
    Lisp_Object tem = oblookup (obarray, str, len, len);

    if (!BARE_SYMBOL_P (tem))
    {
        Lisp_Object string;

#if TODO_NELISP_LATER_AND
        if (NILP (Vpurify_flag))
            string = make_string (str, len);
        else
            string = make_pure_c_string (str, len);
#else
        string = make_pure_c_string(str, len);
#endif

        tem = intern_driver (string, obarray, tem);
    }
    return tem;
}
INLINE Lisp_Object
intern_c_string (const char *str)
{
    return intern_c_string_1 (str, strlen (str));
}

DEFUN ("intern", Fintern, Sintern, 1, 2, 0,
       doc: /* Return the canonical symbol whose name is STRING.
If there is none, one is created by this function and returned.
A second optional argument specifies the obarray to use;
it defaults to the value of `obarray'.  */)
    (Lisp_Object string, Lisp_Object obarray)
{
    Lisp_Object tem;

    obarray = check_obarray (NILP (obarray) ? Vobarray : obarray);
    CHECK_STRING (string);

#if TODO_NELISP_LATER_AND
    char* longhand = NULL;
    ptrdiff_t longhand_chars = 0;
    ptrdiff_t longhand_bytes = 0;
    tem = oblookup_considering_shorthand (obarray, SSDATA (string),
                                          SCHARS (string), SBYTES (string),
                                          &longhand, &longhand_chars,
                                          &longhand_bytes);

    if (!BARE_SYMBOL_P (tem))
    {
        if (longhand)
        {
            tem = intern_driver (make_specified_string (longhand, longhand_chars,
                                                        longhand_bytes, true),
                                 obarray, tem);
            xfree (longhand);
        }
        else
        tem = intern_driver (NILP (Vpurify_flag) ? string : Fpurecopy (string),
                             obarray, tem);
    }
#else
    tem = oblookup(obarray, SSDATA (string), SCHARS (string), SBYTES (string));
    if (!BARE_SYMBOL_P (tem))
        tem = intern_driver (string,obarray,tem);
#endif
    return tem;
}

void
defvar_lisp_nopro (struct Lisp_Objfwd const *o_fwd, char const *namestring)
{
    Lisp_Object sym = intern_c_string (namestring);
    XBARE_SYMBOL (sym)->u.s.declared_special = true;
    XBARE_SYMBOL (sym)->u.s.redirect = SYMBOL_FORWARDED;
    SET_SYMBOL_FWD (XBARE_SYMBOL (sym), o_fwd);
}
void
defvar_lisp (struct Lisp_Objfwd const *o_fwd, char const *namestring)
{
    defvar_lisp_nopro (o_fwd, namestring);
    staticpro (o_fwd->objvar);
}
void
defvar_int (struct Lisp_Intfwd const *i_fwd, char const *namestring)
{
  Lisp_Object sym = intern_c_string (namestring);
  XBARE_SYMBOL (sym)->u.s.declared_special = true;
  XBARE_SYMBOL (sym)->u.s.redirect = SYMBOL_FORWARDED;
  SET_SYMBOL_FWD (XBARE_SYMBOL (sym), i_fwd);
}

void
defsubr (union Aligned_Lisp_Subr *aname)
{
    struct Lisp_Subr *sname = &aname->s;
    Lisp_Object sym, tem;
    sym = intern_c_string (sname->symbol_name);
    XSETPVECTYPE (sname, PVEC_SUBR);
    XSETSUBR (tem, sname);
    set_symbol_function (sym, tem);
}

ptrdiff_t read_from_string_index;
ptrdiff_t read_from_string_index_byte;
ptrdiff_t read_from_string_limit;
#define READCHAR readchar (readcharfun, NULL)
#define UNREAD(c) unreadchar (readcharfun, c)
#define READCHAR_REPORT_MULTIBYTE(multibyte) readchar (readcharfun, multibyte)
int
readchar (Lisp_Object readcharfun, bool *multibyte)
{
    register int c;

    if (multibyte)
        *multibyte = 0;

    if (STRINGP (readcharfun))
    {
        if (read_from_string_index >= read_from_string_limit)
            c = -1;
        else if (STRING_MULTIBYTE (readcharfun)) {
            TODO;
        } else {
            c = SREF (readcharfun, read_from_string_index_byte);
            read_from_string_index++;
            read_from_string_index_byte++;
        }
        return c;
    }
    TODO;
}
void
unreadchar (Lisp_Object readcharfun, int c)
{
    if (c == -1)
        ;
    else if (STRINGP (readcharfun))
    {
    read_from_string_index--;
    read_from_string_index_byte
        = string_char_to_byte (readcharfun, read_from_string_index);
}
    else
    TODO;
}
enum read_entry_type
{
    RE_list_start,
    RE_list,
    RE_list_dot,
    RE_vector,
    RE_record,
    RE_char_table,
    RE_sub_char_table,
    RE_byte_code,
    RE_string_props,
    RE_special,
    RE_numbered,
};
struct read_stack_entry
{
    enum read_entry_type type;
    union {
        struct {
            Lisp_Object head;
            Lisp_Object tail;
        } list;
        struct {
            Lisp_Object elems;
            bool old_locate_syms;
        } vector;
        struct {
            Lisp_Object symbol;
        } special;
        struct {
            Lisp_Object number;
            Lisp_Object placeholder;
        } numbered;
    } u;
};
struct read_stack
{
    struct read_stack_entry *stack;
    ptrdiff_t size;
    ptrdiff_t sp;
};
static struct read_stack rdstack = {NULL, 0, 0};
static inline struct read_stack_entry *
read_stack_top (void)
{
    eassume (rdstack.sp > 0);
    return &rdstack.stack[rdstack.sp - 1];
}
static inline struct read_stack_entry *
read_stack_pop (void)
{
    eassume (rdstack.sp > 0);
    return &rdstack.stack[--rdstack.sp];
}
static inline bool
read_stack_empty_p (ptrdiff_t base_sp)
{
    return rdstack.sp <= base_sp;
}
NO_INLINE static void
grow_read_stack (void)
{
    struct read_stack *rs = &rdstack;
    eassert (rs->sp == rs->size);
    rs->stack = xpalloc (rs->stack, &rs->size, 1, -1, sizeof *rs->stack);
    eassert (rs->sp < rs->size);
}
static inline void
read_stack_push (struct read_stack_entry e)
{
    if (rdstack.sp >= rdstack.size)
        grow_read_stack ();
    rdstack.stack[rdstack.sp++] = e;
}
static void
read_stack_reset (intmax_t sp)
{
    eassert (sp <= rdstack.sp);
    rdstack.sp = sp;
}
static AVOID
end_of_file_error (void)
{
    TODO;
}
static AVOID
invalid_syntax (const char *s, Lisp_Object readcharfun)
{
    UNUSED(s);
    UNUSED(readcharfun);
    TODO;
}
static int
read_char_escape (Lisp_Object readcharfun, int next_char)
{
    int modifiers = 0;
    ptrdiff_t ncontrol = 0;
    int chr;

again: ;
    int c = next_char;

    switch (c)
    {
        case -1:
            end_of_file_error ();

        case 'a': chr = '\a'; break;
        case 'b': chr = '\b'; break;
        case 'd': chr =  127; break;
        case 'e': chr =   27; break;
        case 'f': chr = '\f'; break;
        case 'n': chr = '\n'; break;
        case 'r': chr = '\r'; break;
        case 't': chr = '\t'; break;
        case 'v': chr = '\v'; break;

        case '\n':
            TODO;

        case 'M': TODO;
        case 'S': TODO;
        case 'H': TODO;
        case 'A': TODO;
        case 's': TODO;

        case 'C':
            {
                int c1 = READCHAR;
                if (c1 != '-')
                    TODO;
            }
            FALLTHROUGH;
        case '^':
            {
                ncontrol++;
                int c1 = READCHAR;
                if (c1 == '\\')
                {
                    next_char = READCHAR;
                    goto again;
                }
                chr = c1;
                break;
            }

        case '0': case '1': case '2': case '3':
        case '4': case '5': case '6': case '7':
            {
                int i = c - '0';
                int count = 0;
                while (count < 2)
                {
                    int c = READCHAR;
                    if (c < '0' || c > '7')
                    {
                        UNREAD (c);
                        break;
                    }
                    i = (i << 3) + (c - '0');
                    count++;
                }

                if (i >= 0x80 && i < 0x100)
                    i = BYTE8_TO_CHAR (i);
                chr = i;
                break;
            }

        case 'x':
            TODO;

        case 'U':
            TODO;

        case 'u':
            TODO;

        case 'N':
            TODO;

        default:
            chr = c;
            break;
    }
    eassert (chr >= 0 && chr < (1 << CHARACTERBITS));

    while (ncontrol > 0)
    {
        TODO;
    }

    return chr | modifiers;
}
static Lisp_Object
read_string_literal (Lisp_Object readcharfun)
{
    char stackbuf[1024];
    char *read_buffer = stackbuf;
    ptrdiff_t read_buffer_size = sizeof stackbuf;
    specpdl_ref count = SPECPDL_INDEX ();
    // char *heapbuf = NULL;
    char *p = read_buffer;
    char *end = read_buffer + read_buffer_size;
    bool force_multibyte = false;
    bool force_singlebyte = false;
    ptrdiff_t nchars = 0;
    int ch;
    while ((ch = READCHAR) >= 0 && ch != '\"')
    {
        if (end - p < MAX_MULTIBYTE_LENGTH)
        {
            ptrdiff_t offset = p - read_buffer;
            TODO;
            p = read_buffer + offset;
            end = read_buffer + read_buffer_size;
        }

        if (ch == '\\')
        {
            ch = READCHAR;
            switch (ch)
            {
                case 's':
                    ch = ' ';
                    break;
                case ' ':
                case '\n':
                    continue;
                default:
                    ch = read_char_escape (readcharfun, ch);
                    break;
            }

            int modifiers = ch & CHAR_MODIFIER_MASK;
            ch &= ~CHAR_MODIFIER_MASK;

            if (CHAR_BYTE8_P (ch))
                force_singlebyte = true;
            else if (! ASCII_CHAR_P (ch))
                force_multibyte = true;
            else
            {
                if (modifiers == CHAR_CTL && ch == ' ')
                {
                    ch = 0;
                    modifiers = 0;
                }
                if (modifiers & CHAR_SHIFT)
                {
                    if (ch >= 'A' && ch <= 'Z')
                        modifiers &= ~CHAR_SHIFT;
                    else if (ch >= 'a' && ch <= 'z')
                    {
                    ch -= ('a' - 'A');
                    modifiers &= ~CHAR_SHIFT;
                }
                }

                if (modifiers & CHAR_META)
                {
                    modifiers &= ~CHAR_META;
                    ch = BYTE8_TO_CHAR (ch | 0x80);
                    force_singlebyte = true;
                }
            }

            if (modifiers)
                invalid_syntax ("Invalid modifier in string", readcharfun);
            p += CHAR_STRING (ch, (unsigned char *) p);
        }
        else
    {
            p += CHAR_STRING (ch, (unsigned char *) p);
            if (CHAR_BYTE8_P (ch))
                force_singlebyte = true;
            else if (! ASCII_CHAR_P (ch))
                force_multibyte = true;
        }
        nchars++;
    }

    if (ch < 0)
        end_of_file_error ();

    if (!force_multibyte && force_singlebyte)
    {
        nchars = str_as_unibyte ((unsigned char *) read_buffer,
                                 p - read_buffer);
        p = read_buffer + nchars;
    }

    Lisp_Object obj = make_specified_string (read_buffer, nchars, p - read_buffer,
                                             (force_multibyte
                                             || (p - read_buffer != nchars)));
    return unbind_to (count, obj);
}
static int
digit_to_number (int character, int base)
{
  int digit;

  if ('0' <= character && character <= '9')
    digit = character - '0';
  else if ('a' <= character && character <= 'z')
    digit = character - 'a' + 10;
  else if ('A' <= character && character <= 'Z')
    digit = character - 'A' + 10;
  else
    return -2;

  return digit < base ? digit : -1;
}
Lisp_Object
string_to_number (char const *string, int base, ptrdiff_t *plen)
{
    char const *cp = string;
    bool float_syntax = false;
    double value = 0;

    bool negative = *cp == '-';
    bool positive = *cp == '+';

    bool signedp = negative | positive;
    cp += signedp;

    enum { INTOVERFLOW = 1, LEAD_INT = 2, TRAIL_INT = 4, E_EXP = 16 };
    int state = 0;
    int leading_digit = digit_to_number (*cp, base);
    uintmax_t n = leading_digit;
    if (leading_digit >= 0)
    {
        state |= LEAD_INT;
        for (int digit; 0 <= (digit = digit_to_number (*++cp, base)); )
        {
#if TODO_NELISP_LATER_AND
            if (INT_MULTIPLY_OVERFLOW (n, base))
                state |= INTOVERFLOW;
#endif
            n *= base;
#if TODO_NELISP_LATER_AND
            if (INT_ADD_OVERFLOW (n, digit))
                state |= INTOVERFLOW;
#endif
            n += digit;
        }
    }
    // char const *after_digits = cp;
    if (*cp == '.')
    {
        cp++;
    }

    if (base == 10)
    {
        if ('0' <= *cp && *cp <= '9')
        {
            state |= TRAIL_INT;
            do
            cp++;
            while ('0' <= *cp && *cp <= '9');
        }
        if (*cp == 'e' || *cp == 'E')
        {
            char const *ecp = cp;
            cp++;
            if (*cp == '+' || *cp == '-')
                cp++;
            if ('0' <= *cp && *cp <= '9')
            {
                state |= E_EXP;
                do
                cp++;
                while ('0' <= *cp && *cp <= '9');
            }
            else if (cp[-1] == '+'
                && cp[0] == 'I' && cp[1] == 'N' && cp[2] == 'F')
            {
                state |= E_EXP;
                cp += 3;
                value = INFINITY;
            }
            else if (cp[-1] == '+'
                && cp[0] == 'N' && cp[1] == 'a' && cp[2] == 'N')
            {
                state |= E_EXP;
                cp += 3;
                union ieee754_double u
                    = { .ieee_nan = { .exponent = 0x7ff, .quiet_nan = 1,
                        .mantissa0 = n >> 31 >> 1, .mantissa1 = n }};
                value = u.d;
            }
            else
            cp = ecp;
        }

        float_syntax = ((state & TRAIL_INT)
            || ((state & LEAD_INT) && (state & E_EXP)));
    }

    if (plen)
        *plen = cp - string;

    if (float_syntax)
    {
        if (! value)
            value = atof (string + signedp);
        return make_float (negative ? -value : value);
    }

    if (! (state & LEAD_INT))
        return Qnil;

    if (! (state & INTOVERFLOW))
    {
        if (!negative)
            return make_uint (n);
        if (-MOST_NEGATIVE_FIXNUM < n)
            TODO;
        EMACS_INT signed_n = n;
        return make_fixnum (-signed_n);
    }

    TODO;
}
Lisp_Object
read0 (Lisp_Object readcharfun, bool locate_syms)
{
    TODO_NELISP_LATER;
    char stackbuf[64];
    char *read_buffer = stackbuf;
    ptrdiff_t read_buffer_size = sizeof stackbuf;
    specpdl_ref base_pdl = SPECPDL_INDEX ();
    ptrdiff_t base_sp = rdstack.sp;
    record_unwind_protect_intmax (read_stack_reset, base_sp);
    bool uninterned_symbol;
    bool skip_shorthand;
read_obj: ;
    Lisp_Object obj;
    bool multibyte;
    int c = READCHAR_REPORT_MULTIBYTE (&multibyte);
    switch (c)
    {
        case '(':
            read_stack_push ((struct read_stack_entry) {.type = RE_list_start});
            goto read_obj;
        case ')':
            if (read_stack_empty_p (base_sp))
                invalid_syntax (")", readcharfun);
            switch (read_stack_top ()->type)
            {
                case RE_list_start:
                    read_stack_pop ();
                    obj = Qnil;
                    break;
                case RE_list:
                    obj = read_stack_pop ()->u.list.head;
                    break;
                case RE_record:
                    TODO;
                case RE_string_props:
                    TODO;
                default:
                    invalid_syntax (")", readcharfun);
            }
            break;
        case '[': TODO;
        case ']': TODO;
        case '#': TODO;
        case '?': TODO;
        case '"':
            obj = read_string_literal (readcharfun);
            break;
        case '\'':
            read_stack_push ((struct read_stack_entry) {
                .type = RE_special,
                .u.special.symbol = Qquote,
            });
            goto read_obj;
        case '`': TODO;
        case ',': TODO;
        case ';':
            {
                int c;
                do
                c = READCHAR;
                while (c >= 0 && c != '\n');
                goto read_obj;
            }
        case '.': TODO;
        default:
            if (c <= 32 || c == NO_BREAK_SPACE)
                goto read_obj;

            uninterned_symbol = false;
            skip_shorthand = false;

            char *p = read_buffer;
            char *end = read_buffer + read_buffer_size;
            bool quoted = false;
            do
            {
                if (end - p < MAX_MULTIBYTE_LENGTH + 1)
                {
                    TODO;
                }

                if (c == '\\')
                {
                    c = READCHAR;
                    if (c < 0)
                        end_of_file_error ();
                    quoted = true;
                }

                if (multibyte)
                    p += CHAR_STRING (c, (unsigned char *) p);
                else
                    *p++ = c;
                c = READCHAR;
            }
            while (c > 32
            && c != NO_BREAK_SPACE
            && (c >= 128
            || !(   c == '"' || c == '\'' || c == ';' || c == '#'
            || c == '(' || c == ')'  || c == '[' || c == ']'
            || c == '`' || c == ',')));

            *p = 0;
            ptrdiff_t nbytes = p - read_buffer;
            UNREAD (c);

            char c0 = read_buffer[0];
            if (((c0 >= '0' && c0 <= '9') || c0 == '.' || c0 == '-' || c0 == '+')
                && !quoted && !uninterned_symbol && !skip_shorthand)
            {
                ptrdiff_t len;
                Lisp_Object result = string_to_number (read_buffer, 10, &len);
                if (!NILP (result) && len == nbytes)
                {
                    obj = result;
                    break;
                }
            }

#if TODO_NELISP_LATER_AND
        ptrdiff_t nchars
            = (multibyte
            ? multibyte_chars_in_text ((unsigned char *)read_buffer, nbytes)
            : nbytes);
#else
        ptrdiff_t nchars = nbytes;
#endif
        Lisp_Object result;
        if (uninterned_symbol)
        {
            TODO;
        } else {
            Lisp_Object obarray = check_obarray (Vobarray);

            char *longhand = NULL;
            Lisp_Object found;
#if TODO_NELISP_LATER_AND
            ptrdiff_t longhand_chars = 0;
            ptrdiff_t longhand_bytes = 0;

            if (skip_shorthand || symbol_char_span (read_buffer) >= nbytes)
                found = oblookup (obarray, read_buffer, nchars, nbytes);
            else
                found = oblookup_considering_shorthand (obarray, read_buffer,
                                                        nchars, nbytes, &longhand,
                                                        &longhand_chars,
                                                        &longhand_bytes);
#else
            found = oblookup (obarray, read_buffer, nchars, nbytes);
#endif

            if (BARE_SYMBOL_P (found))
                result = found;
            else if (longhand) {
                TODO;
            } else {
                Lisp_Object name = make_specified_string (read_buffer, nchars,
                                                          nbytes, multibyte);
                result = intern_driver (name, obarray, found);
            }
        }
        if (locate_syms && !NILP (result))
            TODO;

                obj = result;
        break;
    }

    while (rdstack.sp > base_sp)
    {

        struct read_stack_entry *e = read_stack_top ();
        switch (e->type)
        {
            case RE_list_start:
                e->type = RE_list;
                e->u.list.head = e->u.list.tail = Fcons (obj, Qnil);
                goto read_obj;
            case RE_list:
                {
                    Lisp_Object tl = Fcons (obj, Qnil);
                    XSETCDR (e->u.list.tail, tl);
                    e->u.list.tail = tl;
                    goto read_obj;
                }
            case RE_list_dot: TODO;
            case RE_vector:
            case RE_record:
            case RE_char_table:
            case RE_sub_char_table:
            case RE_byte_code:
            case RE_string_props:
                TODO;
            case RE_special:
                read_stack_pop ();
                obj = list2 (e->u.special.symbol, obj);
                break;
            case RE_numbered: TODO;
        }
    }
  return unbind_to (base_pdl, obj);
}
void
mark_lread (void)
{
    for (ptrdiff_t i = 0; i < rdstack.sp; i++)
    {
        struct read_stack_entry *e = &rdstack.stack[i];
        switch (e->type)
        {
            case RE_list_start:
                break;
            case RE_list:
            case RE_list_dot:
                mark_object (e->u.list.head);
                mark_object (e->u.list.tail);
                break;
            case RE_vector:
            case RE_record:
            case RE_char_table:
            case RE_sub_char_table:
            case RE_byte_code:
            case RE_string_props:
                mark_object (e->u.vector.elems);
                break;
            case RE_special:
                mark_object (e->u.special.symbol);
                break;
            case RE_numbered:
                mark_object (e->u.numbered.number);
                mark_object (e->u.numbered.placeholder);
                break;
        }
    }
}


void
syms_of_lread (void) {
    DEFVAR_LISP ("obarray", Vobarray,
                 doc: /* Symbol table for use by `intern' and `read'.
It is a vector whose length ought to be prime for best results.
The vector's contents don't make sense if examined from Lisp programs;
to find all the symbols in an obarray, use `mapatoms'.  */);

    DEFSYM (Qobarray_cache, "obarray-cache");

    defsubr (&Sintern);
}
