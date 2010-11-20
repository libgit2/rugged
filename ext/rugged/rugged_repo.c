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

extern VALUE rb_cRugged;
extern VALUE rb_cRuggedIndex;
VALUE rb_cRuggedRepo;

VALUE rugged_raw_read(git_repository *repo, const git_oid *oid)
{
	git_odb *odb;
	git_rawobj obj;

	VALUE ret_arr;
	unsigned char *data;
	const char *str_type;

	int error;

	odb = git_repository_database(repo);

	error = git_odb_read(&obj, odb, oid);
	rugged_exception_check(error);

	ret_arr = rb_ary_new();
	data = obj.data;
	str_type = git_obj_type_to_string(obj.type);

	rb_ary_store(ret_arr, 0, rb_str_new2(data));
	rb_ary_store(ret_arr, 1, INT2FIX((int)obj.len));
	rb_ary_store(ret_arr, 2, rb_str_new2(str_type));

	return ret_arr;
}

static VALUE rb_git_repo_allocate(VALUE klass)
{
	git_repository *repo = NULL;
	return Data_Wrap_Struct(klass, NULL, NULL, repo);
}

static VALUE rb_git_repo_init(VALUE self, VALUE path)
{
	git_repository *repo;
	int error;

	error = git_repository_open(&repo, RSTRING_PTR(path));
	rugged_exception_check(error);

	DATA_PTR(self) = repo;
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

	(&obj)->data = RSTRING_PTR(content);
	(&obj)->len  = RSTRING_LEN(content);
	(&obj)->type = git_obj_string_to_type(RSTRING_PTR(type));

	error = git_obj_hash(&oid, &obj);
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

	(&obj)->data = RSTRING_PTR(content);
	(&obj)->len  = RSTRING_LEN(content);
	(&obj)->type = git_obj_string_to_type(RSTRING_PTR(type));

	git_obj_hash(&oid, &obj);

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

	return obj ? rugged_object2rb(obj) : Qnil;
}


void Init_rugged_repo()
{
	rb_cRuggedRepo = rb_define_class_under(rb_cRugged, "Repository", rb_cObject);
	rb_define_alloc_func(rb_cRuggedRepo, rb_git_repo_allocate);
	rb_define_method(rb_cRuggedRepo, "initialize", rb_git_repo_init, 1);
	rb_define_method(rb_cRuggedRepo, "exists", rb_git_repo_exists, 1);
	rb_define_method(rb_cRuggedRepo, "hash",   rb_git_repo_obj_hash,  2);
	rb_define_method(rb_cRuggedRepo, "read",   rb_git_repo_read,   1);
	rb_define_method(rb_cRuggedRepo, "write",  rb_git_repo_write,  2);
	rb_define_method(rb_cRuggedRepo, "lookup", rb_git_repo_lookup,  -1);
	rb_define_method(rb_cRuggedRepo, "index",  rb_git_repo_index,  0);
}
