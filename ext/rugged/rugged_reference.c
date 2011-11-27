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

#define UNPACK_REFERENCE(_rb_obj, _rugged_obj) {\
	if (DATA_PTR(_rb_obj) == NULL)\
		rb_raise(rb_eRuntimeError,\
			"This Git Reference has been deleted and no longer exists on the repository");\
	Data_Get_Struct(_rb_obj, git_reference, _rugged_obj);\
}

extern VALUE rb_mRugged;
extern VALUE rb_cRuggedRepo;
VALUE rb_cRuggedReference;

void rb_git_ref__free(git_reference *ref)
{
	git_reference_free(ref);
}

VALUE rugged_ref_new(VALUE klass, VALUE owner, git_reference *ref)
{
	VALUE rb_ref = Data_Wrap_Struct(klass, NULL, &rb_git_ref__free, ref);
	rugged_set_owner(rb_ref, owner);
	return rb_ref;
}

static VALUE rb_git_ref_packall(VALUE klass, VALUE rb_repo)
{
	rugged_repository *repo;
	int error;

	if (!rb_obj_is_kind_of(rb_repo, rb_cRuggedRepo))
		rb_raise(rb_eTypeError, "Expecting a Rugged::Repository instance");

	Data_Get_Struct(rb_repo, rugged_repository, repo);

	error = git_reference_packall(repo->repo);
	rugged_exception_check(error);

	return Qnil;
}

int ref_foreach__list(const char *ref_name, void *opaque)
{
	rb_ary_push((VALUE)opaque, rugged_str_new2(ref_name, NULL));
	return GIT_SUCCESS;
}

int ref_foreach__block(const char *ref_name, void *opaque)
{
	rb_funcall((VALUE)opaque, rb_intern("call"), 1, rugged_str_new2(ref_name, NULL));
	return GIT_SUCCESS;
}

static VALUE rb_git_ref_each(int argc, VALUE *argv, VALUE self)
{
	rugged_repository *repo;
	int error, flags = 0;
	VALUE rb_repo, rb_list, rb_block;

	rb_scan_args(argc, argv, "11&", &rb_repo, &rb_list, &rb_block);

	if (!rb_block_given_p())
		return rb_funcall(self, rb_intern("to_enum"), 3, CSTR2SYM("each"), rb_repo, rb_list);

	if (!rb_obj_is_kind_of(rb_repo, rb_cRuggedRepo))
		rb_raise(rb_eTypeError, "Expecting a Rugged::Repository instance");

	Data_Get_Struct(rb_repo, rugged_repository, repo);

	if (!NIL_P(rb_list)) {
		ID list;

		Check_Type(rb_list, T_SYMBOL);
		list = SYM2ID(rb_list);

		if (list == rb_intern("all"))
			flags = GIT_REF_LISTALL;
		else if (list == rb_intern("direct"))
			flags = GIT_REF_OID|GIT_REF_PACKED;
		else if (list == rb_intern("symbolic"))
			flags = GIT_REF_SYMBOLIC|GIT_REF_PACKED;
	} else {
		flags = GIT_REF_LISTALL;
	}

	error = git_reference_foreach(repo->repo, flags, &ref_foreach__block, (void *)rb_block);
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

	error = git_reference_lookup(&ref, repo->repo, StringValueCStr(rb_name));
	rugged_exception_check(error);

	return rugged_ref_new(klass, rb_repo, ref);
}

static VALUE rb_git_ref_create(int argc, VALUE *argv, VALUE klass)
{
	VALUE rb_repo, rb_name, rb_target, rb_force;
	rugged_repository *repo;
	git_reference *ref;
	git_oid oid;
	int error, force = 0;

	rb_scan_args(argc, argv, "31", &rb_repo, &rb_name, &rb_target, &rb_force);

	Data_Get_Struct(rb_repo, rugged_repository, repo);
	Check_Type(rb_name, T_STRING);
	Check_Type(rb_target, T_STRING);

	if (!NIL_P(rb_force))
		force = rugged_parse_bool(rb_force);

	if (git_oid_fromstr(&oid, StringValueCStr(rb_target)) == GIT_SUCCESS) {
		error = git_reference_create_oid(
			&ref, repo->repo, StringValueCStr(rb_name), &oid, force);
	} else {
		error = git_reference_create_symbolic(
			&ref, repo->repo, StringValueCStr(rb_name), RSTRING_PTR(rb_target), force);
	}

	rugged_exception_check(error);

	return rugged_ref_new(klass, rb_repo, ref);
}

