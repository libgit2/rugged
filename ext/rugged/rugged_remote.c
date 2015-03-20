/*
 * The MIT License
 *
 * Copyright (c) 2014 GitHub, Inc
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
extern VALUE rb_eRuggedError;
VALUE rb_cRuggedRemote;

#define RUGGED_REMOTE_CALLBACKS_INIT {1, progress_cb, NULL, credentials_cb, NULL, transfer_progress_cb, update_tips_cb, NULL, NULL, push_update_reference_cb, NULL}

static int progress_cb(const char *str, int len, void *data)
{
	struct rugged_remote_cb_payload *payload = data;
	VALUE args = rb_ary_new2(2);

	if (NIL_P(payload->progress))
		return 0;

	rb_ary_push(args, payload->progress);
	rb_ary_push(args, rb_str_new(str, len));

	rb_protect(rugged__block_yield_splat, args, &payload->exception);

	return payload->exception ? GIT_ERROR : GIT_OK;
}

static int transfer_progress_cb(const git_transfer_progress *stats, void *data)
{
	struct rugged_remote_cb_payload *payload = data;
	VALUE args = rb_ary_new2(5);

	if (NIL_P(payload->transfer_progress))
		return 0;

	rb_ary_push(args, payload->transfer_progress);
	rb_ary_push(args, UINT2NUM(stats->total_objects));
	rb_ary_push(args, UINT2NUM(stats->indexed_objects));
	rb_ary_push(args, UINT2NUM(stats->received_objects));
	rb_ary_push(args, UINT2NUM(stats->local_objects));
	rb_ary_push(args, UINT2NUM(stats->total_deltas));
	rb_ary_push(args, UINT2NUM(stats->indexed_deltas));
	rb_ary_push(args, INT2FIX(stats->received_bytes));

	rb_protect(rugged__block_yield_splat, args, &payload->exception);

	return payload->exception ? GIT_ERROR : GIT_OK;
}

static int push_update_reference_cb(const char *refname, const char *status, void *data) {
	struct rugged_remote_cb_payload *payload = data;

	if (status != NULL)
		rb_hash_aset(payload->result, rb_str_new_utf8(refname), rb_str_new_utf8(status));

	return GIT_OK;
}

static int update_tips_cb(const char *refname, const git_oid *src, const git_oid *dest, void *data)
{
	struct rugged_remote_cb_payload *payload = data;
	VALUE args = rb_ary_new2(4);

	if (NIL_P(payload->update_tips))
		return 0;

	rb_ary_push(args, payload->update_tips);
	rb_ary_push(args, rb_str_new_utf8(refname));
	rb_ary_push(args, git_oid_iszero(src) ? Qnil : rugged_create_oid(src));
	rb_ary_push(args, git_oid_iszero(dest) ? Qnil : rugged_create_oid(dest));

	rb_protect(rugged__block_yield_splat, args, &payload->exception);

	return payload->exception ? GIT_ERROR : GIT_OK;
}

struct extract_cred_args
{
	VALUE rb_callback;
	git_cred **cred;
	const char *url;
	const char *username_from_url;
	unsigned int allowed_types;
};

static VALUE allowed_types_to_rb_ary(int allowed_types) {
	VALUE rb_allowed_types = rb_ary_new();

	if (allowed_types & GIT_CREDTYPE_USERPASS_PLAINTEXT)
		rb_ary_push(rb_allowed_types, CSTR2SYM("plaintext"));

	if (allowed_types & GIT_CREDTYPE_SSH_KEY)
		rb_ary_push(rb_allowed_types, CSTR2SYM("ssh_key"));

	if (allowed_types & GIT_CREDTYPE_DEFAULT)
		rb_ary_push(rb_allowed_types, CSTR2SYM("default"));

	return rb_allowed_types;
}

static VALUE extract_cred(VALUE data) {
	struct extract_cred_args *args = (struct extract_cred_args*)data;
	VALUE rb_url, rb_username_from_url, rb_cred;

	rb_url = args->url ? rb_str_new2(args->url) : Qnil;
	rb_username_from_url = args->username_from_url ? rb_str_new2(args->username_from_url) : Qnil;

	rb_cred = rb_funcall(args->rb_callback, rb_intern("call"), 3,
		rb_url, rb_username_from_url, allowed_types_to_rb_ary(args->allowed_types));

	rugged_cred_extract(args->cred, args->allowed_types, rb_cred);

	return Qnil;
}

static int credentials_cb(
	git_cred **cred,
	const char *url,
	const char *username_from_url,
	unsigned int allowed_types,
	void *data)
{
	struct rugged_remote_cb_payload *payload = data;
	struct extract_cred_args args = {
		payload->credentials, cred, url, username_from_url, allowed_types
	};

	if (NIL_P(payload->credentials))
		return GIT_PASSTHROUGH;

	rb_protect(extract_cred, (VALUE)&args, &payload->exception);

	return payload->exception ? GIT_ERROR : GIT_OK;
}

#define CALLABLE_OR_RAISE(ret, rb_options, name) \
	do {							\
		ret = rb_hash_aref(rb_options, CSTR2SYM(name)); \
								\
		if (!NIL_P(ret) && !rb_respond_to(ret, rb_intern("call"))) \
			rb_raise(rb_eArgError, "Expected a Proc or an object that responds to #call (:" name " )."); \
	} while (0);

void rugged_remote_init_callbacks_and_payload_from_options(
	VALUE rb_options,
	git_remote_callbacks *callbacks,
	struct rugged_remote_cb_payload *payload)
{
	git_remote_callbacks prefilled = RUGGED_REMOTE_CALLBACKS_INIT;

	prefilled.payload = payload;
	memcpy(callbacks, &prefilled, sizeof(git_remote_callbacks));

	if (!NIL_P(rb_options)) {
		CALLABLE_OR_RAISE(payload->update_tips, rb_options, "update_tips");
		CALLABLE_OR_RAISE(payload->progress, rb_options, "progress");
		CALLABLE_OR_RAISE(payload->transfer_progress, rb_options, "transfer_progress");
		CALLABLE_OR_RAISE(payload->credentials, rb_options, "credentials");
	}
}

static void rb_git_remote__free(git_remote *remote)
{
	git_remote_free(remote);
}

VALUE rugged_remote_new(VALUE owner, git_remote *remote)
{
	VALUE rb_remote;

	rb_remote = Data_Wrap_Struct(rb_cRuggedRemote, NULL, &rb_git_remote__free, remote);
	rugged_set_owner(rb_remote, owner);
	return rb_remote;
}

static VALUE rugged_rhead_new(const git_remote_head *head)
{
	VALUE rb_head = rb_hash_new();

	rb_hash_aset(rb_head, CSTR2SYM("local?"), head->local ? Qtrue : Qfalse);
	rb_hash_aset(rb_head, CSTR2SYM("oid"), rugged_create_oid(&head->oid));
	rb_hash_aset(rb_head, CSTR2SYM("loid"),
			git_oid_iszero(&head->loid) ? Qnil : rugged_create_oid(&head->loid));
	rb_hash_aset(rb_head, CSTR2SYM("name"), rb_str_new_utf8(head->name));

	return rb_head;
}

/*
 *  call-seq:
 *    remote.ls(options = {}) -> an_enumerator
 *    remote.ls(options = {}) { |remote_head_hash| block }
 *
 *  Connects +remote+ to list all references available along with their
 *  associated commit ids.
 *
 *  The given block is called once for each remote head with a Hash containing the
 *  following keys:
 *
 *  :local? ::
 *    +true+ if the remote head is available locally, +false+ otherwise.
 *
 *  :oid ::
 *    The id of the object the remote head is currently pointing to.
 *
 *  :loid ::
 *    The id of the object the local copy of the remote head is currently
 *    pointing to. Set to +nil+ if there is no local copy of the remote head.
 *
 *  :name ::
 *    The fully qualified reference name of the remote head.
 *
 *  If no block is given, an enumerator will be returned.
 *
 *  The following options can be passed in the +options+ Hash:
 *
 *  :credentials ::
 *    The credentials to use for the ls operation. Can be either an instance of one
 *    of the Rugged::Credentials types, or a proc returning one of the former.
 *    The proc will be called with the +url+, the +username+ from the url (if applicable) and
 *    a list of applicable credential types.
 */
