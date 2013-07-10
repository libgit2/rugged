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
#include <git2/sys/repository.h>

extern VALUE rb_mRugged;
extern VALUE rb_eRuggedError;
extern VALUE rb_cRuggedIndex;
extern VALUE rb_cRuggedConfig;
extern VALUE rb_cRuggedBackend;
extern VALUE rb_cRuggedRemote;
extern VALUE rb_cRuggedReference;

VALUE rb_cRuggedRepo;
VALUE rb_cRuggedOdbObject;

static ID id_call;

/*
 *  call-seq:
 *    odb_obj.oid -> hex_oid
 *
 *  Return the Object ID (a 40 character SHA1 hash) for this raw
 *  object.
 *
 *    odb_obj.oid #=> "d8786bfc97485e8d7b19b21fb88c8ef1f199fc3f"
 */
static VALUE rb_git_odbobj_oid(VALUE self)
{
	git_odb_object *obj;
	Data_Get_Struct(self, git_odb_object, obj);
	return rugged_create_oid(git_odb_object_id(obj));
}

/*
 *  call-seq:
 *    odb_obj.data -> buffer
 *
 *  Return an ASCII buffer with the raw bytes that form the Git object.
 *
 *    odb_obj.data #=> "tree 87ebee8367f9cc5ac04858b3bd5610ca74f04df9\n"
 *                 #=> "parent 68d041ee999cb07c6496fbdd4f384095de6ca9e1\n"
 *                 #=> "author Vicent Martí <tanoku@gmail.com> 1326863045 -0800\n"
 *                 #=> ...
 */
static VALUE rb_git_odbobj_data(VALUE self)
{
	git_odb_object *obj;
	Data_Get_Struct(self, git_odb_object, obj);
	return rb_str_new(git_odb_object_data(obj), git_odb_object_size(obj));
}

/*
 *  call-seq:
 *    odb_obj.size -> size
 *
 *  Return the size in bytes of the Git object after decompression. This is
 *  also the size of the +obj.data+ buffer.
 *
 *    odb_obj.size #=> 231
 */
static VALUE rb_git_odbobj_size(VALUE self)
{
	git_odb_object *obj;
	Data_Get_Struct(self, git_odb_object, obj);
	return INT2FIX(git_odb_object_size(obj));
}

/*
 *  call-seq:
 *    odb_obj.type -> Symbol
 *
 *  Return a Ruby symbol representing the basic Git type of this object.
 *  Possible values are +:tree+, +:blob+, +:commit+ and +:tag+.
 *
 *    odb_obj.type #=> :tag
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

static void load_alternates(git_repository *repo, VALUE rb_alternates)
{
	git_odb *odb = NULL;
	int i, error;

	if (NIL_P(rb_alternates))
		return;

	Check_Type(rb_alternates, T_ARRAY);

	if (RARRAY_LEN(rb_alternates) == 0)
		return;

	for (i = 0; i < RARRAY_LEN(rb_alternates); ++i)
		Check_Type(rb_ary_entry(rb_alternates, i), T_STRING);

	error = git_repository_odb(&odb, repo);
	rugged_exception_check(error);

	for (i = 0; !error && i < RARRAY_LEN(rb_alternates); ++i) {
		VALUE alt = rb_ary_entry(rb_alternates, i);
		error = git_odb_add_disk_alternate(odb, StringValueCStr(alt));
	}

	git_odb_free(odb);
	rugged_exception_check(error);
}

/*
 *  call-seq:
 *    Repository.bare(path[, alternates]) -> repository
 *
 *  Open a bare Git repository at +path+ and return a +Repository+
 *  object representing it.
 *
 *  This is faster than Rugged::Repository.new, as it won't attempt to perform
 *  any +.git+ directory discovery, won't try to load the config options to
 *  determine whether the repository is bare and won't try to load the workdir.
 *
 *  Optionally, you can pass a list of alternate object folders.
 *
 *    Rugged::Repository.bare(path, ['./other/repo/.git/objects'])
 */
static VALUE rb_git_repo_open_bare(int argc, VALUE *argv, VALUE klass)
{
	git_repository *repo;
	int error = 0;
	VALUE rb_path, rb_alternates;

	rb_scan_args(argc, argv, "11", &rb_path, &rb_alternates);
	Check_Type(rb_path, T_STRING);

	error = git_repository_open_bare(&repo, StringValueCStr(rb_path));
	rugged_exception_check(error);

	load_alternates(repo, rb_alternates);

	return rugged_repo_new(klass, repo);
}

