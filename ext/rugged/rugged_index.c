/*
 * The MIT License
 *
 * Copyright (c) 2011 GitHub, Inc
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "rugged.h"

extern VALUE rb_mRugged;
VALUE rb_cRuggedIndex;

static void rb_git_indexentry_toC(git_index_entry *entry, VALUE rb_entry);
static VALUE rb_git_indexentry_fromC(git_index_entry *entry);
static VALUE rb_git_unmerged_fromC(const git_index_entry_unmerged *entry);

/*
 * Index
 */

void rb_git_index__free(git_index *index)
{
	git_index_free(index);
}

VALUE rugged_index_new(VALUE klass, VALUE owner, git_index *index)
{
	VALUE rb_index = Data_Wrap_Struct(klass, NULL, &rb_git_index__free, index);
	rugged_set_owner(rb_index, owner);
	return rb_index;
}

static VALUE rb_git_index_new(VALUE klass, VALUE path)
{
	git_index *index;
	int error;

	Check_Type(path, T_STRING);

	error = git_index_open(&index, StringValueCStr(path));
	rugged_exception_check(error);

	return rugged_index_new(klass, Qnil, index);
}

static VALUE rb_git_index_clear(VALUE self)
{
	git_index *index;
	Data_Get_Struct(self, git_index, index);
	git_index_clear(index);
	return Qnil;
}

static VALUE rb_git_index_read(VALUE self)
{
	git_index *index;
	int error;

	Data_Get_Struct(self, git_index, index);

	error = git_index_read(index);
	rugged_exception_check(error);

	return Qnil;
}

static VALUE rb_git_index_write(VALUE self)
{
	git_index *index;
	int error;

	Data_Get_Struct(self, git_index, index);

	error = git_index_write(index);
	rugged_exception_check(error);
		
	return Qnil;
}

static VALUE rb_git_index_uniq(VALUE self)
{
	git_index *index;
	Data_Get_Struct(self, git_index, index);
	git_index_uniq(index);
	return Qnil;
}

static VALUE rb_git_index_count(VALUE self)
{
	git_index *index;
	Data_Get_Struct(self, git_index, index);
	return INT2FIX(git_index_entrycount(index));
}

static VALUE rb_git_index_count_unmerged(VALUE self)
{
	git_index *index;
	Data_Get_Struct(self, git_index, index);
	return INT2FIX(git_index_entrycount_unmerged(index));
}

static VALUE rb_git_index_get(VALUE self, VALUE entry)
{
	git_index *index;
	Data_Get_Struct(self, git_index, index);

	if (TYPE(entry) == T_STRING)
		entry = INT2FIX(git_index_find(index, StringValueCStr(entry)));

	Check_Type(entry, T_FIXNUM);
	return rb_git_indexentry_fromC(git_index_get(index, FIX2INT(entry)));
}

static VALUE rb_git_index_each(VALUE self)
{
	git_index *index;
	unsigned int i, count;

	Data_Get_Struct(self, git_index, index);

	if (!rb_block_given_p())
		return rb_funcall(self, rb_intern("to_enum"), 0);

	count = git_index_entrycount(index);
	for (i = 0; i < count; ++i) {
		git_index_entry *entry = git_index_get(index, i);
		if (entry)
			rb_yield(rb_git_indexentry_fromC(entry));
	}

	return Qnil;
}

static VALUE rb_git_index_get_unmerged(VALUE self, VALUE entry)
{
	git_index *index;
	Data_Get_Struct(self, git_index, index);

	if (TYPE(entry) == T_STRING)
		return rb_git_unmerged_fromC(git_index_get_unmerged_bypath(index, StringValueCStr(entry)));

	if (TYPE(entry) == T_FIXNUM)
		return rb_git_unmerged_fromC(git_index_get_unmerged_byindex(index, FIX2INT(entry)));

	rb_raise(rb_eTypeError, "Expecting a path name or index for unmerged entries");
}

