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

static inline void rugged_validate_remote_url(VALUE rb_url)
{
	Check_Type(rb_url, T_STRING);
	if (!git_remote_valid_url(StringValueCStr(rb_url)))
		rb_raise(rb_eArgError, "Invalid URL format");
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
	int error;

	rugged_check_repo(rb_repo);
	rugged_validate_remote_url(rb_url);

	Data_Get_Struct(rb_repo, git_repository, repo);


	error = git_remote_create_inmemory(
			&remote,
			repo,
			NULL,
			StringValueCStr(rb_url));

	rugged_exception_check(error);

	return rugged_remote_new(klass, rb_repo, remote);
}

/*
 *  	call-seq:
 *   		Remote.add(repository, name, url) -> remote
 *
 *   	Add a new remote with +name+ from +url+ in +repository+
 *	- +url+: a valid remote url
 *	- +name+: a valid remote name
 *
 *	Returns a new <tt>Rugged::Remote</tt> object
 *
 *		Rugged::Remote.add(@repo, 'origin', "git://github.com/libgit2/libgit2.git") #=> #<Rugged::Remote:0x00000001fbfa80>
 */
static VALUE rb_git_remote_add(VALUE klass, VALUE rb_repo,VALUE rb_name, VALUE rb_url)
{
	git_remote *remote;
	git_repository *repo;
	int error;

	Check_Type(rb_name, T_STRING);
	rugged_validate_remote_url(rb_url);
	rugged_check_repo(rb_repo);

	Data_Get_Struct(rb_repo, git_repository, repo);

	error = git_remote_create(
			&remote,
			repo,
			StringValueCStr(rb_name),
			StringValueCStr(rb_url));

	rugged_exception_check(error);

	return rugged_remote_new(klass, rb_repo, remote);
}

/*
 *  	call-seq:
 *   		Remote.lookup(repository, name) -> remote or nil
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

/*
 * 	call-seq:
 * 		remote.disconnect() -> nil
 *
 *	Disconnect from the remote, closes the connection to the remote
 */
static VALUE rb_git_remote_disconnect(VALUE self)
{
	git_remote *remote;
	Data_Get_Struct(self, git_remote, remote);

	git_remote_disconnect(remote);
	return Qnil;
}

/*
 *	call-seq:
 *		remote.connect(direction) -> nil
 *		remote.connect(direction) { |connected_remote| block }
 *
 * 	Open a connection to a remote:
 * 	- +direction+: +:fetch+ or +:pull+
 *
 * 	If a block is given it will be passed the connected remote as argument
 * 	and the remote will be disconnected when the block terminates.
 *
 *	The transport is selected based on the URL.
 *
 *		remote.connect(:fetch) #=> nil
 *
 *		remote.connect(:fetch) do |r|
 *		  r.connected?  #=> true
 *		end
 *
 */
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

/*
 *	call-seq:
 *		remote.ls() -> an_enumerator
 *		remote.ls() { |remote_head_hash| block }
 *
 *	List references avaliable in a connected +remote+ reposotory allong
 *	with the associated commit IDs.
 *
 * 	Call the given block once for each remote head in the +remote+ as a
 * 	+Hash+.
 * 	If no block is given an Enumerator is returned.
 *
 *		remote.connect(:fetch) do |r|
 *		  r.ls.to_a #=> [{:local?=>false, :oid=>"b3ee97a91b02e91c35394950bda6ea622044baad", :loid=>"0000000000000000000000000000000000000000", :name=>"refs/heads/development"}]
 *		end
 *
 *	remote head hash includes:
 * 	[:oid] oid of the remote head
 * 	[:name] name of the remote head
 *
 *
 */
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

/*
 * 	call-seq:
 * 		remote.name() -> string
 *
 *	Returns the remote's name
 *		remote.name #=> "origin"
 */
static VALUE rb_git_remote_name(VALUE self)
{
	git_remote *remote;
	const char * name;
	Data_Get_Struct(self, git_remote, remote);

	name = git_remote_name(remote);

	return name ? rugged_str_new2(name, NULL) : Qnil;
}

/*
 * 	call-seq:
 * 		remote.url() -> string
 *
 *	Returns the remote's url
 *		remote.url #=> "git://github.com/libgit2/rugged.git"
 */
static VALUE rb_git_remote_url(VALUE self)
{
	git_remote *remote;
	Data_Get_Struct(self, git_remote, remote);

	return rugged_str_new2(git_remote_url(remote), NULL);
}

