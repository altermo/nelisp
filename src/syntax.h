#ifndef EMACS_SYNTAX_H
#define EMACS_SYNTAX_H

enum syntaxcode
{
  Swhitespace,
  Spunct,
  Sword,
  Ssymbol,
  Sopen,
  Sclose,
  Squote_,
  Sstring,
  Smath,
  Sescape,
  Scharquote,
  Scomment,
  Sendcomment,
  Sinherit,
  Scomment_fence,
  Sstring_fence,
  Smax_
};

extern unsigned char const syntax_spec_code[0400];

#endif
