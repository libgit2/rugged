/*
 * The MIT License
 *
 * Copyright (c) 2013 GitHub, Inc
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

/*
 *  call-seq:
 *    Branch.create(repository, name, target, force = false) -> branch
 *
 *  Create a new branch in +repository+, with the given +name+, and pointing
 *  to the +target+.
 *
 *  +name+ needs to be a branch name, not an absolute reference path
 *  (e.g. +development+ instead of +/refs/heads/development+).
 *
 *  +target+ needs to be an existing commit in the given +repository+.
 *
 *  If +force+ is +true+, any existing branches will be overwritten.
 *
 *  Returns a Rugged::Branch object with the newly created branch.
 */
static VALUE rb_git_branch_create(int argc, VALUE *argv, VALUE self)
{
	git_reference *branch;
	git_commit *target = NULL;
	git_repository *repo = NULL;
	int error, force = 0;

	VALUE rb_repo, rb_name, rb_target, rb_force;

	rb_scan_args(argc, argv, "31", &rb_repo, &rb_name, &rb_target, &rb_force);

	if (!rb_obj_is_kind_of(rb_repo, rb_cRuggedRepo)) {
		rb_raise(rb_eTypeError, "Expecting a Rugged::Repository instance");
	}

	Data_Get_Struct(rb_repo, git_repository, repo);

	Check_Type(rb_name, T_STRING);

	target = (git_commit *)rugged_object_get(repo, rb_target, GIT_OBJ_COMMIT);

	if (!NIL_P(rb_force)) {
		force = rugged_parse_bool(rb_force);
	}

	error = git_branch_create(&branch, repo, StringValueCStr(rb_name), target, force);
	git_commit_free(target);

	rugged_exception_check(error);

	return rugged_branch_new(rb_repo, branch);
}

/*
 *  call-seq:
 *    Branch.lookup(repository, name, branch_type = :local) -> branch
 *
 *  Lookup a branch in +repository+, with the given +name+.
 *
 *  +name+ needs to be a branch name, not an absolute reference path
 *  (e.g. +development+ instead of +/refs/heads/development+).
 *
 *  +branch_type+ specifies whether the looked up branch is a local branch
 *  or a remote one. It defaults to looking up local branches.
 *
 *  Returns the looked up branch, or +nil+ if the branch doesn't exist.
 */
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

/*
 *  call-seq:
 *    branch.delete! -> nil
 *
 *  Remove a branch from the repository. The branch object will become invalidated
 *  and won't be able to be used for any other operations.
 */
static VALUE rb_git_branch_delete(VALUE self)
{
	git_reference *branch = NULL;

	Data_Get_Struct(self, git_reference, branch);

	rugged_exception_check(
		git_branch_delete(branch)
	);

	return Qnil;
}

static int cb_branch__each_name(const char *branch_name, git_branch_t branch_type, void *data)
{
	struct rugged_cb_payload *payload = data;

	rb_protect(rb_yield, rb_str_new_utf8(branch_name), &payload->exception);

	return payload->exception ? GIT_ERROR : GIT_OK;
}

static int cb_branch__each_obj(const char *branch_name, git_branch_t branch_type, void *data)
{
	git_reference *branch;
	git_repository *repo;
	struct rugged_cb_payload *payload = data;

	Data_Get_Struct(payload->rb_data, git_repository, repo);

	rugged_exception_check(
		git_branch_lookup(&branch, repo, branch_name, branch_type)
	);

	rb_protect(rb_yield, rugged_branch_new(payload->rb_data, branch), &payload->exception);
	return payload->exception ? GIT_ERROR : GIT_OK;
}

static VALUE each_branch(int argc, VALUE *argv, VALUE self, int branch_names_only)
{
	VALUE rb_repo, rb_filter;
	git_repository *repo;
	int error;
	struct rugged_cb_payload payload;
	int filter = (GIT_BRANCH_LOCAL | GIT_BRANCH_REMOTE);

	rb_scan_args(argc, argv, "11", &rb_repo, &rb_filter);

	payload.exception = 0;
	payload.rb_data = rb_repo;

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
		error = git_branch_foreach(repo, filter, &cb_branch__each_name, &payload);
	} else {
		error = git_branch_foreach(repo, filter, &cb_branch__each_obj, &payload);
	}

	if (payload.exception)
		rb_jump_tag(payload.exception);
	rugged_exception_check(error);

	return Qnil;
}

