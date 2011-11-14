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
extern VALUE rb_cRuggedBackend;

VALUE rb_cRuggedRepo;
VALUE rb_cRuggedOdbObject;


git_otype rugged_get_otype(VALUE self)
{
	git_otype type;

	if (NIL_P(self))
		return GIT_OBJ_ANY;

	switch (TYPE(self)) {
	case T_STRING:
		type = git_object_string2type(StringValueCStr(self));
		break;

	case T_FIXNUM:
		type = FIX2INT(self);
		break;

	default:
		type = GIT_OBJ_BAD;
		break;
	}

	if (!git_object_typeisloose(type))
		rb_raise(rb_eTypeError, "Invalid Git object type specifier");

	return type;
}

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
	git_odb_object_close((git_odb_object *)obj);
}

VALUE rugged_raw_read(git_repository *repo, const git_oid *oid)
{
	git_odb *odb;
	git_odb_object *obj;

	int error;

	odb = git_repository_database(repo);
	error = git_odb_read(&obj, odb, oid);
	rugged_exception_check(error);

	return Data_Wrap_Struct(rb_cRuggedOdbObject, NULL, rb_git__odbobj_free, obj);
}

void rb_git_repo__free(rugged_repository *repo)
{
	git_repository_free(repo->repo);
	free(repo);
}

static VALUE rb_git_repo_allocate(VALUE klass)
{
	rugged_repository *repo;

	repo = malloc(sizeof(rugged_repository));
	if (repo == NULL)
		rb_raise(rb_eNoMemError, "out of memory");

	repo->repo = NULL;
	repo->encoding = NULL;

	return Data_Wrap_Struct(klass, NULL, &rb_git_repo__free, repo);
}

static VALUE rb_git_repo_init(int argc, VALUE *argv, VALUE self)
{
	rugged_repository *r_repo;
	git_repository *repo;
	int error = 0;
	VALUE rb_dir, rb_obj_dir, rb_index_file, rb_work_tree;

	if (rb_scan_args(argc, argv, "13", 
				&rb_dir, 
				&rb_obj_dir, 
				&rb_index_file, 
				&rb_work_tree) > 1) {

		const char *git_obj_dir = NULL;
		const char *git_index_file = NULL;
		const char *git_work_tree = NULL;

		Check_Type(rb_dir, T_STRING);

		if (!NIL_P(rb_obj_dir)) {
			Check_Type(rb_obj_dir, T_STRING);
			git_obj_dir = StringValueCStr(rb_obj_dir);
		}

		if (!NIL_P(rb_index_file)) {
			Check_Type(rb_index_file, T_STRING);
			git_index_file = StringValueCStr(rb_index_file);
		}

		if (!NIL_P(rb_work_tree)) {
			Check_Type(rb_work_tree, T_STRING);
			git_work_tree = StringValueCStr(rb_work_tree);
		}

		error = git_repository_open2(&repo,
				StringValueCStr(rb_dir),
				git_obj_dir,
				git_index_file,
				git_work_tree);
	} else {
		Check_Type(rb_dir, T_STRING);
		error = git_repository_open(&repo, StringValueCStr(rb_dir));
	}

	rugged_exception_check(error);
	assert(repo);

	Data_Get_Struct(self, rugged_repository, r_repo);
	r_repo->repo = repo;
	/* TODO: fetch this properly */
	r_repo->encoding = rb_filesystem_encoding();

	return Qnil;
}

static VALUE rb_git_repo_init_at(VALUE klass, VALUE path, VALUE rb_is_bare)
{
	rugged_repository *r_repo;
	git_repository *repo;
	int error, is_bare;

	is_bare = rugged_parse_bool(rb_is_bare);
	Check_Type(path, T_STRING);
	error = git_repository_init(&repo, StringValueCStr(path), is_bare);

	rugged_exception_check(error);

	/* manually allocate a new repository */
	r_repo = malloc(sizeof(rugged_repository));
	if (r_repo == NULL)
		rb_raise(rb_eNoMemError, "out of memory");

	r_repo->repo = repo;
	/* TODO: fetch this properly */
	r_repo->encoding = rb_filesystem_encoding();

	return Data_Wrap_Struct(klass, NULL, &rb_git_repo__free, r_repo);
}

static VALUE rb_git_repo_index(VALUE self)
{
	rugged_repository *repo;
	git_index *index;
	int error;

	Data_Get_Struct(self, rugged_repository, repo);

	error = git_repository_index(&index, repo->repo);
	rugged_exception_check(error);

	return rugged_index_new(self, index);
}

static VALUE rb_git_repo_exists(VALUE self, VALUE hex)
{
	rugged_repository *repo;
	git_odb *odb;
	git_oid oid;

	Data_Get_Struct(self, rugged_repository, repo);
	Check_Type(hex, T_STRING);

	odb = git_repository_database(repo->repo);
	rugged_exception_check(git_oid_fromstr(&oid, StringValueCStr(hex)));

	return git_odb_exists(odb, &oid) ? Qtrue : Qfalse;
}

static VALUE rb_git_repo_read(VALUE self, VALUE hex)
{
	rugged_repository *repo;
	git_oid oid;
	int error;

	Data_Get_Struct(self, rugged_repository, repo);
	Check_Type(hex, T_STRING);

	error = git_oid_fromstr(&oid, StringValueCStr(hex));
	rugged_exception_check(error);

	return rugged_raw_read(repo->repo, &oid);
}

