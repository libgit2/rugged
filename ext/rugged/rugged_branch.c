/*
 * The MIT License
 *
 * Copyright (c) 2012 GitHub, Inc
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
extern VALUE rb_cRuggedRepo;
extern VALUE rb_cRuggedObject;
extern VALUE rb_cRuggedReference;
VALUE rb_cRuggedBranch;

static inline VALUE rugged_branch_new(VALUE owner, git_reference *ref)
{
	return rugged_ref_new(rb_cRuggedBranch, owner, ref);
}

static int parse_branch_type(VALUE rb_filter)
{
	ID id_filter;

	Check_Type(rb_filter, T_SYMBOL);
	id_filter = SYM2ID(rb_filter);

	if (id_filter == rb_intern("local")) {
		return GIT_BRANCH_LOCAL;
	} else if (id_filter == rb_intern("remote")) {
		return GIT_BRANCH_REMOTE;
	} else {
		rb_raise(rb_eTypeError,
			"Invalid branch filter. Expected `:remote`, `:local` or `nil`");
	}
}

static VALUE rb_git_branch_create(int argc, VALUE *argv, VALUE self)
{
	git_reference *branch;
	git_object *target = NULL;
	git_repository *repo = NULL;
	int error, force = 0;

	VALUE rb_repo, rb_name, rb_target, rb_force;

	rb_scan_args(argc, argv, "31", &rb_repo, &rb_name, &rb_target, &rb_force);

	if (!rb_obj_is_kind_of(rb_repo, rb_cRuggedRepo)) {
		rb_raise(rb_eTypeError, "Expecting a Rugged::Repository instance");
	}

	Data_Get_Struct(rb_repo, git_repository, repo);

	Check_Type(rb_name, T_STRING);

	target = rugged_object_load(repo, rb_target, GIT_OBJ_ANY);

	if (!NIL_P(rb_force)) {
		force = rugged_parse_bool(rb_force);
	}

	error = git_branch_create(&branch, repo, StringValueCStr(rb_name), target, force);
	git_object_free(target);

	rugged_exception_check(error);

	return rugged_branch_new(rb_repo, branch);
}

static VALUE rb_git_branch_lookup(int argc, VALUE *argv, VALUE self)
{
	git_reference *branch;
	git_repository *repo;

	VALUE rb_repo, rb_name, rb_type;
	int error;
	int branch_type = GIT_BRANCH_LOCAL;

	rb_scan_args(argc, argv, "21", &rb_repo, &rb_name, &rb_type);

	if (!rb_obj_is_kind_of(rb_repo, rb_cRuggedRepo)) {
		rb_raise(rb_eTypeError, "Expecting a Rugged::Repository instance");
	}

	Data_Get_Struct(rb_repo, git_repository, repo);

	Check_Type(rb_name, T_STRING);
	
	if (!NIL_P(rb_type))
		branch_type = parse_branch_type(rb_type);

	error = git_branch_lookup(&branch, repo, StringValueCStr(rb_name), branch_type);
	if (error == GIT_ENOTFOUND)
		return Qnil;

	rugged_exception_check(error);

	return rugged_branch_new(rb_repo, branch);
}

static VALUE rb_git_branch_delete(VALUE self)
{
	git_reference *branch = NULL;

	RUGGED_UNPACK_REFERENCE(self, branch);

	rugged_exception_check(
		git_branch_delete(branch)
	);

	DATA_PTR(self) = NULL; /* this reference has been free'd */
	rugged_set_owner(self, Qnil); /* and is no longer owned */
	return Qnil;
}

static int cb_branch__each_name(const char *branch_name, git_branch_t branch_type, void *payload)
{
	rb_yield(rugged_str_new2(branch_name, rb_utf8_encoding()));
	return GIT_OK;
}

static int cb_branch__each_obj(const char *branch_name, git_branch_t branch_type, void *payload)
{
	VALUE rb_repo = (VALUE)payload;

	git_reference *branch;
	git_repository *repo;

	Data_Get_Struct(rb_repo, git_repository, repo);

	rugged_exception_check(
		git_branch_lookup(&branch, repo, branch_name, branch_type)
	);

	rb_yield(rugged_branch_new(rb_repo, branch));
	return GIT_OK;
}

static VALUE each_branch(int argc, VALUE *argv, VALUE self, int branch_names_only)
{
	VALUE rb_repo, rb_filter;
	git_repository *repo;
	int error;
	int filter = (GIT_BRANCH_LOCAL | GIT_BRANCH_REMOTE);

	rb_scan_args(argc, argv, "11", &rb_repo, &rb_filter);

	if (!rb_block_given_p()) {
		VALUE symbol = branch_names_only ? CSTR2SYM("each_name") : CSTR2SYM("each");
		return rb_funcall(self, rb_intern("to_enum"), 3, symbol, rb_repo, rb_filter);
	}

	if (!rb_obj_is_kind_of(rb_repo, rb_cRuggedRepo)) {
		rb_raise(rb_eTypeError, "Expecting a Rugged::Repository instance");
	}

	if (!NIL_P(rb_filter))
		filter = parse_branch_type(rb_filter);

	Data_Get_Struct(rb_repo, git_repository, repo);

	if (branch_names_only) {
		error = git_branch_foreach(repo, filter, &cb_branch__each_name, NULL);
	} else {
		error = git_branch_foreach(repo, filter, &cb_branch__each_obj, (void *)rb_repo);
	}

	rugged_exception_check(error);
	return Qnil;
}

static VALUE rb_git_branch_each_name(int argc, VALUE *argv, VALUE self)
{
	return each_branch(argc, argv, self, 1);
}

static VALUE rb_git_branch_each(int argc, VALUE *argv, VALUE self)
{
	return each_branch(argc, argv, self, 0);
}

static VALUE rb_git_branch_move(int argc, VALUE *argv, VALUE self)
{
	VALUE rb_new_branch_name, rb_force;
	git_reference *old_branch = NULL;
	int error, force = 0;

	rb_scan_args(argc, argv, "11", &rb_new_branch_name, &rb_force);

	RUGGED_UNPACK_REFERENCE(self, old_branch);
	Check_Type(rb_new_branch_name, T_STRING);

	if (!NIL_P(rb_force))
		force = rugged_parse_bool(rb_force);

	error = git_branch_move(old_branch, StringValueCStr(rb_new_branch_name), force);
	rugged_exception_check(error);

	return Qnil;
}

void Init_rugged_branch()
{
	rb_cRuggedBranch = rb_define_class_under(rb_mRugged, "Branch", rb_cRuggedReference);

	rb_define_singleton_method(rb_cRuggedBranch, "create", rb_git_branch_create, -1);
	rb_define_singleton_method(rb_cRuggedBranch, "lookup", rb_git_branch_lookup, -1);
	rb_define_singleton_method(rb_cRuggedBranch, "each_name", rb_git_branch_each_name, -1);
	rb_define_singleton_method(rb_cRuggedBranch, "each", rb_git_branch_each, -1);

	rb_define_method(rb_cRuggedBranch, "delete!", rb_git_branch_delete, 0);
	rb_define_method(rb_cRuggedBranch, "rename", rb_git_branch_move, -1);
	rb_define_method(rb_cRuggedBranch, "move", rb_git_branch_move, -1);
}