static VALUE rb_git_remote_ls(int argc, VALUE *argv, VALUE self)
{
	git_remote *remote;
	git_remote_callbacks callbacks = GIT_REMOTE_CALLBACKS_INIT;
	const git_remote_head **heads;

	struct rugged_remote_cb_payload payload = { Qnil, Qnil, Qnil, Qnil, Qnil, Qnil, 0 };

	VALUE rb_options;

	int error;
	size_t heads_len, i;

	Data_Get_Struct(self, git_remote, remote);

	rb_scan_args(argc, argv, ":", &rb_options);

	if (!rb_block_given_p())
		return rb_funcall(self, rb_intern("to_enum"), 2, CSTR2SYM("ls"), rb_options);

	rugged_remote_init_callbacks_and_payload_from_options(rb_options, &callbacks, &payload);

	if ((error = git_remote_set_callbacks(remote, &callbacks)) ||
	    (error = git_remote_connect(remote, GIT_DIRECTION_FETCH)) ||
	    (error = git_remote_ls(&heads, &heads_len, remote)))
		goto cleanup;

	for (i = 0; i < heads_len && !payload.exception; i++)
		rb_protect(rb_yield, rugged_rhead_new(heads[i]), &payload.exception);

	cleanup:

	git_remote_disconnect(remote);

	if (payload.exception)
		rb_jump_tag(payload.exception);

	rugged_exception_check(error);

	return Qnil;
}