/*
 * 	call-seq:
 * 		remote.url = url -> url
 *
 *	Sets the remote's url without persisting it in the config.
 *	Existing connections will not be updated.
 *		remote.url = 'git://github.com/libgit2/rugged.git' #=> "git://github.com/libgit2/rugged.git"
 */
static VALUE rb_git_remote_set_url(VALUE self, VALUE rb_url)
{
	git_remote *remote;

	rugged_validate_remote_url(rb_url);
	Data_Get_Struct(self, git_remote, remote);

	rugged_exception_check(git_remote_set_url(remote, StringValueCStr(rb_url)));
	return rb_url;
}

/*
 * 	call-seq:
 * 		remote.push_url() -> string or nil
 *
 *	Returns the remote's url for pushing or nil if no special url for
 *	pushing is set.
 *		remote.push_url #=> "git://github.com/libgit2/rugged.git"
 */
static VALUE rb_git_remote_push_url(VALUE self)
{
	git_remote *remote;
	const char * push_url;

	Data_Get_Struct(self, git_remote, remote);

	push_url = git_remote_pushurl(remote);
	return push_url ? rugged_str_new2(push_url, NULL) : Qnil;
}

/*
 * 	call-seq:
 * 		remote.push_url = url -> url
 *
 *	Sets the remote's url for pushing without persisting it in the config.
 *	Existing connections will not be updated.
 *		remote.push_url = 'git@github.com/libgit2/rugged.git' #=> "git@github.com/libgit2/rugged.git"
 */
static VALUE rb_git_remote_set_push_url(VALUE self, VALUE rb_url)
{
	git_remote *remote;

	rugged_validate_remote_url(rb_url);
	Data_Get_Struct(self, git_remote, remote);

	rugged_exception_check(
		git_remote_set_pushurl(remote, StringValueCStr(rb_url))
	);

	return rb_url;
}

/*
 * 	call-seq:
 * 		remote.connected? -> true or false
 *
 *	Returns if the remote is connected
 *
 *		remote.connected? #=> false
 *		remote.connect(:fetch)
 *		remote.connected? #=> true
 */
static VALUE rb_git_remote_connected(VALUE self)
{
	git_remote *remote;
	Data_Get_Struct(self, git_remote, remote);

	return git_remote_connected(remote) ? Qtrue : Qfalse;
}

/*
 * 	call-seq:
 * 		remote.download() -> nil
 *
 * 	Download the packfile from a connected remote
 *
 *	Negotiate what objects should be downloaded and download the
 *	packfile with those objects.
 *
 *		remote.connect(:fetch) do |r|
 *		  r.download
 *		end
 */
static VALUE rb_git_remote_download(VALUE self)
{
	int error;
	git_remote *remote;

	Data_Get_Struct(self, git_remote, remote);

	error = git_remote_download(remote, NULL, NULL);
	rugged_exception_check(error);

	return Qnil;
}

static int cb_remote__update_tips(const char *refname, const git_oid *src, const git_oid *dest, void *data)
{
	rb_yield_values(
			3,
			rugged_str_new2(refname, rb_utf8_encoding()),
			git_oid_iszero(src) ? Qnil : rugged_create_oid(src),
			git_oid_iszero(dest) ? Qnil : rugged_create_oid(dest));

	return GIT_OK;
}

static VALUE rb_git_remote__update_tips(VALUE self)
{
	git_remote *remote;
	Data_Get_Struct(self, git_remote, remote);

	rugged_exception_check(
		git_remote_update_tips(remote)
	);

	return Qnil;
}

/*
 * 	call-seq:
 * 		remote.update_tips! -> nil
 * 		remote.update_tips! { |reference, source, destination| block }
 *
 *	Update the tips to a new state from a connected remote. The target
 *	objects must be downloaded before the tips are updated.
 *
 *		  r.update_tips! do |ref, src, dst|
 *		    puts "#{ref}: #{src}..#{dst}"
 *		  end
 */
static VALUE rb_git_remote_update_tips(VALUE self)
{
	git_remote *remote;

	Data_Get_Struct(self, git_remote, remote);

	if (rb_block_given_p()) {
		int exception = 0;
		git_remote_callbacks callbacks = GIT_REMOTE_CALLBACKS_INIT;

		callbacks.update_tips = &cb_remote__update_tips;
		rugged_exception_check(
			git_remote_set_callbacks(remote, &callbacks)
		);

		rb_protect(rb_git_remote__update_tips, self, &exception);

		callbacks.update_tips = NULL;
		rugged_exception_check(
			git_remote_set_callbacks(remote, &callbacks)
		);

		if (exception)
			rb_jump_tag(exception);
	} else {
		rugged_exception_check(git_remote_update_tips(remote));
	}

	return Qnil;
}

