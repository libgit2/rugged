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
VALUE rb_cRuggedRawObject;

VALUE rugged_raw_read(git_repository *repo, const git_oid *oid)
{
	git_odb *odb;
	git_rawobj obj;

	VALUE ret_args[3];

	int error;

	odb = git_repository_database(repo);
	error = git_odb_read(&obj, odb, oid);
	rugged_exception_check(error);

	ret_args[0] = rb_str_new(obj.data, obj.len);
	ret_args[1] = INT2FIX((int)obj.len);
	ret_args[2] = INT2FIX(obj.type);

	git_rawobj_close(&obj);

	return rb_class_new_instance(3, ret_args, rb_cRuggedRawObject);
}

static VALUE rb_git_repo_allocate(VALUE klass)
{
	git_repository *repo = NULL;
	return Data_Wrap_Struct(klass, NULL, NULL, repo);
}

static VALUE rb_git_repo_init(int argc, VALUE *argv, VALUE self)
{
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

	DATA_PTR(self) = repo;
	return Qnil;
}

static VALUE rb_git_repo_add_backend(VALUE self, VALUE rb_backend)
{
	git_repository *repo;
	rugged_backend *backend;
	int error;

	Data_Get_Struct(self, git_repository, repo);

	if (!rb_obj_is_kind_of(rb_backend, rb_cRuggedBackend))
		raise(rb_eTypeError, "expecting a subclass of Rugged::Backend");

	if (rb_obj_is_instance_of(rb_backend, rb_cRuggedBackend))
		raise(rb_eTypeError, "create a subclass of Rugged::Backend to define your custom backend");

	Data_Get_Struct(rb_backend, rugged_backend, backend);

	error = git_odb_add_backend(git_repository_database(repo), (git_odb_backend *)backend);
	rugged_exception_check(error);

	return Qnil;
}

static VALUE rb_git_repo_index(VALUE self)
{
	git_repository *repo;
	git_index *index;

	Data_Get_Struct(self, git_repository, repo);

	if ((index = git_repository_index(repo)) == NULL)
		rb_raise(rb_eRuntimeError, "failed to open Index file");

	return Data_Wrap_Struct(rb_cRuggedIndex, NULL, NULL, index);
}

static VALUE rb_git_repo_exists(VALUE self, VALUE hex)
{
	git_repository *repo;
	git_odb *odb;
	git_oid oid;

	Data_Get_Struct(self, git_repository, repo);

	odb = git_repository_database(repo);
	git_oid_mkstr(&oid, RSTRING_PTR(hex));

	return git_odb_exists(odb, &oid) ? Qtrue : Qfalse;
}

static VALUE rb_git_repo_read(VALUE self, VALUE hex)
{
	git_repository *repo;
	git_oid oid;
	int error;

	Data_Get_Struct(self, git_repository, repo);

	error = git_oid_mkstr(&oid, RSTRING_PTR(hex));
	rugged_exception_check(error);

	return rugged_raw_read(repo, &oid);
}

static VALUE rb_git_repo_obj_hash(VALUE self, VALUE content, VALUE type)
{
	git_rawobj obj;
	char out[40];
	int error;
	git_oid oid;

	obj.data = RSTRING_PTR(content);
	obj.len  = RSTRING_LEN(content);
	obj.type = git_object_string2type(RSTRING_PTR(type));

	error = git_rawobj_hash(&oid, &obj);
	rugged_exception_check(error);

	git_oid_fmt(out, &oid);
	return rb_str_new(out, 40);
}

static VALUE rb_git_repo_write(VALUE self, VALUE content, VALUE type)
{
	git_repository *repo;
	git_odb *odb;
	git_rawobj obj;
	git_oid oid;
	int error;
	char out[40];

	Data_Get_Struct(self, git_repository, repo);
	odb = git_repository_database(repo);

	obj.data = RSTRING_PTR(content);
	obj.len  = RSTRING_LEN(content);
	obj.type = git_object_string2type(RSTRING_PTR(type));

	error = git_odb_write(&oid, odb, &obj);
	rugged_exception_check(error);

	git_oid_fmt(out, &oid);
	return rb_str_new(out, 40);
}

static VALUE rb_git_repo_lookup(int argc, VALUE *argv, VALUE self)
{
	git_repository *repo;
	git_otype type;
	git_object *obj;
	git_oid oid;
	int error;

	VALUE rb_type, rb_sha;

	Data_Get_Struct(self, git_repository, repo);

	rb_scan_args(argc, argv, "11", &rb_sha, &rb_type);

	type = NIL_P(rb_type) ? GIT_OBJ_ANY : FIX2INT(rb_type);
	git_oid_mkstr(&oid, RSTRING_PTR(rb_sha));

	error = git_repository_lookup(&obj, repo, &oid, type);
	rugged_exception_check(error);

	return obj ? rugged_object_c2rb(obj) : Qnil;
}


void Init_rugged_repo()
{
	rb_cRuggedRepo = rb_define_class_under(rb_mRugged, "Repository", rb_cObject);
	rb_define_alloc_func(rb_cRuggedRepo, rb_git_repo_allocate);
	rb_define_method(rb_cRuggedRepo, "initialize", rb_git_repo_init, -1);
	rb_define_method(rb_cRuggedRepo, "exists", rb_git_repo_exists, 1);
	rb_define_method(rb_cRuggedRepo, "hash",   rb_git_repo_obj_hash,  2);
	rb_define_method(rb_cRuggedRepo, "read",   rb_git_repo_read,   1);
	rb_define_method(rb_cRuggedRepo, "write",  rb_git_repo_write,  2);
	rb_define_method(rb_cRuggedRepo, "lookup", rb_git_repo_lookup,  -1);
	rb_define_method(rb_cRuggedRepo, "index",  rb_git_repo_index,  0);
	rb_define_method(rb_cRuggedRepo, "add_backend",  rb_git_repo_add_backend,  1);

	rb_cRuggedRawObject = rb_define_class_under(rb_mRugged, "RawObject", rb_cObject);
}