/*
 *  call-seq:
 *    remote.name() -> string
 *
 *	Returns the remote's name.
 *
 *	  remote.name #=> "origin"
 */
static VALUE rb_git_remote_name(VALUE self)
{
	git_remote *remote;
	const char * name;
	Data_Get_Struct(self, git_remote, remote);

	name = git_remote_name(remote);

	return name ? rb_str_new_utf8(name) : Qnil;
}

/*
 *  call-seq:
 *    remote.url() -> string
 *
 *  Returns the remote's url
 *
 *    remote.url #=> "git://github.com/libgit2/rugged.git"
 */
static VALUE rb_git_remote_url(VALUE self)
{
	git_remote *remote;
	Data_Get_Struct(self, git_remote, remote);

	return rb_str_new_utf8(git_remote_url(remote));
}

/*
 *  call-seq:
 *    remote.url = url -> url
 *
 *  Sets the remote's url without persisting it in the config.
 *  Existing connections will not be updated.
 *
 *    remote.url = 'git://github.com/libgit2/rugged.git' #=> "git://github.com/libgit2/rugged.git"
 */
static VALUE rb_git_remote_set_url(VALUE self, VALUE rb_url)
{
	git_remote *remote;

	Check_Type(rb_url, T_STRING);
	Data_Get_Struct(self, git_remote, remote);

	rugged_exception_check(
		git_remote_set_url(remote, StringValueCStr(rb_url))
	);
	return rb_url;
}

/*
 *  call-seq:
 *    remote.push_url() -> string or nil
 *
 *  Returns the remote's url for pushing or nil if no special url for
 *  pushing is set.
 *
 *    remote.push_url #=> "git://github.com/libgit2/rugged.git"
 */
static VALUE rb_git_remote_push_url(VALUE self)
{
	git_remote *remote;
	const char * push_url;

	Data_Get_Struct(self, git_remote, remote);

	push_url = git_remote_pushurl(remote);
	return push_url ? rb_str_new_utf8(push_url) : Qnil;
}