static VALUE rb_git_ref_target(VALUE self)
{
	git_reference *ref;

	UNPACK_REFERENCE(self, ref);

	if (git_reference_type(ref) == GIT_REF_OID) {
		return rugged_create_oid(git_reference_oid(ref));
	} else {
		return rugged_str_new2(git_reference_target(ref), NULL);
	}
}

static VALUE rb_git_ref_set_target(VALUE self, VALUE rb_target)
{
	git_reference *ref;
	int error;

	UNPACK_REFERENCE(self, ref);
	Check_Type(rb_target, T_STRING);

	if (git_reference_type(ref) == GIT_REF_OID) {
		git_oid target;

		error = git_oid_fromstr(&target, StringValueCStr(rb_target));
		rugged_exception_check(error);

		error = git_reference_set_oid(ref, &target);
	} else {
		error = git_reference_set_target(ref, StringValueCStr(rb_target));
	}

	rugged_exception_check(error);
	return Qnil;
}

static VALUE rb_git_ref_type(VALUE self)
{
	git_reference *ref;
	UNPACK_REFERENCE(self, ref);
	return rugged_str_new2(git_object_type2string(git_reference_type(ref)), NULL);
}

static VALUE rb_git_ref_packed(VALUE self)
{
	git_reference *ref;
	UNPACK_REFERENCE(self, ref);

	return git_reference_is_packed(ref) ? Qtrue : Qfalse;
}

static VALUE rb_git_ref_reload(VALUE self)
{
	int error;
	git_reference *ref;
	UNPACK_REFERENCE(self, ref);

	error = git_reference_reload(ref);

	/* If reload fails, the reference is invalidated */
	if (error < GIT_SUCCESS)
		ref = NULL;

	rugged_exception_check(error);
	return Qnil;
}

static VALUE rb_git_ref_name(VALUE self)
{
	git_reference *ref;
	UNPACK_REFERENCE(self, ref);
	return rugged_str_new2(git_reference_name(ref), NULL);
}

static VALUE rb_git_ref_resolve(VALUE self)
{
	git_reference *ref;
	git_reference *resolved;
	int error;

	UNPACK_REFERENCE(self, ref);

	error = git_reference_resolve(&resolved, ref);
	rugged_exception_check(error);

	return rugged_ref_new(rb_cRuggedReference, rugged_owner(self), resolved);
}

static VALUE rb_git_ref_rename(int argc, VALUE *argv, VALUE self)
{
	git_reference *ref;
	VALUE rb_name, rb_force;
	int error, force = 0;

	UNPACK_REFERENCE(self, ref);
	rb_scan_args(argc, argv, "11", &rb_name, &rb_force);

	Check_Type(rb_name, T_STRING);
	if (!NIL_P(rb_force))
		force = rugged_parse_bool(rb_force);

	error = git_reference_rename(ref, StringValueCStr(rb_name), force);
	rugged_exception_check(error);

	return Qnil;
}

static VALUE rb_git_ref_delete(VALUE self)
{
	git_reference *ref;
	int error;

	UNPACK_REFERENCE(self, ref);

	error = git_reference_delete(ref);
	rugged_exception_check(error);

	DATA_PTR(self) = NULL; /* this reference has been free'd */
	rugged_set_owner(self, Qnil); /* and is no longer owned */
	return Qnil;
}

