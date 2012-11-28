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
 *	call-seq:
 *		commit.message -> msg
 *
 *	Return the message of this commit. This includes the full body of the
 *	message, with the short description, detailed descritpion, and any
 *	optional footers or signatures after it.
 *
 *	In Ruby 1.9.X, the returned string will be encoded with the encoding
 *	specified in the +Encoding+ header of the commit, if available.
 *
 *		commit.message #=> "add a lot of RDoc docs\n\nthis includes docs for commit and blob"
 */
static VALUE rb_git_commit_message_GET(VALUE self)
{
	git_commit *commit;

#ifdef HAVE_RUBY_ENCODING_H
	rb_encoding *encoding = rb_utf8_encoding();
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

/*
 *	call-seq:
 *		commit.committer -> signature
 *
 *	Return the signature for the committer of this +commit+. The signature
 *	is returned as a +Hash+ containing +:name+, +:email+ of the author
 *	and +:time+ of the change.
 *
 *	The committer of a commit is the person who actually applied the changes
 *	of the commit; in most cases it's the same as the author.
 *
 *	In Ruby 1.9.X, the returned string will be encoded with the encoding
 *	specified in the +Encoding+ header of the commit, if available.
 *
 *		commit.committer #=> {:email=>"tanoku@gmail.com", :time=>Tue Jan 24 05:42:45 UTC 2012, :name=>"Vicent Mart\303\255"}
 */
static VALUE rb_git_commit_committer_GET(VALUE self)
{
	git_commit *commit;
	Data_Get_Struct(self, git_commit, commit);

	return rugged_signature_new(
		git_commit_committer(commit),
		git_commit_message_encoding(commit));
}

/*
 *	call-seq:
 *		commit.author -> signature
 *
 *	Return the signature for the author of this +commit+. The signature
 *	is returned as a +Hash+ containing +:name+, +:email+ of the author
 *	and +:time+ of the change.
 *
 *	The author of the commit is the person who intially created the changes.
 *
 *	In Ruby 1.9.X, the returned string will be encoded with the encoding
 *	specified in the +Encoding+ header of the commit, if available.
 *
 *		commit.author #=> {:email=>"tanoku@gmail.com", :time=>Tue Jan 24 05:42:45 UTC 2012, :name=>"Vicent Mart\303\255"}
 */
static VALUE rb_git_commit_author_GET(VALUE self)
{
	git_commit *commit;
	Data_Get_Struct(self, git_commit, commit);

	return rugged_signature_new(
		git_commit_author(commit),
		git_commit_message_encoding(commit));
}

/*
 *	call-seq:
 *		commit.epoch_time -> t
 *
 *	Return the time when this commit was made effective. This is the same value
 *	as the +:time+ attribute for +commit.committer+, but represented as an +Integer+
 *	value in seconds since the Epoch.
 *
 *		commit.time #=> 1327383765
 */
static VALUE rb_git_commit_epoch_time_GET(VALUE self)
{
	git_commit *commit;
	Data_Get_Struct(self, git_commit, commit);

	return ULONG2NUM(git_commit_time(commit));
}

/*
 *	call-seq:
 *		commit.tree -> tree
 *
 *	Return the tree pointed at by this +commit+. The tree is
 *	returned as a +Rugged::Tree+ object.
 *
 *		commit.tree #=> #<Rugged::Tree:0x10882c680>
 */
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

/*
 *	call-seq:
 *		commit.tree_id -> oid
 *
 *	Return the tree oid pointed at by this +commit+. The tree is
 *	returned as a String object.
 *
 *		commit.tree_id #=> "f148106ca58764adc93ad4e2d6b1d168422b9796"
 */
static VALUE rb_git_commit_tree_id_GET(VALUE self)
{
	git_commit *commit;
	const git_oid *tree_id;

	Data_Get_Struct(self, git_commit, commit);

	tree_id = git_commit_tree_id(commit);

	return rugged_create_oid(tree_id);
}

/*
 *	call-seq:
 *		commit.parents -> [commit, ...]
 *
 *	Return the parent(s) of this commit as an array of +Rugged::Commit+
 *	objects. An array is always returned even when the commit has only
 *	one or zero parents.
 *
 *		commit.parents #=> => [#<Rugged::Commit:0x108828918>]
 *		root.parents #=> []
 */
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

/*
 *	call-seq:
 *		commit.parent_ids -> [oid, ...]
 *
 *	Return the parent oid(s) of this commit as an array of oid String
 *	objects. An array is always returned even when the commit has only
 *	one or zero parents.
 *
 *		commit.parent_ids #=> => ["2cb831a8aea28b2c1b9c63385585b864e4d3bad1", ...]
 *		root.parent_ids #=> []
 */
static VALUE rb_git_commit_parent_ids_GET(VALUE self)
{
	git_commit *commit;
	const git_oid *parent_id;
	unsigned int n, parent_count;
	VALUE ret_arr;

	Data_Get_Struct(self, git_commit, commit);

	parent_count = git_commit_parentcount(commit);
	ret_arr = rb_ary_new2((long)parent_count);

	for (n = 0; n < parent_count; n++) {
		parent_id = git_commit_parent_id(commit, n);
		if (parent_id) {
			rb_ary_push(ret_arr, rugged_create_oid(parent_id));
		}
	}

	return ret_arr;
}

/*
 *	call-seq:
 *		Commit.create(repository, data = {}) -> oid
 *
 *	Write a new +Commit+ object to +repository+, with the given +data+
 *	arguments, passed as a +Hash+:
 *
 *	- +:message+: a string with the full text for the commit's message
 *	- +:committer+: a hash with the signature for the committer
 *	- +:author+: a hash with the signature for the author
 *	- +:parents+: an +Array+ with zero or more parents for this commit,
 *	  represented as <tt>Rugged::Commit</tt> instances, or OID +String+.
 *	- +:tree+: the tree for this commit, represented as a <tt>Rugged::Tree</tt>
 *	  instance or an OID +String+.
 *	- +:update_ref+ (optional): a +String+ with the name of a reference in the
 *	repository which should be updated to point to this commit (e.g. "HEAD")
 *
 *	When the commit is successfully written to disk, its +oid+ will be
 *	returned as a hex +String+.
 *
 *		author = {:email=>"tanoku@gmail.com", :time=>Time.now, :name=>"Vicent Mart\303\255"}
 *
 *		Rugged::Commit.create(r,
 *			:author => author,
 *			:message => "Hello world\n\n",
 *			:committer => author,
 *			:parents => ["2cb831a8aea28b2c1b9c63385585b864e4d3bad1"],
 *			:tree => some_tree) #=> "f148106ca58764adc93ad4e2d6b1d168422b9796"
 */
static VALUE rb_git_commit_create(VALUE self, VALUE rb_repo, VALUE rb_data)
{
	VALUE rb_message, rb_tree, rb_parents, rb_ref;
	int parent_count, i, error;
	const git_commit **parents = NULL;
	git_commit **free_list = NULL;
	git_tree *tree;
	git_signature *author, *committer;
	git_oid commit_oid;
	git_repository *repo;
	const char *update_ref = NULL;

	Check_Type(rb_data, T_HASH);

	if (!rb_obj_is_kind_of(rb_repo, rb_cRuggedRepo))
		rb_raise(rb_eTypeError, "Expecting a Rugged::Repository instance");
	Data_Get_Struct(rb_repo, git_repository, repo);

	rb_ref = rb_hash_aref(rb_data, CSTR2SYM("update_ref"));
	if (!NIL_P(rb_ref)) {
		Check_Type(rb_ref, T_STRING);
		update_ref = StringValueCStr(rb_ref);
	}

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

	parents = xmalloc(RARRAY_LEN(rb_parents) * sizeof(void *));
	free_list = xmalloc(RARRAY_LEN(rb_parents) * sizeof(void *));
	parent_count = 0;

	for (i = 0; i < (int)RARRAY_LEN(rb_parents); ++i) {
		VALUE p = rb_ary_entry(rb_parents, i);
		git_commit *parent = NULL;
		git_commit *free_ptr = NULL;

		if (NIL_P(p))
			continue;

		if (TYPE(p) == T_STRING) {
			git_oid oid;

			error = git_oid_fromstr(&oid, StringValueCStr(p));
			if (error < GIT_OK)
				goto cleanup;

			error = git_commit_lookup(&parent, repo, &oid);
			if (error < GIT_OK)
				goto cleanup;

			free_ptr = parent;

		} else if (rb_obj_is_kind_of(p, rb_cRuggedCommit)) {
			Data_Get_Struct(p, git_commit, parent);
		} else {
			rb_raise(rb_eTypeError, "Invalid type for parent object");
		}

		parents[parent_count] = parent;
		free_list[parent_count] = free_ptr;
		parent_count++;
	}

	error = git_commit_create(
		&commit_oid,
		repo,
		update_ref,
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
	rb_define_method(rb_cRuggedCommit, "epoch_time", rb_git_commit_epoch_time_GET, 0);
	rb_define_method(rb_cRuggedCommit, "committer", rb_git_commit_committer_GET, 0);
	rb_define_method(rb_cRuggedCommit, "author", rb_git_commit_author_GET, 0);
	rb_define_method(rb_cRuggedCommit, "tree", rb_git_commit_tree_GET, 0);

	rb_define_method(rb_cRuggedCommit, "tree_id", rb_git_commit_tree_id_GET, 0);
	rb_define_method(rb_cRuggedCommit, "tree_oid", rb_git_commit_tree_id_GET, 0);

	rb_define_method(rb_cRuggedCommit, "parents", rb_git_commit_parents_GET, 0);
	rb_define_method(rb_cRuggedCommit, "parent_ids", rb_git_commit_parent_ids_GET, 0);
	rb_define_method(rb_cRuggedCommit, "parent_oids", rb_git_commit_parent_ids_GET, 0);
}

