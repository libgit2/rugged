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
	unsigned int n, parent_count;
	VALUE ret_arr, owner;
	int error;

	RUGGED_OBJ_UNWRAP(self, git_commit, commit);
	RUGGED_OBJ_OWNER(self, owner);

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
	VALUE rb_message, rb_committer, rb_author, rb_tree, rb_parents;
	int parent_count, i, error;
	const git_commit **parents;
	git_tree *tree;
	git_signature *author, *committer;
	git_oid commit_oid;
	rugged_repository *repo;

	Check_Type(rb_data, T_HASH);

	if (!rb_obj_is_kind_of(rb_repo, rb_cRuggedRepo))
		rb_raise(rb_eTypeError, "Expeting a Rugged::Repository instance");
	Data_Get_Struct(rb_repo, rugged_repository, repo);

	rb_message = rb_hash_aref(rb_data, CSTR2SYM("message"));
	Check_Type(rb_message, T_STRING);

	rb_committer = rb_hash_aref(rb_data, CSTR2SYM("committer"));
	if (!rb_obj_is_kind_of(rb_committer, rb_cRuggedSignature))
		rb_raise(rb_eTypeError, "Expeting a Rugged::Signature instance");
	Data_Get_Struct(rb_committer, git_signature, committer);

	rb_author = rb_hash_aref(rb_data, CSTR2SYM("author"));
	if (!rb_obj_is_kind_of(rb_author, rb_cRuggedSignature))
		rb_raise(rb_eTypeError, "Expeting a Rugged::Signature instance");
	Data_Get_Struct(rb_author, git_signature, author);

	rb_parents = rb_hash_aref(rb_data, CSTR2SYM("parents"));
	Check_Type(rb_parents, T_ARRAY);

	rb_tree = rb_hash_aref(rb_data, CSTR2SYM("tree"));
	tree = (git_tree *)rugged_object_get(repo->repo, rb_tree, GIT_OBJ_TREE);

	parent_count = RARRAY_LEN(rb_parents);
	parents = malloc(parent_count * sizeof(void*));

	for (i = 0; i < parent_count; ++i) {
		VALUE p = rb_ary_entry(rb_parents, i);
		git_commit *parent = NULL;

		if (TYPE(p) == T_STRING) {
			git_oid oid;

			error = git_oid_fromstr(&oid, StringValueCStr(p));
			if (error < GIT_SUCCESS)
				goto cleanup;

			error = git_commit_lookup(&parent, repo->repo, &oid);
			if (error < GIT_SUCCESS)
				goto cleanup;

		} else if (rb_obj_is_kind_of(p, rb_cRuggedCommit)) {
			RUGGED_OBJ_UNWRAP(p, git_commit, parent);
		} else {
			error = GIT_EINVALIDTYPE;
			goto cleanup;
		}

		parents[i] = parent;
	}

	error = git_commit_create(
		&commit_oid,
		repo->repo,
		NULL,
		author,
		committer,
		NULL,
		StringValueCStr(rb_message),
		tree,
		parent_count,
		parents);

cleanup:
	git_object_close((git_object *)tree);

	for (i = 0; i < parent_count; ++i)
		git_object_close((git_object *)parents[i]);

	free(parents);
	rugged_exception_check(error);

	return rugged_create_oid(&commit_oid);
}

void Init_rugged_commit()
{
	rb_cRuggedCommit = rb_define_class_under(rb_mRugged, "Commit", rb_cRuggedObject);

	rb_define_singleton_method(rb_cRuggedCommit, "create", rb_git_commit_create, 2);

	rb_define_method(rb_cRuggedCommit, "message", rb_git_commit_message_GET, 0);
	rb_define_method(rb_cRuggedCommit, "message_short", rb_git_commit_message_short_GET, 0); 
	rb_define_method(rb_cRuggedCommit, "time", rb_git_commit_time_GET, 0);
	rb_define_method(rb_cRuggedCommit, "committer", rb_git_commit_committer_GET, 0);
	rb_define_method(rb_cRuggedCommit, "author", rb_git_commit_author_GET, 0);
	rb_define_method(rb_cRuggedCommit, "tree", rb_git_commit_tree_GET, 0);
	rb_define_method(rb_cRuggedCommit, "parents", rb_git_commit_parents_GET, 0);
}

