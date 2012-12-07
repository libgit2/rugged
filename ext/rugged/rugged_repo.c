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
extern VALUE rb_cRuggedIndex;
extern VALUE rb_cRuggedConfig;
extern VALUE rb_cRuggedBackend;
extern VALUE rb_cRuggedObject;

VALUE rb_cRuggedRepo;
VALUE rb_cRuggedOdbObject;

int rb_git_repo__conflict_callback(	const char* conflicting_path, const git_oid* index_oid,
																		unsigned int index_mode, unsigned int wd_mode, void* payload);
void rb_git_repo__progress_callback(const char* path, size_t completed_steps, size_t total_steps, void* payload);

/*
 *	call-seq:
 *		odb_obj.oid -> hex_oid
 *
 *	Return the Object ID (a 40 character SHA1 hash) for this raw
 *	object.
 *
 *		odb_obj.oid #=> "d8786bfc97485e8d7b19b21fb88c8ef1f199fc3f"
 */
static VALUE rb_git_odbobj_oid(VALUE self)
{
	git_odb_object *obj;
	Data_Get_Struct(self, git_odb_object, obj);
	return rugged_create_oid(git_odb_object_id(obj));
}

/*
 *	call-seq:
 *		odb_obj.data -> buffer
 *
 *	Return an ASCII buffer with the raw bytes that form the Git object.
 *
 *		odb_obj.data #=> "tree 87ebee8367f9cc5ac04858b3bd5610ca74f04df9\n"
 *		             #=> "parent 68d041ee999cb07c6496fbdd4f384095de6ca9e1\n"
 *		             #=> "author Vicent Martí <tanoku@gmail.com> 1326863045 -0800\n"
 *		             #=> ...
 */
static VALUE rb_git_odbobj_data(VALUE self)
{
	git_odb_object *obj;
	Data_Get_Struct(self, git_odb_object, obj);
	return rugged_str_ascii(git_odb_object_data(obj), git_odb_object_size(obj));
}

/*
 *	call-seq:
 *		odb_obj.size -> size
 *
 *	Return the size in bytes of the Git object after decompression. This is
 *	also the size of the +obj.data+ buffer.
 *
 *		odb_obj.size #=> 231
 */
static VALUE rb_git_odbobj_size(VALUE self)
{
	git_odb_object *obj;
	Data_Get_Struct(self, git_odb_object, obj);
	return INT2FIX(git_odb_object_size(obj));
}

/*
 *	call-seq:
 *		odb_obj.type -> Symbol
 *
 *	Return a Ruby symbol representing the basic Git type of this object.
 *	Possible values are +:tree+, +:blob+, +:commit+ and +:tag+
 *
 *		odb_obj.type #=> :tag
 */
static VALUE rb_git_odbobj_type(VALUE self)
{
	git_odb_object *obj;
	Data_Get_Struct(self, git_odb_object, obj);
	return rugged_otype_new(git_odb_object_type(obj));
}

void rb_git__odbobj_free(void *obj)
{
	git_odb_object_free((git_odb_object *)obj);
}

VALUE rugged_raw_read(git_repository *repo, const git_oid *oid)
{
	git_odb *odb;
	git_odb_object *obj;

	int error;

	error = git_repository_odb(&odb, repo);
	rugged_exception_check(error);

	error = git_odb_read(&obj, odb, oid);
	git_odb_free(odb);
	rugged_exception_check(error);

	return Data_Wrap_Struct(rb_cRuggedOdbObject, NULL, rb_git__odbobj_free, obj);
}

void rb_git_repo__free(git_repository *repo)
{
	git_repository_free(repo);
}

/* Helper function used by the checkout_* methods to parse the argument hash.
 * Takes the hash (ruby_opts) and a git_checkout_opts struct to fill with the results
 * of the parsing process. Be sure to use GIT_CHECKOUT_OPTS_INIT for initialising
 * `p_opts'. */
static void rb_git__parse_checkout_options(const VALUE* ruby_opts, git_checkout_opts* p_opts, VALUE* p_progress_cb, VALUE* p_conflict_cb)
{
	VALUE opts_strategy;
	VALUE opts_disable_filters;
	VALUE opts_dir_mode;
	VALUE opts_file_mode;
	VALUE opts_file_open_flags;

	opts_strategy						= rb_hash_aref(*ruby_opts, ID2SYM(rb_intern("strategy")));
	opts_disable_filters		= rb_hash_aref(*ruby_opts, ID2SYM(rb_intern("disable_filters")));
	opts_dir_mode						= rb_hash_aref(*ruby_opts, ID2SYM(rb_intern("dir_mode")));
	opts_file_mode					= rb_hash_aref(*ruby_opts, ID2SYM(rb_intern("file_mode")));
	opts_file_open_flags		= rb_hash_aref(*ruby_opts, ID2SYM(rb_intern("file_open_flags")));
	*p_progress_cb	= rb_hash_aref(*ruby_opts, ID2SYM(rb_intern("progress_cb")));
	*p_conflict_cb	= rb_hash_aref(*ruby_opts, ID2SYM(rb_intern("conflict_cb")));

	/* Convert the Ruby hash values to C stuff */
	if (RTEST(opts_disable_filters))
		p_opts->disable_filters = 1; /* boolean */
	if (TYPE(opts_dir_mode) == T_FIXNUM)
		p_opts->dir_mode = FIX2INT(opts_dir_mode);
	if (TYPE(opts_file_mode) == T_FIXNUM)
		p_opts->file_mode = FIX2INT(opts_file_mode);
	if (TYPE(opts_file_open_flags) == T_FIXNUM)
		p_opts->file_open_flags = FIX2INT(opts_file_open_flags);
	if (RTEST(*p_progress_cb)) {
		p_opts->progress_payload = p_progress_cb;
		p_opts->progress_cb = rb_git_repo__progress_callback;
	}
	if (RTEST(*p_conflict_cb)) {
		p_opts->conflict_payload = p_conflict_cb;
		p_opts->conflict_cb = rb_git_repo__conflict_callback;
	}

	if (TYPE(opts_strategy) == T_ARRAY){
		p_opts->checkout_strategy = 0;

		/* Now OR-in all requested flags */
		if (rb_ary_includes(opts_strategy, ID2SYM(rb_intern("default"))))
			p_opts->checkout_strategy |= GIT_CHECKOUT_DEFAULT;
		if (rb_ary_includes(opts_strategy, ID2SYM(rb_intern("update_unmodified"))))
			p_opts->checkout_strategy |= GIT_CHECKOUT_UPDATE_UNMODIFIED;
		if (rb_ary_includes(opts_strategy, ID2SYM(rb_intern("update_missing"))))
			p_opts->checkout_strategy |= GIT_CHECKOUT_UPDATE_MISSING;
		if (rb_ary_includes(opts_strategy, ID2SYM(rb_intern("safe"))))
			p_opts->checkout_strategy |= GIT_CHECKOUT_SAFE;
		if (rb_ary_includes(opts_strategy, ID2SYM(rb_intern("update_modified"))))
			p_opts->checkout_strategy |= GIT_CHECKOUT_UPDATE_MODIFIED;
		if (rb_ary_includes(opts_strategy, ID2SYM(rb_intern("update_untracked"))))
			p_opts->checkout_strategy |=	GIT_CHECKOUT_UPDATE_UNTRACKED;
		if (rb_ary_includes(opts_strategy, ID2SYM(rb_intern("force"))))
			p_opts->checkout_strategy |=	GIT_CHECKOUT_FORCE;
		if (rb_ary_includes(opts_strategy, ID2SYM(rb_intern("allow_conflicts"))))
			p_opts->checkout_strategy |=	GIT_CHECKOUT_ALLOW_CONFLICTS;
		if (rb_ary_includes(opts_strategy, ID2SYM(rb_intern("remove_untracked"))))
			p_opts->checkout_strategy |=	GIT_CHECKOUT_REMOVE_UNTRACKED;
		if (rb_ary_includes(opts_strategy, ID2SYM(rb_intern("update_only"))))
			p_opts->checkout_strategy |=	GIT_CHECKOUT_UPDATE_ONLY;
		// The following options are not yet implemented in libgit2 as of 3368c520
		if (rb_ary_includes(opts_strategy, ID2SYM(rb_intern("skip_unmerged"))))
			p_opts->checkout_strategy |=	GIT_CHECKOUT_SKIP_UNMERGED;
		if (rb_ary_includes(opts_strategy, ID2SYM(rb_intern("use_ours"))))
			p_opts->checkout_strategy |=	GIT_CHECKOUT_USE_OURS;
		if (rb_ary_includes(opts_strategy, ID2SYM(rb_intern("use_theirs"))))
			p_opts->checkout_strategy |=	GIT_CHECKOUT_USE_THEIRS;
		if (rb_ary_includes(opts_strategy, ID2SYM(rb_intern("update_submodules"))))
			p_opts->checkout_strategy |=	GIT_CHECKOUT_UPDATE_SUBMODULES;
		if (rb_ary_includes(opts_strategy, ID2SYM(rb_intern("update_submodules_if_changed"))))
			p_opts->checkout_strategy |=	GIT_CHECKOUT_UPDATE_SUBMODULES_IF_CHANGED;
	}
}

