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

#define UNPACK_REFERENCE(_rb_obj, _rugged_obj) {\
	Data_Get_Struct(_rb_obj, rugged_reference, _rugged_obj);\
	if (_rugged_obj->ref == NULL)\
		rb_raise(rb_eRuntimeError,\
			"This Git Reference has been deleted and no longer exists on the repository");\
}

extern VALUE rb_mRugged;
VALUE rb_cRuggedReference;

void rb_git_ref__mark(rugged_reference *ref)
{
	if (ref->ref != NULL)
		rb_gc_mark(ref->owner);
}

void rb_git_ref__free(rugged_reference *ref)
{
	free(ref);
}

VALUE rugged_ref_new(VALUE rb_repo, git_reference *ref)
{
	rugged_reference *r_ref;

	r_ref = malloc(sizeof(rugged_reference));
	if (r_ref == NULL)
		rb_raise(rb_eNoMemError, "out of memory");

	r_ref->ref = ref;
	r_ref->owner = rb_repo;

	return Data_Wrap_Struct(rb_cRuggedReference, rb_git_ref__mark, rb_git_ref__free, r_ref);
}

static VALUE rb_git_ref_packall(VALUE klass, VALUE rb_repo)
{
	rugged_repository *repo;
	int error;

	Data_Get_Struct(rb_repo, rugged_repository, repo);

	error = git_reference_packall(repo->repo);
	rugged_exception_check(error);

	return Qnil;
}

static VALUE rb_git_ref_lookup(VALUE klass, VALUE rb_repo, VALUE rb_name)
{
	rugged_repository *repo;
	git_reference *ref;
	int error;

	Data_Get_Struct(rb_repo, rugged_repository, repo);
	Check_Type(rb_name, T_STRING);

	error = git_reference_lookup(&ref, repo->repo, RSTRING_PTR(rb_name));
	rugged_exception_check(error);

	return rugged_ref_new(rb_repo, ref);
}

static VALUE rb_git_ref_create(VALUE klass, VALUE rb_repo, VALUE rb_name, VALUE rb_target)
{
	rugged_repository *repo;
	git_reference *ref;
	git_oid oid;

	int error;

	Data_Get_Struct(rb_repo, rugged_repository, repo);
	Check_Type(rb_name, T_STRING);
	Check_Type(rb_target, T_STRING);

	if (git_oid_mkstr(&oid, RSTRING_PTR(rb_target)) == GIT_SUCCESS) {
		error = git_reference_create_oid(&ref, repo->repo, RSTRING_PTR(rb_name), &oid);
	} else {
		error = git_reference_create_symbolic(&ref, repo->repo, RSTRING_PTR(rb_name), RSTRING_PTR(rb_target));
	}

	rugged_exception_check(error);

	return rugged_ref_new(rb_repo, ref);
}

static VALUE rb_git_ref_target(VALUE self)
{
	rugged_reference *ref;

	UNPACK_REFERENCE(self, ref);

	if (git_reference_type(ref->ref) == GIT_REF_OID) {
		char out[40];
		git_oid_fmt(out, git_reference_oid(ref->ref));
		return rugged_str_new(out, 40, NULL);
	} else {
		return rugged_str_new2(git_reference_target(ref->ref), NULL);
	}
}

static VALUE rb_git_ref_set_target(VALUE self, VALUE rb_target)
{
	rugged_reference *ref;
	int error;

	UNPACK_REFERENCE(self, ref);
	Check_Type(rb_target, T_STRING);

	if (git_reference_type(ref->ref) == GIT_REF_OID) {
		git_oid target;

		error = git_oid_mkstr(&target, RSTRING_PTR(rb_target));
		rugged_exception_check(error);

		error = git_reference_set_oid(ref->ref, &target);
	} else {
		error = git_reference_set_target(ref->ref, RSTRING_PTR(rb_target));
	}

	rugged_exception_check(error);
	return Qnil;
}

static VALUE rb_git_ref_type(VALUE self)
{
	rugged_reference *ref;
	UNPACK_REFERENCE(self, ref);
	return rugged_str_new2(git_object_type2string(git_reference_type(ref->ref)), NULL);
}

static VALUE rb_git_ref_name(VALUE self)
{
	rugged_reference *ref;
	UNPACK_REFERENCE(self, ref);
	return rugged_str_new2(git_reference_name(ref->ref), NULL);
}

static VALUE rb_git_ref_resolve(VALUE self)
{
	rugged_reference *ref;
	git_reference *resolved;
	int error;

	UNPACK_REFERENCE(self, ref);

	error = git_reference_resolve(&resolved, ref->ref);
	rugged_exception_check(error);

	return rugged_ref_new(ref->owner, resolved);
}

static VALUE rb_git_ref_rename(VALUE self, VALUE rb_name)
{
	rugged_reference *ref;
	int error;

	UNPACK_REFERENCE(self, ref);

	Check_Type(rb_name, T_STRING);
	error = git_reference_rename(ref->ref, RSTRING_PTR(rb_name));
	rugged_exception_check(error);

	return Qnil;
}

static VALUE rb_git_ref_delete(VALUE self)
{
	rugged_reference *ref;
	int error;

	UNPACK_REFERENCE(self, ref);

	error = git_reference_delete(ref->ref);
	rugged_exception_check(error);

	ref->ref = NULL; /* this has been free'd */
	return Qnil;
}

void Init_rugged_reference()
{
	rb_cRuggedReference = rb_define_class_under(rb_mRugged, "Reference", rb_cObject);

	rb_define_singleton_method(rb_cRuggedReference, "lookup", rb_git_ref_lookup, 2);
	rb_define_singleton_method(rb_cRuggedReference, "create", rb_git_ref_create, 3);
	rb_define_singleton_method(rb_cRuggedReference, "pack_all", rb_git_ref_packall, 1);

	rb_define_method(rb_cRuggedReference, "target", rb_git_ref_target, 0);
	rb_define_method(rb_cRuggedReference, "target=", rb_git_ref_set_target, 1);

	rb_define_method(rb_cRuggedReference, "type", rb_git_ref_type, 0);

	rb_define_method(rb_cRuggedReference, "name", rb_git_ref_name, 0);
	rb_define_method(rb_cRuggedReference, "name=", rb_git_ref_rename, 1);

	rb_define_method(rb_cRuggedReference, "resolve", rb_git_ref_resolve, 0);
	rb_define_method(rb_cRuggedReference, "delete", rb_git_ref_delete, 0);
}
