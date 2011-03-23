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
extern VALUE rb_cRuggedObject;
VALUE rb_cRuggedCommit;

/*
 * Commit code
 */
static VALUE rb_git_commit_message_GET(VALUE self)
{
	git_commit *commit;
	RUGGED_OBJ_UNWRAP(self, git_commit, commit);

	return rugged_str_new2(git_commit_message(commit), NULL);
}

static VALUE rb_git_commit_message_short_GET(VALUE self)
{
	git_commit *commit;
	RUGGED_OBJ_UNWRAP(self, git_commit, commit);

	return rugged_str_new2(git_commit_message_short(commit), NULL);
}

static VALUE rb_git_commit_committer_GET(VALUE self)
{
	git_commit *commit;
	RUGGED_OBJ_UNWRAP(self, git_commit, commit);

	return rugged_signature_new(git_commit_committer(commit));
}

static VALUE rb_git_commit_author_GET(VALUE self)
{
	git_commit *commit;
	RUGGED_OBJ_UNWRAP(self, git_commit, commit);

	return rugged_signature_new(git_commit_author(commit));
}

static VALUE rb_git_commit_time_GET(VALUE self)
{
	git_commit *commit;
	RUGGED_OBJ_UNWRAP(self, git_commit, commit);

	return ULONG2NUM(git_commit_time(commit));
}

static VALUE rb_git_commit_tree_GET(VALUE self)
{
	git_commit *commit;
	git_tree *tree;
	VALUE owner;
	int error;

	RUGGED_OBJ_UNWRAP(self, git_commit, commit);
	RUGGED_OBJ_OWNER(self, owner);

	error = git_commit_tree(&tree, commit);
	rugged_exception_check(error);

	return rugged_object_new(owner, (git_object *)tree);
}

static VALUE rb_git_commit_parents_GET(VALUE self)
{
	git_commit *commit;
	git_commit *parent;
	unsigned int n;
	VALUE ret_arr, owner;
	int error;

	RUGGED_OBJ_UNWRAP(self, git_commit, commit);
	RUGGED_OBJ_OWNER(self, owner);

	ret_arr = rb_ary_new();
	for (n = 0; n < git_commit_parentcount(commit); n++) {
		error = git_commit_parent(&parent, commit, n);
		rugged_exception_check(error);
		rb_ary_store(ret_arr, n, rugged_object_new(owner, (git_object *)parent));
	}

	return ret_arr;
}

void Init_rugged_commit()
{
	rb_cRuggedCommit = rb_define_class_under(rb_mRugged, "Commit", rb_cRuggedObject);

	rb_define_method(rb_cRuggedCommit, "message", rb_git_commit_message_GET, 0);
	rb_define_method(rb_cRuggedCommit, "message_short", rb_git_commit_message_short_GET, 0); 
	rb_define_method(rb_cRuggedCommit, "time", rb_git_commit_time_GET, 0); /* READ ONLY */
	rb_define_method(rb_cRuggedCommit, "committer", rb_git_commit_committer_GET, 0);
	rb_define_method(rb_cRuggedCommit, "author", rb_git_commit_author_GET, 0);
	rb_define_method(rb_cRuggedCommit, "tree", rb_git_commit_tree_GET, 0);
	rb_define_method(rb_cRuggedCommit, "parents", rb_git_commit_parents_GET, 0);
}

