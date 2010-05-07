#include "ruby.h"

static VALUE rb_cRibbit;

static VALUE
print_test(VALUE self)
{
  int test = git__prefixcmp("aa", "aa");
  printf("test: %d\n", test);
 
  return Qnil;
}

void
Init_ribbit()
{
  rb_cRibbit = rb_define_class("Ribbit", rb_cObject);

  rb_define_method(rb_cRibbit, "print_test", print_test, 0);
}

