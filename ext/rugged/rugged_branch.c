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
extern VALUE rb_cRuggedRepo;
extern VALUE rb_cRuggedObject;
VALUE rb_cRuggedBranch;

static VALUE rb_git_branch_create(int argc, VALUE *argv, VALUE self)
{
	git_oid branch_oid;
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

	error = git_branch_create(&branch_oid, repo, StringValueCStr(rb_name), target, force);

	git_object_free(target);

	rugged_exception_check(error);

	return rugged_create_oid(&branch_oid);
}

static VALUE rb_git_branch_delete(int argc, VALUE *argv, VALUE self)
{
	VALUE rb_repo, rb_name, rb_type;
	git_repository *repo = NULL;
	ID id_type;
	int error, type;

	rb_scan_args(argc, argv, "21", &rb_repo, &rb_name, &rb_type);

	if (!rb_obj_is_kind_of(rb_repo, rb_cRuggedRepo)) {
		rb_raise(rb_eTypeError, "Expecting a Rugged::Repository instance");
	}

	Data_Get_Struct(rb_repo, git_repository, repo);

	Check_Type(rb_name, T_STRING);

	if (!NIL_P(rb_type)) {
		Check_Type(rb_type, T_SYMBOL);
		id_type = SYM2ID(rb_type);

		if (id_type == rb_intern("local")) {
			type = GIT_BRANCH_LOCAL;
		} else if (id_type == rb_intern("remote")) {
			type = GIT_BRANCH_REMOTE;
		} else {
			rb_raise(rb_eTypeError,
				"Invalid branch type. Expected `:remote`, `:local` or `nil`");
		}
	} else {
		type = GIT_BRANCH_LOCAL;
	}

	error = git_branch_delete(repo, StringValueCStr(rb_name), type);

	rugged_exception_check(error);

	return Qnil;
}

static int cb_branch__each(const char *branch_name, git_branch_t branch_type, void *payload)
{
	rb_funcall((VALUE)payload, rb_intern("call"), 1, rugged_str_new2(branch_name, rb_utf8_encoding()));
	return GIT_OK;
}

static VALUE rb_git_branch_each(int argc, VALUE *argv, VALUE self)
{
	VALUE rb_repo, rb_filter;
	ID id_filter;
	git_repository *repo;
	int error, i, filter;

	rb_scan_args(argc, argv, "11", &rb_repo, &rb_filter);

	if (!rb_obj_is_kind_of(rb_repo, rb_cRuggedRepo)) {
		rb_raise(rb_eTypeError, "Expecting a Rugged::Repository instance");
	}

	if (!rb_block_given_p()) {
		return rb_funcall(self, rb_intern("to_enum"), 3, CSTR2SYM("each"), rb_repo, rb_filter);
	}

	if (!NIL_P(rb_filter)) {
		Check_Type(rb_filter, T_SYMBOL);
		id_filter = SYM2ID(rb_filter);

		if (id_filter == rb_intern("local")) {
			filter = GIT_BRANCH_LOCAL;
		} else if (id_filter == rb_intern("remote")) {
			filter = GIT_BRANCH_REMOTE;
		} else {
			rb_raise(rb_eTypeError,
				"Invalid branch filter. Expected `:remote`, `:local` or `nil`");
		}
	} else {
		filter = GIT_BRANCH_LOCAL | GIT_BRANCH_REMOTE;
	}

	Data_Get_Struct(rb_repo, git_repository, repo);

	error = git_branch_foreach(repo, filter, &cb_branch__each, (void *)rb_block_proc());
	rugged_exception_check(error);

	return Qnil;
}

static VALUE rb_git_branch_move(int argc, VALUE *argv, VALUE self)
{
	VALUE rb_repo, rb_old_branch_name, rb_new_branch_name, rb_force;
	git_repository *repo = NULL;
	int error, force;

	rb_scan_args(argc, argv, "31", &rb_repo, &rb_old_branch_name, &rb_new_branch_name, &rb_force);

	if (!rb_obj_is_kind_of(rb_repo, rb_cRuggedRepo)) {
		rb_raise(rb_eTypeError, "Expecting a Rugged::Repository instance");
	}

	Data_Get_Struct(rb_repo, git_repository, repo);

	Check_Type(rb_old_branch_name, T_STRING);
	Check_Type(rb_new_branch_name, T_STRING);

	if (!NIL_P(rb_force))
		force = rugged_parse_bool(rb_force);

	error = git_branch_move(repo, StringValueCStr(rb_old_branch_name), StringValueCStr(rb_new_branch_name), force);

	rugged_exception_check(error);

	return Qnil;
}

void Init_rugged_branch()
{
	rb_cRuggedBranch = rb_define_class_under(rb_mRugged, "Branch", rb_cObject);

	rb_define_singleton_method(rb_cRuggedBranch, "each", rb_git_branch_each, -1);
	rb_define_singleton_method(rb_cRuggedBranch, "delete", rb_git_branch_delete, -1);
	rb_define_singleton_method(rb_cRuggedBranch, "create", rb_git_branch_create, -1);
	rb_define_singleton_method(rb_cRuggedBranch, "move", rb_git_branch_move, -1);
}
