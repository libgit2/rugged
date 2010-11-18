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

extern VALUE rb_cRugged;
extern VALUE rb_cRuggedObject;
VALUE rb_cRuggedCommit;

VALUE rugged_person2hash(git_person *person)
{
	VALUE rb_person;

	if (person == NULL)
		return Qnil;

	rb_person = rb_hash_new();
	rb_hash_aset(rb_person, rb_str_new2("name"), rb_str_new2(git_person_name(person)));
	rb_hash_aset(rb_person, rb_str_new2("email"), rb_str_new2(git_person_email(person)));
	rb_hash_aset(rb_person, rb_str_new2("time"), ULONG2NUM(git_person_time(person)));

	return rb_person;
}

static VALUE rb_git_commit_allocate(VALUE klass)
{
	git_commit *commit = NULL;
	return Data_Wrap_Struct(klass, NULL, NULL, commit);
}

static VALUE rb_git_commit_init(int argc, VALUE *argv, VALUE self)
{
	return rb_git_object_init(GIT_OBJ_COMMIT, argc, argv, self);
}

static VALUE rb_git_commit_message_GET(VALUE self)
{
	git_commit *commit;
	Data_Get_Struct(self, git_commit, commit);

	return rb_str_new2(git_commit_message(commit));
}

static VALUE rb_git_commit_message_SET(VALUE self, VALUE val)
{
	git_commit *commit;
	Data_Get_Struct(self, git_commit, commit);

	Check_Type(val, T_STRING);
	git_commit_set_message(commit, RSTRING_PTR(val));
	return Qnil;
}

static VALUE rb_git_commit_message_short_GET(VALUE self)
{
	git_commit *commit;
	Data_Get_Struct(self, git_commit, commit);

	return rb_str_new2(git_commit_message_short(commit));
}

static VALUE rb_git_commit_committer_GET(VALUE self)
{
	git_commit *commit;
	Data_Get_Struct(self, git_commit, commit);

	return rugged_person2hash((git_person *)git_commit_committer(commit));
}

static VALUE rb_git_commit_committer_SET(VALUE self, VALUE rb_name, VALUE rb_email, VALUE rb_time)
{
	git_commit *commit;
	Data_Get_Struct(self, git_commit, commit);

	Check_Type(rb_name, T_STRING);
	Check_Type(rb_email, T_STRING);
	Check_Type(rb_time, T_FIXNUM);

	git_commit_set_committer(commit, RSTRING_PTR(rb_name), RSTRING_PTR(rb_email), FIX2INT(rb_time));
	return Qnil;
}

static VALUE rb_git_commit_author_GET(VALUE self)
{
	git_commit *commit;
	Data_Get_Struct(self, git_commit, commit);

	return rugged_person2hash((git_person *)git_commit_author(commit));
}

static VALUE rb_git_commit_author_SET(VALUE self, VALUE rb_name, VALUE rb_email, VALUE rb_time)
{
	git_commit *commit;
	Data_Get_Struct(self, git_commit, commit);

	Check_Type(rb_name, T_STRING);
	Check_Type(rb_email, T_STRING);
	Check_Type(rb_time, T_FIXNUM);

	git_commit_set_author(commit, RSTRING_PTR(rb_name), RSTRING_PTR(rb_email), FIX2INT(rb_time));
	return Qnil;
}


static VALUE rb_git_commit_time_GET(VALUE self)
{
	git_commit *commit;
	Data_Get_Struct(self, git_commit, commit);

	return ULONG2NUM(git_commit_time(commit));
}

static VALUE rb_git_commit_tree_GET(VALUE self)
{
	git_commit *commit;
	const git_tree *tree;
	Data_Get_Struct(self, git_commit, commit);

	tree = git_commit_tree(commit);
	return tree ? rugged_object2rb((git_object *)tree) : Qnil;
}

static VALUE rb_git_commit_tree_SET(VALUE self, VALUE val)
{
	git_commit *commit;
	git_tree *tree;
	Data_Get_Struct(self, git_commit, commit);

	tree = (git_tree *)rugged_rb2object(git_object_owner((git_object *)commit), val, GIT_OBJ_TREE);
	git_commit_set_tree(commit, tree);
	return Qnil;
}

void Init_rugged_commit()
{
	rb_cRuggedCommit = rb_define_class_under(rb_cRugged, "Commit", rb_cRuggedObject);
	rb_define_alloc_func(rb_cRuggedCommit, rb_git_commit_allocate);
	rb_define_method(rb_cRuggedCommit, "initialize", rb_git_commit_init, -1);

	rb_define_method(rb_cRuggedCommit, "message", rb_git_commit_message_GET, 0);
	rb_define_method(rb_cRuggedCommit, "message=", rb_git_commit_message_SET, 1);

	/* Read only */
	rb_define_method(rb_cRuggedCommit, "message_short", rb_git_commit_message_short_GET, 0); 

	/* Read only */
	rb_define_method(rb_cRuggedCommit, "time", rb_git_commit_time_GET, 0); /* READ ONLY */

	rb_define_method(rb_cRuggedCommit, "committer", rb_git_commit_committer_GET, 0);
	rb_define_method(rb_cRuggedCommit, "committer=", rb_git_commit_committer_SET, 3);

	rb_define_method(rb_cRuggedCommit, "author", rb_git_commit_author_GET, 0);
	rb_define_method(rb_cRuggedCommit, "author=", rb_git_commit_author_SET, 3);

	rb_define_method(rb_cRuggedCommit, "tree", rb_git_commit_tree_GET, 0);
	rb_define_method(rb_cRuggedCommit, "tree=", rb_git_commit_tree_SET, 1);
}