static VALUE rb_git_index_remove(VALUE self, VALUE entry)
{
	git_index *index;
	int error;
	Data_Get_Struct(self, git_index, index);

	if (TYPE(entry) == T_STRING)
		entry = INT2FIX(git_index_find(index, StringValueCStr(entry)));

	Check_Type(entry, T_FIXNUM);
	
	error = git_index_remove(index, FIX2INT(entry));
	rugged_exception_check(error);

	return Qnil;
}

static VALUE rb_git_index_add(int argc, VALUE *argv, VALUE self)
{
	git_index *index;
	int error;
	VALUE rb_entry, rb_stage;

	rb_scan_args(argc, argv, "11", &rb_entry, &rb_stage);

	Data_Get_Struct(self, git_index, index);

	if (TYPE(rb_entry) == T_HASH) {
		git_index_entry entry;
		if (argc > 1)
			rb_raise(rb_eTypeError,
				"Wrong number or arguments (only an Index Entry is expected");

		rb_git_indexentry_toC(&entry, rb_entry);
		error = git_index_add2(index, &entry);
	} else if (TYPE(rb_entry) == T_STRING) {
		int stage = 0;
		if (!NIL_P(rb_stage))
			stage = NUM2INT(rb_stage);
		error = git_index_add(index, StringValueCStr(rb_entry), stage);
	} else {
		rb_raise(rb_eTypeError, 
			"Expecting a hash defining an Index Entry or a path to a file in the repository");
	}

	rugged_exception_check(error);
	return Qnil;
}

static VALUE rb_git_index_append(int argc, VALUE *argv, VALUE self)
{
	git_index *index;
	int error;
	VALUE rb_entry, rb_stage;

	rb_scan_args(argc, argv, "11", &rb_entry, &rb_stage);

	Data_Get_Struct(self, git_index, index);

	if (TYPE(rb_entry) == T_HASH) {
		git_index_entry entry;
		rb_git_indexentry_toC(&entry, rb_entry);
		error = git_index_append2(index, &entry);
	} else if (TYPE(rb_entry) == T_STRING) {
		Check_Type(rb_stage, T_FIXNUM);
		error = git_index_append(index, StringValueCStr(rb_entry), FIX2INT(rb_stage));
	} else {
		rb_raise(rb_eTypeError,
			"Expecting a hash defining an Index Entry or a path to a file in the repository");
	}

	rugged_exception_check(error);
	return Qnil;
}


/*
 * Index Entry
 */
static VALUE rb_git_unmerged_fromC(const git_index_entry_unmerged *entry)
{
	int i;
	VALUE rb_entry, rb_modes, rb_oids;

	if (!entry)
		return Qnil;

	rb_modes = rb_ary_new2(3);
	rb_oids = rb_ary_new2(3);
	for (i = 0; i < 3; ++i) {
		rb_ary_push(rb_modes, INT2FIX(entry->mode[i]));
		rb_ary_push(rb_oids, rugged_create_oid(&entry->oid[i]));
	}

	rb_entry = rb_hash_new();
	rb_hash_aset(rb_entry, CSTR2SYM("path"), rugged_str_new2(entry->path, NULL));
	rb_hash_aset(rb_entry, CSTR2SYM("oids"), rb_oids);
	rb_hash_aset(rb_entry, CSTR2SYM("modes"), rb_modes);

	return rb_entry;
}



