#include "ruby.h"
#include <git/oid.h>

static VALUE rb_cRibbit;
static VALUE rb_cRibbitLib;

static VALUE
hex_to_raw(VALUE self, VALUE hex)
{
  Check_Type(hex, T_STRING);
  git_oid oid;
  git_oid_mkstr(&oid, RSTRING_PTR(hex));
  return rb_str_new((&oid)->id, 20);
}

static VALUE
raw_to_hex(VALUE self, VALUE raw)
{
  Check_Type(raw, T_STRING);
  git_oid oid;
  git_oid_mkraw(&oid, RSTRING_PTR(raw));
  char out[40];
  git_oid_fmt(out, &oid);
  return rb_str_new(out, 40);
}

// git_oid_mkstr(&id1, hello_id);
// must_be_true(git_oid_cmp(&id1, &id2) == 0);

void
Init_ribbit()
{
  rb_cRibbit = rb_define_class("Ribbit", rb_cObject);
  rb_cRibbitLib = rb_define_class_under(rb_cRibbit, "Lib", rb_cObject);

  rb_define_module_function(rb_cRibbitLib, "hex_to_raw", hex_to_raw, 1);
  rb_define_module_function(rb_cRibbitLib, "raw_to_hex", raw_to_hex, 1);
}

