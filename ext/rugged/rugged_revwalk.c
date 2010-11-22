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
VALUE rb_cRuggedWalker;


static VALUE rb_git_walker_allocate(VALUE klass)
{
	git_revwalk *walker = NULL;
	return Data_Wrap_Struct(klass, NULL, NULL, walker);
}

static VALUE rb_git_walker_init(VALUE self, VALUE rb_repo)
{
	git_repository *repo;
	git_revwalk *walk;
	int error;

	Data_Get_Struct(rb_repo, git_repository, repo);

	error = git_revwalk_new(&walk, repo);
	rugged_exception_check(error);

	DATA_PTR(self) = walk;
	return Qnil;
}


static VALUE rb_git_walker_next(VALUE self)
{
	git_revwalk *walk;
	git_commit *commit;

	Data_Get_Struct(self, git_revwalk, walk);
	commit = git_revwalk_next(walk);

	return commit ? rugged_object_c2rb((git_object*)commit) : Qfalse;
}

static VALUE rb_git_walker_push(VALUE self, VALUE rb_commit)
{
	git_revwalk *walk;
	git_commit *commit;

	Data_Get_Struct(self, git_revwalk, walk);
	commit = (git_commit *)rugged_object_rb2c(git_revwalk_repository(walk), rb_commit, GIT_OBJ_COMMIT);
	git_revwalk_push(walk, commit);

	return Qnil;
}

static VALUE rb_git_walker_hide(VALUE self, VALUE rb_commit)
{
	git_revwalk *walk;
	git_commit *commit;

	Data_Get_Struct(self, git_revwalk, walk);
	commit = (git_commit *)rugged_object_rb2c(git_revwalk_repository(walk), rb_commit, GIT_OBJ_COMMIT);
	git_revwalk_hide(walk, commit);

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
	rb_define_alloc_func(rb_cRuggedWalker, rb_git_walker_allocate);
	rb_define_method(rb_cRuggedWalker, "initialize", rb_git_walker_init, 1);
	rb_define_method(rb_cRuggedWalker, "push", rb_git_walker_push, 1);
	rb_define_method(rb_cRuggedWalker, "next", rb_git_walker_next, 0);
	rb_define_method(rb_cRuggedWalker, "hide", rb_git_walker_hide, 1);
	rb_define_method(rb_cRuggedWalker, "reset", rb_git_walker_reset, 0);
	rb_define_method(rb_cRuggedWalker, "sorting", rb_git_walker_sorting, 1);
}