/*
 *  call-seq:
 *    remote.push_url = url -> url
 *
 *  Sets the remote's url for pushing without persisting it in the config.
 *  Existing connections will not be updated.
 *
 *    remote.push_url = 'git@github.com/libgit2/rugged.git' #=> "git@github.com/libgit2/rugged.git"
 */
static VALUE rb_git_remote_set_push_url(VALUE self, VALUE rb_url)
{
	git_remote *remote;

	Check_Type(rb_url, T_STRING);
	Data_Get_Struct(self, git_remote, remote);

	rugged_exception_check(
		git_remote_set_pushurl(remote, StringValueCStr(rb_url))
	);

	return rb_url;
}

static VALUE rb_git_remote_refspecs(VALUE self, git_direction direction)
{
	git_remote *remote;
	int error = 0;
	git_strarray refspecs;
	VALUE rb_refspec_array;

	Data_Get_Struct(self, git_remote, remote);

	if (direction == GIT_DIRECTION_FETCH)
		error = git_remote_get_fetch_refspecs(&refspecs, remote);
	else
		error = git_remote_get_push_refspecs(&refspecs, remote);

	rugged_exception_check(error);

	rb_refspec_array = rugged_strarray_to_rb_ary(&refspecs);
	git_strarray_free(&refspecs);
	return rb_refspec_array;
}

/*
 *  call-seq:
 *  remote.fetch_refspecs -> array
 *
 *  Get the remote's list of fetch refspecs as +array+.
 */
static VALUE rb_git_remote_fetch_refspecs(VALUE self)
{
	return rb_git_remote_refspecs(self, GIT_DIRECTION_FETCH);
}

/*
 *  call-seq:
 *  remote.push_refspecs -> array
 *
 *  Get the remote's list of push refspecs as +array+.
 */
static VALUE rb_git_remote_push_refspecs(VALUE self)
{
	return rb_git_remote_refspecs(self, GIT_DIRECTION_PUSH);
}

static VALUE rb_git_remote_add_refspec(VALUE self, VALUE rb_refspec, git_direction direction)
{
	git_remote *remote;
	int error = 0;

	Data_Get_Struct(self, git_remote, remote);

	Check_Type(rb_refspec, T_STRING);

	if (direction == GIT_DIRECTION_FETCH)
		error = git_remote_add_fetch(remote, StringValueCStr(rb_refspec));
	else
		error = git_remote_add_push(remote, StringValueCStr(rb_refspec));

	rugged_exception_check(error);

	return Qnil;
}

/*
 *  call-seq:
 *    remote.add_fetch(refspec) -> nil
 *
 *  Add a fetch refspec to the remote.
 */
static VALUE rb_git_remote_add_fetch(VALUE self, VALUE rb_refspec)
{
	return rb_git_remote_add_refspec(self, rb_refspec, GIT_DIRECTION_FETCH);
}

/*
 *  call-seq:
 *    remote.add_push(refspec) -> nil
 *
 *  Add a push refspec to the remote.
 */
static VALUE rb_git_remote_add_push(VALUE self, VALUE rb_refspec)
{
	return rb_git_remote_add_refspec(self, rb_refspec, GIT_DIRECTION_PUSH);
}

/*
 *  call-seq:
 *    remote.clear_refspecs -> nil
 *
 *  Remove all configured fetch and push refspecs from the remote.
 */
static VALUE rb_git_remote_clear_refspecs(VALUE self)
{
	git_remote *remote;

	Data_Get_Struct(self, git_remote, remote);

	git_remote_clear_refspecs(remote);

	return Qnil;
}