/*
 *  call-seq:
 *    Repository.new(path, options = {}) -> repository
 *
 *  Open a Git repository in the given +path+ and return a +Repository+ object
 *  representing it. An exception will be thrown if +path+ doesn't point to a
 *  valid repository. If you need to create a repository from scratch, use
 *  Rugged::Repository.init instead.
 *
 *  The +path+ must point to either the actual folder (+.git+) of a Git repository,
 *  or to the directorly that contains the +.git+ folder.
 *
 *  See also Rugged::Repository.discover and Rugged::Repository.bare.
 *
 *  The following options can be passed in the +options+ Hash:
 *
 *  :alternates ::
 *    A list of alternate object folders.
 *
 *  Examples:
 *
 *    Rugged::Repository.new('~/test/.git') #=> #<Rugged::Repository:0x108849488>
 *    Rugged::Repository.new(path, :alternates => ['./other/repo/.git/objects'])
 */
static VALUE rb_git_repo_new(int argc, VALUE *argv, VALUE klass)
{
	git_repository *repo;
	int error = 0;
	VALUE rb_path, rb_options;

	rb_scan_args(argc, argv, "10:", &rb_path, &rb_options);
	Check_Type(rb_path, T_STRING);

	error = git_repository_open(&repo, StringValueCStr(rb_path));
	rugged_exception_check(error);

	if (!NIL_P(rb_options)) {
		/* Check for `:alternates` */
		load_alternates(repo, rb_hash_aref(rb_options, CSTR2SYM("alternates")));
	}

	return rugged_repo_new(klass, repo);
}

/*
 *  call-seq:
 *    Repository.init_at(path, is_bare = false) -> repository
 *
 *  Initialize a Git repository in +path+. This implies creating all the
 *  necessary files on the FS, or re-initializing an already existing
 *  repository if the files have already been created.
 *
 *  The +is_bare+ (optional, defaults to false) attribute specifies whether
 *  the Repository should be created on disk as bare or not.
 *  Bare repositories have no working directory and are created in the root
 *  of +path+. Non-bare repositories are created in a +.git+ folder and
 *  use +path+ as working directory.
 *
 *    Rugged::Repository.init_at('~/repository', :bare) #=> #<Rugged::Repository:0x108849488>
 */
static VALUE rb_git_repo_init_at(int argc, VALUE *argv, VALUE klass)
{
	git_repository *repo;
	VALUE rb_path, rb_is_bare;
	int error, is_bare = 0;

	rb_scan_args(argc, argv, "11", &rb_path, &rb_is_bare);
	Check_Type(rb_path, T_STRING);

	if (!NIL_P(rb_is_bare))
		is_bare = rb_is_bare ? 1 : 0;

	error = git_repository_init(&repo, StringValueCStr(rb_path), is_bare);
	rugged_exception_check(error);

	return rugged_repo_new(klass, repo);
}

struct clone_fetch_callback_payload
{
	VALUE proc;
	VALUE exception;
	const git_transfer_progress *stats;
};

static VALUE clone_fetch_callback_inner(struct clone_fetch_callback_payload *fetch_payload)
{
	rb_funcall(fetch_payload->proc, id_call, 4,
		UINT2NUM(fetch_payload->stats->total_objects),
		UINT2NUM(fetch_payload->stats->indexed_objects),
		UINT2NUM(fetch_payload->stats->received_objects),
		INT2FIX(fetch_payload->stats->received_bytes));
	return GIT_OK;
}

static VALUE clone_fetch_callback_rescue(struct clone_fetch_callback_payload *fetch_payload, VALUE exception)
{
	fetch_payload->exception = exception;
	return GIT_ERROR;
}

static int clone_fetch_callback(const git_transfer_progress *stats, void *payload)
{
	struct clone_fetch_callback_payload *fetch_payload = payload;
	fetch_payload->stats = stats;
	return rb_rescue(clone_fetch_callback_inner, (VALUE) fetch_payload,
		clone_fetch_callback_rescue, (VALUE) fetch_payload);
}

static void parse_clone_options(git_clone_options *ret, VALUE rb_options_hash, struct clone_fetch_callback_payload *fetch_progress_payload)
{
	if (!NIL_P(rb_options_hash)) {
		VALUE val;

		val = rb_hash_aref(rb_options_hash, CSTR2SYM("bare"));
		if (RTEST(val))
			ret->bare = 1;

		val = rb_hash_aref(rb_options_hash, CSTR2SYM("progress"));
		if (RTEST(val)) {
			if (rb_respond_to(val, rb_intern("call"))) {
				fetch_progress_payload->proc = val;
				ret->fetch_progress_payload = fetch_progress_payload;
				ret->fetch_progress_cb = clone_fetch_callback;
			} else {
				rb_raise(rb_eArgError, "Expected a Proc or an object that responds to call (:progress).");
			}
		}
	}
}