static VALUE rb_git_indexentry_fromC(git_index_entry *entry)
{
	VALUE rb_entry, rb_mtime, rb_ctime;
	unsigned int valid, stage;

	if (!entry)
		return Qnil;

	rb_entry = rb_hash_new();

	rb_hash_aset(rb_entry, CSTR2SYM("path"), rugged_str_new2(entry->path, NULL));
	rb_hash_aset(rb_entry, CSTR2SYM("oid"), rugged_create_oid(&entry->oid));

	rb_hash_aset(rb_entry, CSTR2SYM("dev"), INT2FIX(entry->dev));
	rb_hash_aset(rb_entry, CSTR2SYM("ino"), INT2FIX(entry->ino));
	rb_hash_aset(rb_entry, CSTR2SYM("mode"), INT2FIX(entry->mode));
	rb_hash_aset(rb_entry, CSTR2SYM("gid"), INT2FIX(entry->gid));
	rb_hash_aset(rb_entry, CSTR2SYM("uid"), INT2FIX(entry->uid));
	rb_hash_aset(rb_entry, CSTR2SYM("file_size"), INT2FIX(entry->file_size));

	valid = (entry->flags & GIT_IDXENTRY_VALID);
	rb_hash_aset(rb_entry, CSTR2SYM("valid"), valid ? Qtrue : Qfalse);

	stage = (entry->flags & GIT_IDXENTRY_STAGEMASK) >> GIT_IDXENTRY_STAGESHIFT;
	rb_hash_aset(rb_entry, CSTR2SYM("stage"), INT2FIX(stage));

	rb_mtime = rb_time_new(entry->mtime.seconds, entry->mtime.nanoseconds / 1000);
	rb_ctime = rb_time_new(entry->ctime.seconds, entry->ctime.nanoseconds / 1000);

	rb_hash_aset(rb_entry, CSTR2SYM("ctime"), rb_ctime);
	rb_hash_aset(rb_entry, CSTR2SYM("mtime"), rb_mtime);

	return rb_entry;
}

static void rb_git_indexentry_toC(git_index_entry *entry, VALUE rb_entry)
{
	VALUE val;

	Check_Type(rb_entry, T_HASH);

#define GET_ENTRY_VAL(v, t) \
	rb_hash_aref(rb_entry, CSTR2SYM(v)); \
	Check_Type(val, t);

	val = GET_ENTRY_VAL("path", T_STRING);
	entry->path = StringValueCStr(val);

	val = GET_ENTRY_VAL("oid", T_STRING);
	rugged_exception_check(git_oid_fromstr(&entry->oid, StringValueCStr(val)));

	val = GET_ENTRY_VAL("dev", T_FIXNUM);
	entry->dev = FIX2INT(val);

	val = GET_ENTRY_VAL("ino", T_FIXNUM);
	entry->ino = FIX2INT(val);

	val = GET_ENTRY_VAL("mode", T_FIXNUM);
	entry->mode = FIX2INT(val);

	val = GET_ENTRY_VAL("gid", T_FIXNUM);
	entry->gid = FIX2INT(val);

	val = GET_ENTRY_VAL("uid", T_FIXNUM);
	entry->uid = FIX2INT(val);

	val = GET_ENTRY_VAL("file_size", T_FIXNUM);
	entry->file_size = FIX2INT(val);

	entry->flags = 0x0;
	entry->flags_extended = 0x0;

	val = rb_hash_aref(rb_entry, CSTR2SYM("stage"));
	if (!NIL_P(val)) {
		unsigned int stage = NUM2INT(val);
		entry->flags &= ~GIT_IDXENTRY_STAGEMASK;
		entry->flags |= (stage << GIT_IDXENTRY_STAGESHIFT) & GIT_IDXENTRY_STAGEMASK;
	}

	val = rb_hash_aref(rb_entry, CSTR2SYM("valid"));
	if (!NIL_P(val)) {
		entry->flags &= ~GIT_IDXENTRY_VALID;
		if (rugged_parse_bool(val))
			entry->flags |= GIT_IDXENTRY_VALID;
	} /* TODO: always valid by default? */

	val = rb_hash_aref(rb_entry, CSTR2SYM("mtime"));
	if (!rb_obj_is_kind_of(val, rb_cTime))
		rb_raise(rb_eTypeError, ":mtime must be a Time instance");
	entry->mtime.seconds = NUM2INT(rb_funcall(val, rb_intern("to_i"), 0));
	entry->mtime.nanoseconds = NUM2INT(rb_funcall(val, rb_intern("usec"), 0)) * 1000;

	val = rb_hash_aref(rb_entry, CSTR2SYM("ctime"));
	if (!rb_obj_is_kind_of(val, rb_cTime))
		rb_raise(rb_eTypeError, ":ctime must be a Time instance");
	entry->ctime.seconds = NUM2INT(rb_funcall(val, rb_intern("to_i"), 0));
	entry->ctime.nanoseconds = NUM2INT(rb_funcall(val, rb_intern("usec"), 0)) * 1000;

#undef GET_ENTRY_VAL
}