/*
 *  call-seq:
 *    remote.save -> true
 *
 *  Saves the remote data (url, fetchspecs, ...) to the config.
 *
 *  Anonymous, in-memory remotes created through
 *  +ReferenceCollection#create_anonymous+ can not be saved.
 *  Doing so will result in an exception being raised.
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

/*
 *  call-seq:
 *    remote.check_connection(direction, options = {}) -> boolean
 *
 *  Try to connect to the +remote+. Useful to simulate
 *  <tt>git fetch --dry-run</tt> and <tt>git push --dry-run</tt>.
 *
 *  Returns +true+ if connection is successful, +false+ otherwise.
 *
 *  +direction+ must be either +:fetch+ or +:push+.
 *
 *  The following options can be passed in the +options+ Hash:
 *
 *  +credentials+ ::
 *    The credentials to use for the connection. Can be either an instance of
 *    one of the Rugged::Credentials types, or a proc returning one of the
 *    former.
 *    The proc will be called with the +url+, the +username+ from the url (if
 *    applicable) and a list of applicable credential types.
 *
 *  Example:
 *
 *    remote = repo.remotes["origin"]
 *    success = remote.check_connection(:fetch)
 *    raise Error("Unable to pull without credentials") unless success
 */
static VALUE rb_git_remote_check_connection(int argc, VALUE *argv, VALUE self)
{
	git_remote *remote;
	git_remote_callbacks callbacks = GIT_REMOTE_CALLBACKS_INIT;
	struct rugged_remote_cb_payload payload = { Qnil, Qnil, Qnil, Qnil, Qnil, Qnil, 0 };
	VALUE rb_direction, rb_options;
	ID id_direction;
	int error, direction;

	Data_Get_Struct(self, git_remote, remote);
	rb_scan_args(argc, argv, "01:", &rb_direction, &rb_options);

	Check_Type(rb_direction, T_SYMBOL);
	id_direction = SYM2ID(rb_direction);
	if (id_direction == rb_intern("fetch"))
		direction = GIT_DIRECTION_FETCH;
	else if (id_direction == rb_intern("push"))
		direction = GIT_DIRECTION_PUSH;
	else
		rb_raise(rb_eTypeError, "Invalid direction. Expected :fetch or :push");

	rugged_remote_init_callbacks_and_payload_from_options(rb_options, &callbacks, &payload);

	if ((error = git_remote_set_callbacks(remote, &callbacks)) < 0)
		goto cleanup;

	if (git_remote_connect(remote, direction))
		return Qfalse;
	else {
		git_remote_disconnect(remote);
		return Qtrue;
	}

cleanup:
	if (payload.exception)
		rb_jump_tag(payload.exception);
	rugged_exception_check(error);
	return Qfalse;
}

/*
 *  call-seq:
 *    remote.fetch(refspecs = nil, options = {}) -> hash
 *
 *  Downloads new data from the remote for the given +refspecs+ and updates tips.
 *
 *  You can optionally pass in a single or multiple alternative +refspecs+ to use instead of the fetch
 *  refspecs already configured for +remote+.
 *
 *  Returns a hash containing statistics for the fetch operation.
 *
 *  The following options can be passed in the +options+ Hash:
 *
 *  :credentials ::
 *    The credentials to use for the fetch operation. Can be either an instance of one
 *    of the Rugged::Credentials types, or a proc returning one of the former.
 *    The proc will be called with the +url+, the +username+ from the url (if applicable) and
 *    a list of applicable credential types.
 *
 *  :progress ::
 *    A callback that will be executed with the textual progress received from the remote.
 *    This is the text send over the progress side-band (ie. the "counting objects" output).
 *
 *  :transfer_progress ::
 *    A callback that will be executed to report clone progress information. It will be passed
 *    the amount of +total_objects+, +indexed_objects+, +received_objects+, +local_objects+,
 *    +total_deltas+, +indexed_deltas+ and +received_bytes+.
 *
 *  :update_tips ::
 *    A callback that will be executed each time a reference is updated locally. It will be
 *    passed the +refname+, +old_oid+ and +new_oid+.
 *
 *  :message ::
 *    The message to insert into the reflogs. Defaults to "fetch".
 *
 *  :signature ::
 *    The signature to be used for updating the reflogs.
 *
 *  Example:
 *
 *    remote = Rugged::Remote.lookup(@repo, 'origin')
 *    remote.fetch({
 *      transfer_progress: lambda { |total_objects, indexed_objects, received_objects, local_objects, total_deltas, indexed_deltas, received_bytes|
 *        # ...
 *      }
 *    })
 */
