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
extern VALUE rb_cRuggedTag;
extern VALUE rb_cRuggedTree;
extern VALUE rb_cRuggedCommit;

VALUE rb_cRuggedObject;

git_object *rugged_rb2object(git_repository *repo, VALUE object_value, git_otype type)
{
	git_object *object = NULL;

	if (TYPE(object_value) == T_STRING) {
		git_oid oid;
		int error;

		git_oid_mkstr(&oid, RSTRING_PTR(object_value));
		error = git_repository_lookup(&object, repo, &oid, type);
		rugged_exception_check(error);

	} else if (rb_obj_is_kind_of(object_value, rb_cRuggedObject)) {
		Data_Get_Struct(object_value, git_object, object);

		if (type != GIT_OBJ_ANY && git_object_type(object) != type)
			rb_raise(rb_eTypeError, "Object is not of the required type");
	} else {
		rb_raise(rb_eTypeError, "Invalid GIT object; an object reference must be a SHA1 id or an object itself");
	}

	assert(object);
	return object;
}

VALUE rugged_object2rb(git_object *object)
{
	git_otype type;
	char sha1[40];
	VALUE obj, klass;

	type = git_object_type(object);

	switch (type)
	{
		case GIT_OBJ_COMMIT:
			klass = rb_cRuggedCommit;
			break;

		case GIT_OBJ_TAG:
			klass = rb_cRuggedTag;
			break;

		case GIT_OBJ_TREE:
			klass = rb_cRuggedTree;
			break;

		case GIT_OBJ_BLOB:
		default:
			klass = rb_cRuggedObject;
			break;
	}

	obj = Data_Wrap_Struct(klass, NULL, NULL, object);

	git_oid_fmt(sha1, git_object_id(object));

	return obj;
}

static VALUE rb_git_object_allocate(VALUE klass)
{
	git_object *object = NULL;
	return Data_Wrap_Struct(klass, NULL, NULL, object);
}

static VALUE rb_git_object_equal(VALUE self, VALUE other)
{
	git_object *a, *b;

	if (!rb_obj_is_kind_of(other, rb_cRuggedObject))
		return Qfalse;

	Data_Get_Struct(self, git_object, a);
	Data_Get_Struct(other, git_object, b);

	return git_oid_cmp(git_object_id(a), git_object_id(b)) == 0 ? Qtrue : Qfalse;
}

static VALUE rb_git_object_sha_GET(VALUE self)
{
	git_object *object;
	char hex[40];

	Data_Get_Struct(self, git_object, object);

	git_oid_fmt(hex, git_object_id(object));
	return rb_str_new(hex, 40);
}

static VALUE rb_git_object_type_GET(VALUE self)
{
	git_object *object;
	Data_Get_Struct(self, git_object, object);

	return rb_str_new2(git_obj_type_to_string(git_object_type(object)));
}

static VALUE rb_git_object_init(git_otype type, int argc, VALUE *argv, VALUE self)
{
	git_repository *repo;
	git_object *object;
	VALUE rb_repo, hex;
	int error;

	rb_scan_args(argc, argv, "11", &rb_repo, &hex);
	Data_Get_Struct(rb_repo, git_repository, repo);

	if (NIL_P(hex)) {
		error = git_repository_newobject(&object, repo, type);
		rugged_exception_check(error);
	} else {
		git_oid oid;

		git_oid_mkstr(&oid, RSTRING_PTR(hex));
		error = git_repository_lookup(&object, repo, &oid, type);
		rugged_exception_check(error);
	}

	DATA_PTR(self) = object;
	return Qnil;
}

static VALUE rb_git_object_read_raw(VALUE self)
{
	git_object *object;
	Data_Get_Struct(self, git_object, object);

	return rugged_raw_read(git_object_owner(object), git_object_id(object));
}

static VALUE rb_git_object_write(VALUE self)
{
	git_object *object;
	char new_hex[40];
	VALUE sha;
	int error;

	Data_Get_Struct(self, git_object, object);

	error = git_object_write(object);
	rugged_exception_check(error);

	git_oid_fmt(new_hex, git_object_id(object));
	sha = rb_str_new(new_hex, 40);

	return sha;
}


void Init_rugged_object()
{
	rb_cRuggedObject = rb_define_class_under(rb_cRugged, "Object", rb_cObject);
	rb_define_alloc_func(rb_cRuggedObject, rb_git_object_allocate);
	rb_define_method(rb_cRuggedObject, "read_raw", rb_git_object_read_raw, 0);
	rb_define_method(rb_cRuggedObject, "write", rb_git_object_write, 0);
	rb_define_method(rb_cRuggedObject, "==", rb_git_object_equal, 1);
	rb_define_method(rb_cRuggedObject, "sha", rb_git_object_sha_GET, 0);
	rb_define_method(rb_cRuggedObject, "type", rb_git_object_type_GET, 0);
}
