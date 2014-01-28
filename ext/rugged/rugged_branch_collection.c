/*
 * The MIT License
 *
 * Copyright (c) 2014 GitHub, Inc
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
extern VALUE rb_cRuggedBranch;

VALUE rb_cRuggedBranchCollection;

static inline VALUE rugged_branch_new(VALUE owner, git_reference *ref)
{
	return rugged_ref_new(rb_cRuggedBranch, owner, ref);
}

/*
 *  call-seq:
 *    BranchCollection.new(repo) -> refs
 */
static VALUE rb_git_branch_collection_initialize(VALUE self, VALUE repo)
{
	rugged_set_owner(self, repo);
	return self;
}

static git_branch_t parse_branch_type(VALUE rb_filter)
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
 *  (e.g. +development+ instead of +refs/heads/development+).
 *
 *  +target+ needs to be an existing commit in the given +repository+.
 *
 *  If +force+ is +true+, any existing branches will be overwritten.
 *
 *  Returns a Rugged::Branch object with the newly created branch.
 */
static VALUE rb_git_branch_collection_create(int argc, VALUE *argv, VALUE self)
{
	VALUE rb_repo = rugged_owner(self), rb_name, rb_target, rb_force;
	git_repository *repo;
	git_reference *branch;
	git_commit *target;
	int error, force = 0;

	rb_scan_args(argc, argv, "21", &rb_name, &rb_target, &rb_force);

	rugged_check_repo(rb_repo);
	Data_Get_Struct(rb_repo, git_repository, repo);

	Check_Type(rb_name, T_STRING);
	Check_Type(rb_target, T_STRING);

	if (!NIL_P(rb_force))
		force = rugged_parse_bool(rb_force);

	target = (git_commit *)rugged_object_get(repo, rb_target, GIT_OBJ_COMMIT);

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
static VALUE rb_git_branch_collection_aref(int argc, VALUE *argv, VALUE self) {
	git_reference *branch;
	git_repository *repo;

	VALUE rb_repo = rugged_owner(self), rb_name, rb_type;
	int error;
	git_branch_t branch_type = GIT_BRANCH_LOCAL;

	rb_scan_args(argc, argv, "11", &rb_name, &rb_type);

	rugged_check_repo(rb_repo);
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

static VALUE each_branch(int argc, VALUE *argv, VALUE self, int branch_names_only)
{
	VALUE rb_repo = rugged_owner(self), rb_filter;
	git_repository *repo;
	git_branch_iterator *iter;
	int error, exception = 0;
	git_branch_t filter = (GIT_BRANCH_LOCAL | GIT_BRANCH_REMOTE), branch_type;

	rb_scan_args(argc, argv, "01", &rb_filter);

	if (!rb_block_given_p()) {
		VALUE symbol = branch_names_only ? CSTR2SYM("each_name") : CSTR2SYM("each");
		return rb_funcall(self, rb_intern("to_enum"), 2, symbol, rb_filter);
	}

	rugged_check_repo(rb_repo);

	if (!NIL_P(rb_filter))
		filter = parse_branch_type(rb_filter);

	Data_Get_Struct(rb_repo, git_repository, repo);

	error = git_branch_iterator_new(&iter, repo, filter);

	if (branch_names_only) {
		git_reference *branch;
		while (!exception && (error = git_branch_next(&branch, &branch_type, iter)) == GIT_OK) {
			rb_protect(rb_yield, rb_str_new_utf8(git_reference_shorthand(branch)), &exception);
		}
	} else {
		git_reference *branch;
		while (!exception && (error = git_branch_next(&branch, &branch_type, iter)) == GIT_OK) {
			rb_protect(rb_yield, rugged_branch_new(rb_repo, branch), &exception);
		}
	}

	git_branch_iterator_free(iter);

	if (exception)
		rb_jump_tag(exception);

	if (error != GIT_ITEROVER)
		rugged_exception_check(error);

	return Qnil;
}

/*
 *  call-seq:
 *    branches.each(repository, filter = nil) { |branch| block }
 *    branches.each(repository, filter = nil) -> enumerator
 *
 *  Iterate through the branches in +repository+. Iteration can be
 *  optionally filtered to yield only +:local+ or +:remote+ branches.
 *
 *  The given block will be called once with a +Rugged::Branch+ object
 *  for each branch in the repository. If no block is given, an enumerator
 *  will be returned.
 */
static VALUE rb_git_branch_collection_each(int argc, VALUE *argv, VALUE self)
{
	return each_branch(argc, argv, self, 0);
}

/*
 *  call-seq:
 *    branches.each_name(repository, filter = nil) { |branch_name| block }
 *    branches.each_name(repository, filter = nil) -> enumerator
 *
 *  Iterate through the names of the branches in +repository+. Iteration can be
 *  optionally filtered to yield only +:local+ or +:remote+ branches.
 *
 *  The given block will be called once with the name of each branch as a +String+.
 *  If no block is given, an enumerator will be returned.
 */
static VALUE rb_git_branch_collection_each_name(int argc, VALUE *argv, VALUE self)
{
	return each_branch(argc, argv, self, 1);
}

void Init_rugged_branch_collection(void)
{
	rb_cRuggedBranchCollection = rb_define_class_under(rb_mRugged, "BranchCollection", rb_cObject);
	rb_include_module(rb_cRuggedBranchCollection, rb_mEnumerable);

	rb_define_method(rb_cRuggedBranchCollection, "initialize", rb_git_branch_collection_initialize, 1);

	rb_define_method(rb_cRuggedBranchCollection, "each",       rb_git_branch_collection_each, -1);
	rb_define_method(rb_cRuggedBranchCollection, "each_name",  rb_git_branch_collection_each_name, -1);

	rb_define_method(rb_cRuggedBranchCollection, "[]",         rb_git_branch_collection_aref, -1);
	rb_define_method(rb_cRuggedBranchCollection, "create",     rb_git_branch_collection_create, -1);
}