static VALUE rb_git_remote_fetch(int argc, VALUE *argv, VALUE self)
{
	git_remote *remote;
	git_repository *repo;
	git_signature *signature = NULL;
	git_strarray refspecs;
	git_remote_callbacks callbacks = GIT_REMOTE_CALLBACKS_INIT;
	struct rugged_remote_cb_payload payload = { Qnil, Qnil, Qnil, Qnil, Qnil, Qnil, 0 };

	char *log_message = NULL;
	int error;

	VALUE rb_options, rb_refspecs, rb_result = Qnil, rb_repo = rugged_owner(self);

	rb_scan_args(argc, argv, "01:", &rb_refspecs, &rb_options);

	rugged_rb_ary_to_strarray(rb_refspecs, &refspecs);

	Data_Get_Struct(self, git_remote, remote);
	rugged_check_repo(rb_repo);
	RUGGED_GET_REPO(rb_repo, repo);

	rugged_remote_init_callbacks_and_payload_from_options(rb_options, &callbacks, &payload);

	if (!NIL_P(rb_options)) {
		VALUE rb_val = rb_hash_aref(rb_options, CSTR2SYM("signature"));
		if (!NIL_P(rb_val))
			signature = rugged_signature_get(rb_val, repo);

		rb_val = rb_hash_aref(rb_options, CSTR2SYM("message"));
		if (!NIL_P(rb_val))
			log_message = StringValueCStr(rb_val);
	}

	if ((error = git_remote_set_callbacks(remote, &callbacks)))
		goto cleanup;

	if ((error = git_remote_fetch(remote, &refspecs, signature, log_message)) == GIT_OK) {
		const git_transfer_progress *stats = git_remote_stats(remote);

		rb_result = rb_hash_new();
		rb_hash_aset(rb_result, CSTR2SYM("total_objects"),    UINT2NUM(stats->total_objects));
		rb_hash_aset(rb_result, CSTR2SYM("indexed_objects"),  UINT2NUM(stats->indexed_objects));
		rb_hash_aset(rb_result, CSTR2SYM("received_objects"), UINT2NUM(stats->received_objects));
		rb_hash_aset(rb_result, CSTR2SYM("local_objects"),    UINT2NUM(stats->local_objects));
		rb_hash_aset(rb_result, CSTR2SYM("total_deltas"),     UINT2NUM(stats->total_deltas));
		rb_hash_aset(rb_result, CSTR2SYM("indexed_deltas"),   UINT2NUM(stats->indexed_deltas));
		rb_hash_aset(rb_result, CSTR2SYM("received_bytes"),   INT2FIX(stats->received_bytes));
	}

	cleanup:

	xfree(refspecs.strings);
	git_signature_free(signature);

	if (payload.exception)
		rb_jump_tag(payload.exception);

	rugged_exception_check(error);

	return rb_result;
}

/*
 *  call-seq:
 *    remote.push(refspecs = nil, options = {}) -> hash
 *
 *  Pushes the given +refspecs+ to the given +remote+. Returns a hash that contains
 *  key-value pairs that reflect pushed refs and error messages, if applicable.
 *
 *  You can optionally pass in an alternative list of +refspecs+ to use instead of the push
 *  refspecs already configured for +remote+.
 *
 *  The following options can be passed in the +options+ Hash:
 *
 *  :credentials ::
 *    The credentials to use for the push operation. Can be either an instance of one
 *    of the Rugged::Credentials types, or a proc returning one of the former.
 *    The proc will be called with the +url+, the +username+ from the url (if applicable) and
 *    a list of applicable credential types.
 *
 *  :update_tips ::
 *    A callback that will be executed each time a reference is updated remotely. It will be
 *    passed the +refname+, +old_oid+ and +new_oid+.
 *
 *  :message ::
 *    A single line log message to be appended to the reflog of each local remote-tracking
 *    branch that gets updated. Defaults to: "fetch".
 *
 *  :signature ::
 *    The signature to be used for populating the reflog entries.
 *
 *  Example:
 *
 *    remote = Rugged::Remote.lookup(@repo, 'origin')
 *    remote.push(["refs/heads/master", ":refs/heads/to_be_deleted"])
 */
