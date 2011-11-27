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
extern VALUE rb_cRuggedTag;
extern VALUE rb_cRuggedTree;
extern VALUE rb_cRuggedCommit;
extern VALUE rb_cRuggedBlob;
extern VALUE rb_cRuggedRepo;

VALUE rb_cRuggedObject;

git_object *rugged_object_load(git_repository *repo, VALUE object_value, git_otype type)
{
	git_object *object = NULL;

	if (TYPE(object_value) == T_STRING) {
		git_oid oid;
		int error;

		error = git_oid_fromstr(&oid, StringValueCStr(object_value));
		rugged_exception_check(error);

		error = git_object_lookup(&object, repo, &oid, type);
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

static void rb_git_object__free(git_object *object)
{
	git_object_free(object);
}

VALUE rugged_object_new(VALUE owner, git_object *object)
{
	VALUE klass, rb_object;

	switch (git_object_type(object))
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
			rb_raise(rb_eTypeError, "Invalid type for Rugged::Object");
			return Qnil; /* never reached */
	}

	rb_object = Data_Wrap_Struct(klass, NULL, &rb_git_object__free, object);
	rugged_set_owner(rb_object, owner);
	return rb_object;
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
	int oid_length;

	rugged_repository *repo;

	type = class2otype(rb_obj_class(klass));

	if (type == GIT_OBJ_BAD)
		type = GIT_OBJ_ANY;

	Check_Type(rb_hex, T_STRING);
	oid_length = (int)RSTRING_LEN(rb_hex);

	if (!rb_obj_is_instance_of(rb_repo, rb_cRuggedRepo))
		rb_raise(rb_eTypeError, "Expecting a Rugged Repository");

	if (oid_length > GIT_OID_HEXSZ)
		rb_raise(rb_eTypeError, "The given OID is too long");

	Data_Get_Struct(rb_repo, rugged_repository, repo);

	error = git_oid_fromstrn(&oid, RSTRING_PTR(rb_hex), oid_length);
	rugged_exception_check(error);

	if (oid_length < GIT_OID_HEXSZ)
		error = git_object_lookup_prefix(&object, repo->repo, &oid, oid_length, type);
	else
		error = git_object_lookup(&object, repo->repo, &oid, type);

	rugged_exception_check(error);

	return rugged_object_new(rb_repo, object);
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

static VALUE rb_git_object_oid_GET(VALUE self)
{
	git_object *object;
	Data_Get_Struct(self, git_object, object);
	return rugged_create_oid(git_object_id(object));
}

static VALUE rb_git_object_type_GET(VALUE self)
{
	git_object *object;
	Data_Get_Struct(self, git_object, object);

	return rugged_str_new2(git_object_type2string(git_object_type(object)), NULL);
}

static VALUE rb_git_object_read_raw(VALUE self)
{
	git_object *object;
	Data_Get_Struct(self, git_object, object);

	return rugged_raw_read(git_object_owner(object), git_object_id(object));
}

void Init_rugged_object()
{
	rb_cRuggedObject = rb_define_class_under(rb_mRugged, "Object", rb_cObject);
	rb_define_singleton_method(rb_cRuggedObject, "lookup", rb_git_object_lookup, 2);
	rb_define_singleton_method(rb_cRuggedObject, "new", rb_git_object_lookup, 2);

	rb_define_method(rb_cRuggedObject, "read_raw", rb_git_object_read_raw, 0);
	rb_define_method(rb_cRuggedObject, "==", rb_git_object_equal, 1);
	rb_define_method(rb_cRuggedObject, "oid", rb_git_object_oid_GET, 0);
	rb_define_method(rb_cRuggedObject, "type", rb_git_object_type_GET, 0);
}
