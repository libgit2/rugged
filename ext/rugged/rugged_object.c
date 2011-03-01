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
extern VALUE rb_cRuggedTag;
extern VALUE rb_cRuggedTree;
extern VALUE rb_cRuggedCommit;
extern VALUE rb_cRuggedBlob;

VALUE rb_cRuggedObject;

git_object *rugged_object_get(git_repository *repo, VALUE object_value, git_otype type)
{
	git_object *object = NULL;

	if (TYPE(object_value) == T_STRING) {
		git_oid oid;
		int error;

		error = git_oid_mkstr(&oid, RSTRING_PTR(object_value));
		rugged_exception_check(error);

		error = git_object_lookup(&object, repo, &oid, type);
		rugged_exception_check(error);

	} else if (rb_obj_is_kind_of(object_value, rb_cRuggedObject)) {
		RUGGED_OBJ_UNWRAP(object_value, git_object, object);

		if (type != GIT_OBJ_ANY && git_object_type(object) != type)
			rb_raise(rb_eTypeError, "Object is not of the required type");
	} else {
		rb_raise(rb_eTypeError, "Invalid GIT object; an object reference must be a SHA1 id or an object itself");
	}

	assert(object);
	return object;
}

void rb_git_object__free(rugged_object *o)
{
	git_object_close(o->object);
	free(o);
}

void rb_git_object__mark(rugged_object *o)
{
	/* The Repository doesn't go away as long
	 * as we still have objects lying around */
	rb_gc_mark(o->owner);
}

VALUE rugged_object_new(VALUE rb_repo, git_object *object)
{
	rugged_object *r_object;
	git_otype type;
	VALUE klass;

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
			klass = rb_cRuggedBlob;
			break;

		default:
			klass = rb_cRuggedObject;
			break;
	}

	r_object = malloc(sizeof(rugged_object));
	if (r_object == NULL)
		rb_raise(rb_eNoMemError, "out of memory");

	r_object->object = object;
	r_object->owner = rb_repo;

	return Data_Wrap_Struct(klass, rb_git_object__mark, rb_git_object__free, r_object);
}

static VALUE rb_git_object_allocate(VALUE klass)
{
	rugged_object *object;

	object = malloc(sizeof(rugged_object));
	if (object == NULL)
		rb_raise(rb_eNoMemError, "out of memory");

	object->object = NULL;
	object->owner = Qnil;

	return Data_Wrap_Struct(klass, rb_git_object__mark, rb_git_object__free, object);
}

static git_otype class2otype(VALUE klass)
{
	if (klass == rb_cRuggedCommit)
		return GIT_OBJ_COMMIT;

	if (klass == rb_cRuggedTag)
		return GIT_OBJ_TAG;

	if (klass == rb_cRuggedBlob)
		return GIT_OBJ_BLOB;

	if (klass == rb_cRuggedTree)
		return GIT_OBJ_TREE;
	
	return GIT_OBJ_BAD;
}

VALUE rb_git_object_lookup(VALUE klass, VALUE rb_repo, VALUE rb_hex)
{
	git_object *object;
	git_otype type;
	git_oid oid;
	int error;

	rugged_repository *repo;

	type = class2otype(rb_obj_class(klass));

	if (type == GIT_OBJ_BAD)
		type = GIT_OBJ_ANY;

	Check_Type(rb_hex, T_STRING);
	Data_Get_Struct(rb_repo, rugged_repository, repo);

	error = git_oid_mkstr(&oid, RSTRING_PTR(rb_hex));
	rugged_exception_check(error);

	error = git_object_lookup(&object, repo->repo, &oid, type);
	rugged_exception_check(error);

	return rugged_object_new(rb_repo, object);
}

VALUE rb_git_object_new(VALUE self, VALUE rb_repo)
{
	git_object *object;
	git_otype type;
	int error;

	rugged_object *r_obj;
	rugged_repository *repo;

	type = class2otype(rb_obj_class(self));

	if (type == GIT_OBJ_BAD)
		rb_raise(rb_eTypeError, "Cannot instantiate an abstract Git Object");

	Data_Get_Struct(rb_repo, rugged_repository, repo);
	Data_Get_Struct(self, rugged_object, r_obj);

	error = git_object_new(&object, repo->repo, type);
	rugged_exception_check(error);

	r_obj->object = object;
	r_obj->owner = rb_repo;

	return Qnil;
}

static VALUE rb_git_object_equal(VALUE self, VALUE other)
{
	git_object *a, *b;

	if (!rb_obj_is_kind_of(other, rb_cRuggedObject))
		return Qfalse;

	RUGGED_OBJ_UNWRAP(self, git_object, a);
	RUGGED_OBJ_UNWRAP(other, git_object, b);

	return git_oid_cmp(git_object_id(a), git_object_id(b)) == 0 ? Qtrue : Qfalse;
}

static VALUE rb_git_object_sha_GET(VALUE self)
{
	git_object *object;
	char hex[40];

	RUGGED_OBJ_UNWRAP(self, git_object, object);

	git_oid_fmt(hex, git_object_id(object));
	return rugged_str_new(hex, 40, NULL);
}

static VALUE rb_git_object_type_GET(VALUE self)
{
	git_object *object;
	RUGGED_OBJ_UNWRAP(self, git_object, object);

	return rugged_str_new2(git_object_type2string(git_object_type(object)), NULL);
}

static VALUE rb_git_object_read_raw(VALUE self)
{
	git_object *object;
	RUGGED_OBJ_UNWRAP(self, git_object, object);

	return rugged_raw_read(git_object_owner(object), git_object_id(object));
}

static VALUE rb_git_object_write(VALUE self)
{
	git_object *object;
	char new_hex[40];
	VALUE sha;
	int error;

	RUGGED_OBJ_UNWRAP(self, git_object, object);

	error = git_object_write(object);
	rugged_exception_check(error);

	git_oid_fmt(new_hex, git_object_id(object));
	sha = rugged_str_new(new_hex, 40, NULL);

	return sha;
}


void Init_rugged_object()
{
	rb_cRuggedObject = rb_define_class_under(rb_mRugged, "Object", rb_cObject);
	rb_define_alloc_func(rb_cRuggedObject, rb_git_object_allocate);

	rb_define_method(rb_cRuggedObject, "initialize", rb_git_object_new, 1);
	rb_define_singleton_method(rb_cRuggedObject, "lookup", rb_git_object_lookup, 2);

	rb_define_method(rb_cRuggedObject, "read_raw", rb_git_object_read_raw, 0);
	rb_define_method(rb_cRuggedObject, "write", rb_git_object_write, 0);
	rb_define_method(rb_cRuggedObject, "==", rb_git_object_equal, 1);
	rb_define_method(rb_cRuggedObject, "sha", rb_git_object_sha_GET, 0);
	rb_define_method(rb_cRuggedObject, "type", rb_git_object_type_GET, 0);
}