static VALUE rb_git_remote_push(int argc, VALUE *argv, VALUE self)
{
	VALUE rb_refspecs, rb_options, rb_val;
	VALUE rb_repo = rugged_owner(self);

	git_repository *repo;
	git_remote *remote;
	git_remote_callbacks callbacks = GIT_REMOTE_CALLBACKS_INIT;
	git_signature *signature = NULL;
	git_strarray refspecs;
	git_push_options opts = GIT_PUSH_OPTIONS_INIT;

	int error = 0;
	char *log_message = NULL;

	struct rugged_remote_cb_payload payload = { Qnil, Qnil, Qnil, Qnil, Qnil, rb_hash_new(), 0 };

	rb_scan_args(argc, argv, "01:", &rb_refspecs, &rb_options);

	rugged_rb_ary_to_strarray(rb_refspecs, &refspecs);

	rugged_check_repo(rb_repo);
	RUGGED_GET_REPO(rb_repo, repo);
	Data_Get_Struct(self, git_remote, remote);

	rugged_remote_init_callbacks_and_payload_from_options(rb_options, &callbacks, &payload);

	if (!NIL_P(rb_options)) {
		rb_val = rb_hash_aref(rb_options, CSTR2SYM("message"));
		if (!NIL_P(rb_val))
			log_message = StringValueCStr(rb_val);

		rb_val = rb_hash_aref(rb_options, CSTR2SYM("signature"));
		if (!NIL_P(rb_val))
			signature = rugged_signature_get(rb_val, repo);
	}

	if ((error = git_remote_set_callbacks(remote, &callbacks)))
	    goto cleanup;

	error = git_remote_push(remote, &refspecs, &opts, signature, log_message);

cleanup:
	xfree(refspecs.strings);
	git_signature_free(signature);

	if (payload.exception)
		rb_jump_tag(payload.exception);

	rugged_exception_check(error);

	return payload.result;
}

void Init_rugged_remote(void)
{
	rb_cRuggedRemote = rb_define_class_under(rb_mRugged, "Remote", rb_cObject);


	rb_define_method(rb_cRuggedRemote, "name", rb_git_remote_name, 0);
	rb_define_method(rb_cRuggedRemote, "url", rb_git_remote_url, 0);
	rb_define_method(rb_cRuggedRemote, "url=", rb_git_remote_set_url, 1);
	rb_define_method(rb_cRuggedRemote, "push_url", rb_git_remote_push_url, 0);
	rb_define_method(rb_cRuggedRemote, "push_url=", rb_git_remote_set_push_url, 1);
	rb_define_method(rb_cRuggedRemote, "fetch_refspecs", rb_git_remote_fetch_refspecs, 0);
	rb_define_method(rb_cRuggedRemote, "push_refspecs", rb_git_remote_push_refspecs, 0);
	rb_define_method(rb_cRuggedRemote, "add_fetch", rb_git_remote_add_fetch, 1);
	rb_define_method(rb_cRuggedRemote, "add_push", rb_git_remote_add_push, 1);
	rb_define_method(rb_cRuggedRemote, "ls", rb_git_remote_ls, -1);
	rb_define_method(rb_cRuggedRemote, "check_connection", rb_git_remote_check_connection, -1);
	rb_define_method(rb_cRuggedRemote, "fetch", rb_git_remote_fetch, -1);
	rb_define_method(rb_cRuggedRemote, "push", rb_git_remote_push, -1);
	rb_define_method(rb_cRuggedRemote, "clear_refspecs", rb_git_remote_clear_refspecs, 0);
	rb_define_method(rb_cRuggedRemote, "save", rb_git_remote_save, 0);
}
