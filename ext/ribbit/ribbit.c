#include "ruby.h"

static VALUE rb_cRibbit;

static VALUE
compare_prefix(VALUE self, VALUE string1, VALUE string2)
{
  return git__prefixcmp(string1, string2);
}

void
Init_ribbit()
{
  rb_cRibbit = rb_define_class("Ribbit", rb_cObject);

  rb_define_method(rb_cRibbit, "compare_prefix", compare_prefix, 2);
}

