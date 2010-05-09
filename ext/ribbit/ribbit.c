#include "ruby.h"
#include <git/oid.h>
#include <git/odb.h>

static VALUE rb_cRibbit;

/*
 * Ribbit Lib
 */

static VALUE rb_cRibbitLib;

static VALUE rb_git_hex_to_raw(VALUE self, VALUE hex)
{
  Check_Type(hex, T_STRING);
  git_oid oid;
  git_oid_mkstr(&oid, RSTRING_PTR(hex));
  return rb_str_new((&oid)->id, 20);
}

static VALUE rb_git_raw_to_hex(VALUE self, VALUE raw)
{
  Check_Type(raw, T_STRING);
  git_oid oid;
  git_oid_mkraw(&oid, RSTRING_PTR(raw));
  char out[40];
  git_oid_fmt(out, &oid);
  return rb_str_new(out, 40);
}

/*
 * Ribbit Object Database
 */

static VALUE rb_cRibbitOdb;

static VALUE rb_git_odb_init(VALUE self, VALUE path) {
  rb_iv_set(self, "@path", path);

  git_odb *odb;
  git_odb_open(&odb, RSTRING_PTR(path));

  rb_iv_set(self, "odb", (VALUE)odb);
}

static VALUE rb_git_odb_exists(VALUE self, VALUE hex) {
  git_odb *odb;
  odb = (git_odb*)rb_iv_get(self, "odb");

  git_oid oid;
  git_oid_mkstr(&oid, RSTRING_PTR(hex));

  int exists = git_odb_exists(odb, &oid);
  if(exists == 1) {
    return Qtrue;
  }
  return Qfalse;
}

/*
typedef struct {
	void *data;     // Raw, decompressed object data.
	size_t len;     // Total number of bytes in data.
	git_otype type; // Type of this object.
} git_obj;
*/

static VALUE rb_git_odb_read(VALUE self, VALUE hex) {
  git_odb *odb;
  odb = (git_odb*)rb_iv_get(self, "odb");

  git_oid oid;
  git_oid_mkstr(&oid, RSTRING_PTR(hex));

  git_obj obj;

  int read = git_odb_read(&obj, odb, &oid);
  unsigned char *data = (&obj)->data;

  if(read == GIT_SUCCESS) {
    return rb_str_new2(data);
  }
  return Qfalse;
}
// GIT_EXTERN(int) git_obj_hash(git_oid *id, git_obj *obj);
// GIT_EXTERN(int) git_odb_write(git_oid *id, git_odb *db, git_obj *obj);
// GIT_EXTERN(void) git_odb_close(git_odb *db);

//GIT_EXTERN(const char *) git_obj_type_to_string(git_otype type);
//GIT_EXTERN(git_otype) git_obj_string_to_type(const char *str);


/*
 * Ribbit Revwalking - almost none of this is implemented yet in libgit2
 */

//   GIT_EXTERN(git_revpool *) gitrp_alloc(git_odb *db);
//   GIT_EXTERN(void) gitrp_reset(git_revpool *pool);
//   GIT_EXTERN(void) gitrp_push(git_revpool *pool, git_commit *commit);
//   GIT_EXTERN(void) gitrp_hide(git_revpool *pool, git_commit *commit);
//   GIT_EXTERN(git_commit *) gitrp_next(git_revpool *pool);
//   GIT_EXTERN(void) gitrp_free(git_revpool *pool);


/*
 * Ribbit Init Call
 */

void
Init_ribbit()
{
  rb_cRibbit = rb_define_class("Ribbit", rb_cObject);
  rb_cRibbitLib = rb_define_class_under(rb_cRibbit, "Lib", rb_cObject);

  rb_define_module_function(rb_cRibbitLib, "hex_to_raw", rb_git_hex_to_raw, 1);
  rb_define_module_function(rb_cRibbitLib, "raw_to_hex", rb_git_raw_to_hex, 1);

  rb_cRibbitOdb = rb_define_class_under(rb_cRibbit, "Odb", rb_cObject);
  rb_define_method(rb_cRibbitOdb, "initialize", rb_git_odb_init, 1);
  rb_define_method(rb_cRibbitOdb, "exists", rb_git_odb_exists, 1);
  rb_define_method(rb_cRibbitOdb, "read", rb_git_odb_read, 1);
}