static VALUE rb_git_repo_hash(VALUE self, VALUE rb_buffer, VALUE rub_type)
{
	int error;
	git_oid oid;

	Check_Type(rb_buffer, T_STRING);

	error = git_odb_hash(&oid, RSTRING_PTR(rb_buffer), RSTRING_LEN(rb_buffer), rugged_get_otype(rub_type));
	rugged_exception_check(error);

	return rugged_create_oid(&oid);
}

static VALUE rb_git_repo_write(VALUE self, VALUE rb_buffer, VALUE rub_type)
{
	rugged_repository *repo;
	git_odb_stream *stream;

	git_odb *odb;
	git_oid oid;
	int error;

	git_otype type;

	Data_Get_Struct(self, rugged_repository, repo);

	odb = git_repository_database(repo->repo);
	type = rugged_get_otype(rub_type);

	error = git_odb_open_wstream(&stream, odb, RSTRING_LEN(rb_buffer), type);
	rugged_exception_check(error);

	error = stream->write(stream, RSTRING_PTR(rb_buffer), RSTRING_LEN(rb_buffer));
	rugged_exception_check(error);

	error = stream->finalize_write(&oid, stream);
	rugged_exception_check(error);

	return rugged_create_oid(&oid);
}

#define GIT_REPO_GETTER(method) \
static VALUE rb_git_repo_##method(VALUE self) \
{ \
	rugged_repository *repo; \
	int error; \
	Data_Get_Struct(self, rugged_repository, repo); \
	error = git_repository_##method(repo->repo); \
	rugged_exception_check(error); \
	return error ? Qtrue : Qfalse; \
}

GIT_REPO_GETTER(is_bare); /* git_repository_is_bare */
GIT_REPO_GETTER(is_empty); /* git_repository_is_empty */
GIT_REPO_GETTER(head_detached); /* git_repository_head_detached */
GIT_REPO_GETTER(head_orphan); /* git_repository_head_orphan */

static VALUE rb_git_repo_paths(VALUE self)
{
	rugged_repository *repo;
	VALUE rb_paths;
	Data_Get_Struct(self, rugged_repository, repo);

	rb_paths = rb_hash_new();

	rb_hash_aset(rb_paths, CSTR2SYM("repository"),
		rugged_str_new2(git_repository_path(repo->repo, GIT_REPO_PATH), NULL));

	rb_hash_aset(rb_paths, CSTR2SYM("index"),
		rugged_str_new2(git_repository_path(repo->repo, GIT_REPO_PATH_INDEX), NULL));

	rb_hash_aset(rb_paths, CSTR2SYM("odb"),
		rugged_str_new2(git_repository_path(repo->repo, GIT_REPO_PATH_ODB), NULL));

	rb_hash_aset(rb_paths, CSTR2SYM("workdir"),
		rugged_str_new2(git_repository_path(repo->repo, GIT_REPO_PATH_WORKDIR), NULL));

	return rb_paths;
}

static VALUE rb_git_repo_config(VALUE self)
{
	git_config *config;
	rugged_repository *repo;
	char user_config[GIT_PATH_MAX];
	int error;

	Data_Get_Struct(self, rugged_repository, repo);

	error = git_config_find_global(user_config);
	rugged_exception_check(error);

	error = git_repository_config(&config, repo->repo, user_config, NULL);
	rugged_exception_check(error);

	return rugged_config_new(config);
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

void Init_rugged_repo()
{
	rb_cRuggedRepo = rb_define_class_under(rb_mRugged, "Repository", rb_cObject);
	rb_define_alloc_func(rb_cRuggedRepo, rb_git_repo_allocate);
	rb_define_method(rb_cRuggedRepo, "initialize", rb_git_repo_init, -1);
	rb_define_method(rb_cRuggedRepo, "exists", rb_git_repo_exists, 1);
	rb_define_method(rb_cRuggedRepo, "read",   rb_git_repo_read,   1);
	rb_define_method(rb_cRuggedRepo, "write",  rb_git_repo_write,  2);
	rb_define_method(rb_cRuggedRepo, "index",  rb_git_repo_index,  0);
	rb_define_method(rb_cRuggedRepo, "paths",  rb_git_repo_paths,  0);

	rb_define_method(rb_cRuggedRepo, "config",  rb_git_repo_config,  0);

	rb_define_method(rb_cRuggedRepo, "bare?",  rb_git_repo_is_bare,  0);
	rb_define_method(rb_cRuggedRepo, "empty?",  rb_git_repo_is_empty,  0);
	rb_define_method(rb_cRuggedRepo, "head_detached?",  rb_git_repo_head_detached,  0);
	rb_define_method(rb_cRuggedRepo, "head_orphan?",  rb_git_repo_head_orphan,  0);

	rb_define_singleton_method(rb_cRuggedRepo, "hash",   rb_git_repo_hash,  2);
	rb_define_singleton_method(rb_cRuggedRepo, "init_at", rb_git_repo_init_at, 2);
	rb_define_singleton_method(rb_cRuggedRepo, "discover", rb_git_repo_discover, -1);

	rb_cRuggedOdbObject = rb_define_class_under(rb_mRugged, "OdbObject", rb_cObject);
	rb_define_method(rb_cRuggedOdbObject, "data",  rb_git_odbobj_data,  0);
	rb_define_method(rb_cRuggedOdbObject, "len",  rb_git_odbobj_size,  0);
	rb_define_method(rb_cRuggedOdbObject, "type",  rb_git_odbobj_type,  0);
	rb_define_method(rb_cRuggedOdbObject, "hash",  rb_git_odbobj_hash,  0);
}