static VALUE rugged_repo_new(VALUE klass, git_repository *repo)
{
	VALUE rb_repo = Data_Wrap_Struct(klass, NULL, &rb_git_repo__free, repo);

#ifdef HAVE_RUBY_ENCODING_H
	/* TODO: set this properly */
	rb_iv_set(rb_repo, "@encoding",
		rb_enc_from_encoding(rb_filesystem_encoding()));
#endif

	rb_iv_set(rb_repo, "@config", Qnil);
	rb_iv_set(rb_repo, "@index", Qnil);

	return rb_repo;
}

static void set_repository_options(git_repository *repo, VALUE rb_options)
{
	int error = 0;

	if (NIL_P(rb_options))
		return;

	Check_Type(rb_options, T_HASH);

	/* Check for `:alternates` */
	{
		git_odb *odb = NULL;
		VALUE rb_alternates = rb_hash_aref(rb_options, CSTR2SYM("alternates"));
		int i;

		if (!NIL_P(rb_alternates)) {
			Check_Type(rb_alternates, T_ARRAY);

			error = git_repository_odb(&odb, repo);
			rugged_exception_check(error);

			for (i = 0; !error && i < RARRAY_LEN(rb_alternates); ++i) {
				VALUE alt = rb_ary_entry(rb_alternates, i);
				Check_Type(alt, T_STRING);
				/* TODO: this leaks when alt != STRING */

				error = git_odb_add_disk_alternate(odb, StringValueCStr(alt));
			}

			git_odb_free(odb);
			rugged_exception_check(error);
		}
	}
}

/*
 *	call-seq:
 *		Rugged::Repository.new(path, options = {}) -> repository
 *
 *	Open a Git repository in the given +path+ and return a +Repository+ object
 *	representing it. An exception will be thrown if +path+ doesn't point to a
 *	valid repository. If you need to create a repository from scratch, use
 *	+Rugged::Repository.init+ instead.
 *
 *	The +path+ must point to the actual folder (+.git+) of a Git repository.
 *	If you're unsure of where is this located, use +Rugged::Repository.discover+
 *	instead.
 *
 *		Rugged::Repository.new('~/test/.git') #=> #<Rugged::Repository:0x108849488>
 *
 *	+options+ is an optional hash with the following keys:
 *
 *	- +:alternates+: +Array+ with a list of alternate object folders, e.g.
 *
 *		Rugged::Repository.new(path, :alternates => ['./other/repo/.git/objects'])
 */
static VALUE rb_git_repo_new(int argc, VALUE *argv, VALUE klass)
{
	git_repository *repo;
	int error = 0;
	VALUE rb_path, rb_options;

	rb_scan_args(argc, argv, "11", &rb_path, &rb_options);
	Check_Type(rb_path, T_STRING);

	error = git_repository_open(&repo, StringValueCStr(rb_path));
	rugged_exception_check(error);
	set_repository_options(repo, rb_options);

	return rugged_repo_new(klass, repo);
}

/*
 *	call-seq:
 *		init_at(path, is_bare) -> repository
 *
 *	Initialize a Git repository in +path+. This implies creating all the
 *	necessary files on the FS, or re-initializing an already existing
 *	repository if the files have already been created.
 *
 *	The +is_bare+ attribute specifies whether the Repository should be
 *	created on disk as bare or not. Bare repositories have no working
 *	directory and are created in the root of +path+. Non-bare repositories
 *	are created in a +.git+ folder and use +path+ as working directory.
 *
 *		Rugged::Repository.init_at('~/repository') #=> #<Rugged::Repository:0x108849488>
 */
static VALUE rb_git_repo_init_at(VALUE klass, VALUE path, VALUE rb_is_bare)
{
	git_repository *repo;
	int error, is_bare;

	is_bare = rugged_parse_bool(rb_is_bare);
	Check_Type(path, T_STRING);

	error = git_repository_init(&repo, StringValueCStr(path), is_bare);
	rugged_exception_check(error);

	return rugged_repo_new(klass, repo);
}

/* Helper struct used to pass information to
 * the nonblocking clone functions. The parameters are those
 * required for the libgit2 clone functions; p_opts is not
 * used for bare cloning. On allocation, please set a default
 * value of DEFAULT_CLONE_PARAMS so that default values get
 * initialised correctly. */
struct _clone_params {
	git_repository** pp_repo;			/* This will get assigned the cloned repo object */
	git_checkout_opts* p_opts;		/* Options for the checkout after the clone (not used for bare clones) */
	char* url;										/* URL of the remote host with protocol prefix */
	char* target_path;				 		/* path where to clone to */
	VALUE* p_ruby_transfer_proc;	/* The block passed to Repo::clone, if any (NULL otherwise) */

