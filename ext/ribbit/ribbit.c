#include "ruby.h"
#include <git/oid.h>

static VALUE rb_cRibbit;
static VALUE rb_cRibbitLib;

static VALUE
hex_to_oid(VALUE self, VALUE hex)
{
  Check_Type(hex, T_STRING);
  git_oid *oid;
  git_oid_mkstr(oid, RSTRING_PTR(hex));
  return rb_str_new2(oid->id);
}

// git_oid_mkstr(&id1, hello_id);
// must_be_true(git_oid_cmp(&id1, &id2) == 0);

void
Init_ribbit()
{
  rb_cRibbit = rb_define_class("Ribbit", rb_cObject);
  rb_cRibbitLib = rb_define_class_under(rb_cRibbit, "Lib", rb_cObject);

  rb_define_module_function(rb_cRibbitLib, "hex_to_oid", hex_to_oid, 1);
}