static VALUE reflog_entry_new(const git_reflog_entry *entry)
{
	VALUE rb_entry = rb_hash_new();

	rb_hash_aset(rb_entry,
		CSTR2SYM("oid_old"),
		rugged_create_oid(git_reflog_entry_oidold(entry))
	);

	rb_hash_aset(rb_entry,
		CSTR2SYM("oid_new"),
		rugged_create_oid(git_reflog_entry_oidnew(entry))
	);

	rb_hash_aset(rb_entry,
		CSTR2SYM("committer"),
		rugged_signature_new(git_reflog_entry_committer(entry), NULL)
	);

	rb_hash_aset(rb_entry,
		CSTR2SYM("message"),
		rugged_str_new2(git_reflog_entry_msg(entry), NULL)
	);

	return rb_entry;
}

static VALUE rb_git_reflog(VALUE self)
{
	git_reflog *reflog;
	git_reference *ref;
	int error;
	VALUE rb_log;
	unsigned int i, ref_count;

	UNPACK_REFERENCE(self, ref);

	error = git_reflog_read(&reflog, ref);
	rugged_exception_check(error);

	ref_count = git_reflog_entrycount(reflog);
	rb_log = rb_ary_new2(ref_count);

	for (i = 0; i < ref_count; ++i) {
		const git_reflog_entry *entry =
			git_reflog_entry_byindex(reflog, i);

		rb_ary_push(rb_log, reflog_entry_new(entry));
	}

	git_reflog_free(reflog);
	return rb_log;
}

static VALUE rb_git_reflog_write(
	VALUE self,
	VALUE rb_oid_old,
	VALUE rb_committer,
	VALUE rb_message)
{
	git_reference *ref;
	int error;
	git_signature *committer;
	const char *message = NULL;
	git_oid oid_old;

	UNPACK_REFERENCE(self, ref);

	if (!NIL_P(rb_message)) {
		Check_Type(rb_message, T_STRING);
		message = StringValueCStr(rb_message);
	}

	if (!NIL_P(rb_oid_old)) {
		Check_Type(rb_oid_old, T_STRING);
		error = git_oid_fromstr(&oid_old, StringValueCStr(rb_oid_old));
		rugged_exception_check(error);
	}

	committer = rugged_signature_get(rb_committer);

	error = git_reflog_write(
		ref,
		NIL_P(rb_oid_old) ? NULL : &oid_old,
		committer,
		message
	);

	git_signature_free(committer);

	rugged_exception_check(error);
	return Qnil;
}


void Init_rugged_reference()
{
	rb_cRuggedReference = rb_define_class_under(rb_mRugged, "Reference", rb_cObject);

	rb_define_singleton_method(rb_cRuggedReference, "lookup", rb_git_ref_lookup, 2);
	rb_define_singleton_method(rb_cRuggedReference, "create", rb_git_ref_create, -1);
	rb_define_singleton_method(rb_cRuggedReference, "pack_all", rb_git_ref_packall, 1);
	rb_define_singleton_method(rb_cRuggedReference, "each", rb_git_ref_each, -1);

	rb_define_method(rb_cRuggedReference, "target", rb_git_ref_target, 0);
	rb_define_method(rb_cRuggedReference, "target=", rb_git_ref_set_target, 1);

	rb_define_method(rb_cRuggedReference, "type", rb_git_ref_type, 0);

	rb_define_method(rb_cRuggedReference, "name", rb_git_ref_name, 0);
	rb_define_method(rb_cRuggedReference, "rename", rb_git_ref_rename, -1);

	rb_define_method(rb_cRuggedReference, "reload!", rb_git_ref_reload, 0);
	rb_define_method(rb_cRuggedReference, "resolve", rb_git_ref_resolve, 0);
	rb_define_method(rb_cRuggedReference, "delete!", rb_git_ref_delete, 0);

	rb_define_method(rb_cRuggedReference, "packed?", rb_git_ref_packed, 0);
	rb_define_method(rb_cRuggedReference, "log", rb_git_reflog, 0);
	rb_define_method(rb_cRuggedReference, "log!", rb_git_reflog_write, 3);
}