/* :nodoc: */
static VALUE rb_git_remote_each(VALUE self, VALUE rb_repo)
{
	git_repository *repo;
	git_strarray remotes;
	size_t i;
	int error;

	if (!rb_block_given_p())
		return rb_funcall(self, rb_intern("to_enum"), 2, CSTR2SYM("each"), rb_repo);

	rugged_check_repo(rb_repo);

	Data_Get_Struct(rb_repo, git_repository, repo);

	error = git_remote_list(&remotes, repo);
	rugged_exception_check(error);

	for (i = 0; i < remotes.count; ++i)
		rb_yield(rugged_str_new2(remotes.strings[i], NULL));

	git_strarray_free(&remotes);
	return Qnil;
}

/*
 * 	call-seq:
 * 		remote.save -> true
 *
 * 	Saves the remote data ( url, fetchspecs, ...) to the config
 *
 *	One can't save a in-memory remote. Doing so will
 *	result in an exception being raised.
*/
static VALUE rb_git_remote_save(VALUE self)
{
	git_remote *remote;

	Data_Get_Struct(self, git_remote, remote);

	rugged_exception_check(
		git_remote_save(remote)
	);
	return Qtrue;
}

static int cb_remote__rename_problem(const char* refspec_name, void *payload)
{

	rb_ary_push((VALUE) payload,
			rugged_str_new2(refspec_name, rb_utf8_encoding()));
	return 0;
}

/*
 * 	call-seq:
 * 		remote.rename!(new_name) -> array or nil
 *
 * 	Renames a remote
 *
 * 	All remote-tracking branches and configuration settings
 * 	for the remote are updated.
 *
 * 	Returns +nil+ if everything was updated or array of fetch refspecs
 * 	that haven't been automatically updated and need potential manual
 * 	tweaking.
 *
 * 	A temporary in-memory remote cannot be given a name with this method.
 * 		remote = Rugged::Remote.lookup(@repo, 'origin')
 * 		remote.rename!('upstream') #=> nil
 *
*/
static VALUE rb_git_remote_rename(VALUE self, VALUE rb_new_name)
{
	git_remote *remote;
	int error = 0;
	VALUE rb_refspec_ary = rb_ary_new();

	Check_Type(rb_new_name, T_STRING);
	Data_Get_Struct(self, git_remote, remote);
	error = git_remote_rename(
			remote,
			StringValueCStr(rb_new_name),
			cb_remote__rename_problem, (void *)rb_refspec_ary);

	rugged_exception_check(error);

	return RARRAY_LEN(rb_refspec_ary) == 0 ? Qnil : rb_refspec_ary;
}

void Init_rugged_remote()
{
	rb_cRuggedRemote = rb_define_class_under(rb_mRugged, "Remote", rb_cObject);

	rb_define_singleton_method(rb_cRuggedRemote, "new", rb_git_remote_new, 2);
	rb_define_singleton_method(rb_cRuggedRemote, "add", rb_git_remote_add, 3);
	rb_define_singleton_method(rb_cRuggedRemote, "lookup", rb_git_remote_lookup, 2);
	rb_define_singleton_method(rb_cRuggedRemote, "each", rb_git_remote_each, 1);

	rb_define_method(rb_cRuggedRemote, "connect", rb_git_remote_connect, 1);
	rb_define_method(rb_cRuggedRemote, "disconnect", rb_git_remote_disconnect, 0);
	rb_define_method(rb_cRuggedRemote, "name", rb_git_remote_name, 0);
	rb_define_method(rb_cRuggedRemote, "url", rb_git_remote_url, 0);
	rb_define_method(rb_cRuggedRemote, "url=", rb_git_remote_set_url, 1);
	rb_define_method(rb_cRuggedRemote, "push_url", rb_git_remote_push_url, 0);
	rb_define_method(rb_cRuggedRemote, "push_url=", rb_git_remote_set_push_url, 1);
	rb_define_method(rb_cRuggedRemote, "connected?", rb_git_remote_connected, 0);
	rb_define_method(rb_cRuggedRemote, "ls", rb_git_remote_ls, 0);
	rb_define_method(rb_cRuggedRemote, "download", rb_git_remote_download, 0);
	rb_define_method(rb_cRuggedRemote, "update_tips!", rb_git_remote_update_tips, 0);
	rb_define_method(rb_cRuggedRemote, "save", rb_git_remote_save, 0);
	rb_define_method(rb_cRuggedRemote, "rename!", rb_git_remote_rename, 1);
}
