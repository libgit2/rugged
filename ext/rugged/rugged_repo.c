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
extern VALUE rb_cRuggedIndex;
extern VALUE rb_cRuggedConfig;
extern VALUE rb_cRuggedBackend;

VALUE rb_cRuggedRepo;
VALUE rb_cRuggedOdbObject;

static VALUE rb_git_odbobj_hash(VALUE self)
{
	git_odb_object *obj;
	Data_Get_Struct(self, git_odb_object, obj);
	return rugged_create_oid(git_odb_object_id(obj));
}

static VALUE rb_git_odbobj_data(VALUE self)
{
	git_odb_object *obj;
	Data_Get_Struct(self, git_odb_object, obj);
	return rugged_str_ascii(git_odb_object_data(obj), git_odb_object_size(obj));
}

static VALUE rb_git_odbobj_size(VALUE self)
{
	git_odb_object *obj;
	Data_Get_Struct(self, git_odb_object, obj);
	return INT2FIX(git_odb_object_size(obj));
}

static VALUE rb_git_odbobj_type(VALUE self)
{
	git_odb_object *obj;
	Data_Get_Struct(self, git_odb_object, obj);
	return INT2FIX(git_odb_object_type(obj));
}

void rb_git__odbobj_free(void *obj)
{
	git_odb_object_free((git_odb_object *)obj);
}

VALUE rugged_raw_read(git_repository *repo, const git_oid *oid)
{
	git_odb *odb;
	git_odb_object *obj;

	int error;

	error = git_repository_odb(&odb, repo);
	rugged_exception_check(error);

	error = git_odb_read(&obj, odb, oid);
	rugged_exception_check(error);

	git_odb_free(odb);

	return Data_Wrap_Struct(rb_cRuggedOdbObject, NULL, rb_git__odbobj_free, obj);
}

void rb_git_repo__free(git_repository *repo)
{
	git_repository_free(repo);
}

static VALUE rugged_repo_new(VALUE klass, git_repository *repo)
{
	VALUE rb_repo = Data_Wrap_Struct(klass, NULL, &rb_git_repo__free, repo);

#ifdef HAVE_RUBY_ENCODING_H
	/* TODO: set this properly */
	rb_iv_set(rb_repo, "@encoding",
		rb_enc_from_encoding(rb_filesystem_encoding()));
#endif

	return rb_repo;
}

static VALUE rb_git_repo_new(VALUE klass, VALUE rb_path)
{
	git_repository *repo;
	int error = 0;

	Check_Type(rb_path, T_STRING);

	error = git_repository_open(&repo, StringValueCStr(rb_path));
	rugged_exception_check(error);

	return rugged_repo_new(klass, repo);
}

static VALUE rb_git_repo_init_at(VALUE klass, VALUE path, VALUE rb_is_bare)
{
	git_repository *repo;
	int error, is_bare;

	is_bare = rugged_parse_bool(rb_is_bare);
	Check_Type(path, T_STRING);

	error = git_repository_init(&repo, StringValueCStr(path), is_bare);
	rugged_exception_check(error);

	return rugged_repo_new(klass, repo);
}

