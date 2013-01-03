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
VALUE rb_cRuggedRemote;

static void rb_git_remote__free(git_remote *remote)
{
	git_remote_free(remote);
}

VALUE rugged_remote_new(VALUE klass, VALUE owner, git_remote *remote)
{
	VALUE rb_remote = Data_Wrap_Struct(klass, NULL, &rb_git_remote__free, remote);
	rugged_set_owner(rb_remote, owner);
	return rb_remote;
}

/*
 *  	call-seq:
 *   		Remote.new(repository, url) -> remote
 *
 *   	Return a new remote from +url+ in +repository+ , the remote is not persisted:
 *	- +url+: a valid remote url
 *
 *	Returns a new <tt>Rugged::Remote</tt> object
 *
 *		Rugged::Remote.new(@repo, 'git://github.com/libgit2/libgit2.git') #=> #<Rugged::Remote:0x00000001fbfa80>
 */
static VALUE rb_git_remote_new(VALUE klass, VALUE rb_repo, VALUE rb_url)
{
	git_remote *remote;
	git_repository *repo;
	const char *url;
	int error;

	Check_Type(rb_url, T_STRING);
	rugged_check_repo(rb_repo);

	Data_Get_Struct(rb_repo, git_repository, repo);

	url = StringValueCStr(rb_url);

	if (git_remote_valid_url(url)) {
		error = git_remote_create_inmemory(
				&remote,
				repo,
				NULL,
				url);
	} else {
		rb_raise(rb_eArgError, "Invalid URL format");
	}

	rugged_exception_check(error);

	return rugged_remote_new(klass, rb_repo, remote);
}

/*
 *  	call-seq:
 *   		Remote.lookup(repository, name) -> new_remote or nil
 *
 *   	Return an existing remote with +name+ in +repository+:
 *	- +name+: a valid remote name
 *
 *	Returns a new <tt>Rugged::Remote</tt> object or +nil+ if the
 *	remote doesn't exist
 *
 *		Rugged::Remote.lookup(@repo, 'origin') #=> #<Rugged::Remote:0x00000001fbfa80>
 */
static VALUE rb_git_remote_lookup(VALUE klass, VALUE rb_repo, VALUE rb_name)
{
	git_remote *remote;
	git_repository *repo;
	int error;

	Check_Type(rb_name, T_STRING);
	rugged_check_repo(rb_repo);
	Data_Get_Struct(rb_repo, git_repository, repo);

	error = git_remote_load(&remote, repo, StringValueCStr(rb_name));

	if (error == GIT_ENOTFOUND)
		return Qnil;

	rugged_exception_check(error);

	return rugged_remote_new(klass, rb_repo, remote);
}

static VALUE rb_git_remote_disconnect(VALUE self)
{
	git_remote *remote;
	Data_Get_Struct(self, git_remote, remote);

	git_remote_disconnect(remote);
	return Qnil;
}

static VALUE rb_git_remote_connect(VALUE self, VALUE rb_direction)
{
	int error, direction = 0;
	git_remote *remote;
	ID id_direction;

	Data_Get_Struct(self, git_remote, remote);

	Check_Type(rb_direction, T_SYMBOL);
	id_direction = SYM2ID(rb_direction);

	if (id_direction == rb_intern("fetch"))
		direction = GIT_DIRECTION_FETCH;
	else if (id_direction == rb_intern("push"))
		direction = GIT_DIRECTION_PUSH;
	else
		rb_raise(rb_eTypeError,
			"Invalid remote direction. Expected `:fetch` or `:push`");

	error = git_remote_connect(remote, direction);
	rugged_exception_check(error);

	if (rb_block_given_p())
		rb_ensure(rb_yield, self, rb_git_remote_disconnect, self);

	return Qnil;
}

static VALUE rugged_rhead_new(git_remote_head *head)
{
	VALUE rb_head = rb_hash_new();

	rb_hash_aset(rb_head, CSTR2SYM("local?"), head->local ? Qtrue : Qfalse);
	rb_hash_aset(rb_head, CSTR2SYM("oid"), rugged_create_oid(&head->oid));
	rb_hash_aset(rb_head, CSTR2SYM("loid"), rugged_create_oid(&head->loid));
	rb_hash_aset(rb_head, CSTR2SYM("name"), rugged_str_new2(head->name, NULL));

	return rb_head;
}

