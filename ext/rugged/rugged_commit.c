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
extern VALUE rb_cRuggedObject;
extern VALUE rb_cRuggedRepo;
extern VALUE rb_cRuggedSignature;
VALUE rb_cRuggedCommit;

/*
 * Commit code
 */
static VALUE rb_git_commit_message_GET(VALUE self)
{
	git_commit *commit;

#ifdef HAVE_RUBY_ENCODING_H
	rb_encoding *encoding = NULL;
	const char *encoding_name;
#endif

	Data_Get_Struct(self, git_commit, commit);

#ifdef HAVE_RUBY_ENCODING_H
	encoding_name = git_commit_message_encoding(commit);
	if (encoding_name != NULL)
		encoding = rb_enc_find(encoding_name);
#endif

	return rugged_str_new2(git_commit_message(commit), encoding);
}

static VALUE rb_git_commit_committer_GET(VALUE self)
{
	git_commit *commit;
	Data_Get_Struct(self, git_commit, commit);

	return rugged_signature_new(
		git_commit_committer(commit),
		git_commit_message_encoding(commit));
}

static VALUE rb_git_commit_author_GET(VALUE self)
{
	git_commit *commit;
	Data_Get_Struct(self, git_commit, commit);

	return rugged_signature_new(
		git_commit_author(commit),
		git_commit_message_encoding(commit));
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
	git_tree *tree;
	VALUE owner;
	int error;

	Data_Get_Struct(self, git_commit, commit);
	owner = rugged_owner(self);

	error = git_commit_tree(&tree, commit);
	rugged_exception_check(error);

	return rugged_object_new(owner, (git_object *)tree);
}

static VALUE rb_git_commit_parents_GET(VALUE self)
{
	git_commit *commit;
	git_commit *parent;
	unsigned int n, parent_count;
	VALUE ret_arr, owner;
	int error;

	Data_Get_Struct(self, git_commit, commit);
	owner = rugged_owner(self);

	parent_count = git_commit_parentcount(commit);
	ret_arr = rb_ary_new2((long)parent_count);

	for (n = 0; n < parent_count; n++) {
		error = git_commit_parent(&parent, commit, n);
		rugged_exception_check(error);
		rb_ary_push(ret_arr, rugged_object_new(owner, (git_object *)parent));
	}

	return ret_arr;
}

static VALUE rb_git_commit_create(VALUE self, VALUE rb_repo, VALUE rb_data)
{
	VALUE rb_message, rb_tree, rb_parents;
	int parent_count, i, error;
	const git_commit **parents = NULL;
	git_commit **free_list = NULL;
	git_tree *tree;
	git_signature *author, *committer;
	git_oid commit_oid;
	git_repository *repo;

	Check_Type(rb_data, T_HASH);

	if (!rb_obj_is_kind_of(rb_repo, rb_cRuggedRepo))
		rb_raise(rb_eTypeError, "Expeting a Rugged::Repository instance");
	Data_Get_Struct(rb_repo, git_repository, repo);

	rb_message = rb_hash_aref(rb_data, CSTR2SYM("message"));
	Check_Type(rb_message, T_STRING);

	committer = rugged_signature_get(
		rb_hash_aref(rb_data, CSTR2SYM("committer"))
	);

	author = rugged_signature_get(
		rb_hash_aref(rb_data, CSTR2SYM("author"))
	);

	rb_parents = rb_hash_aref(rb_data, CSTR2SYM("parents"));
	Check_Type(rb_parents, T_ARRAY);

	rb_tree = rb_hash_aref(rb_data, CSTR2SYM("tree"));
	tree = (git_tree *)rugged_object_load(repo, rb_tree, GIT_OBJ_TREE);

	parent_count = (int)RARRAY_LEN(rb_parents);
	parents = xmalloc(parent_count * sizeof(void*));
	free_list = xmalloc(parent_count * sizeof(void*));

	for (i = 0; i < parent_count; ++i) {
		VALUE p = rb_ary_entry(rb_parents, i);
		git_commit *parent = NULL;
		git_commit *free_ptr = NULL;

		if (TYPE(p) == T_STRING) {
			git_oid oid;

			error = git_oid_fromstr(&oid, StringValueCStr(p));
			if (error < GIT_SUCCESS)
				goto cleanup;

			error = git_commit_lookup(&parent, repo, &oid);
			if (error < GIT_SUCCESS)
				goto cleanup;

			free_ptr = parent;

		} else if (rb_obj_is_kind_of(p, rb_cRuggedCommit)) {
			Data_Get_Struct(p, git_commit, parent);
		} else {
			rb_raise(rb_eTypeError, "Invalid type for parent object");
		}

		parents[i] = parent;
		free_list[i] = free_ptr;
	}

	error = git_commit_create(
		&commit_oid,
		repo,
		NULL,
		author,
		committer,
		NULL,
		StringValueCStr(rb_message),
		tree,
		parent_count,
		parents);

cleanup:
	git_signature_free(author);
	git_signature_free(committer);

	git_object_free((git_object *)tree);

	for (i = 0; i < parent_count; ++i)
		git_object_free((git_object *)free_list[i]);

	xfree(parents);
	xfree(free_list);
	rugged_exception_check(error);

	return rugged_create_oid(&commit_oid);
}

void Init_rugged_commit()
{
	rb_cRuggedCommit = rb_define_class_under(rb_mRugged, "Commit", rb_cRuggedObject);

	rb_define_singleton_method(rb_cRuggedCommit, "create", rb_git_commit_create, 2);

	rb_define_method(rb_cRuggedCommit, "message", rb_git_commit_message_GET, 0);
	rb_define_method(rb_cRuggedCommit, "time", rb_git_commit_time_GET, 0);
	rb_define_method(rb_cRuggedCommit, "committer", rb_git_commit_committer_GET, 0);
	rb_define_method(rb_cRuggedCommit, "author", rb_git_commit_author_GET, 0);
	rb_define_method(rb_cRuggedCommit, "tree", rb_git_commit_tree_GET, 0);
	rb_define_method(rb_cRuggedCommit, "parents", rb_git_commit_parents_GET, 0);
}