#define RB_GIT_REPO_OWNED_ATTR(_klass, _object) \
	static VALUE rb_git_repo_get_##_object(VALUE self) \
	{ \
		VALUE rb_data = rb_iv_get(self, "@" #_object); \
		if (NIL_P(rb_data)) { \
			git_repository *repo; \
			git_##_object *data; \
			int error; \
			Data_Get_Struct(self, git_repository, repo); \
			error = git_repository_##_object(&data, repo); \
			rugged_exception_check(error); \
			rb_data = rugged_##_object##_new(_klass, self, data); \
			rb_iv_set(self, "@" #_object, rb_data); \
		} \
		return rb_data; \
	}\
	static VALUE rb_git_repo_set_##_object(VALUE self, VALUE rb_data) \
	{ \
		VALUE rb_old_data; \
		git_repository *repo; \
		git_##_object *data; \
		if (!rb_obj_is_kind_of(rb_data, _klass))\
			rb_raise(rb_eTypeError, \
				"The given object is not a Rugged::" #_object); \
		if (!NIL_P(rugged_owner(rb_data))) \
			rb_raise(rb_eRuntimeError, \
				"The given object is already owned by another repository"); \
		Data_Get_Struct(self, git_repository, repo); \
		Data_Get_Struct(rb_data, git_##_object, data); \
		git_repository_set_##_object(repo, data); \
		rb_old_data = rb_iv_get(self, "@" #_object); \
		if (!NIL_P(rb_old_data)) rugged_set_owner(rb_old_data, Qnil); \
		rugged_set_owner(rb_data, self); \
		rb_iv_set(self, "@" #_object, rb_data); \
		return Qnil; \
	}

/* git_repository_index, git_repository_set_index */
RB_GIT_REPO_OWNED_ATTR(rb_cRuggedIndex, index);

/* git_repository_config, git_repository_set_config */
RB_GIT_REPO_OWNED_ATTR(rb_cRuggedConfig, config);

static VALUE rb_git_repo_exists(VALUE self, VALUE hex)
{
	git_repository *repo;
	git_odb *odb;
	git_oid oid;
	int error;
	VALUE rb_result;

	Data_Get_Struct(self, git_repository, repo);
	Check_Type(hex, T_STRING);

	error = git_repository_odb(&odb, repo);
	rugged_exception_check(error);

	error = git_oid_fromstr(&oid, StringValueCStr(hex));
	rugged_exception_check(error);

	rb_result = git_odb_exists(odb, &oid) ? Qtrue : Qfalse;
	git_odb_free(odb);

	return rb_result;
}

static VALUE rb_git_repo_read(VALUE self, VALUE hex)
{
	git_repository *repo;
	git_oid oid;
	int error;

	Data_Get_Struct(self, git_repository, repo);
	Check_Type(hex, T_STRING);

	error = git_oid_fromstr(&oid, StringValueCStr(hex));
	rugged_exception_check(error);

	return rugged_raw_read(repo, &oid);
}

static VALUE rb_git_repo_hash(VALUE self, VALUE rb_buffer, VALUE rb_type)
{
	int error;
	git_oid oid;

	Check_Type(rb_buffer, T_STRING);

	error = git_odb_hash(&oid,
		RSTRING_PTR(rb_buffer),
		RSTRING_LEN(rb_buffer),
		rugged_otype_get(rb_type)
	);
	rugged_exception_check(error);

	return rugged_create_oid(&oid);
}

static VALUE rb_git_repo_hashfile(VALUE self, VALUE rb_path, VALUE rb_type)
{
	int error;
	git_oid oid;

	Check_Type(rb_path, T_STRING);

	error = git_odb_hashfile(&oid,
		StringValueCStr(rb_path),
		rugged_otype_get(rb_type)
	);
	rugged_exception_check(error);

	return rugged_create_oid(&oid);
}

static VALUE rb_git_repo_write(VALUE self, VALUE rb_buffer, VALUE rub_type)
{
	git_repository *repo;
	git_odb_stream *stream;

	git_odb *odb;
	git_oid oid;
	int error;

	git_otype type;

	Data_Get_Struct(self, git_repository, repo);

	error = git_repository_odb(&odb, repo);
	rugged_exception_check(error);

	type = rugged_otype_get(rub_type);

	error = git_odb_open_wstream(&stream, odb, RSTRING_LEN(rb_buffer), type);
	rugged_exception_check(error);

	git_odb_free(odb);

	error = stream->write(stream, RSTRING_PTR(rb_buffer), RSTRING_LEN(rb_buffer));
	rugged_exception_check(error);

	error = stream->finalize_write(&oid, stream);
	rugged_exception_check(error);

	return rugged_create_oid(&oid);
}

#define GIT_REPO_GETTER(method) \
static VALUE rb_git_repo_##method(VALUE self) \
{ \
	git_repository *repo; \
	int error; \
	Data_Get_Struct(self, git_repository, repo); \
	error = git_repository_##method(repo); \
	rugged_exception_check(error); \
	return error ? Qtrue : Qfalse; \
}

GIT_REPO_GETTER(is_bare); /* git_repository_is_bare */
GIT_REPO_GETTER(is_empty); /* git_repository_is_empty */
GIT_REPO_GETTER(head_detached); /* git_repository_head_detached */
GIT_REPO_GETTER(head_orphan); /* git_repository_head_orphan */

static VALUE rb_git_repo_path(VALUE self)
{
	git_repository *repo;
	Data_Get_Struct(self, git_repository, repo);
	return rugged_str_new2(git_repository_path(repo), NULL);
}

static VALUE rb_git_repo_workdir(VALUE self)
{
	git_repository *repo;
	const char *workdir;

	Data_Get_Struct(self, git_repository, repo);
	workdir = git_repository_workdir(repo);

	return workdir ? rugged_str_new2(workdir, NULL) : Qnil;
}

static VALUE rb_git_repo_set_workdir(VALUE self, VALUE rb_workdir)
{
	git_repository *repo;

	Data_Get_Struct(self, git_repository, repo);
	Check_Type(rb_workdir, T_STRING);

	rugged_exception_check(
		git_repository_set_workdir(repo, StringValueCStr(rb_workdir))
	);

	return Qnil;
}

static VALUE rb_git_repo_discover(int argc, VALUE *argv, VALUE self)
{
	VALUE rb_path, rb_across_fs;
	char repository_path[GIT_PATH_MAX];
	int error, across_fs = 0;

	rb_scan_args(argc, argv, "02", &rb_path, &rb_across_fs);

	if (NIL_P(rb_path)) {
		VALUE rb_dir = rb_const_get(rb_cObject, rb_intern("Dir"));
		rb_path = rb_funcall(rb_dir, rb_intern("pwd"), 0);
	}

	if (!NIL_P(rb_across_fs)) {
		across_fs = rugged_parse_bool(rb_across_fs);
	}

	Check_Type(rb_path, T_STRING);

	error = git_repository_discover(
		repository_path, GIT_PATH_MAX,
		StringValueCStr(rb_path),
		across_fs,
		NULL
	);

	rugged_exception_check(error);
	return rugged_str_new2(repository_path, NULL);
}

static VALUE flags_to_rb(unsigned int flags)
{
	VALUE rb_flags = rb_ary_new();

	if (flags & GIT_STATUS_INDEX_NEW)
		rb_ary_push(rb_flags, CSTR2SYM("index_new"));

	if (flags & GIT_STATUS_INDEX_MODIFIED)
		rb_ary_push(rb_flags, CSTR2SYM("index_modified"));

	if (flags & GIT_STATUS_INDEX_DELETED)
		rb_ary_push(rb_flags, CSTR2SYM("index_deleted"));

	if (flags & GIT_STATUS_WT_NEW)
		rb_ary_push(rb_flags, CSTR2SYM("worktree_new"));

	if (flags & GIT_STATUS_WT_MODIFIED)
		rb_ary_push(rb_flags, CSTR2SYM("worktree_modified"));

	if (flags & GIT_STATUS_WT_DELETED)
		rb_ary_push(rb_flags, CSTR2SYM("worktree_deleted"));

	return rb_flags;
}

static int rugged__status_cb(const char *path, unsigned int flags, void *payload)
{
	rb_funcall((VALUE)payload, rb_intern("call"), 2,
		rugged_str_new2(path, NULL),
		flags_to_rb(flags)
	);

	return GIT_SUCCESS;
}

static VALUE rb_git_repo_status(int argc, VALUE *argv, VALUE self)
{
	int error;
	VALUE rb_path;
	git_repository *repo;

	Data_Get_Struct(self, git_repository, repo);

	if (rb_scan_args(argc, argv, "01", &rb_path) == 1) {
		unsigned int flags;
		Check_Type(rb_path, T_STRING);
		error = git_status_file(&flags, repo, StringValueCStr(rb_path));
		rugged_exception_check(error);

		return flags_to_rb(flags);
	}

	if (!rb_block_given_p())
		rb_raise(rb_eRuntimeError,
			"A block was expected for iterating through "
			"the repository contents.");

	error = git_status_foreach(
		repo,
		&rugged__status_cb,
		(void *)rb_block_proc()
	);

	rugged_exception_check(error);
	return Qnil;
}

void Init_rugged_repo()
{
	rb_cRuggedRepo = rb_define_class_under(rb_mRugged, "Repository", rb_cObject);

	rb_define_singleton_method(rb_cRuggedRepo, "new", rb_git_repo_new, 1);
	rb_define_singleton_method(rb_cRuggedRepo, "hash",   rb_git_repo_hash,  2);
	rb_define_singleton_method(rb_cRuggedRepo, "hash_file",   rb_git_repo_hashfile,  2);
	rb_define_singleton_method(rb_cRuggedRepo, "init_at", rb_git_repo_init_at, 2);
	rb_define_singleton_method(rb_cRuggedRepo, "discover", rb_git_repo_discover, -1);

	rb_define_method(rb_cRuggedRepo, "exists", rb_git_repo_exists, 1);
	rb_define_method(rb_cRuggedRepo, "read",   rb_git_repo_read,   1);
	rb_define_method(rb_cRuggedRepo, "write",  rb_git_repo_write,  2);
	rb_define_method(rb_cRuggedRepo, "path",  rb_git_repo_path, 0);
	rb_define_method(rb_cRuggedRepo, "workdir",  rb_git_repo_workdir, 0);
	rb_define_method(rb_cRuggedRepo, "workdir=",  rb_git_repo_set_workdir, 1);
	rb_define_method(rb_cRuggedRepo, "status",  rb_git_repo_status,  -1);

	rb_define_method(rb_cRuggedRepo, "index",  rb_git_repo_get_index,  0);
	rb_define_method(rb_cRuggedRepo, "index=",  rb_git_repo_set_index,  1);
	rb_define_method(rb_cRuggedRepo, "config",  rb_git_repo_get_config,  0);
	rb_define_method(rb_cRuggedRepo, "config=",  rb_git_repo_set_config,  1);

	rb_define_method(rb_cRuggedRepo, "bare?",  rb_git_repo_is_bare,  0);
	rb_define_method(rb_cRuggedRepo, "empty?",  rb_git_repo_is_empty,  0);
	rb_define_method(rb_cRuggedRepo, "head_detached?",  rb_git_repo_head_detached,  0);
	rb_define_method(rb_cRuggedRepo, "head_orphan?",  rb_git_repo_head_orphan,  0);

	rb_cRuggedOdbObject = rb_define_class_under(rb_mRugged, "OdbObject", rb_cObject);
	rb_define_method(rb_cRuggedOdbObject, "data",  rb_git_odbobj_data,  0);
	rb_define_method(rb_cRuggedOdbObject, "len",  rb_git_odbobj_size,  0);
	rb_define_method(rb_cRuggedOdbObject, "type",  rb_git_odbobj_type,  0);
	rb_define_method(rb_cRuggedOdbObject, "hash",  rb_git_odbobj_hash,  0);
}
