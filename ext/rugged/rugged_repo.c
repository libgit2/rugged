/*
 * The MIT License
 *
 * Copyright (c) 2010 Scott Chacon
 * Copyright (c) 2010 Vicent Marti
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
		type = git_object_string2type(RSTRING_PTR(self));
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

void rb_git_repo__mark(rugged_repository *repo)
{
	int i;

	for (i = 0; i < RARRAY_LEN(repo->backends); ++i)
		rb_gc_mark(rb_ary_entry(repo->backends, i));
}

static VALUE rb_git_repo_allocate(VALUE klass)
{
	rugged_repository *repo;

	repo = malloc(sizeof(rugged_repository));
	if (repo == NULL)
		rb_raise(rb_eNoMemError, "out of memory");

	repo->repo = NULL;
	repo->backends = rb_ary_new();

	return Data_Wrap_Struct(klass, rb_git_repo__mark, rb_git_repo__free, repo);
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
			git_obj_dir = RSTRING_PTR(rb_obj_dir);
		}

		if (!NIL_P(rb_index_file)) {
			Check_Type(rb_index_file, T_STRING);
			git_index_file = RSTRING_PTR(rb_index_file);
		}

		if (!NIL_P(rb_work_tree)) {
			Check_Type(rb_work_tree, T_STRING);
			git_work_tree = RSTRING_PTR(rb_work_tree);
		}

		error = git_repository_open2(&repo,
				RSTRING_PTR(rb_dir),
				git_obj_dir,
				git_index_file,
				git_work_tree);
	} else {
		Check_Type(rb_dir, T_STRING);
		error = git_repository_open(&repo, RSTRING_PTR(rb_dir));
	}

	rugged_exception_check(error);
	assert(repo);

	Data_Get_Struct(self, rugged_repository, r_repo);
	r_repo->repo = repo;

	return Qnil;
}

static VALUE rb_git_repo_init_at(VALUE klass, VALUE path, VALUE rb_is_bare)
{
	rugged_repository *r_repo;
	git_repository *repo;
	int error, is_bare;

	is_bare = rugged_parse_bool(rb_is_bare);
	Check_Type(path, T_STRING);
	error = git_repository_init(&repo, RSTRING_PTR(path), is_bare);

	rugged_exception_check(error);

	/* manually allocate a new repository */
	r_repo = malloc(sizeof(rugged_repository));
	if (r_repo == NULL)
		rb_raise(rb_eNoMemError, "out of memory");

	r_repo->repo = repo;
	r_repo->backends = rb_ary_new();

	return Data_Wrap_Struct(klass, rb_git_repo__mark, rb_git_repo__free, r_repo);
}

static VALUE rb_git_repo_add_backend(VALUE self, VALUE rb_backend, VALUE rb_priority)
{
	rugged_repository *repo;
	rugged_backend *backend;
	int error;

	Data_Get_Struct(self, rugged_repository, repo);

	Check_Type(rb_priority, T_FIXNUM);

	if (!rb_obj_is_kind_of(rb_backend, rb_cRuggedBackend))
		rb_raise(rb_eTypeError, "expecting a subclass of Rugged::Backend");

	if (rb_obj_is_instance_of(rb_backend, rb_cRuggedBackend))
		rb_raise(rb_eTypeError, "create a subclass of Rugged::Backend to define your custom backend");

	Data_Get_Struct(rb_backend, rugged_backend, backend);

	error = git_odb_add_backend(git_repository_database(repo->repo), (git_odb_backend *)backend, FIX2INT(rb_priority));
	rugged_exception_check(error);

	rb_ary_push(repo->backends, rb_backend);

	return Qnil;
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
	rugged_exception_check(git_oid_mkstr(&oid, RSTRING_PTR(hex)));

	return git_odb_exists(odb, &oid) ? Qtrue : Qfalse;
}

static VALUE rb_git_repo_read(VALUE self, VALUE hex)
{
	rugged_repository *repo;
	git_oid oid;
	int error;

	Data_Get_Struct(self, rugged_repository, repo);
	Check_Type(hex, T_STRING);

	error = git_oid_mkstr(&oid, RSTRING_PTR(hex));
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

static VALUE rb_git_repo_lookup_object(int argc, VALUE *argv, VALUE self)
{
	rugged_repository *repo;
	git_otype type;
	git_object *obj;
	git_oid oid;
	int error;

	VALUE rub_type, rb_sha;

	Data_Get_Struct(self, rugged_repository, repo);

	rb_scan_args(argc, argv, "11", &rb_sha, &rub_type);

	type = rugged_get_otype(rub_type);

	Check_Type(rb_sha, T_STRING);
	error = git_oid_mkstr(&oid, RSTRING_PTR(rb_sha));
	rugged_exception_check(error);

	error = git_object_lookup(&obj, repo->repo, &oid, type);
	rugged_exception_check(error);

	return obj ? rugged_object_new(self, obj) : Qnil;
}


void Init_rugged_repo()
{
	rb_cRuggedRepo = rb_define_class_under(rb_mRugged, "Repository", rb_cObject);
	rb_define_alloc_func(rb_cRuggedRepo, rb_git_repo_allocate);
	rb_define_method(rb_cRuggedRepo, "initialize", rb_git_repo_init, -1);
	rb_define_method(rb_cRuggedRepo, "exists", rb_git_repo_exists, 1);
	rb_define_method(rb_cRuggedRepo, "read",   rb_git_repo_read,   1);
	rb_define_method(rb_cRuggedRepo, "write",  rb_git_repo_write,  2);
	rb_define_method(rb_cRuggedRepo, "lookup", rb_git_repo_lookup_object,  -1);
	rb_define_method(rb_cRuggedRepo, "index",  rb_git_repo_index,  0);
	rb_define_method(rb_cRuggedRepo, "add_backend",  rb_git_repo_add_backend,  1);

	rb_define_singleton_method(rb_cRuggedRepo, "hash",   rb_git_repo_hash,  2);
	rb_define_singleton_method(rb_cRuggedRepo, "init_at", rb_git_repo_init_at, 2);

	rb_cRuggedOdbObject = rb_define_class_under(rb_mRugged, "OdbObject", rb_cObject);
	rb_define_method(rb_cRuggedOdbObject, "data",  rb_git_odbobj_data,  0);
	rb_define_method(rb_cRuggedOdbObject, "len",  rb_git_odbobj_size,  0);
	rb_define_method(rb_cRuggedOdbObject, "type",  rb_git_odbobj_type,  0);
	rb_define_method(rb_cRuggedOdbObject, "hash",  rb_git_odbobj_hash,  0);
	/* TODO: getters */
}
