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
static VALUE rb_git_reuc_fromC(const git_index_reuc_entry *entry);

/*
 * Index
 */

static void rb_git_index__free(git_index *index)
{
	git_index_free(index);
}

VALUE rugged_index_new(VALUE klass, VALUE owner, git_index *index)
{
	VALUE rb_index = Data_Wrap_Struct(klass, NULL, &rb_git_index__free, index);
	rugged_set_owner(rb_index, owner);
	return rb_index;
}

static VALUE rb_git_index_new(int argc, VALUE *argv, VALUE klass)
{
	git_index *index;
	int error;

	VALUE rb_path;
	const char *path = NULL;

	if (rb_scan_args(argc, argv, "01", &rb_path) == 1) {
		Check_Type(rb_path, T_STRING);
		path = StringValueCStr(rb_path);
	}

	error = git_index_open(&index, path);
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

static VALUE rb_git_index_count(VALUE self)
{
	git_index *index;
	Data_Get_Struct(self, git_index, index);
	return INT2FIX(git_index_entrycount(index));
}


static VALUE rb_git_index_get(int argc, VALUE *argv, VALUE self)
{
	git_index *index;
	git_index_entry *entry = NULL;

	int error;

	VALUE rb_entry, rb_stage;

	Data_Get_Struct(self, git_index, index);

	rb_scan_args(argc, argv, "11", &rb_entry, &rb_stage);

	if (TYPE(rb_entry) == T_STRING) {
		int stage = 0;

		if (!NIL_P(rb_stage)) {
			Check_Type(rb_stage, T_FIXNUM);
			stage = FIX2INT(rb_stage);
		}

		entry = git_index_get_bypath(index, StringValueCStr(rb_entry), stage);
	}

	else if (TYPE(rb_entry) == T_FIXNUM) {
		if (argc > 1) {
			rb_raise(rb_eArgError,
				"Too many arguments when trying to lookup entry by index");
		}

		entry = git_index_get_byindex(index, FIX2INT(rb_entry));
	} else {
		rb_raise(rb_eArgError,
			"Invalid type for `entry`: expected String or Fixnum");
	}

	return entry ? rb_git_indexentry_fromC(entry) : Qnil;
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
		git_index_entry *entry = git_index_get_byindex(index, i);
		if (entry)
			rb_yield(rb_git_indexentry_fromC(entry));
	}

	return Qnil;
}

static VALUE rb_git_index_remove(int argc, VALUE *argv, VALUE self)
{
	git_index *index;
	int error, stage = 0;

	VALUE rb_entry, rb_stage;

	Data_Get_Struct(self, git_index, index);

	if (rb_scan_args(argc, argv, "11", &rb_entry, &rb_stage) > 1) {
		Check_Type(rb_stage, T_FIXNUM);
		stage = FIX2INT(rb_stage);
	}

	Check_Type(rb_entry, T_STRING);

	error = git_index_remove(index, StringValueCStr(rb_entry), stage);
	rugged_exception_check(error);

	return Qnil;
}

static VALUE rb_git_index_add(VALUE self, VALUE rb_entry)
{
	git_index *index;
	int error = 0;

	Data_Get_Struct(self, git_index, index);

	if (TYPE(rb_entry) == T_HASH) {
		git_index_entry entry;

		rb_git_indexentry_toC(&entry, rb_entry);
		error = git_index_add(index, &entry);
	}

	else if (TYPE(rb_entry) == T_STRING) {
		error = git_index_add_from_workdir(index, StringValueCStr(rb_entry));
	}

	else {
		rb_raise(rb_eTypeError, 
			"Expecting a hash defining an Index Entry or a path to a file in the repository");
	}

	rugged_exception_check(error);
	return Qnil;
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

static inline unsigned int
default_entry_value(VALUE rb_entry, const char *key)
{
	VALUE val = rb_hash_aref(rb_entry, CSTR2SYM(key));
	if (NIL_P(val))
		return 0;

	Check_Type(val, T_FIXNUM);
	return FIX2INT(val);
}

static void rb_git_indexentry_toC(git_index_entry *entry, VALUE rb_entry)
{
	VALUE val;

	Check_Type(rb_entry, T_HASH);

	val = rb_hash_aref(rb_entry, CSTR2SYM("path"));
	Check_Type(val, T_STRING);
	entry->path = StringValueCStr(val);

	val = rb_hash_aref(rb_entry, CSTR2SYM("oid"));
	Check_Type(val, T_STRING);
	rugged_exception_check(
		git_oid_fromstr(&entry->oid, StringValueCStr(val))
	);

	entry->dev = default_entry_value(rb_entry, "dev");
	entry->ino = default_entry_value(rb_entry, "ino");
	entry->mode = default_entry_value(rb_entry, "mode");
	entry->gid = default_entry_value(rb_entry, "gid");
	entry->uid = default_entry_value(rb_entry, "uid");
	entry->file_size = (git_off_t)default_entry_value(rb_entry, "file_size");

	if ((val = rb_hash_aref(rb_entry, CSTR2SYM("mtime"))) != Qnil) {
		if (!rb_obj_is_kind_of(val, rb_cTime))
			rb_raise(rb_eTypeError, ":mtime must be a Time instance");

		entry->mtime.seconds = NUM2INT(rb_funcall(val, rb_intern("to_i"), 0));
		entry->mtime.nanoseconds = NUM2INT(rb_funcall(val, rb_intern("usec"), 0)) * 1000;
	} else {
		entry->mtime.seconds = entry->mtime.nanoseconds = 0;
	}

	if ((val = rb_hash_aref(rb_entry, CSTR2SYM("ctime"))) != Qnil) {
		if (!rb_obj_is_kind_of(val, rb_cTime))
			rb_raise(rb_eTypeError, ":ctime must be a Time instance");

		entry->ctime.seconds = NUM2INT(rb_funcall(val, rb_intern("to_i"), 0));
		entry->ctime.nanoseconds = NUM2INT(rb_funcall(val, rb_intern("usec"), 0)) * 1000;
	} else {
		entry->ctime.seconds = entry->ctime.nanoseconds = 0;
	}

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
	} else {
		entry->flags |= GIT_IDXENTRY_VALID;
	}
}