	/* This value will automatically be set by the nonblocking
	 * cloning functions. Setting it is meaningless, but after
	 * rb_thread_blocking_region returns this is set to the
	 * return value of the git_clone* functions. */
	int git_error;
};

/* Safely initialize a _clone_params struct by assigning this on allocation */
const struct _clone_params DEFAULT_CLONE_PARAMS = {
	NULL, NULL, NULL, NULL, NULL, 0
};

/* Callback function passed to libgit2 when the clone functions
 * are called with a block. `payload' is the block passed to Repo::clone
 * as a Proc. */
void rb_git_repo__transfer_callback(const git_transfer_progress* stats, void* payload)
{
	VALUE* p_ruby_transfer_cb_proc = (VALUE*)payload;
	rb_funcall(	*p_ruby_transfer_cb_proc, rb_intern("call"), 4,
							UINT2NUM(stats->total_objects),
							UINT2NUM(stats->indexed_objects),
							UINT2NUM(stats->received_objects),
							LONG2NUM(stats->received_bytes));
}

/* Callback function passed to libgit2 when the checkout functions
 * determine a conflict and a conflict callback was specified.
 * `payload' is the ruby lambda for the callback. */
int rb_git_repo__conflict_callback(const char* conflicting_path, const git_oid* index_oid,
																	 unsigned int index_mode, unsigned int wd_mode, void* payload)
{
	VALUE result;
	VALUE* p_ruby_lambda = (VALUE*)payload;
	char oid_str[GIT_OID_HEXSZ + 1];

	git_oid_tostr(oid_str, GIT_OID_HEXSZ + 1, index_oid);
	result = rb_funcall(	*p_ruby_lambda, rb_intern("call"), 4,
										 		rb_str_new2(conflicting_path),
												rb_str_new2(oid_str),
												UINT2NUM(index_mode),
												UINT2NUM(wd_mode));

	if (RTEST(result))
		return 1;
	else
		return 0;
}

/* Callback function passed to libgit2 when you passed a callback
 * to the checkout functions. `payload' is the ruby lamda for the
 * callback. */
void rb_git_repo__progress_callback(const char* path, size_t completed_steps, size_t total_steps, void* payload)
{
	VALUE* p_ruby_lambda = (VALUE*)payload;

	if (path) {
		rb_funcall(	*p_ruby_lambda, rb_intern("call"), 3,
								rb_str_new2(path),
								LONG2NUM(completed_steps),
								LONG2NUM(total_steps));
	}
	else {
		rb_funcall(	*p_ruby_lambda, rb_intern("call"), 3,
								Qnil,
								LONG2NUM(completed_steps),
								LONG2NUM(total_steps));
	}
}

/* Runs the git-clone operation with or without the GVL. `params' is a pointer
 * to a _clone_params struct. Called by rb_git_repo_clone(). */
static VALUE rb_git_repo__internal_clone(void* p_params)
{
	struct _clone_params* p_clone_params = (struct _clone_params*) p_params;
	int error;

	/* Check if we got a transfer callback passed, and if so, hand it over
	 * to libgit2. The two checkout-related callbacks for progress and conficts
	 * have already been handled when parsing the checkout options in the ::clone
	 * method. */
	if (p_clone_params->p_ruby_transfer_proc) {
		error = git_clone(	p_clone_params->pp_repo,
												p_clone_params->url,
												p_clone_params->target_path,
												p_clone_params->p_opts,
												rb_git_repo__transfer_callback,
												p_clone_params->p_ruby_transfer_proc);

	}
	else { /* In 1.9 we may run without the GVL here (if no checkout callbacks have been set) */
		error = git_clone(	p_clone_params->pp_repo,
												p_clone_params->url,
												p_clone_params->target_path,
												p_clone_params->p_opts,
												NULL, NULL);

	}
	p_clone_params->git_error = error;

	return Qnil;
}

/* Runs the git-clone --bare operation with or without the GVL. `params' is a pointer
 * to a _clone_params struct. Called by rb_git_repo_clone(). */
static VALUE rb_git_repo__internal_clone_bare(void* p_params)
{
	struct _clone_params* p_clone_params = (struct _clone_params*) p_params;
	int error;

	/* Check if we got a callback passed and if so, hand it over to
	 * libgit2. A side effect of this check is that if we don't got
	 * a transfer callback passed, we know for sure that we're currently
	 * running without the GVL. */
	if (p_clone_params->p_ruby_transfer_proc) { /* With GVL */
		error = git_clone_bare(	p_clone_params->pp_repo,
														p_clone_params->url,
														p_clone_params->target_path,
														rb_git_repo__transfer_callback,
														p_clone_params->p_ruby_transfer_proc);
	}
	else { /* In 1.9 we're running without the GVL here */
		error = git_clone_bare(	p_clone_params->pp_repo,
														p_clone_params->url,
														p_clone_params->target_path,
														NULL, NULL);
	}

	p_clone_params->git_error = error;

	return Qnil;
}

/* Currently libgit2 has no way to abort a running clone
 * operation. If one is added, this function should be
 * adjusted apropriately. `p_param' is a pointer to
 * a _clone_params struct (but beware; depending on the
 * abortion time, the pp_repo parameter may not be fully
 * initialised). Called by rb_git_repo_clone(). */
static void rb_git_repo__abort_clone(void* p_param)
{
	/* struct _clone_params* p_repo = (struct _clone_params*) p_param; */
	return; /* Yes, this currently DOES NOTHING. See comments above. */
}

/*
 * call-seq:
 *   clone_repo_bare(url, target_path) -> repository
 *   clone_repo_bare(url, target_path){|total_objects, indexed_objects, received_objects, received_bytes| ...} -> repository
 *
 * Creates a bare clone (in the Git sense, think <tt>git clone --bare</tt>) of
 * the remote repository at +url+ in the local directory +target_path+ and returns
 * a Repository object pointing to the newly created repo at +target_path+.
 *
 * If a block is given, it gets passed statistics about the fetching process
 * while download is in progress (see method signature).
 *
 * If no block is given, this method releases the GVL during the clone operation.
 *
 *   Rugged::Repository.clone_repo_bare("git://github.com/libgit2/libgit2.git", "~/libgit2.git")
 *   Rugged::Repository.clone_repo_bare("git://github.com/libgit2/libgit2.git", "~/libgit2.git") do |total, indexed, received_objs, bytes|
 *     print "\rReceived #{received_objs}/#{total} (indexed #{indexed}, #{bytes} bytes)"
 *   end
 */