VALUE rb_git_indexer(VALUE self, VALUE rb_packfile_path)
{
	int error;
	git_indexer *indexer;
	VALUE rb_oid;

	Check_Type(rb_packfile_path, T_STRING);

	error = git_indexer_new(&indexer, StringValueCStr(rb_packfile_path));
	rugged_exception_check(error);

	error = git_indexer_run(indexer, NULL);
	rugged_exception_check(error);

	error = git_indexer_write(indexer);
	rugged_exception_check(error);

	rb_oid = rugged_create_oid(git_indexer_hash(indexer));

	git_indexer_free(indexer);
	return rb_oid;
}

VALUE rb_git_index_writetree(VALUE self)
{
	git_index *index;
	git_oid tree_oid;
	int error;

	Data_Get_Struct(self, git_index, index);

	error = git_tree_create_fromindex(&tree_oid, index);
	rugged_exception_check(error);

	return rugged_create_oid(&tree_oid);
}

void Init_rugged_index()
{
	/*
	 * Index 
	 */
	rb_cRuggedIndex = rb_define_class_under(rb_mRugged, "Index", rb_cObject);
	rb_define_singleton_method(rb_cRuggedIndex, "new", rb_git_index_new, 1);

	rb_define_method(rb_cRuggedIndex, "count", rb_git_index_count, 0);
	rb_define_method(rb_cRuggedIndex, "count_unmerged", rb_git_index_count_unmerged, 0);
	rb_define_method(rb_cRuggedIndex, "reload", rb_git_index_read, 0);
	rb_define_method(rb_cRuggedIndex, "clear", rb_git_index_clear, 0);
	rb_define_method(rb_cRuggedIndex, "write", rb_git_index_write, 0);
	rb_define_method(rb_cRuggedIndex, "uniq!", rb_git_index_uniq, 0);
	rb_define_method(rb_cRuggedIndex, "get_entry", rb_git_index_get, 1);
	rb_define_method(rb_cRuggedIndex, "get_unmerged", rb_git_index_get_unmerged, 1);
	rb_define_method(rb_cRuggedIndex, "[]", rb_git_index_get, 1);
	rb_define_method(rb_cRuggedIndex, "each", rb_git_index_each, 0);

	rb_define_method(rb_cRuggedIndex, "add", rb_git_index_add, -1);
	rb_define_method(rb_cRuggedIndex, "update", rb_git_index_add, -1);

	rb_define_method(rb_cRuggedIndex, "append", rb_git_index_append, -1);
	rb_define_method(rb_cRuggedIndex, "<<", rb_git_index_append, -1);

	rb_define_method(rb_cRuggedIndex, "remove", rb_git_index_remove, 1);
	rb_define_method(rb_cRuggedIndex, "write_tree", rb_git_index_writetree, 0);

	rb_define_singleton_method(rb_cRuggedIndex, "index_pack", rb_git_indexer, 1);

	rb_const_set(rb_cRuggedIndex, rb_intern("ENTRY_FLAGS_STAGE"), INT2FIX(GIT_IDXENTRY_STAGEMASK));
	rb_const_set(rb_cRuggedIndex, rb_intern("ENTRY_FLAGS_STAGE_SHIFT"), INT2FIX(GIT_IDXENTRY_STAGESHIFT));
	rb_const_set(rb_cRuggedIndex, rb_intern("ENTRY_FLAGS_VALID"), INT2FIX(GIT_IDXENTRY_VALID));
}