static VALUE rb_git_indexer(VALUE self, VALUE rb_packfile_path)
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

static VALUE rb_git_index_writetree(int argc, VALUE *argv, VALUE self)
{
	git_index *index;
	git_oid tree_oid;
	int error;
	VALUE rb_repo;

	Data_Get_Struct(self, git_index, index);

	if (rb_scan_args(argc, argv, "01", &rb_repo) == 1) {
		git_repository *repo = NULL;
		rugged_check_repo(rb_repo);
		Data_Get_Struct(rb_repo, git_repository, repo);
		error = git_index_write_tree_to(&tree_oid, index, repo);
	}
	else {
		error = git_index_write_tree(&tree_oid, index);
	}

	rugged_exception_check(error);
	return rugged_create_oid(&tree_oid);
}

/*
 *	call-seq:
 *		index.read_tree(tree)
 *
 *      Clear the current index and start the index again on top of +tree+
 *
 *      Further index operations (+add+, +update+, +remove+, etc) will
 *      be considered changes on top of +tree+.
 */
static VALUE rb_git_index_readtree(VALUE self, VALUE rb_tree)
{
	git_index *index;
	git_tree *tree;
	int error;

	Data_Get_Struct(self, git_index, index);
	Data_Get_Struct(rb_tree, git_tree, tree);

	error = git_index_read_tree(index, tree);
	rugged_exception_check(error);

	return Qnil;
}

void Init_rugged_index()
{
	/*
	 * Index
	 */
	rb_cRuggedIndex = rb_define_class_under(rb_mRugged, "Index", rb_cObject);
	rb_define_singleton_method(rb_cRuggedIndex, "new", rb_git_index_new, -1);

	rb_define_method(rb_cRuggedIndex, "count", rb_git_index_count, 0);
	rb_define_method(rb_cRuggedIndex, "reload", rb_git_index_read, 0);
	rb_define_method(rb_cRuggedIndex, "clear", rb_git_index_clear, 0);
	rb_define_method(rb_cRuggedIndex, "write", rb_git_index_write, 0);
	rb_define_method(rb_cRuggedIndex, "get", rb_git_index_get, -1);
	rb_define_method(rb_cRuggedIndex, "[]", rb_git_index_get, -1);
	rb_define_method(rb_cRuggedIndex, "each", rb_git_index_each, 0);

	rb_define_method(rb_cRuggedIndex, "add", rb_git_index_add, 1);
	rb_define_method(rb_cRuggedIndex, "update", rb_git_index_add, 1);
	rb_define_method(rb_cRuggedIndex, "<<", rb_git_index_add, 1);

	rb_define_method(rb_cRuggedIndex, "remove", rb_git_index_remove, -1);

	rb_define_method(rb_cRuggedIndex, "write_tree", rb_git_index_writetree, -1);
	rb_define_method(rb_cRuggedIndex, "read_tree", rb_git_index_readtree, 1);

	rb_define_singleton_method(rb_cRuggedIndex, "index_pack", rb_git_indexer, 1);

	rb_const_set(rb_cRuggedIndex, rb_intern("ENTRY_FLAGS_STAGE"), INT2FIX(GIT_IDXENTRY_STAGEMASK));
	rb_const_set(rb_cRuggedIndex, rb_intern("ENTRY_FLAGS_STAGE_SHIFT"), INT2FIX(GIT_IDXENTRY_STAGESHIFT));
	rb_const_set(rb_cRuggedIndex, rb_intern("ENTRY_FLAGS_VALID"), INT2FIX(GIT_IDXENTRY_VALID));
}