static VALUE rb_git_repo_clone_bare(VALUE self, VALUE url, VALUE target_path)
{
	git_repository *p_repo;
	struct _clone_params clone_params = DEFAULT_CLONE_PARAMS;
	VALUE transfer_callback = Qnil;

	clone_params.pp_repo		 = &p_repo;
	clone_params.url				 = StringValueCStr(url);
	clone_params.target_path = StringValueCStr(target_path);
	clone_params.p_opts			 = NULL; /* Not required for bare clone */

	if (rb_block_given_p()) {
		/* If a transfer callback was given, we cannot release the GVL due to callback */
		transfer_callback = rb_block_proc(); /* &block */
		clone_params.p_ruby_transfer_proc = &transfer_callback;
		rb_git_repo__internal_clone_bare(&clone_params);
	}
	else { /* No callback given -- release the GVL if Ruby version allows */
#ifdef USING_RUBY_19
		rb_thread_blocking_region(	rb_git_repo__internal_clone_bare,
																&clone_params,
																rb_git_repo__abort_clone,
																&clone_params);
#else
		/* Poor 1.8 guys, you cannot have concurrency */
		rb_git_repo__internal_clone_bare(&clone_params);
#endif
	}

	/* Check for success and return new Repository instance */
	rugged_exception_check(clone_params.git_error);
	return rugged_repo_new(self, p_repo);
}

/*
 *	call-seq:
 *		clone_repo(url, target_path [, opts = {} ]) -> repository
 *		clone_rpeo(url, target_path [, opts = {} ]){|total_objects, indexed_objects, received_objects, received_bytes| ...}
 *
 *	Clone a Git repository from the remote at +url+ into the given
 *	local +target_path+ and return a Repository object pointing to
 *	the freshly cloned repository's <tt>.git</tt> directory.
 *
 *	After the cloning has completed, Rugged immediately checks out the branch
 *	pointed to by the remote HEAD. To suppress this, you can add the option
 *	<tt>:bare</tt> to the +opts+ hash, which (as the name indicates)
 *	will cause a bare repository to be created. Other than that,
 *	+opts+ will be passed on to the checkout operation, so see
 *	#checkout_index for possible values you can assign to +opts+.
 *
 *	If a block is given, it gets passed statistics about the fetching process
 *	(all values are integers).
 *
 *	If no block is given, this method releases the GVL during the clone operation.
 *
 *		Rugged::Repository.clone_repo("git://github.com/libgit2/libgit2.git", "~/libgit2")
 *		Rugged::Repository.clone_repo("git://github.com/libgit2/libgit2.git", "~/libgit2.git", bare: true)
 *		Rugged::Repository.clone_repo("git://github.com/libgit2/libgit2.git", "~/libgit2", strategy: [:force]) do |total, indexed, received_objs, bytes|
 *			print "\rReceived #{received_objs}/#{total} (indexed #{indexed}, #{bytes} bytes)"
 *		end
 */
static VALUE rb_git_repo_clone(int argc, VALUE argv[], VALUE self)
{
	VALUE url;
	VALUE target_path;
	VALUE ruby_opts;
	VALUE transfer_callback = Qnil;
	VALUE progress_callback = Qnil;
	VALUE conflict_callback = Qnil;
	git_repository *p_repo;
	git_checkout_opts checkout_opts = GIT_CHECKOUT_OPTS_INIT;
	struct _clone_params clone_params = DEFAULT_CLONE_PARAMS;
	short can_release_gvl = 1; /* bool */

	rb_scan_args(argc, argv, "21", &url, &target_path, &ruby_opts);
	if (TYPE(ruby_opts) != T_HASH)
		ruby_opts = rb_hash_new(); /* Assume empty hash if none given */

	clone_params.pp_repo		 = &p_repo;
	clone_params.url				 = StringValueCStr(url);
	clone_params.target_path = StringValueCStr(target_path);

	/* Parse the checkout options */
	rb_git__parse_checkout_options(&ruby_opts, &checkout_opts, &progress_callback, &conflict_callback);
	clone_params.p_opts = &checkout_opts;

	/* Check if we got any of the three callbacks that would prevent us
	 * from releasing the GVL.*/

	/* 1. Transfer callback. Note that this callback is specific to the
	 * clone methods and is not handled by rb_git__parse_checkout_options()
	 * therefore, hence we must pass this to the C-side callback ourselves. */
	if (rb_block_given_p()) {
		transfer_callback = rb_block_proc(); /* &block */
		clone_params.p_ruby_transfer_proc = &transfer_callback;
		can_release_gvl = 0;
	}

	/* 2. Conflict callback. Handling is done by the checkout option
	 * parsing function, we just need to know about it for GVL. */
	if (checkout_opts.conflict_payload)
		can_release_gvl = 0;

	/* 3. Progress callback. Handling is done by the checkout option
	 * parsing function, we just need to know about it for GVL. */
	if (checkout_opts.progress_payload)
		can_release_gvl = 0;

	/* Release the GVL if the Ruby version permits it */
	if (can_release_gvl) {
#ifdef USING_RUBY_19
		rb_thread_blocking_region(	rb_git_repo__internal_clone,
																&clone_params,
																rb_git_repo__abort_clone,
																&clone_params);
#else
		/* Poor 1.8 guys, you cannot have concurrency */
		rb_git_repo__internal_clone(&clone_params);
#endif
	}
	else {
		/* If any block or callback was given, we cannot release the GVL due to callback */
		rb_git_repo__internal_clone(&clone_params);
	}

	/* Check for success and return new Repository instance */
	rugged_exception_check(clone_params.git_error);
	return rugged_repo_new(self, p_repo);
}