/*
 *  call-seq:
 *    Repository.clone_at(url, local_path[, options]) -> repository
 *
 *  Clone a repository from +url+ to +local_path+.
 *
 *  The following options can be passed in the +options+ Hash:
 *
 *  :bare ::
 *    If +true+, the clone will be created as a bare repository.
 *    Defaults to +false+.
 *
 *  :progress ::
 *    A callback that will be executed to report clone progress information. It will be passed
 *    the amount of +total_objects+, +indexed_objects+, +received_objects+ and +received_bytes+.
 *
 *  Example:
 *
 *    progress = lambda { |total_objects, indexed_objects, received_objects, received_bytes|
 *      # ...
 *    }
 *    Repository.clone_at("https://github.com/libgit2/rugged.git", "./some/dir", :progress => progress)
 */
static VALUE rb_git_repo_clone_at(int argc, VALUE *argv, VALUE klass)
{
	VALUE url, local_path, rb_options_hash;
	git_clone_options options = GIT_CLONE_OPTIONS_INIT;
	struct clone_fetch_callback_payload fetch_payload;
	git_repository *repo;
	int error;

	rb_scan_args(argc, argv, "21", &url, &local_path, &rb_options_hash);
	Check_Type(url, T_STRING);
	Check_Type(local_path, T_STRING);

	fetch_payload.proc = Qnil;
	fetch_payload.exception = Qnil;
	parse_clone_options(&options, rb_options_hash, &fetch_payload);

	error = git_clone(&repo, StringValueCStr(url), StringValueCStr(local_path), &options);
	if (RTEST(fetch_payload.exception))
		rb_exc_raise(fetch_payload.exception);
	rugged_exception_check(error);

	return rugged_repo_new(klass, repo);
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
 *  call-seq:
 *    repo.index = idx
 *
 *  Set the index for this +Repository+. +idx+ must be a instance of
 *  Rugged::Index. This index will be used internally by all
 *  operations that use the Git index on +repo+.
 *
 *  Note that it's not necessary to set the +index+ for any repository;
 *  by default repositories are loaded with the index file that can be
 *  located on the +.git+ folder in the filesystem.
 */
static VALUE rb_git_repo_set_index(VALUE self, VALUE rb_data)
{
	RB_GIT_REPO_OWNED_SET(rb_cRuggedIndex, index);
}

/*
 *  call-seq:
 *    repo.index -> idx
 *
 *  Return the default index for this repository.
 */
static VALUE rb_git_repo_get_index(VALUE self)
{
	RB_GIT_REPO_OWNED_GET(rb_cRuggedIndex, index);
}

/*
 *  call-seq:
 *    repo.config = cfg
 *
 *  Set the configuration file for this +Repository+. +cfg+ must be a instance of
 *  Rugged::Config. This config file will be used internally by all
 *  operations that need to lookup configuration settings on +repo+.
 *
 *  Note that it's not necessary to set the +config+ for any repository;
 *  by default repositories are loaded with their relevant config files
 *  on the filesystem, and the corresponding global and system files if
 *  they can be found.
 */
static VALUE rb_git_repo_set_config(VALUE self, VALUE rb_data)
{
	RB_GIT_REPO_OWNED_SET(rb_cRuggedConfig, config);
}

/*
 *  call-seq:
 *    repo.config -> cfg
 *
 *  Return a Rugged::Config object representing this repository's config.
 */
static VALUE rb_git_repo_get_config(VALUE self)
{
	RB_GIT_REPO_OWNED_GET(rb_cRuggedConfig, config);
}

/*
 *  call-seq:
 *    repo.merge_base(oid1, oid2, ...)
 *    repo.merge_base(ref1, ref2, ...)
 *    repo.merge_base(commit1, commit2, ...)
 *
 *  Find a merge base, given two or more commits or oids.
 *  Returns nil if a merge base is not found.
 */
static VALUE rb_git_repo_merge_base(VALUE self, VALUE rb_args)
{
	int error = GIT_OK, i;
	git_repository *repo;
	git_oid base, *input_array = xmalloc(sizeof(git_oid) * RARRAY_LEN(rb_args));
	int len = (int)RARRAY_LEN(rb_args);

	if (len < 2)
		rb_raise(rb_eArgError, "wrong number of arguments (%d for 2+)", len);

	Data_Get_Struct(self, git_repository, repo);

	for (i = 0; !error && i < len; ++i) {
		error = rugged_oid_get(&input_array[i], repo, rb_ary_entry(rb_args, i));
	}

	if (error) {
		xfree(input_array);
		rugged_exception_check(error);
	}

	error = git_merge_base_many(&base, repo, input_array, len);
	xfree(input_array);

	if (error == GIT_ENOTFOUND)
		return Qnil;

	rugged_exception_check(error);

	return rugged_create_oid(&base);
}

/*
 *  call-seq:
 *    repo.include?(oid) -> true or false
 *    repo.exists?(oid) -> true or false
 *
 *  Return whether an object with the given SHA1 OID (represented as
 *  a 40-character string) exists in the repository.
 *
 *    repo.include?("d8786bfc97485e8d7b19b21fb88c8ef1f199fc3f") #=> true
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

	error = git_oid_fromstr(&oid, StringValueCStr(hex));
	rugged_exception_check(error);

	error = git_repository_odb(&odb, repo);
	rugged_exception_check(error);

	rb_result = git_odb_exists(odb, &oid) ? Qtrue : Qfalse;
	git_odb_free(odb);

	return rb_result;
}

/*
 *  call-seq:
 *    repo.read(oid) -> str
 *
 *  Read and return the raw data of the object identified by the given +oid+.
 */
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

/*
 *  call-seq:
 *    repo.read_header(oid) -> hash
 *
 *  Read and return the header information in +repo+'s ODB
 *  for the object identified by the given +oid+.
 *
 *  Returns a Hash object with the following key/value pairs:
 *
 *  :type ::
 *    A Symbol denoting the object's type. Possible values are:
 *    +:tree+, +:blob+, +:commit+ or +:tag+.
 *  :len ::
 *    A Number representing the object's length, in bytes.
 */
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
 *  call-seq:
 *    Repository.hash(buffer, type) -> oid
 *
 *  Hash the contents of +buffer+ as raw bytes (ignoring any encoding
 *  information) and adding the relevant header corresponding to +type+,
 *  and return a hex string representing the result from the hash.
 *
 *    Repository.hash('hello world', :commit) #=> "de5ba987198bcf2518885f0fc1350e5172cded78"
 *
 *    Repository.hash('hello_world', :tag) #=> "9d09060c850defbc7711d08b57def0d14e742f4e"
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
 *  call-seq:
 *    Repository.hash_file(path, type) -> oid
 *
 *  Hash the contents of the file pointed at by +path+, assuming
 *  that it'd be stored in the ODB with the given +type+, and return
 *  a hex string representing the SHA1 OID resulting from the hash.
 *
 *    Repository.hash_file('foo.txt', :commit) #=> "de5ba987198bcf2518885f0fc1350e5172cded78"
 *
 *    Repository.hash_file('foo.txt', :tag) #=> "9d09060c850defbc7711d08b57def0d14e742f4e"
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

/*
 *  call-seq:
 *    repo.write(buffer, type) -> oid
 *
 *  Write the data contained in the +buffer+ string as a raw object of the
 *  given +type+ into the repository's object database.
 *
 *  +type+ can be either +:tag+, +:commit+, +:tree+ or +:blob+.
 *
 *  Returns the newly created object's oid.
 */
static VALUE rb_git_repo_write(VALUE self, VALUE rb_buffer, VALUE rub_type)
{
	git_repository *repo;
	git_odb_stream *stream;

	git_odb *odb;
	git_oid oid;
	int error;

	git_otype type;

	Data_Get_Struct(self, git_repository, repo);
	Check_Type(rb_buffer, T_STRING);

	error = git_repository_odb(&odb, repo);
	rugged_exception_check(error);

	type = rugged_otype_get(rub_type);

	error = git_odb_open_wstream(&stream, odb, RSTRING_LEN(rb_buffer), type);
	git_odb_free(odb);
	rugged_exception_check(error);

	error = stream->write(stream, RSTRING_PTR(rb_buffer), RSTRING_LEN(rb_buffer));
	if (!error)
		error = stream->finalize_write(&oid, stream);

	stream->free(stream);
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
 *  call-seq:
 *    repo.bare? -> true or false
 *
 *  Return whether a repository is bare or not. A bare repository has no
 *  working directory.
 */
static VALUE rb_git_repo_is_bare(VALUE self)
{
	RB_GIT_REPO_GETTER(is_bare);
}


/*
 *  call-seq:
 *    repo.empty? -> true or false
 *
 *  Return whether a repository is empty or not. An empty repository has just
 *  been initialized and has no commits yet.
 */
static VALUE rb_git_repo_is_empty(VALUE self)
{
	RB_GIT_REPO_GETTER(is_empty);
}

/*
 *  call-seq:
 *    repo.head_detached? -> true or false
 *
 *  Return whether the +HEAD+ of a repository is detached or not.
 */
static VALUE rb_git_repo_head_detached(VALUE self)
{
	RB_GIT_REPO_GETTER(head_detached);
}

/*
 *  call-seq:
 *    repo.head_orphan? -> true or false
 *
 *  Return whether the +HEAD+ of a repository is orphaned or not.
 */
static VALUE rb_git_repo_head_orphan(VALUE self)
{
	RB_GIT_REPO_GETTER(head_orphan);
}

/*
 *  call-seq:
 *    repo.head = str
 *
 *  Make the repository's +HEAD+ point to the specified reference.
 */
static VALUE rb_git_repo_set_head(VALUE self, VALUE rb_head)
{
	git_repository *repo;
	int error;

	Data_Get_Struct(self, git_repository, repo);

	Check_Type(rb_head, T_STRING);
	error = git_repository_set_head(repo, StringValueCStr(rb_head));
	rugged_exception_check(error);

	return Qnil;
}

/*
 *  call-seq:
 *    repo.head -> ref
 *
 *  Retrieve and resolve the reference pointed at by the repository's +HEAD+.
 *
 *  Returns +nil+ if +HEAD+ is missing.
 */
static VALUE rb_git_repo_get_head(VALUE self)
{
	git_repository *repo;
	git_reference *head;
	int error;

	Data_Get_Struct(self, git_repository, repo);

	error = git_repository_head(&head, repo);
	if (error == GIT_ENOTFOUND)
		return Qnil;
	else
		rugged_exception_check(error);

	return rugged_ref_new(rb_cRuggedReference, self, head);
}

/*
 *  call-seq:
 *    repo.path -> path
 *
 *  Return the full, normalized path to this repository. For non-bare repositories,
 *  this is the path of the actual +.git+ folder, not the working directory.
 *
 *    repo.path #=> "/home/foo/workthing/.git"
 */
static VALUE rb_git_repo_path(VALUE self)
{
	git_repository *repo;
	Data_Get_Struct(self, git_repository, repo);
	return rb_str_new_utf8(git_repository_path(repo));
}

/*
 *  call-seq:
 *    repo.workdir -> path or nil
 *
 *  Return the working directory for this repository, or +nil+ if
 *  the repository is bare.
 *
 *    repo1.bare? #=> false
 *    repo1.workdir #=> "/home/foo/workthing/"
 *
 *    repo2.bare? #=> true
 *    repo2.workdir #=> nil
 */
static VALUE rb_git_repo_workdir(VALUE self)
{
	git_repository *repo;
	const char *workdir;

	Data_Get_Struct(self, git_repository, repo);
	workdir = git_repository_workdir(repo);

	return workdir ? rb_str_new_utf8(workdir) : Qnil;
}

/*
 *  call-seq:
 *    repo.workdir = path
 *
 *  Sets the working directory of +repo+ to +path+. All internal
 *  operations on +repo+ that affect the working directory will
 *  instead use +path+.
 *
 *  The +workdir+ can be set on bare repositories to temporarily
 *  turn them into normal repositories.
 *
 *    repo.bare? #=> true
 *    repo.workdir = "/tmp/workdir"
 *    repo.bare? #=> false
 *    repo.checkout
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
 *  call-seq:
 *    Repository.discover(path = nil, across_fs = true) -> repository
 *
 *  Traverse +path+ upwards until a Git working directory with a +.git+
 *  folder has been found, open it and return it as a +Repository+
 *  object.
 *
 *  If +path+ is +nil+, the current working directory will be used as
 *  a starting point.
 *
 *  If +across_fs+ is +true+, the traversal won't stop when reaching
 *  a different device than the one that contained +path+ (only applies
 *  to UNIX-based OSses).
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
	return rb_str_new_utf8(repository_path);
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
		rb_str_new_utf8(path), flags_to_rb(flags)
	);

	return GIT_OK;
}

/*
 *  call-seq:
 *    repo.status { |status_data| block }
 *    repo.status(path) -> status_data
 *
 *  Returns the status for one or more files in the working directory
 *  of the repository. This is equivalent to the +git status+ command.
 *
 *  The returned +status_data+ is always an array containing one or more
 *  status flags as Ruby symbols. Possible flags are:
 *
 *  - +:index_new+: the file is new in the index
 *  - +:index_modified+: the file has been modified in the index
 *  - +:index_deleted+: the file has been deleted from the index
 *  - +:worktree_new+: the file is new in the working directory
 *  - +:worktree_modified+: the file has been modified in the working directory
 *  - +:worktree_deleted+: the file has been deleted from the working directory
 *
 *  If a +block+ is given, status information will be gathered for every
 *  single file on the working dir. The +block+ will be called with the
 *  status data for each file.
 *
 *    repo.status { |status_data| puts status_data.inspect }
 *
 *  results in, for example:
 *
 *    [:index_new, :worktree_new]
 *    [:worktree_modified]
 *
 *  If a +path+ is given instead, the function will return the +status_data+ for
 *  the file pointed to by path, or raise an exception if the path doesn't exist.
 *
 *  +path+ must be relative to the repository's working directory.
 *
 *    repo.status('src/diff.c') #=> [:index_new, :worktree_new]
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
	int *exception = (int *)payload;
	rb_protect(rb_yield, rugged_create_oid(id), exception);
	return *exception ? GIT_ERROR : GIT_OK;
}

/*
 *  call-seq:
 *    repo.each_id { |id| block }
 *    repo.each_id -> Iterator
 *
 *  Call the given +block+ once with every object ID found in +repo+
 *  and all its alternates. Object IDs are passed as 40-character
 *  strings.
 */
static VALUE rb_git_repo_each_id(VALUE self)
{
	git_repository *repo;
	git_odb *odb;
	int error, exception = 0;

	if (!rb_block_given_p())
		return rb_funcall(self, rb_intern("to_enum"), 1, CSTR2SYM("each_id"));

	Data_Get_Struct(self, git_repository, repo);

	error = git_repository_odb(&odb, repo);
	rugged_exception_check(error);

	error = git_odb_foreach(odb, &rugged__each_id_cb, &exception);
	git_odb_free(odb);

	if (exception)
		rb_jump_tag(exception);
	rugged_exception_check(error);

	return Qnil;
}

static int parse_reset_type(VALUE rb_reset_type)
{
	ID id_reset_type;

	Check_Type(rb_reset_type, T_SYMBOL);
	id_reset_type = SYM2ID(rb_reset_type);

	if (id_reset_type == rb_intern("soft")) {
		return GIT_RESET_SOFT;
	} else if (id_reset_type == rb_intern("mixed")) {
		return GIT_RESET_MIXED;
	} else if (id_reset_type == rb_intern("hard")) {
		return GIT_RESET_HARD;
	} else {
		rb_raise(rb_eArgError,
			"Invalid reset type. Expected `:soft`, `:mixed` or `:hard`");
	}
}

/*
 *  call-seq:
 *    repo.reset(target, reset_type) -> nil
 *
 *  Sets the current head to the specified commit oid and optionally
 *  resets the index and working tree to match.
 *  - +target+: Rugged::Commit, Rugged::Tag or rev that resolves to a commit or tag object
 *  - +reset_type+: +:soft+, +:mixed+ or +:hard+
 *
 *  [:soft]
 *    the head will be moved to the commit.
 *  [:mixed]
 *    will trigger a +:soft+ reset, plus the index will be replaced
 *    with the content of the commit tree.
 *  [:hard]
 *    will trigger a +:mixed+ reset and the working directory will be
 *    replaced with the content of the index. (Untracked and ignored files
 *    will be left alone)
 *
 *  Examples:
 *
 *    repo.reset('origin/master', :hard) #=> nil
 */
static VALUE rb_git_repo_reset(VALUE self, VALUE rb_target, VALUE rb_reset_type)
{
	git_repository *repo;
	int reset_type;
	git_object *target = NULL;
	int error;

	Data_Get_Struct(self, git_repository, repo);

	reset_type = parse_reset_type(rb_reset_type);
	target = rugged_object_get(repo, rb_target, GIT_OBJ_ANY);

	error = git_reset(repo, target, reset_type);

	git_object_free(target);
	rugged_exception_check(error);

	return Qnil;
}

/*
 *  call-seq:
 *    repo.reset_path(pathspecs, target=nil) -> nil
 *
 *  Updates entries in the index from the +target+ commit tree, matching
 *  the given +pathspecs+.
 *
 *  Passing a nil +target+ will result in removing
 *  entries in the index matching the provided pathspecs.
 *
 *  - +pathspecs+: list of pathspecs to operate on (+String+ or +Array+ of +String+ objects)
 *  - +target+(optional): Rugged::Commit, Rugged::Tag or rev that resolves to a commit or tag object.
 *
 *  Examples:
 *    reset_path(File.join('subdir','file.txt'), '441034f860c1d5d90e4188d11ae0d325176869a8') #=> nil
 */
static VALUE rb_git_repo_reset_path(int argc, VALUE *argv, VALUE self)
{
	git_repository *repo;
	git_object *target = NULL;
	git_strarray pathspecs;
	VALUE rb_target, rb_paths;
	VALUE rb_path_array;
	int i, error = 0;

	pathspecs.strings = NULL;
	pathspecs.count = 0;

	Data_Get_Struct(self, git_repository, repo);

	rb_scan_args(argc, argv, "11", &rb_paths, &rb_target);

	rb_path_array = rb_ary_to_ary(rb_paths);

	for (i = 0; i < RARRAY_LEN(rb_path_array); ++i)
		Check_Type(rb_ary_entry(rb_path_array, i), T_STRING);

	pathspecs.count = RARRAY_LEN(rb_path_array);
	pathspecs.strings = xmalloc(pathspecs.count * sizeof(char *));

	for (i = 0; i < RARRAY_LEN(rb_path_array); ++i) {
		VALUE fpath = rb_ary_entry(rb_path_array, i);
		pathspecs.strings[i] = StringValueCStr(fpath);
	}

	if (!NIL_P(rb_target))
		target = rugged_object_get(repo, rb_target, GIT_OBJ_ANY);

	error = git_reset_default(repo, target, &pathspecs);

	xfree(pathspecs.strings);
	git_object_free(target);

	rugged_exception_check(error);

	return Qnil;
}


static int rugged__push_status_cb(const char *ref, const char *msg, void *payload)
{
	VALUE rb_result_hash = (VALUE)payload;
	if (msg != NULL)
		rb_hash_aset(rb_result_hash, rb_str_new_utf8(ref), rb_str_new_utf8(msg));

	return GIT_OK;
}

/*
 *  call-seq:
 *    repo.push(remote, refspecs) -> hash
 *
 *  Pushes the given +refspecs+ to the given +remote+. Returns a hash that contains
 *  key-value pairs that reflect pushed refs and error messages, if applicable.
 *
 *  Example:
 *
 *    repo.push("origin", ["refs/heads/master", ":refs/heads/to_be_deleted"])
 */
static VALUE rb_git_repo_push(VALUE self, VALUE rb_remote, VALUE rb_refspecs)
{
	VALUE rb_refspec, rb_exception = Qnil, rb_result = rb_hash_new();
	git_repository *repo;
	git_remote *remote = NULL;
	git_push *push = NULL;

	int error = 0, i = 0;

	Check_Type(rb_refspecs, T_ARRAY);
	for (i = 0; i < RARRAY_LEN(rb_refspecs); ++i) {
		rb_refspec = rb_ary_entry(rb_refspecs, i);
		Check_Type(rb_refspec, T_STRING);
	}

	Data_Get_Struct(self, git_repository, repo);

	if (rb_obj_is_kind_of(rb_remote, rb_cRuggedRemote)) {
		Data_Get_Struct(rb_remote, git_remote, remote);
	} else if (TYPE(rb_remote) == T_STRING) {
		error = git_remote_load(&remote, repo, StringValueCStr(rb_remote));
		if (error) goto cleanup;
	} else {
		rb_raise(rb_eTypeError, "Expecting a String or Rugged::Remote instance");
	}

	error = git_push_new(&push, remote);
	if (error) goto cleanup;

	for (i = 0; !error && i < RARRAY_LEN(rb_refspecs); ++i) {
		rb_refspec = rb_ary_entry(rb_refspecs, i);
		error = git_push_add_refspec(push, StringValueCStr(rb_refspec));
	}
	if (error) goto cleanup;

	error = git_push_finish(push);

	if (error) {
		if (error == GIT_ENONFASTFORWARD) {
			rb_exception = rb_exc_new2(rb_eRuggedError, "non-fast-forward update rejected");
		} else if (error == -1) {
			rb_exception = rb_exc_new2(rb_eRuggedError, "could not push to repo (check for non-bare repo)");
		}

		goto cleanup;
	}

	if (!git_push_unpack_ok(push)) {
		rb_exception = rb_exc_new2(rb_eRuggedError, "the remote side did not unpack successfully");
		goto cleanup;
	}

	error = git_push_status_foreach(push, &rugged__push_status_cb, (void *)rb_result);
	if (error) goto cleanup;

	error = git_push_update_tips(push);

cleanup:
	git_push_free(push);

	// We can only free the remote if we have loaded it ourselves.
	if (!rb_obj_is_kind_of(rb_remote, rb_cRuggedRemote)) {
		git_remote_free(remote);
	}

	if (!NIL_P(rb_exception))
		rb_exc_raise(rb_exception);

	rugged_exception_check(error);

	return rb_result;
}

/*
 *  call-seq:
 *    repo.close -> nil
 *
 *  Frees all the resources used by this repository immediately. The repository can
 *  still be used after this call. Resources will be opened as necessary.
 *
 *  It is not required to call this method explicitly. Repositories are closed
 *  automatically before garbage collection
 */
static VALUE rb_git_repo_close(VALUE self)
{
	git_repository *repo;
	Data_Get_Struct(self, git_repository, repo);

	git_repository__cleanup(repo);

	return Qnil;
}

/*
 *  call-seq:
 *    repo.namespace = new_namespace
 *
 *  Sets the active namespace for the repository.
 *  If set to nil, no namespace will be active.
 */
static VALUE rb_git_repo_set_namespace(VALUE self, VALUE rb_namespace)
{
	git_repository *repo;
	int error;

	Data_Get_Struct(self, git_repository, repo);

	if (!NIL_P(rb_namespace)) {
		Check_Type(rb_namespace, T_STRING);
		error = git_repository_set_namespace(repo, StringValueCStr(rb_namespace));
	} else {
		error = git_repository_set_namespace(repo, NULL);
	}
	rugged_exception_check(error);

	return Qnil;
}

/*
 *  call-seq:
 *    repo.namespace -> str
 *
 *  Returns the active namespace for the repository.
 */
static VALUE rb_git_repo_get_namespace(VALUE self)
{
	git_repository *repo;
	const char *namespace;

	Data_Get_Struct(self, git_repository, repo);

	namespace = git_repository_get_namespace(repo);
	return namespace ? rb_str_new_utf8(namespace) : Qnil;
}

/*
 *  call-seq:
 *    repo.ahead_behind(local, upstream) -> Array
 *
 *  Returns a 2 element Array containing the number of commits
 *  that the upstream object is ahead and behind the local object.
 *
 *  +local+ and +upstream+ can either be strings containing SHA1 OIDs or
 *  Rugged::Object instances.
 */
static VALUE rb_git_repo_ahead_behind(VALUE self, VALUE rb_local, VALUE rb_upstream) {
	git_repository *repo;
	int error;
	git_oid local, upstream;
	size_t ahead, behind;
	VALUE rb_result;

	Data_Get_Struct(self, git_repository, repo);

	error = rugged_oid_get(&local, repo, rb_local);
	rugged_exception_check(error);

	error = rugged_oid_get(&upstream, repo, rb_upstream);
	rugged_exception_check(error);

	error = git_graph_ahead_behind(&ahead, &behind, repo, &local, &upstream);
	rugged_exception_check(error);

	rb_result = rb_ary_new2(2);
	rb_ary_push(rb_result, INT2FIX((int) ahead));
	rb_ary_push(rb_result, INT2FIX((int) behind));
	return rb_result;
}

void Init_rugged_repo(void)
{
	id_call = rb_intern("call");

	rb_cRuggedRepo = rb_define_class_under(rb_mRugged, "Repository", rb_cObject);

	rb_define_singleton_method(rb_cRuggedRepo, "new", rb_git_repo_new, -1);
	rb_define_singleton_method(rb_cRuggedRepo, "bare", rb_git_repo_open_bare, -1);
	rb_define_singleton_method(rb_cRuggedRepo, "hash",   rb_git_repo_hash,  2);
	rb_define_singleton_method(rb_cRuggedRepo, "hash_file",   rb_git_repo_hashfile,  2);
	rb_define_singleton_method(rb_cRuggedRepo, "init_at", rb_git_repo_init_at, -1);
	rb_define_singleton_method(rb_cRuggedRepo, "discover", rb_git_repo_discover, -1);
	rb_define_singleton_method(rb_cRuggedRepo, "clone_at", rb_git_repo_clone_at, -1);

	rb_define_method(rb_cRuggedRepo, "close", rb_git_repo_close, 0);

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

	rb_define_method(rb_cRuggedRepo, "push", rb_git_repo_push, 2);

	rb_define_method(rb_cRuggedRepo, "index",  rb_git_repo_get_index,  0);
	rb_define_method(rb_cRuggedRepo, "index=",  rb_git_repo_set_index,  1);
	rb_define_method(rb_cRuggedRepo, "config",  rb_git_repo_get_config,  0);
	rb_define_method(rb_cRuggedRepo, "config=",  rb_git_repo_set_config,  1);

	rb_define_method(rb_cRuggedRepo, "bare?",  rb_git_repo_is_bare,  0);
	rb_define_method(rb_cRuggedRepo, "empty?",  rb_git_repo_is_empty,  0);

	rb_define_method(rb_cRuggedRepo, "head_detached?",  rb_git_repo_head_detached,  0);
	rb_define_method(rb_cRuggedRepo, "head_orphan?",  rb_git_repo_head_orphan,  0);
	rb_define_method(rb_cRuggedRepo, "head=", rb_git_repo_set_head, 1);
	rb_define_method(rb_cRuggedRepo, "head", rb_git_repo_get_head, 0);

	rb_define_method(rb_cRuggedRepo, "merge_base", rb_git_repo_merge_base, -2);
	rb_define_method(rb_cRuggedRepo, "reset", rb_git_repo_reset, 2);
	rb_define_method(rb_cRuggedRepo, "reset_path", rb_git_repo_reset_path, -1);

	rb_define_method(rb_cRuggedRepo, "namespace=", rb_git_repo_set_namespace, 1);
	rb_define_method(rb_cRuggedRepo, "namespace", rb_git_repo_get_namespace, 0);

	rb_define_method(rb_cRuggedRepo, "ahead_behind", rb_git_repo_ahead_behind, 2);

	rb_cRuggedOdbObject = rb_define_class_under(rb_mRugged, "OdbObject", rb_cObject);
	rb_define_method(rb_cRuggedOdbObject, "data",  rb_git_odbobj_data,  0);
	rb_define_method(rb_cRuggedOdbObject, "len",  rb_git_odbobj_size,  0);
	rb_define_method(rb_cRuggedOdbObject, "type",  rb_git_odbobj_type,  0);
	rb_define_method(rb_cRuggedOdbObject, "oid",  rb_git_odbobj_oid,  0);
}
