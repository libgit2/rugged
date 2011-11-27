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
VALUE rb_cRuggedWalker;

static void rb_git_walk__free(git_revwalk *walk)
{
	git_revwalk_free(walk);
}

VALUE rugged_walker_new(VALUE klass, VALUE owner, git_revwalk *walk)
{
	VALUE rb_walk = Data_Wrap_Struct(klass, NULL, &rb_git_walk__free, walk);
	rugged_set_owner(rb_walk, owner);
	return rb_walk;
}

static VALUE rb_git_walker_new(VALUE klass, VALUE rb_repo)
{
	rugged_repository *repo;
	git_revwalk *walk;
	int error;

	Data_Get_Struct(rb_repo, rugged_repository, repo);

	error = git_revwalk_new(&walk, repo->repo);
	rugged_exception_check(error);

	return rugged_walker_new(klass, rb_repo, walk);;
}

static VALUE rb_git_walker_each(VALUE self)
{
	git_revwalk *walk;
	git_commit *commit;
	git_repository *repo;
	git_oid commit_oid;
	int error;

	Data_Get_Struct(self, git_revwalk, walk);
	repo = git_revwalk_repository(walk);

	if (!rb_block_given_p())
		return rb_funcall(self, rb_intern("to_enum"), 0);

	while ((error = git_revwalk_next(&commit_oid, walk)) == 0) {
		error = git_commit_lookup(&commit, repo, &commit_oid);
		rugged_exception_check(error);

		rb_yield(rugged_object_new(rugged_owner(self), (git_object *)commit));
	}

	if (error != GIT_EREVWALKOVER)
		rugged_exception_check(error);

	return Qnil;
}

static VALUE rb_git_walker_push(VALUE self, VALUE rb_commit)
{
	git_revwalk *walk;
	git_commit *commit;

	Data_Get_Struct(self, git_revwalk, walk);

	commit = (git_commit *)rugged_object_load(
		git_revwalk_repository(walk), rb_commit, GIT_OBJ_COMMIT);

	git_revwalk_push(walk, git_object_id((git_object *)commit));
	return Qnil;
}

static VALUE rb_git_walker_hide(VALUE self, VALUE rb_commit)
{
	git_revwalk *walk;
	git_commit *commit;

	Data_Get_Struct(self, git_revwalk, walk);

	commit = (git_commit *)rugged_object_load(
		git_revwalk_repository(walk), rb_commit, GIT_OBJ_COMMIT);

	git_revwalk_hide(walk, git_object_id((git_object *)commit));
	return Qnil;
}

static VALUE rb_git_walker_sorting(VALUE self, VALUE ruby_sort_mode)
{
	git_revwalk *walk;
	Data_Get_Struct(self, git_revwalk, walk);
	git_revwalk_sorting(walk, FIX2INT(ruby_sort_mode));
	return Qnil;
}

static VALUE rb_git_walker_reset(VALUE self)
{
	git_revwalk *walk;
	Data_Get_Struct(self, git_revwalk, walk);
	git_revwalk_reset(walk);
	return Qnil;
}

void Init_rugged_revwalk()
{
	rb_cRuggedWalker = rb_define_class_under(rb_mRugged, "Walker", rb_cObject);
	rb_define_singleton_method(rb_cRuggedWalker, "new", rb_git_walker_new, 1);

	rb_define_method(rb_cRuggedWalker, "push", rb_git_walker_push, 1);
	rb_define_method(rb_cRuggedWalker, "each", rb_git_walker_each, 0);
	rb_define_method(rb_cRuggedWalker, "walk", rb_git_walker_each, 0);
	rb_define_method(rb_cRuggedWalker, "hide", rb_git_walker_hide, 1);
	rb_define_method(rb_cRuggedWalker, "reset", rb_git_walker_reset, 0);
	rb_define_method(rb_cRuggedWalker, "sorting", rb_git_walker_sorting, 1);
}