static int cb_remote__ls(git_remote_head *head, void *payload)
{
	rb_funcall((VALUE)payload, rb_intern("call"), 1, rugged_rhead_new(head));
	return GIT_OK;
}

static VALUE rb_git_remote_ls(VALUE self)
{
	int error;
	git_remote *remote;
	Data_Get_Struct(self, git_remote, remote);

	if (!rb_block_given_p())
		return rb_funcall(self, rb_intern("to_enum"), 1, CSTR2SYM("ls"));

	error = git_remote_ls(remote, &cb_remote__ls, (void *)rb_block_proc());
	rugged_exception_check(error);

	return Qnil;
}

static VALUE rb_git_remote_name(VALUE self)
{
	git_remote *remote;
	const char * name;
	Data_Get_Struct(self, git_remote, remote);

	name = git_remote_name(remote);

	if (name)
		return rugged_str_new2(name, NULL);
	else
		return Qnil;
}

static VALUE rb_git_remote_url(VALUE self)
{
	git_remote *remote;
	Data_Get_Struct(self, git_remote, remote);

	return rugged_str_new2(git_remote_url(remote), NULL);
}

static VALUE rb_git_remote_connected(VALUE self)
{
	git_remote *remote;
	Data_Get_Struct(self, git_remote, remote);

	return git_remote_connected(remote) ? Qtrue : Qfalse;
}

static VALUE rb_git_remote_download(VALUE self)
{
	int error;
	git_remote *remote;

	Data_Get_Struct(self, git_remote, remote);

	error = git_remote_download(remote, NULL, NULL);
	rugged_exception_check(error);

	return Qnil;
}

static VALUE rb_git_remote_update_tips(VALUE self)
{
	int error;
	git_remote *remote;

	Data_Get_Struct(self, git_remote, remote);

	// TODO: Maybe allow passing down a block?
	error = git_remote_update_tips(remote);
	rugged_exception_check(error);

	return Qnil;
}

static VALUE rb_git_remote_each(VALUE self, VALUE rb_repo)
{
	git_repository *repo;
	git_strarray remotes;
	size_t i;
	int error;

	if (!rb_block_given_p())
		return rb_funcall(self, rb_intern("to_enum"), 2, CSTR2SYM("each"), rb_repo);

	if (!rb_obj_is_kind_of(rb_repo, rb_cRuggedRepo))
		rb_raise(rb_eTypeError, "Expeting a Rugged::Repository instance");

	Data_Get_Struct(rb_repo, git_repository, repo);

	error = git_remote_list(&remotes, repo);
	rugged_exception_check(error);

	for (i = 0; i < remotes.count; ++i)
		rb_yield(rugged_str_new2(remotes.strings[i], NULL));

	git_strarray_free(&remotes);
	return Qnil;
}

void Init_rugged_remote()
{
	rb_cRuggedRemote = rb_define_class_under(rb_mRugged, "Remote", rb_cObject);

	rb_define_singleton_method(rb_cRuggedRemote, "new", rb_git_remote_new, 2);
	rb_define_singleton_method(rb_cRuggedRemote, "lookup", rb_git_remote_lookup, 2);
	rb_define_singleton_method(rb_cRuggedRemote, "each", rb_git_remote_each, 1);

	rb_define_method(rb_cRuggedRemote, "connect", rb_git_remote_connect, 1);
	rb_define_method(rb_cRuggedRemote, "disconnect", rb_git_remote_disconnect, 0);
	rb_define_method(rb_cRuggedRemote, "name", rb_git_remote_name, 0);
	rb_define_method(rb_cRuggedRemote, "url", rb_git_remote_url, 0);
	rb_define_method(rb_cRuggedRemote, "connected?", rb_git_remote_connected, 0);
	rb_define_method(rb_cRuggedRemote, "ls", rb_git_remote_ls, 0);
	rb_define_method(rb_cRuggedRemote, "download", rb_git_remote_download, 0);
	rb_define_method(rb_cRuggedRemote, "update_tips!", rb_git_remote_update_tips, 0);
}
