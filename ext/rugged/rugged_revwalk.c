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

void rb_git_walker__mark(rugged_walker *walk)
{
	rb_gc_mark(walk->owner);
}

void rb_git_walker__free(rugged_walker *walk)
{
	git_revwalk_free(walk->walk);
	free(walk);
}

static VALUE rb_git_walker_allocate(VALUE klass)
{
	rugged_walker *walker = NULL;

	walker = malloc(sizeof(rugged_walker));
	if (walker == NULL)
		rb_raise(rb_eNoMemError, "out of memory");

	walker->walk = NULL;
	walker->owner = Qnil;

	return Data_Wrap_Struct(klass, rb_git_walker__mark, rb_git_walker__free, walker);
}

static VALUE rb_git_walker_init(VALUE self, VALUE rb_repo)
{
	rugged_repository *repo;
	rugged_walker *walk;
	int error;

	Data_Get_Struct(self, rugged_walker, walk);
	Data_Get_Struct(rb_repo, rugged_repository, repo);

	error = git_revwalk_new(&walk->walk, repo->repo);
	rugged_exception_check(error);

	return Qnil;
}

static VALUE rb_git_walker_next(VALUE self)
{
	rugged_walker *walk;
	git_commit *commit;
	int error;

	Data_Get_Struct(self, rugged_walker, walk);

	error = git_revwalk_next(&commit, walk->walk);
	if (error == GIT_EREVWALKOVER)
		return Qfalse;

	rugged_exception_check(error);
	return rugged_object_new(walk->owner, (git_object*)commit);
}

static VALUE rb_git_walker_push(VALUE self, VALUE rb_commit)
{
	rugged_walker *walk;
	git_commit *commit;

	Data_Get_Struct(self, rugged_walker, walk);
	commit = (git_commit *)rugged_object_get(git_revwalk_repository(walk->walk), rb_commit, GIT_OBJ_COMMIT);
	git_revwalk_push(walk->walk, commit);

	return Qnil;
}

static VALUE rb_git_walker_hide(VALUE self, VALUE rb_commit)
{
	rugged_walker *walk;
	git_commit *commit;

	Data_Get_Struct(self, rugged_walker, walk);
	commit = (git_commit *)rugged_object_get(git_revwalk_repository(walk->walk), rb_commit, GIT_OBJ_COMMIT);
	git_revwalk_hide(walk->walk, commit);

	return Qnil;
}

static VALUE rb_git_walker_sorting(VALUE self, VALUE ruby_sort_mode)
{
	rugged_walker *walk;
	Data_Get_Struct(self, rugged_walker, walk);
	git_revwalk_sorting(walk->walk, FIX2INT(ruby_sort_mode));
	return Qnil;
}

static VALUE rb_git_walker_reset(VALUE self)
{
	rugged_walker *walk;
	Data_Get_Struct(self, rugged_walker, walk);
	git_revwalk_reset(walk->walk);
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