#define RB_GIT_REPO_OWNED_GET(_klass, _object) \
	VALUE rb_data = rb_iv_get(self, "@" #_object); \
	if (NIL_P(rb_data)) { \
		git_repository *repo; \
		git_##_object *data; \
		int error; \
		Data_Get_Struct(self, git_repository, repo); \
		error = git_repository_##_object(&data, repo); \
		rugged_exception_check(error); \
		rb_data = rugged_##_object##_new(_klass, self, data); \
		rb_iv_set(self, "@" #_object, rb_data); \
	} \
	return rb_data; \

#define RB_GIT_REPO_OWNED_SET(_klass, _object) \
	VALUE rb_old_data; \
	git_repository *repo; \
	git_##_object *data; \
	if (!rb_obj_is_kind_of(rb_data, _klass))\
		rb_raise(rb_eTypeError, \
			"The given object is not a Rugged::" #_object); \
	if (!NIL_P(rugged_owner(rb_data))) \
		rb_raise(rb_eRuntimeError, \
			"The given object is already owned by another repository"); \
	Data_Get_Struct(self, git_repository, repo); \
	Data_Get_Struct(rb_data, git_##_object, data); \
	git_repository_set_##_object(repo, data); \
	rb_old_data = rb_iv_get(self, "@" #_object); \
	if (!NIL_P(rb_old_data)) rugged_set_owner(rb_old_data, Qnil); \
	rugged_set_owner(rb_data, self); \
	rb_iv_set(self, "@" #_object, rb_data); \
	return Qnil; \


/*
 *	call-seq:
 *		repo.index = idx
 *
 *	Set the index for this +Repository+. +idx+ must be a instance of
 *	+Rugged::Index+. This index will be used internally by all
 *	operations that use the Git index on +repo+.
 *
 *	Note that it's not necessary to set the +index+ for any repository;
 *	by default repositories are loaded with the index file that can be
 *	located on the +.git+ folder in the filesystem.
 */
static VALUE rb_git_repo_set_index(VALUE self, VALUE rb_data)
{
	RB_GIT_REPO_OWNED_SET(rb_cRuggedIndex, index);
}

static VALUE rb_git_repo_get_index(VALUE self)
{
	RB_GIT_REPO_OWNED_GET(rb_cRuggedIndex, index);
}

/*
 *	call-seq:
 *		repo.config = cfg
 *
 *	Set the configuration file for this +Repository+. +cfg+ must be a instance of
 *	+Rugged::Config+. This config file will be used internally by all
 *	operations that need to lookup configuration settings on +repo+.
 *
 *	Note that it's not necessary to set the +config+ for any repository;
 *	by default repositories are loaded with their relevant config files
 *	on the filesystem, and the corresponding global and system files if
 *	they can be found.
 */
static VALUE rb_git_repo_set_config(VALUE self, VALUE rb_data)
{
	RB_GIT_REPO_OWNED_SET(rb_cRuggedConfig, config);
}

static VALUE rb_git_repo_get_config(VALUE self)
{
	RB_GIT_REPO_OWNED_GET(rb_cRuggedConfig, config);
}

/*
 *	call-seq:
 *		repo.include?(oid) -> true or false
 *		repo.exists?(oid) -> true or false
 *
 *	Return whether an object with the given SHA1 OID (represented as
 *	a 40-character string) exists in the repository.
 *
 *		repo.include?("d8786bfc97485e8d7b19b21fb88c8ef1f199fc3f") #=> true
 */
static VALUE rb_git_repo_exists(VALUE self, VALUE hex)
{
	git_repository *repo;
	git_odb *odb;
	git_oid oid;
	int error;
	VALUE rb_result;

	Data_Get_Struct(self, git_repository, repo);
	Check_Type(hex, T_STRING);

	error = git_repository_odb(&odb, repo);
	rugged_exception_check(error);

	error = git_oid_fromstr(&oid, StringValueCStr(hex));
	rugged_exception_check(error);

	rb_result = git_odb_exists(odb, &oid) ? Qtrue : Qfalse;
	git_odb_free(odb);

	return rb_result;
}

static VALUE rb_git_repo_read(VALUE self, VALUE hex)
{
	git_repository *repo;
	git_oid oid;
	int error;

	Data_Get_Struct(self, git_repository, repo);
	Check_Type(hex, T_STRING);

	error = git_oid_fromstr(&oid, StringValueCStr(hex));
	rugged_exception_check(error);

	return rugged_raw_read(repo, &oid);
}

static VALUE rb_git_repo_read_header(VALUE self, VALUE hex)
{
	git_repository *repo;
	git_oid oid;
	git_odb *odb;
	git_otype type;
	size_t len;
	VALUE rb_hash;
	int error;

	Data_Get_Struct(self, git_repository, repo);
	Check_Type(hex, T_STRING);

	error = git_oid_fromstr(&oid, StringValueCStr(hex));
	rugged_exception_check(error);

	error = git_repository_odb(&odb, repo);
	rugged_exception_check(error);

	error = git_odb_read_header(&len, &type, odb, &oid);
	git_odb_free(odb);
	rugged_exception_check(error);

	rb_hash = rb_hash_new();
	rb_hash_aset(rb_hash, CSTR2SYM("type"), CSTR2SYM(git_object_type2string(type)));
	rb_hash_aset(rb_hash, CSTR2SYM("len"), INT2FIX(len));

	return rb_hash;
}

/*
 *	call-seq:
 *		Repository.hash(buffer, type) -> oid
 *
 *	Hash the contents of +buffer+ as raw bytes (ignoring any encoding
 *	information) and adding the relevant header corresponding to +type+,
 *	and return a hex string representing the result from the hash.
 *
 *		Repository.hash('hello world', :commit) #=> "de5ba987198bcf2518885f0fc1350e5172cded78"
 *
 *		Repository.hash('hello_world', :tag) #=> "9d09060c850defbc7711d08b57def0d14e742f4e"
 */
static VALUE rb_git_repo_hash(VALUE self, VALUE rb_buffer, VALUE rb_type)
{
	int error;
	git_oid oid;

	Check_Type(rb_buffer, T_STRING);

	error = git_odb_hash(&oid,
		RSTRING_PTR(rb_buffer),
		RSTRING_LEN(rb_buffer),
		rugged_otype_get(rb_type)
	);
	rugged_exception_check(error);

	return rugged_create_oid(&oid);
}

/*
 *	call-seq:
 *		Repository.hash_file(path, type) -> oid
 *
 *	Hash the contents of the file pointed at by +path+, assuming
 *	that it'd be stored in the ODB with the given +type+, and return
 *	a hex string representing the SHA1 OID resulting from the hash.
 *
 *		Repository.hash_file('foo.txt', :commit) #=> "de5ba987198bcf2518885f0fc1350e5172cded78"
 *
 *		Repository.hash_file('foo.txt', :tag) #=> "9d09060c850defbc7711d08b57def0d14e742f4e"
 */
static VALUE rb_git_repo_hashfile(VALUE self, VALUE rb_path, VALUE rb_type)
{
	int error;
	git_oid oid;

	Check_Type(rb_path, T_STRING);

	error = git_odb_hashfile(&oid,
		StringValueCStr(rb_path),
		rugged_otype_get(rb_type)
	);
	rugged_exception_check(error);

	return rugged_create_oid(&oid);
}

static VALUE rb_git_repo_write(VALUE self, VALUE rb_buffer, VALUE rub_type)
{
	git_repository *repo;
	git_odb_stream *stream;

	git_odb *odb;
	git_oid oid;
	int error;

	git_otype type;

	Data_Get_Struct(self, git_repository, repo);

	error = git_repository_odb(&odb, repo);
	rugged_exception_check(error);

	type = rugged_otype_get(rub_type);

	error = git_odb_open_wstream(&stream, odb, RSTRING_LEN(rb_buffer), type);
	git_odb_free(odb);
	rugged_exception_check(error);

	error = stream->write(stream, RSTRING_PTR(rb_buffer), RSTRING_LEN(rb_buffer));
	rugged_exception_check(error);

	error = stream->finalize_write(&oid, stream);
	rugged_exception_check(error);

	return rugged_create_oid(&oid);
}

#define RB_GIT_REPO_GETTER(method) \
	git_repository *repo; \
	int error; \
	Data_Get_Struct(self, git_repository, repo); \
	error = git_repository_##method(repo); \
	rugged_exception_check(error); \
	return error ? Qtrue : Qfalse; \

/*
 *	call-seq:
 *		repo.bare? -> true or false
 *
 *	Return whether a repository is bare or not. A bare repository has no
 *	working directory.
 */
static VALUE rb_git_repo_is_bare(VALUE self)
{
	RB_GIT_REPO_GETTER(is_bare);
}


/*
 *	call-seq:
 *		repo.empty? -> true or false
 *
 *	Return whether a repository is empty or not. An empty repository has just
 *	been initialized and has no commits yet.
 */
static VALUE rb_git_repo_is_empty(VALUE self)
{
	RB_GIT_REPO_GETTER(is_empty);
}

/*
 *	call-seq:
 *		repo.head_detached? -> true or false
 *
 *	Return whether the +HEAD+ of a repository is detached or not.
 */
static VALUE rb_git_repo_head_detached(VALUE self)
{
	RB_GIT_REPO_GETTER(head_detached);
}

/*
 *	call-seq:
 *		repo.head_orphan? -> true or false
 *
 *	Return whether the +HEAD+ of a repository is orphaned or not.
 */
static VALUE rb_git_repo_head_orphan(VALUE self)
{
	RB_GIT_REPO_GETTER(head_orphan);
}

/*
 *	call-seq:
 *		repo.path -> path
 *
 *	Return the full, normalized path to this repository. For non-bare repositories,
 *	this is the path of the actual +.git+ folder, not the working directory.
 *
 *		repo.path #=> "/home/foo/workthing/.git"
 */
static VALUE rb_git_repo_path(VALUE self)
{
	git_repository *repo;
	Data_Get_Struct(self, git_repository, repo);
	return rugged_str_new2(git_repository_path(repo), NULL);
}

/*
 *	call-seq:
 *		repo.workdir -> path or nil
 *
 *	Return the working directory for this repository, or +nil+ if
 *	the repository is bare.
 *
 *		repo1.bare? #=> false
 *		repo1.workdir # => "/home/foo/workthing/"
 *
 *		repo2.bare? #=> true
 *		repo2.workdir #=> nil
 */
static VALUE rb_git_repo_workdir(VALUE self)
{
	git_repository *repo;
	const char *workdir;

	Data_Get_Struct(self, git_repository, repo);
	workdir = git_repository_workdir(repo);

	return workdir ? rugged_str_new2(workdir, NULL) : Qnil;
}

/*
 *	call-seq:
 *		repo.workdir = path
 *
 *	Sets the working directory of +repo+ to +path+. All internal
 *	operations on +repo+ that affect the working directory will
 *	instead use +path+.
 *
 *	The +workdir+ can be set on bare repositories to temporarily
 *	turn them into normal repositories.
 *
 *		repo.bare? #=> true
 *		repo.workdir = "/tmp/workdir"
 *		repo.bare? #=> false
 *		repo.checkout
 */
static VALUE rb_git_repo_set_workdir(VALUE self, VALUE rb_workdir)
{
	git_repository *repo;

	Data_Get_Struct(self, git_repository, repo);
	Check_Type(rb_workdir, T_STRING);

	rugged_exception_check(
		git_repository_set_workdir(repo, StringValueCStr(rb_workdir), 0)
	);

	return Qnil;
}

/*
 * call-seq:
 *   repo.checkout_index( [ opts = {} ] )
 *
 * Writes the files in the index ("staging area") out to the working
 * tree. Does not update the HEAD pointer automatically.
 *
 * Checking out git trees is a fairly complex operation that can
 * be adjusted in a variety of ways, the full documentation on this
 * including background information can be found in libgit2's
 * <tt>checkout.h</tt> header. Rugged's checkout methods support
 * all operations mentioned there, but note that libgit2 itself
 * doesn't implement them all yet.
 *
 * +opts+ is a hash which can take the following values:
 * [:strategy (<tt>[:default]</tt>)]
 *   Defines how to exactly check out the given +treeish+. This is an
 *   array containing one or more of the following symbols:
 *
 *   [:default]
 *     Consider everything that would need a change a conflict.
 *     This is effectively a dry run that does nothing.
 *   [:update_unmodified]
 *    Update entries in the working tree that don't have uncommited
 *    data.
 *   [:update_missing]
 *     Update entries that don't exist in the working tree. Note that
 *     due to bug #1121 libgit2 currently always behaves as if this
 *     option was passed, i.e. there's no way to prevent this.
 *   [:safe]
 *     Update entries that don't have uncommited data. Equal
 *     to passing :update_unmodified and :update_missing.
 *   [:update_modified]
 *     Overwrite entries with uncommitted data.
 *   [:update_untracked]
 *     Overwrite entries not checked into git if necessary.
 *   [:force]
 *     Force the working tree to like like the index regardless of
 *     any conflicts. Equal to specifying :safe, :update_modified,
 *     and :update_untracked.
 *   [:allow_conflicts]
 *     Check out even if conflicts are present. This causes libgit2
 *     to continue checking out after the conflict callback has been
 *     called (see below).
 *   [:remove_untracked]
 *     Remove files not checked into Git (these would normally be
 *     ignored).
 *   [:update_only]
 *     Only update existing files and don't create new ones.
 *   [:skip_unmerged (not implemented)]
 *     Don't check out unmerged files, but continue.
 *   [:use_ours (not implemented)]
 *     Accept our version as the resolution of a merge conflict (i.e.
 *     check out stage 2 from the index).
 *   [:use_theirs (not implemented)]
 *     Accept the other's version as the resolution of a merge conflict
 *     (i.e. check out stage 3 from the index).
 *   [:update_submodules (not implemented)]
 *     Recursively check out submodules with the same options.
 *   [:update_submodules_if_changed (not implemented)]
 *     Recursively check out submodules if HEAD moved in parent repo.
 *
 * [:disable_filters (false)]
 *   Don't apply filters like CR-LF conversion.
 * [:dir_mode (0755)]
 *   Not documented by libgit2.
 * [:file_mode (0664)]
 *   Not documented by libgit2.
 * [:file_open_flags (<tt>IO::CREAT | IO::TRUNC | IO::WRONLY</tt>)]
 *   Not documented by libgit2.
 * [:progress_cb]
 *   This callback is invoked during the checkout process and gets passed the
 *   path to the file currently being checked out, the number of completed steps,
 *   and the number of total steps.
 * [:conflict_cb]
 *   Whenever the checkout algorithm encounters a file that it isn't allowed
 *   to check out using the policies outlined in :strategy, this callback is
 *   invoked. It gets passed the path to the conflicting file, the OID
 *   of the object in the index (as a hex-encoded string), the entry's mode
 *   in the index and the mode in the working directory. A return value of
 *   anything Ruby considers to be true will abort the checkout operation.
 */
static VALUE rb_git_repo_checkout_index(int argc, VALUE argv[], VALUE self)
{
	VALUE ruby_opts;
	VALUE progress_callback = Qnil;
	VALUE conflict_callback = Qnil;
	git_repository *repo;
	int error;
  git_checkout_opts opts = GIT_CHECKOUT_OPTS_INIT;

  Data_Get_Struct(self, git_repository, repo);

  /* Grab the options hash */
  rb_scan_args(argc, argv, "01", &ruby_opts);
  if (TYPE(ruby_opts) != T_HASH)
    ruby_opts = rb_hash_new(); /* If not passed, assume an empty hash */

  /* Parse the wealth of options and collect the results in `opts' */
  rb_git__parse_checkout_options(&ruby_opts, &opts, &progress_callback, &conflict_callback);

  /* Checkout operation */
  error = git_checkout_index(repo, NULL, &opts);
  rugged_exception_check(error);

  return Qnil;
}

/*
 * call-seq:
 *   repo.checkout_head( [ opts = {} ] )
 *
 * Updates files in the index and the working tree to match the commit pointed
 * at by HEAD.
 *
 * See #checkout_index for a description of the values you can assign
 * to +opts+.
 */
static VALUE rb_git_repo_checkout_head(int argc, VALUE argv[], VALUE self)
{
	VALUE ruby_opts;
	VALUE progress_callback = Qnil;
	VALUE conflict_callback = Qnil;
	git_repository *repo;
  int error;
  git_checkout_opts opts = GIT_CHECKOUT_OPTS_INIT;

  Data_Get_Struct(self, git_repository, repo);

  /* Grab the options hash */
  rb_scan_args(argc, argv, "01", &ruby_opts);
  if (TYPE(ruby_opts) != T_HASH)
    ruby_opts = rb_hash_new(); /* If not passed, assume an empty hash */

  /* Parse the wealth of options and collect the results in `opts' */
  rb_git__parse_checkout_options(&ruby_opts, &opts, &progress_callback, &conflict_callback);

  /* Checkout operation */
  error = git_checkout_head(repo, &opts);
  rugged_exception_check(error);

  return Qnil;
}

/*
 * call-seq:
 *   repo.checkout_tree(treeish [, opts = {}])
 *
 * Reads the tree pointed to by +treeish+ into the index and
 * writes the index out to the working tree afterwards. Note this method
 * does *not* update the HEAD pointer, so you probably want to do this
 * afterwards:
 *
 *   # Checkout the branch "yourbranch"
 *   ref = Rugged::Reference.new(repo, "refs/heads/yourbranch")
 *   commit = Rugged::Commit.lookup(repo, ref.target)
 *   repo.checkout_tree(commit, strategy: [:force])
 *
 *   # Update the HEAD pointer to point to "yourbranch"
 *   head = Rugged::Reference.lookup(repo, "HEAD")
 *   head.target = ref.name
 *
 * See #checkout_index for the values you can pass via +opts+. +treeish+ may be
 * a real Tree instance, a Commit or a Tag object.
 */
static VALUE rb_git_repo_checkout_tree(int argc, VALUE argv[], VALUE self)
{ /* NOTE: Checking out a tree is the same as pushing a tree onto the index (read-tree) and then checking out the index (checkout-index) */
	VALUE target;
	VALUE ruby_opts;
	VALUE progress_callback = Qnil;
	VALUE conflict_callback = Qnil;
	git_repository *repo;
  int error;
  git_object *treeish;
  git_checkout_opts opts = GIT_CHECKOUT_OPTS_INIT;

  Data_Get_Struct(self, git_repository, repo);

  /* Grab the options hash */
  rb_scan_args(argc, argv, "11", &target, &ruby_opts);
  if (TYPE(ruby_opts) != T_HASH)
    ruby_opts = rb_hash_new(); /* If not passed, assume an empty hash */

  if (!rb_obj_is_kind_of(target, rb_cRuggedObject))
    rb_raise(rb_eTypeError, "Expected argument #1 to be a Rugged::Object (a commit, a tag or a tree)!");
  Data_Get_Struct(target, git_object, treeish);

  /* Parse the wealth of options and collect the results in `opts' */
  rb_git__parse_checkout_options(&ruby_opts, &opts, &progress_callback, &conflict_callback);

  /* Update the index */
  error = git_checkout_tree(repo, treeish, &opts);
  rugged_exception_check(error);
  return Qnil;
}

/*
 *	call-seq:
 *		Repository.discover(path = nil, across_fs = true) -> repository
 *
 *	Traverse +path+ upwards until a Git working directory with a +.git+
 *	folder has been found, open it and return it as a +Repository+
 *	object.
 *
 *	If +path+ is +nil+, the current working directory will be used as
 *	a starting point.
 *
 *	If +across_fs+ is +true+, the traversal won't stop when reaching
 *	a different device than the one that contained +path+ (only applies
 *	to UNIX-based OSses).
 */
static VALUE rb_git_repo_discover(int argc, VALUE *argv, VALUE self)
{
	VALUE rb_path, rb_across_fs;
	char repository_path[GIT_PATH_MAX];
	int error, across_fs = 0;

	rb_scan_args(argc, argv, "02", &rb_path, &rb_across_fs);

	if (NIL_P(rb_path)) {
		VALUE rb_dir = rb_const_get(rb_cObject, rb_intern("Dir"));
		rb_path = rb_funcall(rb_dir, rb_intern("pwd"), 0);
	}

	if (!NIL_P(rb_across_fs)) {
		across_fs = rugged_parse_bool(rb_across_fs);
	}

	Check_Type(rb_path, T_STRING);

	error = git_repository_discover(
		repository_path, GIT_PATH_MAX,
		StringValueCStr(rb_path),
		across_fs,
		NULL
	);

	rugged_exception_check(error);
	return rugged_str_new2(repository_path, NULL);
}

static VALUE flags_to_rb(unsigned int flags)
{
	VALUE rb_flags = rb_ary_new();

	if (flags & GIT_STATUS_INDEX_NEW)
		rb_ary_push(rb_flags, CSTR2SYM("index_new"));

	if (flags & GIT_STATUS_INDEX_MODIFIED)
		rb_ary_push(rb_flags, CSTR2SYM("index_modified"));

	if (flags & GIT_STATUS_INDEX_DELETED)
		rb_ary_push(rb_flags, CSTR2SYM("index_deleted"));

	if (flags & GIT_STATUS_WT_NEW)
		rb_ary_push(rb_flags, CSTR2SYM("worktree_new"));

	if (flags & GIT_STATUS_WT_MODIFIED)
		rb_ary_push(rb_flags, CSTR2SYM("worktree_modified"));

	if (flags & GIT_STATUS_WT_DELETED)
		rb_ary_push(rb_flags, CSTR2SYM("worktree_deleted"));

	return rb_flags;
}

static int rugged__status_cb(const char *path, unsigned int flags, void *payload)
{
	rb_funcall((VALUE)payload, rb_intern("call"), 2,
		rugged_str_new2(path, NULL),
		flags_to_rb(flags)
	);

	return GIT_OK;
}

/*
 *	call-seq:
 *		repo.status { |status_data| block }
 *		repo.status(path) -> status_data
 *
 *	Returns the status for one or more files in the working directory
 *	of the repository. This is equivalent to the +git status+ command.
 *
 *	The returned +status_data+ is always an array containing one or more
 *	status flags as Ruby symbols. Possible flags are:
 *
 *	- +:index_new+: the file is new in the index
 *	- +:index_modified+: the file has been modified in the index
 *	- +:index_deleted+: the file has been deleted from the index
 *	- +:worktree_new+: the file is new in the working directory
 *	- +:worktree_modified+: the file has been modified in the working directory
 *	- +:worktree_deleted+: the file has been deleted from the working directory
 *
 *	If a +block+ is given, status information will be gathered for every
 *	single file on the working dir. The +block+ will be called with the
 *	status data for each file.
 *
 *		repo.status { |status_data| puts status_data.inspect }
 *
 *	results in, for example:
 *
 *		[:index_new, :worktree_new]
 *		[:worktree_modified]
 *
 *	If a +path+ is given instead, the function will return the +status_data+ for
 *	the file pointed to by path, or raise an exception if the path doesn't exist.
 *
 *	+path+ must be relative to the repository's working directory.
 *
 *		repo.status('src/diff.c') #=> [:index_new, :worktree_new]
 */
static VALUE rb_git_repo_status(int argc, VALUE *argv, VALUE self)
{
	int error;
	VALUE rb_path;
	git_repository *repo;

	Data_Get_Struct(self, git_repository, repo);

	if (rb_scan_args(argc, argv, "01", &rb_path) == 1) {
		unsigned int flags;
		Check_Type(rb_path, T_STRING);
		error = git_status_file(&flags, repo, StringValueCStr(rb_path));
		rugged_exception_check(error);

		return flags_to_rb(flags);
	}

	if (!rb_block_given_p())
		rb_raise(rb_eRuntimeError,
			"A block was expected for iterating through "
			"the repository contents.");

	error = git_status_foreach(
		repo,
		&rugged__status_cb,
		(void *)rb_block_proc()
	);

	rugged_exception_check(error);
	return Qnil;
}

static int rugged__each_id_cb(const git_oid *id, void *payload)
{
	rb_yield(rugged_create_oid(id));
	return 0;
}

/*
 *	call-seq:
 *		repo.each_id { |id| block }
 *		repo.each_id -> Iterator
 *
 *	Call the given +block+ once with every object ID found in +repo+
 *	and all its alternates. Object IDs are passed as 40-character
 *	strings.
 */
static VALUE rb_git_repo_each_id(VALUE self)
{
	git_repository *repo;
	git_odb *odb;
	int error;

	if (!rb_block_given_p())
		return rb_funcall(self, rb_intern("to_enum"), 1, CSTR2SYM("each_id"));

	Data_Get_Struct(self, git_repository, repo);

	error = git_repository_odb(&odb, repo);
	rugged_exception_check(error);

	error = git_odb_foreach(odb, &rugged__each_id_cb, NULL);
	git_odb_free(odb);

	rugged_exception_check(error);
	return Qnil;
}

void Init_rugged_repo()
{
	rb_cRuggedRepo = rb_define_class_under(rb_mRugged, "Repository", rb_cObject);

	rb_define_singleton_method(rb_cRuggedRepo, "new", rb_git_repo_new, -1);
	rb_define_singleton_method(rb_cRuggedRepo, "hash",   rb_git_repo_hash,  2);
	rb_define_singleton_method(rb_cRuggedRepo, "hash_file",   rb_git_repo_hashfile,  2);
	rb_define_singleton_method(rb_cRuggedRepo, "init_at", rb_git_repo_init_at, 2);
	rb_define_singleton_method(rb_cRuggedRepo, "clone_repo", rb_git_repo_clone, -1); /* Cannot name it ::clone b/c Ruby already has a method with this name */
	rb_define_singleton_method(rb_cRuggedRepo, "clone_repo_bare", rb_git_repo_clone_bare, 2);
	rb_define_singleton_method(rb_cRuggedRepo, "discover", rb_git_repo_discover, -1);

	rb_define_method(rb_cRuggedRepo, "exists?", rb_git_repo_exists, 1);
	rb_define_method(rb_cRuggedRepo, "include?", rb_git_repo_exists, 1);

	rb_define_method(rb_cRuggedRepo, "read",   rb_git_repo_read,   1);
	rb_define_method(rb_cRuggedRepo, "read_header",   rb_git_repo_read_header,   1);
	rb_define_method(rb_cRuggedRepo, "write",  rb_git_repo_write,  2);
	rb_define_method(rb_cRuggedRepo, "each_id",  rb_git_repo_each_id,  0);

	rb_define_method(rb_cRuggedRepo, "path",  rb_git_repo_path, 0);
	rb_define_method(rb_cRuggedRepo, "workdir",  rb_git_repo_workdir, 0);
	rb_define_method(rb_cRuggedRepo, "workdir=",  rb_git_repo_set_workdir, 1);
	rb_define_method(rb_cRuggedRepo, "status",  rb_git_repo_status,  -1);

	rb_define_method(rb_cRuggedRepo, "index",  rb_git_repo_get_index,  0);
	rb_define_method(rb_cRuggedRepo, "index=",  rb_git_repo_set_index,  1);
	rb_define_method(rb_cRuggedRepo, "config",  rb_git_repo_get_config,  0);
	rb_define_method(rb_cRuggedRepo, "config=",  rb_git_repo_set_config,  1);

	rb_define_method(rb_cRuggedRepo, "bare?",  rb_git_repo_is_bare,  0);
	rb_define_method(rb_cRuggedRepo, "empty?",  rb_git_repo_is_empty,  0);

	rb_define_method(rb_cRuggedRepo, "head_detached?",  rb_git_repo_head_detached,  0);
	rb_define_method(rb_cRuggedRepo, "head_orphan?",  rb_git_repo_head_orphan,  0);

	rb_define_method(rb_cRuggedRepo, "checkout_index", rb_git_repo_checkout_index, -1);
	rb_define_method(rb_cRuggedRepo, "checkout_head", rb_git_repo_checkout_head, -1);
	rb_define_method(rb_cRuggedRepo, "checkout_tree", rb_git_repo_checkout_tree, -1);

	rb_cRuggedOdbObject = rb_define_class_under(rb_mRugged, "OdbObject", rb_cObject);
	rb_define_method(rb_cRuggedOdbObject, "data",  rb_git_odbobj_data,  0);
	rb_define_method(rb_cRuggedOdbObject, "len",  rb_git_odbobj_size,  0);
	rb_define_method(rb_cRuggedOdbObject, "type",  rb_git_odbobj_type,  0);
	rb_define_method(rb_cRuggedOdbObject, "oid",  rb_git_odbobj_oid,  0);
}