/*
 *  call-seq:
 *    Branch.each_name(repository, filter = :all) { |branch_name| block }
 *    Branch.each_name(repository, filter = :all) -> enumerator
 *
 *  Iterate through the names of the branches in +repository+. Iteration can be
 *  optionally filtered to yield only +:local+ or +:remote+ branches.
 *
 *  The given block will be called once with the name of each branch as a +String+.
 *  If no block is given, an enumerator will be returned.
 */
static VALUE rb_git_branch_each_name(int argc, VALUE *argv, VALUE self)
{
	return each_branch(argc, argv, self, 1);
}


/*
 *  call-seq:
 *    Branch.each(repository, filter = :all) { |branch| block }
 *    Branch.each(repository, filter = :all) -> enumerator
 *
 *  Iterate through the branches in +repository+. Iteration can be
 *  optionally filtered to yield only +:local+ or +:remote+ branches.
 *
 *  The given block will be called once with a +Rugged::Branch+ object
 *  for each branch in the repository. If no block is given, an enumerator
 *  will be returned.
 */
static VALUE rb_git_branch_each(int argc, VALUE *argv, VALUE self)
{
	return each_branch(argc, argv, self, 0);
}

/*
 *  call-seq:
 *    branch.move(new_name, force = false) -> new_branch
 *    branch.rename(new_name, force = false) -> new_branch
 *
 *  Rename a branch to +new_name+.
 *
 *  +new_name+ needs to be a branch name, not an absolute reference path
 *  (e.g. +development+ instead of +/refs/heads/development+).
 *
 *  If +force+ is +true+, the branch will be renamed even if a branch
 *  with +new_name+ already exists.
 *
 *  A new Rugged::Branch object for the renamed branch will be returned.
 */
static VALUE rb_git_branch_move(int argc, VALUE *argv, VALUE self)
{
	VALUE rb_new_branch_name, rb_force;
	git_reference *old_branch = NULL, *new_branch = NULL;
	int error, force = 0;

	rb_scan_args(argc, argv, "11", &rb_new_branch_name, &rb_force);

	Data_Get_Struct(self, git_reference, old_branch);
	Check_Type(rb_new_branch_name, T_STRING);

	if (!NIL_P(rb_force))
		force = rugged_parse_bool(rb_force);

	error = git_branch_move(&new_branch, old_branch, StringValueCStr(rb_new_branch_name), force);
	rugged_exception_check(error);

	return rugged_branch_new(rugged_owner(self), new_branch);
}

/*
 *  call-seq:
 *    branch.head? -> true or false
 *
 *  Returns +true+ if the branch is pointed at by +HEAD+, +false+ otherwise.
 */
static VALUE rb_git_branch_head_p(VALUE self)
{
	git_reference *branch;
	Data_Get_Struct(self, git_reference, branch);
	return git_branch_is_head(branch) ? Qtrue : Qfalse;
}

void Init_rugged_branch(void)
{
	rb_cRuggedBranch = rb_define_class_under(rb_mRugged, "Branch", rb_cRuggedReference);

	rb_define_singleton_method(rb_cRuggedBranch, "create", rb_git_branch_create, -1);
	rb_define_singleton_method(rb_cRuggedBranch, "lookup", rb_git_branch_lookup, -1);
	rb_define_singleton_method(rb_cRuggedBranch, "each_name", rb_git_branch_each_name, -1);
	rb_define_singleton_method(rb_cRuggedBranch, "each", rb_git_branch_each, -1);

	rb_define_method(rb_cRuggedBranch, "delete!", rb_git_branch_delete, 0);
	rb_define_method(rb_cRuggedBranch, "rename", rb_git_branch_move, -1);
	rb_define_method(rb_cRuggedBranch, "move", rb_git_branch_move, -1);
	rb_define_method(rb_cRuggedBranch, "head?", rb_git_branch_head_p, 0);
}
